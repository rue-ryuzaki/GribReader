#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QMap>
#include <QVector>
#include <string>
#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->lineEdit->setText(currDir);
    // даты прогноза
    ui->spinBoxStep->setValue(stepForecast);
    // исходя из тестовых данных, можно изменить через ui
    minTime.setDate(QDate(2014, 6, 24));
    minTime.setTime(QTime(18, 0));
    maxTime.setDate(QDate(2014, 6, 25));
    maxTime.setTime(QTime(0, 0));
    //
    ui->dateTimeEditFrom->setDateTime(minTime);
    connect(ui->dateTimeEditFrom, SIGNAL(editingFinished()), this,  SLOT(changeDateFrom()));
    ui->dateTimeEditTo->setDateTime(maxTime);
    connect(ui->dateTimeEditTo, SIGNAL(editingFinished()), this,  SLOT(changeDateTo()));
    // заблаговременности
    ui->spinBoxEFrom->setValue(minEarliness);
    ui->spinBoxETo->setValue(maxEarliness);
    ui->spinBoxEStep->setValue(stepEarliness);

    ui->labelCount->setText(QString::number(errorCount));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::changeDateFrom() {
    minTime = ui->dateTimeEditFrom->dateTime();
}

void MainWindow::changeDateTo() {
    maxTime = ui->dateTimeEditTo->dateTime();
}

void MainWindow::selectDirectory() {
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    if (dialog.exec()) {
        currDir = dialog.selectedFiles()[0];
    }
    ui->lineEdit->setText(currDir);
}

void MainWindow::startAnalize() {
    ui->listWidget->clear();
    errorCount = 0;
    stepForecast = ui->spinBoxStep->value();
    // Проверка введенных дат
    QString qfrom = "";
    QString qto = "";
    QString qminEar = "";
    QString qmaxEar = "";
    minEarliness = ui->spinBoxEFrom->value();
    maxEarliness = ui->spinBoxETo->value();
    stepEarliness = ui->spinBoxEStep->value();
    {
        std::string sfrom;
        sfrom.resize(12);
        sprintf(&sfrom[0], "%4.4d%2.2d%2.2d%2.2d%2.2d", minTime.date().year(), minTime.date().month(),
                minTime.date().day(), minTime.time().hour(), minTime.time().minute());
        qfrom = QString::fromStdString(sfrom);
        std::string sto;
        sto.resize(12);
        sprintf(&sto[0], "%4.4d%2.2d%2.2d%2.2d%2.2d", maxTime.date().year(), maxTime.date().month(),
                maxTime.date().day(), maxTime.time().hour(), maxTime.time().minute());
        qto = QString::fromStdString(sto);
        if (qto < qfrom) {
            QMessageBox::warning(this, "Ошибка", "Некорректные временные границы прогноза");
            ui->labelCount->setText(QString::number(errorCount));
            return;
        }
        if (maxEarliness < minEarliness) {
            QMessageBox::warning(this, "Ошибка", "Некорректные границы заблаговременностей");
            ui->labelCount->setText(QString::number(errorCount));
            return;
        }
        std::string sminEar;
        sminEar.resize(3);
        sprintf(&sminEar[0], "%3.3d", minEarliness);
        qminEar = QString::fromStdString(sminEar);
        std::string smaxEar;
        smaxEar.resize(3);
        sprintf(&smaxEar[0], "%3.3d", maxEarliness);
        qmaxEar = QString::fromStdString(smaxEar);
    }
    QString dir = currDir;
    QDir myDir(dir);
    if (dir == "") {
        QMessageBox::warning(this, "Ошибка", "Пустой путь к каталогу, поиск в каталоге с программой");
    } else {
        dir += "/";
    }
    Log log;

    QStringList files = myDir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    if (files.size() == 0) {
        QMessageBox::warning(this, "Ошибка", "Директория пуста");
        ui->labelCount->setText(QString::number(errorCount));
        return;
    }
    log.Add(tr("%1 files in directory %2").arg(files.size()).arg(dir));
    // M< дата, V<з. часы + мин> >
    QMap<QString, QVector<QString> > qmap;
    for (QString & path : files) {
        QRegExp regExp("(.)*([0-9]{12}).([0-9]{3})H([0-9]{2})M(.)*");
        if (regExp.exactMatch(path)) {
            if (regExp.cap(2) < qfrom || regExp.cap(2) > qto) {
                QString error =  "Файл " + path + ": Некорректные временные границы";
                log.Add(error);
                ui->listWidget->addItem(error);
                ++errorCount;
                continue;
            }
            if (regExp.cap(3) < qminEar || regExp.cap(3) > qmaxEar) {
                QString error =  "Файл " + path + ": Некорректное значение часов заблаговременности";
                log.Add(error);
                ui->listWidget->addItem(error);
                ++errorCount;
                continue;
            }
            if (regExp.cap(3) == qmaxEar && regExp.cap(4) != "00") {
                QString error =  "Файл " + path + ": Некорректное значение заблаговременности";
                log.Add(error);
                ui->listWidget->addItem(error);
                ++errorCount;
                continue;
            }
            if (regExp.cap(4) < "00" || regExp.cap(4) > "60") {
                QString error =  "Файл " + path + ": Некорректное значение минут заблаговременности";
                log.Add(error);
                ui->listWidget->addItem(error);
                ++errorCount;
                continue;
            }
            /*if (regExp.cap(4) != "00") { // ожидаемое значение минут заблаговременности = "00"
                QString error =  "Файл " + path + ": Некорректное значение минут заблаговременности";
                log.Add(error);
                ui->listWidget->addItem(error);
                ++errorCount;
                continue;
            }*/
            QStringList errors;
            if (!checkGrib(dir + path, regExp.cap(2), regExp.cap(3), regExp.cap(4), errors)) {
                for (QString & e : errors) {
                    QString error = "Файл " + path + ": " + e;
                    log.Add(error);
                    ui->listWidget->addItem(error);
                    ++errorCount;
                }
            } /* else */ { //
                /*  Можно не добавлять файлы с некорректным содержанием в qmap,
                    но тогда при проверке на наличие файлов прогноза и заблаговременности
                    будут выдано несоответствие об отсутствии этих файлов.
                    Чтобы не дублировать несоответствия, будут добавлены все файлы. */
                if (!qmap.contains(regExp.cap(2))) {
                    QVector<QString> vec;
                    qmap.insert(regExp.cap(2), vec);
                }
                qmap[regExp.cap(2)].push_back(regExp.cap(3) + regExp.cap(4));
            }
        } else {
            QString error = "Некорректное имя файла: " + path;
            log.Add(error);
            ui->listWidget->addItem(error);
            ++errorCount;
        }
    }
    if (qmap.size() == 0) {
        ui->labelCount->setText(QString::number(errorCount));
        return;
    }
    // Проверка на отсутствие исходя из начала, конца и интервала
    {
        quint64 step = stepForecast * 60 * 60 * 1000;
        // если разница 6 часов = 360 минут = 21600 секунд = 21600000 мсекунд
        for (quint64 i = minTime.toMSecsSinceEpoch(); i <= maxTime.toMSecsSinceEpoch(); i += step) {
            QDateTime t = QDateTime::fromMSecsSinceEpoch(i);
            std::string stime;
            stime.resize(12);
            sprintf(&stime[0], "%4.4d%2.2d%2.2d%2.2d%2.2d", t.date().year(), t.date().month(),
                    t.date().day(), t.time().hour(), t.time().minute());
            QString qtime = QString::fromStdString(stime);
            if (!qmap.contains(qtime)) {
                QString error = "Пропущен прогноз " + qtime;
                log.Add(error);
                ui->listWidget->addItem(error);
                ++errorCount;
            }
        }
    }
    // проверка заблаговременности
    for (auto it = qmap.begin(); it != qmap.end(); ++it) {
        QStringList hourlist = it->toList();
        for (int i = minEarliness * 60; i <= maxEarliness * 60; i += stepEarliness) {
            int ihour = i / 60;
            int imin  = i % 60;
            std::string s;
            s.resize(5);
            sprintf(&s[0], "%3.3d%2.2d", ihour, imin);
            QString q = QString::fromStdString(s);
            if (!hourlist.contains(q)) {
                QString error = ((QString)"Прогноз %1: Пропущен файл с заблаговременностью %2 ч %3 мин.").arg(it.key()).arg(ihour).arg(imin);
                log.Add(error);
                ui->listWidget->addItem(error);
                ++errorCount;
            }
        }
    }
    ui->labelCount->setText(QString::number(errorCount));
}
