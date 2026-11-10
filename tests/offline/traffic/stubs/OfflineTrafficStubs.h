#pragma once

// Minimal offline collaborators for the production TrafficLooper/StatsService
// sources. None of these facilities opens a socket, database, or process.

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace Configs {
    struct OfflineOutbound {
        QString DisplayName() const { return {}; }
    };

    class Profile {
    public:
        QString type;
        QString name;
        int id = -1;
        int gid = 0;
        qint64 traffic_downlink = 0;
        qint64 traffic_uplink = 0;
        std::shared_ptr<OfflineOutbound> outbound;
    };

    enum class TrafficChainKind { Main, Route, Aux };

    struct TrafficChainGroup {
        QString watchTag;
        QList<std::shared_ptr<Profile>> profiles;
        TrafficChainKind kind = TrafficChainKind::Main;
        int profileId = -1;
        QString name;
    };

    enum class SortInput { Traffic };

    inline QString DisplayEffectiveAddress(const std::shared_ptr<Profile> &) { return {}; }
    inline void RepositionLater(const QList<std::shared_ptr<Profile>> &, SortInput) {}

    class ProfilesRepo {
    public:
        void SaveTrafficBatch(const QList<std::shared_ptr<Profile>> &) {}
    };

    struct Group {
        QString name;
    };

    class GroupsRepo {
    public:
        std::shared_ptr<Group> GetGroup(int) const { return nullptr; }
    };

    struct SettingsRepo {
        bool disable_traffic_stats = false;
    };

    class DatabaseManager {
    public:
        SettingsRepo settings;
        GroupsRepo groups;
        SettingsRepo *settingsRepo = &settings;
        GroupsRepo *groupsRepo = &groups;
        ProfilesRepo *profilesRepo = nullptr;
    };

    inline DatabaseManager *dataManager = nullptr;
} // namespace Configs

namespace libcore {
    struct QueryStatsResp {
        std::map<std::string, qint64> ups;
        std::map<std::string, qint64> downs;
    };
} // namespace libcore

namespace API {
    class Client {
    public:
        virtual ~Client() = default;
        virtual libcore::QueryStatsResp QueryStats() { return {}; }
        QString CloseConnections(bool *rpcOK, const QStringList &, int *closedCount = nullptr) const {
            if (rpcOK) *rpcOK = true;
            if (closedCount) *closedCount = 0;
            return {};
        }
    };

    inline Client *defaultClient = nullptr;
} // namespace API

namespace Stats {
    class ConnectionMetadata {
    public:
        QString id;
    };

    class ConnectionLister {
    public:
        using ApiSink = std::function<void(const QList<ConnectionMetadata> &, qint64)>;
        std::atomic<bool> suspend{true};

        void Loop() {}
        void ForceUpdate() {}
        void SetApiWatch(bool) {}
        void SetApiSink(ApiSink) {}
        void SetOutboundNames(const QHash<QString, QString> &) {}
    };

    inline ConnectionLister *connection_lister = new ConnectionLister;

    constexpr int DIRECT_STAT_PROFILE_ID = -101;

    class TrafficStatsManager {
    public:
        void AddConfigDelta(int, long long, long long) {}
        void Flush() {}
        void SnapshotConfigMeta(int, const QString &, const QString &, const QString &, const QString &) {}
        void EnsureDirectMeta() {}
    };

    inline TrafficStatsManager *trafficStatsManager = new TrafficStatsManager;
} // namespace Stats

inline const QString UNICODE_LRO = QString::fromUtf8(QByteArray::fromHex("E280AD"));
inline QString ReadableSize(const qint64 &size) { return QString::number(size); }
inline qint64 MonotonicMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void runOnUiThread(const std::function<void()> &callback, bool = false) { callback(); }
inline void runOnNewThread(const std::function<void()> &, bool = false) {}
inline void MW_show_log(const QString &) {}

class MainWindowTr {
public:
    static QString tr(const char *text) { return QString::fromLatin1(text); }
};

#define ENGINE_CHECK_MAIN_THREAD() do {} while (false)
