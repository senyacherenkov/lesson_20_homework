#pragma once
#include <iostream>

struct ThreadData {
    int m_nStrings = 0;
    int m_nBlocks = 0;
    int m_nCommands = 0;
};

std::ostream& operator << (std::ostream& out, ThreadData const& data);
