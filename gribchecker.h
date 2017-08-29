#ifndef GRIBCHECKER_H
#define GRIBCHECKER_H

#include <stdio.h>
#include <stdlib.h>
#include <QString>
#include <QStringList>

bool checkGrib(QString path, QString date, QString ahour, QString amin, QStringList & errors);
int read_grib(FILE *file, long pos, long len_grib, unsigned char *buffer);
unsigned char *seek_grib(FILE *file, unsigned long *pos, long *len_grib,
        unsigned char *buffer, unsigned int buf_len);
long echack(FILE *file, long pos, long len_grib);

#endif // GRIBCHECKER_H
