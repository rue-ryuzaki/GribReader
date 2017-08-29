#include "log.h"
//#include <QDateTime>
#include <QFile>

//disabled

Log::Log() {
    //fileName = QDateTime::currentDateTime().toString(Qt::ISODate) + ".log";
    fileName = "out.log";
/*
    QFile file(fileName);
    file.open(QIODevice::ReadWrite);
*/
}

void Log::Add(QString s) {
/*
    QFile file(fileName);
    if (file.open(QIODevice::Append)) {
        file.write(s.toStdString().c_str());
        file.write("\n");
    }
*/
}
