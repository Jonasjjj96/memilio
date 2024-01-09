#include "memilio/io/history.h"
#include <iostream>
#include <vector>

// We define a Logger that logs the current input multiplied by 2. For that we have to return the input * 2.
struct MyLogger : mio::LogAlways {
    using Type = int;
    static Type log(const int& input)
    {
        return input * 2;
    }
};

int main()
{
    // We construct our History object to store data defined by the Logger MyLogger. To specify how we write the data we use the Writer DataWriterToMemory.
    // That Writer writes the data to a a tuple of std::vector<Logger::Type>, in this case std::tuple<std::vector<int>> as we use MyLogger.
    mio::History<mio::DataWriterToMemory, MyLogger> history;

    // For example purposes we "log" the numbers 0-9. As we multiply it by 2 in the logger, the History stores 0,2,4,... .
    for (int i = 0; i < 10; ++i) {
        history.log(i); 
    }

    // If we want to to access the logged data we can use the get_log() function. 
    // As the History object stores a std::vector<Logger::Type> in a std::tuple for 
    // each logger, we have to use std::get<0>(history.get_log()) to access the data of the first (and only) logger which contains the records of MyLogger.
    std::vector<int> logData = std::get<0>(history.get_log()); 

    // Finally we can print the logged data.
    std::cout << "Logged data from MyLogger:" << std::endl;
    for (const auto& data : logData) {
        std::cout << data << std::endl;
    }

    return 0;
}