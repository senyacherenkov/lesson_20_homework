#include <algorithm>
#include <sstream>
#include "subject.h"
#include "utility.h"

namespace  {
    decltype(seconds_t().count()) get_seconds_since_epoch()
    {
        // get the current time
        const auto now     = std::chrono::system_clock::now();

        // transform the time into a duration since the epoch
        const auto epoch   = now.time_since_epoch();

        // cast the duration into seconds
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);

        // return the number of seconds
        return seconds.count();
    }
}

Subject::~Subject()
{}

void Subject::addObserver(std::shared_ptr<Observer> observer)
{
    m_observers.push_back(observer);
}

void Subject::removeObserver(std::shared_ptr<Observer> observer)
{
    auto it = std::find(m_observers.begin(), m_observers.end(), observer);

    if(it != m_observers.end())
        m_observers.erase(it);
}

ThreadData Reader::readCommands(std::string testInput)
{
    bool dynamicMode = false;
    int openBracketNumber = 0;
    int closeBracketNumber = 0;
    std::string temp;


#ifdef TEST_MODE
    if(!testInput.empty()) {
        temp = testInput;
#else
    while (true) {
        std::getline(std::cin, temp);
#endif

        if(!temp.empty())
        {
            m_threadData.m_nStrings++;
            if(m_commands.empty())
                m_timeOfFirstCommand = get_seconds_since_epoch();

            if(temp == "}") {
                closeBracketNumber++;
                if(dynamicMode && !m_commands.empty() && (closeBracketNumber == openBracketNumber)) {
                    m_threadData.m_nBlocks++;
                    m_threadData.m_nCommands += m_commands.size();
                    notifyObservers();
                    m_commands.clear();
                    dynamicMode = false;
                }
                continue;
            }
            if(temp == "{") {
                openBracketNumber++;
                if(!dynamicMode) {
                    m_commands.clear();
                    dynamicMode = true;
                }
                continue;
            }

            m_commands.push_back(temp);
            if(!dynamicMode && m_commands.size() == m_N) {
               notifyObservers();
               m_threadData.m_nBlocks++;
               m_threadData.m_nCommands += m_commands.size();
               m_commands.clear();
            }
        }
        else if(!m_commands.empty())
        {
            m_threadData.m_nBlocks++;
            m_threadData.m_nCommands += m_commands.size();
            notifyObservers();
            m_commands.clear();
            break;
        }
        else
        {            
            notifyObservers();
            break;
        }
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "main " << m_threadData;
    return m_threadData;
}

void Reader::notifyObservers()
{
    for(const auto& observer: m_observers)
        observer->update(m_commands, m_timeOfFirstCommand);
}


