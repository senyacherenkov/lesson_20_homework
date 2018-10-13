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
#include "utility.h"

class Observer {
public:
    virtual ~Observer();
    virtual void update(const std::vector<std::string>& newCommands, long time) = 0;
};

class Registrator: public Observer {    
    using TTaskCallback = std::function<size_t()>;
public:
    Registrator();
    ~Registrator() override;
    void update(const std::vector<std::string>& newCommands, long time) override;

    std::vector<ThreadData>& getThreadData() { return m_threadDataBuff; }
    std::vector<std::string>& getFileNames() { return m_fileNames; }
private:
    void workerThread();

    std::string prepareData(const std::vector<std::string>& newCommands) const;
    void writeStdOuput();
    void writeFileLog(std::string data, long time);
    void printSummary(ThreadData& data);

private:

    std::atomic_bool                                m_isStopped;
    std::queue<TTaskCallback>                       m_fileLogQueue;
    std::queue<std::pair<std::string, size_t>>      m_stdOutQueue;

    std::vector<std::thread>                        m_workers;
    std::thread                                     m_stdOutWorker;
    std::mutex                                      m_fileMutex;
    std::mutex                                      m_stdoutMutex;
    std::condition_variable                         m_fileCondition;
    std::condition_variable                         m_stdoutCondition;

    std::size_t                                     m_logCounter;
    std::vector<ThreadData>                         m_threadDataBuff;
    std::vector<std::string>                        m_fileNames;
};

