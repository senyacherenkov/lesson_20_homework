#include <string>
#include "observer.h"
#include "subject.h"


int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        std::cerr << "There should be one argument!" << std::endl;
        exit(1);
    }
    {
        std::size_t numberOfCommands = static_cast<std::size_t>(std::stoi(argv[1]));
        Reader readerOfCommands(numberOfCommands);

        readerOfCommands.addObserver(std::make_shared<Registrator>());
        readerOfCommands.readCommands();
    }

    return 0;
}
