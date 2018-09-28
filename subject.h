#pragma once
#include <memory>
#include <iostream>
#include <cstdlib>
#include <list>
#include <chrono>
#include "observer.h"

using seconds_t = std::chrono::seconds;

class Subject {
public:
    Subject() = default;
    virtual ~Subject();

    void addObserver(std::shared_ptr<Observer> observer);

    void removeObserver(std::shared_ptr<Observer> observer);

    virtual void notifyObservers() = 0;

protected:
    std::list<std::shared_ptr<Observer>> m_observers;
};


class Reader: public Subject {
public:
    Reader(std::size_t N):
        m_N(N)
    {}

    ~Reader() = default;

    void readCommands();

private:
    virtual void notifyObservers();
    void printSummary();

private:
    std::size_t m_N;
    std::vector<std::string>    m_commands;
    long                        m_timeOfFirstCommand = 0;
    int                         m_nStrings;
    int                         m_nBlocks;
    int                         m_nCommands;
};
