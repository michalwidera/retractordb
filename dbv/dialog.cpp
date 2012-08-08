#include "dialog.h"
#include "ui_dialog.h"

//http://doc.qt.nokia.com/4.7/graphicsview.html

QPen gpen(Qt::green, 1, Qt::DotLine, Qt::FlatCap, Qt::RoundJoin);
QPen rpen(Qt::red, 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
QPen bpen(Qt::blue, 1, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
QBrush bbr( Qt::blue );
const int csize(500);

int prevx(0);
int prevy(csize/2);

QGraphicsLineItem *table[csize];

QGraphicsEllipseItem *pLeader = NULL ;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    timer(NULL)
{
    ui->setupUi(this);

    scene = new QGraphicsScene();

    view = qFindChild<QGraphicsView*>( this , "graphicsView");
    view->setScene( scene );

//    view->setGeometry( 0 , 0 , csize , csize );
//    this->adjustSize();

    for ( int i = 0 ; i <= csize ; i += 10 )
    {
        scene->addLine( 0 , i , csize  , i  , gpen );
        scene->addLine( i , 0 , i  ,  csize , gpen );
    }

    for ( int i = 0 ; i < csize ; i ++ )
        table[i] = 0;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateIdle()));
    timer->start(10);
}


void Dialog::updateIdle()
{
    //QApplication::beep();
    int x = prevx + 1 ;
    int y = ( prevy + ( 20 - rand() % 40  ) ) ;
    if ( y < 0 ) y = 0 ;
    if ( y > csize ) y = csize ;

    if ( table[x] != 0 ) scene->removeItem( (QGraphicsItem*)table[x] );
    table[x] =  scene->addLine( prevx , prevy , x  , y  , rpen );
    prevx = x ;
    prevy = y ;

    if ( pLeader ) scene->removeItem( (QGraphicsItem*) pLeader );
    pLeader = scene->addEllipse( x , y , 10 , 10 , bpen , bbr );

    if ( x > csize ) { prevx = 0 ; }
}

Dialog::~Dialog()
{
    delete ui;
    if ( timer ) delete timer;
    if ( scene ) delete scene;
}
