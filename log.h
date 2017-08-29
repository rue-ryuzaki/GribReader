#ifndef LOG_H
#define LOG_H

#include <QString>

class Log {
public:
    Log();
    void Add(QString s);
private:
    QString fileName;
};

#endif // LOG_H
