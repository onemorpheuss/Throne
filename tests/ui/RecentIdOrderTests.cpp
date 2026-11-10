#include "include/ui/utils/RecentIdOrder.h"

#include <QtTest>

class RecentIdOrderTests final : public QObject {
    Q_OBJECT

private slots:
    void touchMovesIdToTheMostRecentEnd() {
        RecentIdOrder order;
        order.append(1);
        order.append(2);
        order.append(3);
        order.touch(1);

        QCOMPARE(order.takeFirst(), 2);
        QCOMPARE(order.takeFirst(), 3);
        QCOMPARE(order.takeFirst(), 1);
        QVERIFY(order.isEmpty());
    }

    void duplicateAppendDoesNotCreateTwoEntries() {
        RecentIdOrder order;
        order.append(7);
        order.append(8);
        order.append(7);

        QCOMPARE(order.takeFirst(), 8);
        QCOMPARE(order.takeFirst(), 7);
        QVERIFY(order.isEmpty());
    }

    void clearDropsAllPositions() {
        RecentIdOrder order;
        order.append(1);
        order.append(2);
        order.clear();
        QVERIFY(order.isEmpty());

        order.append(1);
        QCOMPARE(order.takeFirst(), 1);
        QVERIFY(order.isEmpty());
    }
};

QTEST_GUILESS_MAIN(RecentIdOrderTests)
#include "RecentIdOrderTests.moc"
