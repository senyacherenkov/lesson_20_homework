#include <iostream>
#include <string>
#include <algorithm>
#include "observer.h"

Registrator::Registrator():
    m_isStopped(false)
{
    m_threadDataBuff.resize(3);

    for(size_t i = 0; i < 2; i++)
        m_workers.emplace_back(
                    [this, i]
                        {
                            thread_local ThreadData threadData;

                            while (!m_isStopped.load(std::memory_order_relaxed)) {

                                std::unique_lock<std::mutex> lck{m_fileMutex};

                                while (!m_isStopped.load(std::memory_order_relaxed) && m_fileLogQueue.empty())
                                    m_fileCondition.wait(lck);

                                if (m_isStopped.load(std::memory_order_relaxed))
                                    break;

                                auto task = m_fileLogQueue.front();
                                m_fileLogQueue.pop();

                                lck.unlock();

                                threadData.m_nBlocks++;

                                size_t nCommands = task();
                                threadData.m_nCommands += nCommands;
                            }

                            while(!m_fileLogQueue.empty()) {
                                std::unique_lock<std::mutex> lck{m_fileMutex};
                                auto task = m_fileLogQueue.front();
                                m_fileLogQueue.pop();

                                lck.unlock();

                                threadData.m_nBlocks++;
                                size_t nCommands = task();
                                threadData.m_nCommands += nCommands;
                            }
                            m_threadDataBuff[i] = threadData;
                            printSummary(threadData);
                        });

    m_stdOutWorker = std::thread(&Registrator::writeStdOuput, this);
}


Registrator::~Registrator()
{
    m_isStopped.store(true, std::memory_order_relaxed);

    m_fileCondition.notify_all();
    m_stdoutCondition.notify_all();
    for(std::thread& worker: m_workers){
        worker.join();
    }
    m_stdOutWorker.join();
}


std::string Registrator::prepareData(const std::vector<std::string>& newCommands) const
{
    if(newCommands.empty())
        return std::string();

    std::string output;

    output = "bulk: ";

    for(auto it = newCommands.begin(); it < newCommands.end(); it++) {
        output += (*it);
        if(it != std::next(newCommands.begin(), static_cast<long>(newCommands.size() - 1)))
            output += ", ";
    }
    return output;
}

void Registrator::writeStdOuput()
{
    thread_local ThreadData threadData;

    while (!m_isStopped.load(std::memory_order_relaxed)) {

        std::unique_lock<std::mutex> lck{m_stdoutMutex};

        while (!m_isStopped.load(std::memory_order_relaxed) && m_stdOutQueue.empty())
            m_stdoutCondition.wait(lck);

        if (m_isStopped.load(std::memory_order_relaxed))
            break;

        auto pair = m_stdOutQueue.front();
        m_stdOutQueue.pop();

        lck.unlock();

        std::cout << pair.first << std::endl;        

        threadData.m_nBlocks++;
        threadData.m_nCommands += pair.second;
    }

    while(!m_stdOutQueue.empty()) {
        std::unique_lock<std::mutex> lck{m_stdoutMutex};
        auto pair = m_stdOutQueue.front();
        m_stdOutQueue.pop();
        lck.unlock();

        std::cout << pair.first << std::endl;        

        threadData.m_nBlocks++;
        threadData.m_nCommands += pair.second;
    }

    m_threadDataBuff[2] = threadData;
    printSummary(threadData);
}

void Registrator::writeFileLog(std::string data, long time)
{
    m_logCounter++;
    std::string nameOfFile("bulk");
    nameOfFile += std::to_string(time);
    nameOfFile += "_";
    nameOfFile += std::to_string(m_logCounter);
    nameOfFile += ".log";

    m_fileNames.push_back(nameOfFile);

    std::ofstream bulkLog;
    bulkLog.open(nameOfFile.c_str());
    bulkLog << data;
    bulkLog.close();
}

void Registrator::printSummary(ThreadData &data)
{    
    std::cout << "thread - " << std::this_thread::get_id()
              << " " << data << std::endl;
}

void Registrator::update(const std::vector<std::string> &newCommands, long time)
{
    if(newCommands.empty()) {        
        m_isStopped.store(true, std::memory_order_relaxed);
        m_fileCondition.notify_all();
        m_stdoutCondition.notify_all();
        return;
    }

    std::string output = prepareData(newCommands);
    size_t size = newCommands.size();

    auto logTask = [output, time, size, this]() -> size_t {
                                                               writeFileLog(output, time);
                                                               return size;
                                                          };

    {
        std::lock(m_fileMutex, m_stdoutMutex);
        std::lock_guard<std::mutex> lck1(m_fileMutex, std::adopt_lock);
        std::lock_guard<std::mutex> lck2(m_stdoutMutex, std::adopt_lock);

        m_stdOutQueue.push(std::make_pair(output, size));
        m_fileLogQueue.push(logTask);
    }

    m_fileCondition.notify_all();
    m_stdoutCondition.notify_all();
}

Observer::~Observer()
{}
