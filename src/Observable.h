#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include "Observer.h"
#include <vector>
#include <algorithm>

class Observable {
public:
    void adaugaObserver(Observer* observer) {
        m_observers.push_back(observer);
    }

    void stergeObserver(Observer* observer) {
        m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer), m_observers.end());
    }

protected:
    void notifica(const std::string& message) {
        for (Observer* observer : m_observers) {
            observer->update(message);
        }
    }

private:
    std::vector<Observer*> m_observers;
};

#endif // OBSERVABLE_H
