#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include "log.h"
#include "gribchecker.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void changeDateFrom();
    void changeDateTo();
    void selectDirectory();
    void startAnalize();

private:
    Ui::MainWindow *ui;

    QDateTime minTime;
    QDateTime maxTime;

    int stepForecast  = 6; // hours

    int minEarliness  = 0;  // hours
    int maxEarliness  = 60; // hours
    int stepEarliness = 60; // min
    QString currDir = "";

    int errorCount = 0;
};

#endif // MAINWINDOW_H
