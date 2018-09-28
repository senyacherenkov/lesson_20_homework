#include <iostream>
#include <string>
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

                            std::string data;
                            int size;
                            long time;

                            while (!m_isStopped) {

                                std::unique_lock<std::mutex> lck{m_queueMutex};

                                m_condition.wait(lck, [this](){ return m_isStopped && !m_tasks.empty(); });

                                if (m_isStopped)
                                    break;

                                std::tie(data, size, time) = m_tasks.front();
                                m_tasks.pop();

                                lck.unlock();
                                blocksNumber++;
                                commandsNumber += size;

                                writeFileLog(data, time);
                            }

                            while(!m_tasks.empty()) {
                                std::unique_lock<std::mutex> lck{m_queueMutex};
                                std::tie(data, size, time) = m_tasks.front();
                                m_tasks.pop();
                                lck.unlock();
                                writeFileLog(data, time);
                            }

                            printSummary(blocksNumber, commandsNumber);
                        });
    m_workers.emplace_back(
                [this]
                    {
                        thread_local int blocksNumber = 0;
                        thread_local int commandsNumber = 0;

                        std::string data;
                        int size;
                        long time;

                        while (!m_isStopped) {

                            std::unique_lock<std::mutex> lck{m_queueMutex};

                            m_condition.wait(lck, [this](){ return !(!m_isStopped && m_tasks.empty()); });

                            if (m_isStopped)
                                break;

                            std::tie(data, size, time) = m_tasks.front();
                            m_tasks.pop();

                            lck.unlock();
                            blocksNumber++;
                            commandsNumber += size;

                            writeStdOuput(data);
                        }

                        while(!m_tasks.empty()) {
                            std::tie(data, size, time) = m_tasks.front();
                            m_tasks.pop();

                            writeStdOuput(data);
                        }
                    });
}

Registrator::~Registrator()
{}


std::string Registrator::prepareData(const std::vector<std::string>& newCommands) const
{
    std::string output;

    output.append("bulk: ");

    for(const auto& command: newCommands) {
        output.append(command);
        if(command != newCommands[newCommands.size() - 1])
            output.append(", ");
    }
    return output;
}

void Registrator::writeStdOuput(std::string& data)
{
    std::cout << data << std::endl;
}

void Registrator::writeFileLog(std::string& data, long time)
{
    thread_local std::string nameOfFile("bulk");
    std::hash<std::thread::id> h;
    nameOfFile.append(std::to_string(time));
    nameOfFile.append(std::to_string(h(std::this_thread::get_id())));
    nameOfFile.append(".log");

    std::unique_lock<std::mutex> lck{m_queueMutex};
    std::ofstream bulkLog;
    bulkLog.open(nameOfFile.c_str());
    bulkLog << data;
    bulkLog.close();
}

void Registrator::printSummary(int nblocks, int ncommand)
{
    std::unique_lock<std::mutex> lck{m_queueMutex};
    std::cout << std::this_thread::get_id() << " " << nblocks << " blocks, " << ncommand << " commands" << std::endl;
}

void Registrator::update(const std::vector<std::string> &newCommands, long time)
{
    if(newCommands.empty())
        m_isStopped = true;

    std::lock_guard<std::mutex> guard(m_queueMutex);
    std::string output = prepareData(newCommands);

    m_tasks.push(std::make_tuple(output, newCommands.size(), time));
    m_condition.notify_all();
}

Observer::~Observer()
{}
