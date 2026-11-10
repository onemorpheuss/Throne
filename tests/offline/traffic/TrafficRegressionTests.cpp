#include "include/engine/services/StatsService.h"
#include "include/stats/traffic/TrafficLooper.hpp"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QTextStream>

#include <functional>
#include <stdexcept>
#include <string>

namespace {
    class TestFailure final : public std::runtime_error {
    public:
        explicit TestFailure(const std::string &message) : std::runtime_error(message) {}
    };

    void require(bool condition, const char *expression, int line) {
        if (!condition) throw TestFailure(std::string("line ") + std::to_string(line) + ": " + expression);
    }

#define REQUIRE(expression) require(static_cast<bool>(expression), #expression, __LINE__)

    struct ExitRecordingLoop final : std::exception {};

    class RecordingClient final : public API::Client {
    public:
        libcore::QueryStatsResp response;
        int queryStatsCalls = 0;

        libcore::QueryStatsResp QueryStats() override {
            ++queryStatsCalls;
            return response;
        }
    };

    class GlobalStateGuard {
    public:
        GlobalStateGuard()
            : oldClient(API::defaultClient), oldDataManager(Configs::dataManager),
              oldSink(Stats::CurrentStatsSink()) {}

        ~GlobalStateGuard() {
            API::defaultClient = oldClient;
            Configs::dataManager = oldDataManager;
            Stats::SetStatsSink(oldSink);
        }

    private:
        API::Client *oldClient;
        Configs::DatabaseManager *oldDataManager;
        Stats::StatsSink *oldSink;
    };

    Configs::TrafficChainGroup chainGroup(const std::shared_ptr<Configs::Profile> &profile) {
        return Configs::TrafficChainGroup{
            .watchTag = QStringLiteral("route-test"),
            .profiles = {profile},
            .kind = Configs::TrafficChainKind::Route,
            .profileId = profile->id,
            .name = QStringLiteral("Route test"),
        };
    }

    class RecordingSink final : public Stats::StatsSink {
    public:
        QList<Stats::TrafficSample> samples;
        int stoppedCalls = 0;

        void onTrafficLines(const QList<Stats::TrafficLine> &) override {}
        void onTrafficSample(const Stats::TrafficSample &sample) override {
            samples.append(sample);
            // Loop() is intentionally infinite. This test-only unwind lets the
            // real stopped iteration finish without adding a production hook.
            throw ExitRecordingLoop{};
        }
        void onTrafficStopped() override { ++stoppedCalls; }
        void onConnections(const QList<Stats::ConnectionMetadata> &) override {}
    };

    void finalDisabledSampleDrainsPendingIdsWithoutAnotherQuery() {
        GlobalStateGuard globals;
        Configs::DatabaseManager database;
        RecordingClient client;
        Configs::dataManager = &database;
        API::defaultClient = &client;

        auto profile = std::make_shared<Configs::Profile>();
        profile->id = 73;
        client.response.ups["route-test"] = 11;
        client.response.downs["route-test"] = 22;

        RecordingSink sink;
        Stats::SetStatsSink(&sink);
        Stats::TrafficLooper looper;
        looper.SetChainGroups({chainGroup(profile)});

        {
            QMutexLocker lock(&looper.loop_mutex);
            looper.UpdateAll();
        }
        REQUIRE(client.queryStatsCalls == 1);
        REQUIRE(profile->traffic_uplink == 11);
        REQUIRE(profile->traffic_downlink == 22);

        looper.looping = true;
        looper.loop_enabled = false;
        bool exitedThroughRecordingSink = false;
        try {
            looper.Loop();
        } catch (const ExitRecordingLoop &) {
            exitedThroughRecordingSink = true;
        }

        REQUIRE(exitedThroughRecordingSink);
        REQUIRE(sink.stoppedCalls == 1);
        REQUIRE(sink.samples.size() == 1);
        REQUIRE(!sink.samples.first().coreRunning);
        REQUIRE(sink.samples.first().creditedIds == QList<int>{73});
        REQUIRE(client.queryStatsCalls == 1);
    }

    QString idsEvent(const QList<int> &ids) {
        QStringList parts;
        for (const int id : ids) parts << QString::number(id);
        return QStringLiteral("profiles:") + parts.join(QLatin1Char(','));
    }

    void chainGroupResetDrainsRepaintBeforeLinesOnlyOnce() {
        GlobalStateGuard globals;
        Configs::DatabaseManager database;
        RecordingClient client;
        Configs::dataManager = &database;
        API::defaultClient = &client;

        auto profile = std::make_shared<Configs::Profile>();
        profile->id = 73;
        const auto groups = QList<Configs::TrafficChainGroup>{chainGroup(profile)};
        client.response.ups["route-test"] = 5;
        client.response.downs["route-test"] = 7;

        Engine::StatsService service;
        QStringList events;
        QObject::connect(&service, &Engine::StatsService::trafficProfilesChanged,
                         [&events](const QList<int> &ids) { events << idsEvent(ids); });
        QObject::connect(&service, &Engine::StatsService::trafficLinesChanged,
                         [&events](const QList<Stats::TrafficLine> &) { events << QStringLiteral("lines"); });

        Stats::trafficLooper->SetChainGroups(groups);
        events.clear();
        {
            QMutexLocker lock(&Stats::trafficLooper->loop_mutex);
            Stats::trafficLooper->UpdateAll();
        }
        REQUIRE(client.queryStatsCalls == 1);

        Stats::trafficLooper->SetChainGroups(groups);
        REQUIRE(events == QStringList({QStringLiteral("profiles:73"), QStringLiteral("lines")}));

        events.clear();
        Stats::trafficLooper->SetChainGroups(groups);
        REQUIRE(events == QStringList{QStringLiteral("lines")});
        REQUIRE(client.queryStatsCalls == 1);
    }
} // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        finalDisabledSampleDrainsPendingIdsWithoutAnotherQuery();
        chainGroupResetDrainsRepaintBeforeLinesOnlyOnce();
    } catch (const std::exception &error) {
        qFatal("Offline traffic regression test failed: %s", error.what());
    }

    QTextStream(stdout) << "PASS final disabled-loop sample drains credited IDs without QueryStats\n";
    QTextStream(stdout) << "PASS SetChainGroups emits profiles before lines and no duplicate profile repaint\n";
    return 0;
}
