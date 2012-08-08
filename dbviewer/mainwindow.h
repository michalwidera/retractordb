#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <qlistwidget>
#include <qtablewidget>
#include <qtextedit>
#include <qtimer>
#include "qdebugstream.h"
#include "StdAfx.h"
#include "CRSMath.h"
#include "Processor.h"

namespace Ui
{
    class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    //QListWidget *list;
    QTextEdit *text;
    QDebugStream *stream;
    QTableWidget *table;
    QTableWidget *table2;
    QTimer *timer;
    CRationalStreamMath::TimeLine *pTl;
    QS::Processor *proc ;

public slots:
    void addQuery();
    void stopP();
    void startP();
    void loadQuery();
    void updateIdle();
};

#endif // MAINWINDOW_H
