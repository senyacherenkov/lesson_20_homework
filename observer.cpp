#include <iostream>
#include <string>
#include "observer.h"

Registrator::Registrator():
    m_isStopped(false)
{
    for(size_t i = 0; i < 3; i++)
        m_workers.emplace_back(
                    [this]
                        {
                            thread_local int blocksNumber = 0;
                            thread_local int commandsNumber = 0;

                            while (!m_isStopped) {

                                std::unique_lock<std::mutex> lck{m_queueMutex};

                                m_condition.wait(lck, [this](){ return m_isStopped && !m_tasks.empty(); });

                                if (m_isStopped)
                                    break;

                                auto task = m_tasks.front();
                                m_tasks.pop();

                                std::cout << blocksNumber << " " << std::this_thread::get_id() << std::endl;
                                std::cout << commandsNumber << " " << std::this_thread::get_id() << std::endl;
                                lck.unlock();

                                blocksNumber++;
                                std::cout << blocksNumber << " " << std::this_thread::get_id() << std::endl;

                                commandsNumber += task();
                                std::cout << commandsNumber << " " << std::this_thread::get_id() << std::endl;
                            }

                            while(!m_tasks.empty()) {
                                std::unique_lock<std::mutex> lck{m_queueMutex};
                                auto task = m_tasks.front();
                                m_tasks.pop();
                                std::cout << blocksNumber << " " << std::this_thread::get_id() << std::endl;
                                std::cout << commandsNumber << " " << std::this_thread::get_id() << std::endl;
                                lck.unlock();

                                blocksNumber++;
                                std::cout << blocksNumber << " " << std::this_thread::get_id() << std::endl;

                                commandsNumber += task();
                                std::cout << commandsNumber << " " << std::this_thread::get_id() << std::endl;
                            }

                            printSummary(blocksNumber, commandsNumber);
                        });
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

void Registrator::writeStdOuput(std::string data)
{
    std::unique_lock<std::mutex> lck{m_queueMutex};
    std::cout << data << std::endl;    
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
    if(newCommands.empty())
        m_isStopped = true;

    std::lock_guard<std::mutex> guard(m_queueMutex);
    std::string output = prepareData(newCommands);
    size_t size = newCommands.size();

    auto logTask = [output, time, size, this]() -> size_t {
                                                               writeFileLog(output, time);
                                                               return size;
                                                          };

    auto stdTask = [output, size, this]() -> size_t {
                                                        writeStdOuput(output);
                                                        return size;
                                                    };
    m_tasks.push(logTask);
    m_tasks.push(logTask);
    m_tasks.push(stdTask);
    m_condition.notify_all();
}

Observer::~Observer()
{}
