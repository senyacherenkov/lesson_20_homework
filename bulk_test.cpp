#include "observer.h"
#include "subject.h"

#define BOOST_TEST_MODULE test_main
#define TEST_MODE

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_suite_main)

std::vector<std::string> input {"cmd1", "cmd2", "cmd3", "cmd4", "cmd5", "cmd6", "cmd7", ""};

BOOST_AUTO_TEST_CASE(check_proper_sorting)
{
    constexpr size_t N = 3;
    {
        Reader readerOfCommands(N);

        readerOfCommands.addObserver(std::make_shared<Registrator>());
        for(const auto& cmd: input)
            readerOfCommands.readCommands(cmd);
    }


}

BOOST_AUTO_TEST_SUITE_END()
