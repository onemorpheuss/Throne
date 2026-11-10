#pragma once

#include <QHash>

#include <iterator>
#include <list>

class RecentIdOrder {
public:
    void clear() {
        m_order.clear();
        m_positions.clear();
    }

    bool isEmpty() const {
        return m_order.empty();
    }

    void append(int id) {
        if (m_positions.contains(id)) {
            touch(id);
            return;
        }
        m_order.push_back(id);
        m_positions.insert(id, std::prev(m_order.end()));
    }

    void touch(int id) {
        const auto position = m_positions.find(id);
        if (position == m_positions.end()) return;
        m_order.splice(m_order.end(), m_order, position.value());
        position.value() = std::prev(m_order.end());
    }

    int takeFirst() {
        if (m_order.empty()) return -1;
        const int id = m_order.front();
        m_order.pop_front();
        m_positions.remove(id);
        return id;
    }

private:
    std::list<int> m_order;
    QHash<int, std::list<int>::iterator> m_positions;
};
