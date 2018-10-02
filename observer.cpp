#include <iostream>
#include <string>
#include <algorithm>
#include "observer.h"

namespace  {
    struct unique {
      int m_current = 0;
      int operator()() {return ++m_current;}
    } UniqueNumber;
}

Registrator::Registrator():
    m_isStopped(false)
{
    std::generate(m_loadBuffer.begin(), m_loadBuffer.end(), UniqueNumber);

    for(size_t i = 0; i < 2; i++)
        m_workers.emplace_back(
                    [this]
                        {
                            thread_local int blocksNumber = 0;
                            thread_local int commandsNumber = 0;

                            while (!m_isStopped) {

                                std::unique_lock<std::mutex> lck{m_queueMutex};

                                while (!m_isStopped && m_fileLogQueue.empty())
                                    m_condition.wait(lck);

                                if (m_isStopped)
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
                                std::unique_lock<std::mutex> lck{m_queueMutex};
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
    {
        std::unique_lock<std::mutex> lck(m_queueMutex);
        m_isStopped = true;
    }

    m_condition.notify_all();
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

    output.append("bulk: ");

    for(const auto& command: newCommands) {
        output.append(command);
        if(command != newCommands[newCommands.size() - 1])
            output.append(", ");
    }
    return output;
}

void Registrator::writeStdOuput()
{
    thread_local int blocksNumber = 0;
    thread_local int commandsNumber = 0;

    while (!m_isStopped) {

        std::unique_lock<std::mutex> lck{m_queueMutex};

        while (!m_isStopped && m_stdOutQueue.empty())
            m_condition.wait(lck);

        if (m_isStopped)
            break;

        std::random_shuffle(m_loadBuffer.begin(), m_loadBuffer.end());
        auto pair = m_stdOutQueue.front();
        m_stdOutQueue.pop();

        std::cout << pair.first << std::endl;

        lck.unlock();

        blocksNumber++;
        commandsNumber += pair.second;
    }

    while(!m_stdOutQueue.empty()) {
        std::unique_lock<std::mutex> lck{m_queueMutex};
        auto pair = m_stdOutQueue.front();
        m_stdOutQueue.pop();

        std::cout << pair.first << std::endl;
        lck.unlock();

        blocksNumber++;
        commandsNumber += pair.second;
    }

    printSummary(blocksNumber, commandsNumber);
}

void Registrator::writeFileLog(std::string data, long time)
{
    std::unique_lock<std::mutex> lck{m_queueMutex};

    std::string nameOfFile("bulk");
    std::hash<std::thread::id> h;
    nameOfFile.append(std::to_string(time));
    nameOfFile.append("_");
    nameOfFile.append(std::to_string(h(std::this_thread::get_id())));
    nameOfFile.append(".log");

    std::ofstream bulkLog;
    bulkLog.open(nameOfFile.c_str());
    bulkLog << data;
    bulkLog.close();
}

void Registrator::printSummary(int nblocks, int ncommand)
{
    std::unique_lock<std::mutex> lck{m_queueMutex};
    std::cout << "thread - " << std::this_thread::get_id()
              << " " << nblocks << " blocks, " << ncommand << " commands" << std::endl;
}

void Registrator::update(const std::vector<std::string> &newCommands, long time)
{
    if(newCommands.empty()) {
        std::unique_lock<std::mutex> lck(m_queueMutex);
        m_isStopped = true;
        m_condition.notify_all();
        return;
    }

    std::lock_guard<std::mutex> guard(m_queueMutex);
    std::string output = prepareData(newCommands);
    size_t size = newCommands.size();

    auto logTask = [output, time, size, this]() -> size_t {
                                                               writeFileLog(output, time);
                                                               return size;
                                                          };

    m_stdOutQueue.push(std::make_pair(output, size));
    m_fileLogQueue.push(logTask);

    m_condition.notify_all();
}

Observer::~Observer()
{}
