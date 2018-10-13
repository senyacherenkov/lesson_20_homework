#include "utility.h"

std::ostream& operator << (std::ostream& out, ThreadData const& data){
    if(data.m_nStrings != 0)
        out << data.m_nStrings << " strings, ";
    out << data.m_nCommands << " commands, "
        << data.m_nBlocks << " blocks" << std::endl;
    return out;
}
