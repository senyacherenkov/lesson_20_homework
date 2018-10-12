#include <iostream>
#include <string>
#include <algorithm>
#include "observer.h"

Registrator::Registrator():
    m_isStopped(false)
{
    for(size_t i = 0; i < 2; i++)
        m_workers.emplace_back(
                    [this]
                        {
                            thread_local int blocksNumber = 0;
                            thread_local int commandsNumber = 0;

                            while (!m_isStopped.load(std::memory_order_relaxed)) {

                                std::unique_lock<std::mutex> lck{m_fileMutex};

                                while (!m_isStopped.load(std::memory_order_relaxed) && m_fileLogQueue.empty())
                                    m_fileCondition.wait(lck);

                                if (m_isStopped.load(std::memory_order_relaxed))
                                    break;

                                std::random_shuffle(m_loadBuffer.begin(), m_loadBuffer.end());
                                auto task = m_fileLogQueue.front();
                                m_fileLogQueue.pop();

                                lck.unlock();

                                blocksNumber++;

                                size_t nCommands = task();
                                commandsNumber += nCommands;
                            }

                            while(!m_fileLogQueue.empty()) {
                                std::unique_lock<std::mutex> lck{m_fileMutex};
                                auto task = m_fileLogQueue.front();
                                m_fileLogQueue.pop();

                                lck.unlock();

                                blocksNumber++;
                                size_t nCommands = task();
                                commandsNumber += nCommands;
                            }

                            printSummary(blocksNumber, commandsNumber);
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
    thread_local int blocksNumber = 0;
    thread_local int commandsNumber = 0;

    while (!m_isStopped.load(std::memory_order_relaxed)) {

        std::unique_lock<std::mutex> lck{m_stdoutMutex};

        while (!m_isStopped.load(std::memory_order_relaxed) && m_stdOutQueue.empty())
            m_stdoutCondition.wait(lck);

        if (m_isStopped.load(std::memory_order_relaxed))
            break;

        std::random_shuffle(m_loadBuffer.begin(), m_loadBuffer.end());
        auto pair = m_stdOutQueue.front();
        m_stdOutQueue.pop();

        lck.unlock();

        std::cout << pair.first << std::endl;        

        blocksNumber++;
        commandsNumber += pair.second;
    }

    while(!m_stdOutQueue.empty()) {
        std::unique_lock<std::mutex> lck{m_stdoutMutex};
        auto pair = m_stdOutQueue.front();
        m_stdOutQueue.pop();
        lck.unlock();

        std::cout << pair.first << std::endl;        

        blocksNumber++;
        commandsNumber += pair.second;
    }

    printSummary(blocksNumber, commandsNumber);
}

void Registrator::writeFileLog(std::string data, long time)
{
    m_logCounter++;
    std::string nameOfFile("bulk");
    nameOfFile += std::to_string(time);
    nameOfFile += "_";
    nameOfFile += std::to_string(m_logCounter);
    nameOfFile += ".log";

    std::ofstream bulkLog;
    bulkLog.open(nameOfFile.c_str());
    bulkLog << data;
    bulkLog.close();
}

void Registrator::printSummary(int nblocks, int ncommand)
{    
    std::cout << "thread - " << std::this_thread::get_id()
              << " " << nblocks << " blocks, " << ncommand << " commands" << std::endl;
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
