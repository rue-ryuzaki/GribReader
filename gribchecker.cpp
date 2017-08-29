#include "gribchecker.h"
#include <iostream>
#include <string>
#include "bds.h"
#include "bms.h"
#include "gds.h"
#include "pds4.h"

#define LEN_HEADER_PDS (28+8)
#define __LEN24(pds)  ((pds) == NULL ? 0 : (int) ((pds[0]<<16)+(pds[1]<<8)+pds[2]))
#define NMC     7
#define ECMW    98
#define DWD     78
#define CMC     54
#define CPTEC   46
#define CHM     146
#define LAMI    200

bool checkGrib(QString path, QString date, QString ahour, QString amin, QStringList & errors) {
    FILE *input;
    if ((input = fopen(path.toStdString().c_str(),"rb")) == NULL) {
        fprintf(stderr, "could not open file %s\n", path.toStdString().c_str());
        return false;
    }
    unsigned char *buffer = new unsigned char[40000];

    long unsigned pos = 0;
    long int len_grib, buffer_size, count = 1;
    unsigned char *msg, *pds, *gds, *bms, *bds, *pointer;
    bool ok = true;
    // check values
    int checkUGRD = 0, checkVGRD = 0, checkPRES = 0;
    //UGRD & VGRD in 10 m above gnd | PRES in 0 m above MSL
    while (true) {
        msg = seek_grib(input, &pos, &len_grib, buffer, 1024);
        if (msg == NULL) {
            break;
        }

        if (len_grib + msg - buffer > 40000) {
            buffer_size = len_grib + msg - buffer + 1000;
            buffer = (unsigned char *) realloc((void *) buffer, buffer_size);
            if (buffer == NULL) {
                fprintf(stderr, "ran out of memory\n");
                return false;
            }
        }

        if (read_grib(input, pos, len_grib, buffer) == 0) {
            fprintf(stderr, "error, could not read to end of record\n");
            return false;
        }
        // parse
        msg = buffer;
        pds = (msg + 8);
        pointer = pds + PDS_LEN(pds);
        if (PDS_HAS_GDS(pds)) {
            gds = pointer;
            pointer += GDS_LEN(gds);
        } else {
            gds = NULL;
        }
        if (PDS_HAS_BMS(pds)) {
            bms = pointer;
            pointer += BMS_LEN(bms);
        } else {
            bms = NULL;
        }
        bds = pointer;
        pointer += BDS_LEN(bds);
        if (pointer-msg+4 != len_grib) {
            fprintf(stderr, "Len of grib message is inconsistent.\n");
        }
        /* end section - "7777" in ascii */
        if (pointer[0] != 0x37 || pointer[1] != 0x37 ||
            pointer[2] != 0x37 || pointer[3] != 0x37) {
            fprintf(stderr,"\n\n    missing end section\n");
            fprintf(stderr, "%2x %2x %2x %2x\n", pointer[0], pointer[1],
                pointer[2], pointer[3]);
            return false;
        }
        // check
        {
            // check date
            int year, month, day, hour, min;
            year  = PDS_Year4(pds);
            month = PDS_Month(pds);
            day   = PDS_Day(pds);
            hour  = PDS_Hour(pds);
            min   = PDS_Minute(pds);
            std::string sdate;
            sdate.resize(12);
            sprintf(&sdate[0], "%4.4d%2.2d%2.2d%2.2d%2.2d", year, month, day, hour, min);

            if (date != QString::fromStdString(sdate)) {
                errors << "несоответствие даты в записи " + QString::number(count) + ": " + date + " и " + QString::fromStdString(sdate);
                ok = false;
            }
            // check p1 p2
            int p1, p2;
            p1 = PDS_P1(pds);
            p2 = PDS_P2(pds);

            std::string sp1, sp2;
            sp1.resize(3);// hour
            sprintf(&sp1[0], "%3.3d", p1);
            sp2.resize(2);// min
            sprintf(&sp2[0], "%2.2d", p2);

            if (ahour != QString::fromStdString(sp1)) {
                errors << "несоответствие заблаговременности в часах в записи " + QString::number(count) + ": " + ahour + " и " + QString::fromStdString(sp1);
                ok = false;
            }
            if (amin != QString::fromStdString(sp2)) {
                errors << "несоответствие заблаговременности в минутах в записи " + QString::number(count) + ": " + amin + " и " + QString::fromStdString(sp2);
                ok = false;
            }
        }
        // check 2
        {
            int val = PDS_PARAM(pds);
            // param info
            // 33 = UGRD, u wind [m/s]
            // 34 = VGRD, v wind [m/s]
            // 1  = PRES, Pressure [Pa]
            int kpds6 = PDS_KPDS6(pds);
            int kpds7 = PDS_KPDS7(pds);
            // kpds6 info
            // 103 == kpds7 m above MSL
            // 105 == kpds7 m above gnd
            // so for Pressure -> kpds6 = 103 - 0 m above MSL  (kpds7 = 0)
            //    for wind     -> kpds6 = 105 - 10 m above gnd (kpds7 = 10)
            switch (val) {
                case 1:  // PRES
                    if (kpds6 == 103 && kpds7 == 0) {
                        ++checkPRES;
                    } else {
                        errors << ((QString)"неверный формат атм.давления %1 вместо 103, высота %2 вместо 0").arg(kpds6).arg(kpds7);
                        ok = false;
                    }
                    break;
                case 33: // UGRD
                    if (kpds6 == 105 && kpds7 == 10) {
                        ++checkUGRD;
                    } else {
                        errors << ((QString)"неверный формат скорости ветра %1 вместо 105, высота %2 вместо 10").arg(kpds6).arg(kpds7);
                        ok = false;
                    }
                    break;
                case 34: // VGRD
                    if (kpds6 == 105 && kpds7 == 10) {
                        ++checkVGRD;
                    } else {
                        errors << ((QString)"неверный формат скорости ветра %1 вместо 105, высота %2 вместо 10").arg(kpds6).arg(kpds7);
                        ok = false;
                    }
                    break;
                default:
                    // в текущей проверке ничего не делать
                    break;
            }
        }
        pos += len_grib;
        ++count;
    }
    fclose(input);
    --count;
    if (count != 3) {
        errors << QString::number(count) + " записей найдено (ожидалось 3)";
        ok = false;
    }
    if (checkUGRD != 1 && checkVGRD != 1 && checkPRES != 1) {
        errors << ((QString)"неверно определены записи по атм.давлению (%1 вместо 1) и скорости ветра (%2 и %3 вместо 1 и 1)").arg(checkPRES).arg(checkVGRD).arg(checkUGRD);
        ok = false;
    }
    return ok;
}

int read_grib(FILE *file, long pos, long len_grib, unsigned char *buffer) {
    int i;

    if (fseek(file, pos, SEEK_SET) == -1) {
        return 0;
    }

    i = fread(buffer, sizeof (unsigned char), len_grib, file);
    return (i == len_grib);
}

unsigned char *seek_grib(FILE *file, unsigned long *pos, long *len_grib,
        unsigned char *buffer, unsigned int buf_len) {
    int i, j, len;
    static int warn_grib2 = 0;
    long length_grib;

    j = 1;
    clearerr(file);
    while (!feof(file)) {
        if (fseek(file, *pos, SEEK_SET) == -1) break;
        i = fread(buffer, sizeof (unsigned char), buf_len, file);
        if (ferror(file)) break;
        len = i - LEN_HEADER_PDS;

        for (i = 0; i < len; ++i) {
            if (buffer[i] == 'G' && buffer[i+1] == 'R' && buffer[i+2] == 'I'
                && buffer[i+3] == 'B') {
                /* grib edition 1 */
                if (buffer[i+7] == 1) {
                    *pos = i + *pos;
                    *len_grib = length_grib = (buffer[i+4] << 16) + (buffer[i+5] << 8) +
                            buffer[i+6];

                    /* small records don't have ECMWF hack */
                    if ((length_grib & 0x800000) == 0) { return (buffer + i); }

                    /* potential for ECMWF hack */
                    *len_grib = echack(file, *pos, length_grib);
                    return (buffer+i);
                } /* grib edition 2 */ else if (buffer[i+7] == 2) {
                    if (warn_grib2++ < 5) fprintf(stderr,"grib2 message ignored (use wgrib2)\n");
                }
            }
        }

        if (j++ == 100) {
            fprintf(stderr,"found unidentified data \n");
            /* break; // stop seeking after NTRY records */
        }
        *pos = *pos + (buf_len - LEN_HEADER_PDS);
    }

    *len_grib = 0;
    return (unsigned char *) NULL;
}

/* If the encoded grib record length is long enough, we may have an encoding
   of an even longer record length using the ecmwf hack.  To check for this
   requires getting the length of the binary data section.  To get this requires
   getting the lengths of the various sections before the bds.  To see if those
   sections are there requires checking the flags in the pds.  */

long echack(FILE *file, long pos, long len_grib) {
    int gdsflg, bmsflg, center;
    unsigned int pdslen, gdslen, bmslen, bdslen;
    unsigned char buf[8];
    long len = len_grib;

    /* Get pdslen */

    if (fseek(file, pos+8, SEEK_SET) == -1) return 0;
    if (fread(buf, sizeof (unsigned char), 8, file) != 8) return 0;
    pdslen = __LEN24(buf);

    center = buf[4];

    /* know that NCEP and CMC do not use echack */
    if (center == NMC || center == CMC) {
        return len_grib;
    }

    gdsflg = buf[7] & 128;
    bmsflg = buf[7] & 64;

    gdslen=0;
    if (gdsflg) {
        if (fseek(file, pos+8+pdslen, SEEK_SET) == -1) return 0;
        if (fread(buf, sizeof (unsigned char), 3, file) != 3) return 0;
        gdslen = __LEN24(buf);
    }

    /* if there, get length of bms */

    bmslen = 0;
    if (bmsflg) {
        if (fseek(file, pos+8+pdslen+gdslen, SEEK_SET) == -1) return 0;
        if (fread(buf, sizeof (unsigned char), 3, file) != 3) return 0;
        bmslen = __LEN24(buf);
    }

    /* get bds length */

    if (fseek(file, pos+8+pdslen+gdslen+bmslen, SEEK_SET) == -1) return 0;
    if (fread(buf, sizeof (unsigned char), 3, file) != 3) return 0;
    bdslen = __LEN24(buf);

    /* Now we can check if this record is hacked */

    if (bdslen >= 120) {
        /* normal record */
    } else {
        /* ECMWF hack */
        len_grib = (len & 0x7fffff) * 120 - bdslen + 4;
        //len_ec_bds = len_grib - (12 + pdslen + gdslen + bmslen);
    }
    return len_grib;
}
