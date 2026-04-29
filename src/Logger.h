#ifndef LOGGER_H
#define LOGGER_H

#include "Observer.h"
#include <string>
#include <fstream>

class Logger : public Observer {
public:
    explicit Logger(const std::string& logFile);
    void update(const std::string& message) override;

private:
    std::string m_logFile;
    std::ofstream m_logStream;
};

#endif // LOGGER_H
