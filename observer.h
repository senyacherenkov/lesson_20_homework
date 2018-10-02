#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <future>
#include <condition_variable>
#include <functional>
#include <memory>
#include <tuple>

class Observer {
public:
    virtual ~Observer();
    virtual void update(const std::vector<std::string>& newCommands, long time) = 0;
};

class Registrator: public Observer {    
    using TTaskCallback = std::function<size_t()>;
public:
    Registrator();
    ~Registrator();
    void update(const std::vector<std::string>& newCommands, long time);

private:
    void workerThread();

    std::string prepareData(const std::vector<std::string>& newCommands) const;
    void writeStdOuput();
    void writeFileLog(std::string data, long time);
    void printSummary(int nblocks, int ncommand);

private:

    bool                                            m_isStopped;
    std::queue<TTaskCallback>                       m_fileLogQueue;
    std::queue<std::pair<std::string, size_t>>      m_stdOutQueue;

    std::vector<std::thread>                        m_workers;
    std::thread                                     m_stdOutWorker;
    std::mutex                                      m_queueMutex;
    std::condition_variable                         m_condition;

    std::vector<int>                                m_loadBuffer;
};
