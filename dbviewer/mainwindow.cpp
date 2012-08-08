#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qinputdialog>
#include <QFileDialog>
#include <qdir>
#include <iostream>
#include <QMessageBox>

#include "QStruct.h"

#include "IPhStrMgr.h"
#include "Processor.h"

#include <boost/archive/text_iarchive.hpp>

#include "windows.h"

using namespace CRationalStreamMath ;
using namespace QS ;

extern "C" {
    qTree coreInstance ;
    config configInstance ;    
}

enum limits { 
    limitToDeclarations, 
    limitToQueries , 
    noLimits 
};

bool bBreakOnLimits( int dProgSize , limits eLimit )
{
    const bool bStreamIsDecl( dProgSize == 0 );

    if ( eLimit == limitToDeclarations )
        if ( ! bStreamIsDecl )
            return true ;

    if ( eLimit == limitToQueries )
        if ( bStreamIsDecl )
            return true ;

    return false ;
}

set< R > getListFromCore( limits eLimit = noLimits )
{
    set< R > lstTimeIntervals ;

    BOOST_FOREACH( query it , coreInstance )
    {
        if ( bBreakOnLimits( it.lProgram.size() , eLimit ) )
            continue ;

        lstTimeIntervals.insert( it.rInterval );
    }

    return lstTimeIntervals ;
}

void dumpCore( std::ostream & xout , limits eLimit = noLimits )
{
    xout << "In core:" << endl ;

    xout << "Seqence\tInterval\tQuery id" << endl ;

    BOOST_FOREACH( const query & it , coreInstance )
    {
        if ( bBreakOnLimits( it.lProgram.size() , eLimit ) )
            continue ;

        xout << getSeqNr( it.id ) << "\t";
        xout << it.rInterval << "\t";
        xout << it.id  ;
        xout << endl ;
    }
}

set < string > getAwaitedStreamsSet(  TimeLine & tl , limits eLimit = noLimits )
{
    set < string > retVal ;

    BOOST_FOREACH( const query & it , coreInstance )
    {
        if ( bBreakOnLimits( it.lProgram.size() , eLimit ) )
            continue ;

        if ( ! tl.isThisDeltaAwaitCurrentTimeSlot( it.rInterval ) )
            continue ;

        retVal.insert( it.id );
    }

    return retVal ;
}

void showAwaitedStreams( TimeLine & tl , limits eLimit = noLimits )
{
    set< string > strSet = getAwaitedStreamsSet( tl , eLimit );
    BOOST_FOREACH( const string & str , strSet )
            std::cout << "-" << getSeqNr(str) << "-" ;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) ,
    pTl(NULL),
    proc(NULL)
{
    ui->setupUi(this);
    //list = qFindChild<QListWidget*>(this, "listWidget");
    text = qFindChild<QTextEdit*>( this , "textEdit");
    table= qFindChild<QTableWidget*>( this , "tableWidget");
    table2= qFindChild<QTableWidget*>( this , "tableWidget_2");

    timer = new QTimer(this);

    stream = new QDebugStream(std::cout, text );

    connect(timer, SIGNAL(timeout()), this, SLOT(updateIdle()));
    timer->start(1000);
}

R prev_interval(0);
int xRow(0);

void MainWindow::updateIdle()
{
    if ( pTl )
    {
        R nextTimeSlot( pTl->getNextTimeSlot() );
        R interval( nextTimeSlot * 1000 );

        int ms ( rational_cast<int> ( interval - prev_interval ) ) ; // miliseconds
        prev_interval = interval ;

        //std::cout << "ms:" << ms << endl ;
        timer->setInterval( ms );

        try {
            table2->insertRow( xRow ) ;

            BOOST_FOREACH( const query & it , coreInstance )
            {
                if ( ! pTl->isThisDeltaAwaitCurrentTimeSlot( it.rInterval ) )
                    continue ;

                table2->setItem( xRow  , getSeqNr( it.id ) + 1 , new QTableWidgetItem( QString( it.id.c_str() ) ) ) ;
                table2->item( xRow , getSeqNr( it.id ) + 1 )->setBackground( Qt::green )  ;
            }

             table2->setItem( xRow  , 0 , new QTableWidgetItem( QString( tr("%1").arg(ms) ) ) ) ;

            table2->setCurrentCell( xRow , 0 );
            xRow ++ ;


            //showAwaitedStreams( *pTl , noLimits );
            set < string > inSet = getAwaitedStreamsSet( *pTl , noLimits );
            if ( proc ) proc->processRows( inSet  );           
        }
        catch(std::exception& e)
        {
            std::cout  << e.what() << endl ;
        }
    }

    //int x( rand() % 1000 ) ;
    //std::cout << "update:" << x << endl ;
    //QApplication::beep();
}

MainWindow::~MainWindow()
{
    if ( pTl  ) delete pTl;
    if ( proc ) delete proc;
    if ( timer ) delete timer;
    if ( stream ) delete stream;
    delete ui;
}

void MainWindow::stopP()
{
    std::cout << "Process stop" << std::endl;
    timer->stop();
}

void MainWindow::startP()
{
    std::cout << "Process resume" << std::endl;
    timer->start();
}

void MainWindow::addQuery()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                         tr("User name:"), QLineEdit::Normal,
                                         QDir::home().dirName(), &ok);
    //list->addItem( text );
    //if (ok && !text.isEmpty())
    //    textLabel->setText(text);
}

void MainWindow::loadQuery()
{
    QString text = QFileDialog::getOpenFileName
            (this,tr("Open qry file"),tr("."),tr("Compiled query (*.qry)"));

    if ( text != "" )
    {
        QByteArray   bytes  = text.toAscii();
        const char * ptr    = bytes.data();

        std::ifstream ifs( ptr , std::ios::binary);

        if (!ifs)
        {   //Bez tego paskudny error idzie z boost
            QMessageBox::critical(
                 this,
                 tr("dbviewer"),
                 tr("No file") );
            return ;
        }

        boost::archive::text_iarchive ia(ifs);
        ia >> configInstance ;
        ia >> coreInstance ;

        dumpCore( std::cout , noLimits );

        int xCol(0);
        table2->insertColumn( xCol ++ );
        BOOST_FOREACH( const query & it , coreInstance )
        {
            table->insertRow( getSeqNr( it.id ) ) ;
            table->setItem( getSeqNr( it.id ) , 0 , new QTableWidgetItem( tr("%1").arg( getSeqNr( it.id ) ) ) );
            table->setItem( getSeqNr( it.id ) , 1 , new QTableWidgetItem( QString( it.id.c_str() ) ) ) ;
            table->setItem( getSeqNr( it.id ) , 2 , new QTableWidgetItem( tr("%1/%2").arg(  it.rInterval.numerator() ).arg( it.rInterval.denominator() ) ) )  ;

            table2->insertColumn( xCol ++ );
        }

        set< R > deltaset = getListFromCore( noLimits );
        if ( deltaset.size() > 1 )
        {
            pTl = new TimeLine( deltaset );
            proc = new Processor() ;
        }
    }
}


