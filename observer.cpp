#include <iostream>
#include <string>
#include "observer.h"

void Registrator::update(const std::vector<std::string> &newCommands, long time)
{
    std::string output;

    //writing to stdout
    output.append("bulk: ");

    for(const auto& command: newCommands) {        
        output.append(command);
        if(command != newCommands[newCommands.size() - 1])
            output.append(", ");
    }

    std::cout << output << std::endl;

    //writing to file
    std::string nameOfFile("bulk");
    nameOfFile.append(std::to_string(time));
    nameOfFile.append(".log");

    m_bulkLog.open(nameOfFile.c_str());
    m_bulkLog << output;
    m_bulkLog.close();
}

Observer::~Observer()
{}
