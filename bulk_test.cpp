#include "observer.h"
#include "subject.h"
#include <fstream>

#define BOOST_TEST_MODULE test_main


#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_suite_main)

static std::vector<std::string> input {
                                       "{",
                                       "cmd1",
                                       "cmd2",
                                       "{",
                                       "cmd3",
                                       "cmd4",
                                        "}",
                                       "cmd5",
                                       "cmd6",
                                       "}",
                                       ""};

static std::string output {"bulk: cmd1, cmd2, cmd3, cmd4, cmd5, cmd6"};

BOOST_AUTO_TEST_CASE(check_proper_sorting)
{
    constexpr int NUMBER_BLOCKS     = 1;
    constexpr int NUMBER_COMMANDS   = 6;

    constexpr int THREAD_FILELOG1_NUMBER   = 0;
    constexpr int THREAD_FILELOG2_NUMBER   = 1;
    constexpr int THREAD_STDOUT_NUMBER     = 2;

    constexpr int AMOUNT_OF_FILE           = 1;

    {
        ThreadData threadResult;
        Reader readerOfCommands(NUMBER_BLOCKS);

        std::shared_ptr<Registrator> registrator = std::make_shared<Registrator>();
        readerOfCommands.addObserver(registrator);
        for(const auto& cmd: input)
            threadResult = readerOfCommands.readCommands(cmd);

        BOOST_CHECK_MESSAGE( threadResult.m_nBlocks == NUMBER_BLOCKS, "main thread has wrong number of blocks" );
        BOOST_CHECK_MESSAGE( threadResult.m_nStrings == static_cast<int>(input.size() - 1), "main thread has wrong number of strings");
        BOOST_CHECK_MESSAGE( threadResult.m_nCommands == NUMBER_COMMANDS, "main thread has wrong number of commands");

        std::vector<ThreadData>& data = registrator->getThreadData();
        BOOST_CHECK_MESSAGE( data[THREAD_STDOUT_NUMBER].m_nBlocks == NUMBER_BLOCKS,  "stdout thread has wrong number of blocks");
        BOOST_CHECK_MESSAGE( data[THREAD_STDOUT_NUMBER].m_nCommands == NUMBER_COMMANDS, "stdout thread has wrong number of commands");

        BOOST_CHECK_MESSAGE( data[THREAD_FILELOG1_NUMBER].m_nBlocks == NUMBER_BLOCKS ||
                             data[THREAD_FILELOG2_NUMBER].m_nBlocks == NUMBER_BLOCKS, "filelog threads works wrong. Number of commands is incorrect");
        BOOST_CHECK_MESSAGE( data[THREAD_FILELOG1_NUMBER].m_nCommands == NUMBER_COMMANDS ||
                             data[THREAD_FILELOG2_NUMBER].m_nCommands == NUMBER_COMMANDS, "filelog threads works wrong. Number of commands is incorrect");

        std::vector<std::string>& filenames = registrator->getFileNames();
        BOOST_CHECK_MESSAGE(filenames.size() == AMOUNT_OF_FILE, "wrong amount of files");

        std::ifstream fileStream;
        fileStream.open(filenames.front());
        std::string outputData;
        if (fileStream.is_open())
        {
            while (!fileStream.eof())
                std::getline(fileStream, outputData);
            BOOST_CHECK_MESSAGE(outputData == output, "wrong data in file " << filenames.front());
            fileStream.close();
        }
        else
            BOOST_MESSAGE("CANNOT OPEN FILE");

    }
}

BOOST_AUTO_TEST_SUITE_END()
