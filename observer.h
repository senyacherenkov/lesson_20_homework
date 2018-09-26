#pragma once
#include <vector>
#include <string>
#include <fstream>

class Observer {
public:
    virtual ~Observer();
    virtual void update(const std::vector<std::string>& newCommands, long time) = 0;
};

class Registrator: public Observer {
public:
    Registrator() = default;
    void update(const std::vector<std::string>& newCommands, long time);
private:
    std::ofstream m_bulkLog;
};
