#include "include/ui/utils/ConnectionsTreeModel.h"

#include <QSignalSpy>
#include <QtTest>

// The model test needs formatting behavior, not the production global helpers.
QString ReadableSize(const qint64 &size) {
    return QString::number(size);
}

QString ShortDisplayName(const QString &name, int maxGraphemes) {
    return name.left(maxGraphemes);
}

namespace Stats {
    bool SortIsDescending(ConnectionSort sort, bool ascending) {
        if (sort == Default) return false;
        if (sort == ByProcess || sort == BySource || sort == ByProtocol || sort == ByOutbound) return ascending;
        return !ascending;
    }
}

class ConnectionsTreeModelTests final : public QObject {
    Q_OBJECT

    static Stats::ConnectionMetadata connection(qint64 upload = 1) {
        Stats::ConnectionMetadata row;
        row.id = QStringLiteral("connection-1");
        row.createdAtMs = 100;
        row.upload = upload;
        row.download = 2;
        row.outbound = QStringLiteral("proxy");
        row.outboundName = QStringLiteral("proxy");
        row.network = QStringLiteral("tcp");
        row.dest = QStringLiteral("127.0.0.1:443");
        row.domain = QStringLiteral("example.test");
        row.process = QStringLiteral("local-app");
        row.processPath = QStringLiteral("local-app");
        row.source = QStringLiteral("127.0.0.1:10000");
        row.sourceDisplay = QStringLiteral("Local");
        row.sourceLocal = true;
        row.rule = QStringLiteral("final");
        row.uploadSpeed = 3;
        row.downloadSpeed = 4;
        return row;
    }

private slots:
    void unchangedSnapshotDoesNotRelayout() {
        ConnectionsTreeModel model;
        QSignalSpy layouts(&model, &QAbstractItemModel::layoutChanged);
        const QList<Stats::ConnectionMetadata> rows{connection()};

        model.setConnections(rows, Stats::Default, false);
        QCOMPARE(layouts.count(), 1);
        QCOMPARE(model.rowCount(), 1);

        model.setConnections(rows, Stats::Default, false);
        QCOMPARE(layouts.count(), 1);
    }

    void changedTrafficAndSortStillRefresh() {
        ConnectionsTreeModel model;
        QSignalSpy layouts(&model, &QAbstractItemModel::layoutChanged);
        QList<Stats::ConnectionMetadata> rows{connection()};

        model.setConnections(rows, Stats::Default, false);
        rows[0].upload = 99;
        model.setConnections(rows, Stats::Default, false);
        QCOMPARE(layouts.count(), 2);

        const QModelIndex group = model.index(0, ConnectionsTreeModel::ColTraffic);
        QVERIFY(model.data(group).toString().contains(QStringLiteral("99")));
        model.setConnections(rows, Stats::ByTraffic, true);
        QCOMPARE(layouts.count(), 3);
    }

    void unchangedEmptySnapshotDoesNotRelayout() {
        ConnectionsTreeModel model;
        QSignalSpy layouts(&model, &QAbstractItemModel::layoutChanged);
        model.setConnections({}, Stats::Default, false);
        model.setConnections({}, Stats::Default, false);
        QCOMPARE(layouts.count(), 1);
    }

    void processSortDirectionIsNotCachedAway() {
        ConnectionsTreeModel model;
        auto alpha = connection();
        alpha.id = QStringLiteral("alpha");
        alpha.process = QStringLiteral("alpha");
        auto beta = connection();
        beta.id = QStringLiteral("beta");
        beta.process = QStringLiteral("beta");
        const QList<Stats::ConnectionMetadata> rows{alpha, beta};

        model.setConnections(rows, Stats::ByProcess, false);
        QCOMPARE(model.processNameAt(model.index(0, 0)), QStringLiteral("alpha"));
        model.setConnections(rows, Stats::ByProcess, true);
        QCOMPARE(model.processNameAt(model.index(0, 0)), QStringLiteral("beta"));
    }
};

QTEST_GUILESS_MAIN(ConnectionsTreeModelTests)
#include "ConnectionsTreeModelTests.moc"
