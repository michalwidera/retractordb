/*Copyright 2010-2014 Streams Lab Michal Widera

  RESTRICTED RIGHTS LEGEND
  Use, duplication or disclosure by any commercial industry (public or
  private), private individual or by any Government Agency without an
  expressed written consent of release from Streams Lab Michal Widera
  is subject to restriction of the Rights.

  Manufacturer is:
      Streams Lab Michal Widera
      Reymonta str. 56/1
      Poland, Zabrze, 41-800
      (+48) 606 995 918
*/
#include "stdafx.h"

#include "QStruct.h"
#include "dbstream.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "Buffer.h"

using namespace QS ;

extern "C" {
qTree coreInstance ;
config configInstance ;
}

void dumpStreamData( std::ostream & xout ,
                     string filename )
{
  set < string > queryset ;
  BOOST_FOREACH ( query q , coreInstance )
  {
    queryset.insert( q.id );
  }

  CBuffer<std::string> cbuf ;
  cbuf.Load( filename.c_str() );
  cbuf.Dump( xout , & queryset ) ;

  xout << endl ;

  BOOST_FOREACH ( const query & q , coreInstance )
  {
    xout << q.id << endl ;

    shared_array< char > pRawData ;
    pRawData.reset( new char[ cbuf.GetSizeOfFrame( q.id ) ] );

    int dLen ( cbuf.GetLen( q.id ) ) ;
    while( dLen -- )
      {
        cbuf.GetBlock( q.id , dLen , pRawData ) ;

        int fieldCount( cbuf.GetSizeOfFrame( q.id ) / sizeof( UR::varfield ) );

        while ( fieldCount -- )
          {
            xout << UR::getFromRawData( pRawData , fieldCount ) << ( ( fieldCount == 0 ) ? "" : "," ) ;
          }

        xout << endl ;
      }
    xout << endl ;
  }
}

//creating jpg cmd line sequence:
//
//1. dumper.exe out_p.qry --dot >out.dot
//2. dot -Tjpg out.dot -o file.jpg
//3. start file.jgp
//
void dumpGraphiz( std::ostream & xout ,
		  bool bShowFileds ,
		  bool bShowStreamProgs ,
		  bool bShowTags ,
		  bool bShowFieldTypes)
{
  //dot call commandline: dot -Tjpg filewithgraph.txt -o file.jpg
  xout << "digraph structs {" << endl ;
  xout << " node\t[shape=record];" << endl ;
  xout << "" ;

  set <string> planStreamRelationsSet;

  BOOST_FOREACH ( query q , coreInstance )
  {
    //presentation of streams
    xout << " " ;
    xout << q.id << "\t" ;
    xout << "[shape=record,label=\"" ;
    xout << q.id ;
    xout << "|" ;

    xout << "interval=" ;
    xout << q.rInterval ;

    xout << "\\nschem size=" ;
    xout << (int)q.lSchema.size() ;

    xout << "\\nprog size=" ;
    xout << (int)q.lProgram.size() ;

    // end stream specific

    //fields in stream
    if ( bShowFileds )
      {
        xout << "|" ;
        xout << "{" ;
        int dTag( 0 );

        bool isFirst( false );

        BOOST_FOREACH ( field f , q.lSchema )
        {
          if ( isFirst )
            {
              isFirst = false ;
            }
          else
            {
              xout << "|" ;
            }

          if ( bShowTags )
            {
              xout << "<tag" << dTag ++ << ">" ;
            }

          //
          //Patch on gramma problem -
          //dot program is using { as important sign - we need to convert { to <
          //
          std::string sFieldName ( f.getFieldNameSet() ) ;
          std::replace( sFieldName.begin() , sFieldName.end() , '{' , '/' );
          std::replace( sFieldName.begin() , sFieldName.end() , '}' , '/' );

          if ( bShowFieldTypes )
            {
              xout << "{" ;
            }

          xout << sFieldName ;

          if ( bShowFieldTypes )
            switch ( f.dFieldType )
              {
              case field::BAD :
                xout << "|BAD}";
                break ;
              case field::BYTE :
                xout << "|BYTE}";
                break ;
              case field::INTEGER:
                xout << "|INT}";
                break ;
              case field::RATIONAL:
                xout << "|RAT}";
                break ;
              case field::CRATIONAL:
                xout << "|CRT}";
                break ;
              default:
                throw std::invalid_argument("niezdefiniowany typ w polu");
              }
        }
        xout << "}" ;
      } //if ( bShowFileds ) - end of fields in stream

    // stream specific

    xout << "\"]" ;
    xout << endl ;

    if ( bShowStreamProgs )
      {
        xout << " prg_" ;
        xout << q.id << "\t" ;
        xout << "[shape=box,style=filled," ;

        if ( q.isDeclaration() )
          {   //deklaracja
            xout << "fillcolor=Sienna,color=Black," ;
            xout << "label=\"Declaration" ;
            if ( ! q.filename.empty() )
              {
                xout << "\\nfile=";
                xout << q.filename ;
              }
          }
        else if ( q.isReductionRequired() )
          {   //wymagajacy redukcji
            xout << "fillcolor=Green,color=Black," ;
            xout << "label=\"";
          }
        else if ( q.isGenerated() )
          {
            xout << "fillcolor=Yellow,color=Black," ;
            xout << "label=\"";
          }
        else
          {   //zwykly strumien
            xout << "label=\"";
          }
        BOOST_FOREACH ( token t , q.lProgram )
        {
          xout << t.getStrTokenName() << " " ;
          xout << t.getValue() ;
          xout << "\\n" ;
        }
        xout << "\"]" ;
        xout << endl ;

        //Relation stream plan to stream
        string relation( q.id + " -> "  + "prg_" + q.id  ) ;
        planStreamRelationsSet.insert( relation );
      }
  }
  BOOST_FOREACH( string s , planStreamRelationsSet )
      xout << s << endl;

  set <string> streamRelationsSet; //dzieki tej zmiennej eliminujemy powtorzenia relacji na wykresie

  //programy opisujace budowe pol strumienia
  if ( bShowTags )
    {
      BOOST_FOREACH ( query q , coreInstance )
      {
        int dTag( 0 ) ;

        BOOST_FOREACH ( field f , q.lSchema )
        {
          xout << " " ;
          xout << q.id << "_tag" << dTag << "\t" ;
          xout << "[shape=record,label=\"" ;

          std::string sFieldName ( f.getFieldNameSet() ) ;
          std::replace( sFieldName.begin() , sFieldName.end() , '{' , '/' );
          std::replace( sFieldName.begin() , sFieldName.end() , '}' , '/' );

          xout << sFieldName ;

          xout << "|" ;
          xout << "{" ;

          bool isFirst(true);

          BOOST_FOREACH ( token t , f.lProgram )
          {
            isFirst ? isFirst = false : xout << "|" ;

            std::string sTokenName ( t.getStrTokenName() ) ;
            std::replace( sTokenName.begin() , sTokenName.end() , '{' , '/' );
            std::replace( sTokenName.begin() , sTokenName.end() , '}' , '/' );

            xout << sTokenName ;

            //Tokeny PUSH_ cos tam maja zawsze wartosc
            //cala reszta nie powinna prezentowac wartosc
            std::basic_string <char>::size_type idx = sTokenName.find ( "PUSH_" );
            if ( idx != string::npos )
              {
                xout << " " ;
                xout << t.getValue() ;

                //bo po kompilacji znika schema[1,2] na schema i crvalue
                if ( sTokenName == "PUSH_ID" )
                  {
                    xout << " " << t.getCRValue() ;
                  }
              }
          }

          xout << "}" ;

          xout << "\"" ;
          xout << "]" ;
          xout << endl ;

          //Relation1
          //zamiast xout - po to zeby nie powstawaly podwojne relacje
          //set jest odsiewa
          string relation(
                q.id + ":" + "tag" + lexical_cast<string>(dTag) +
                " -> " +
                q.id + "_tag" + lexical_cast<string>(dTag) +
                " [style=dotted]" );

          streamRelationsSet.insert( relation );

          ++ dTag ;
        }
      }
    }

  //relacje pomiedzy strumieniami

  BOOST_FOREACH ( query q , coreInstance )
  {
    BOOST_FOREACH ( token t , q.lProgram )
    {
      if ( t.getStrTokenName() == "PUSH_STREAM" )
        {
          string relation( q.id + " -> " + t.getValue() ) ;

          if ( bShowStreamProgs )
            {
              relation = "prg_" + relation ;
            }

          streamRelationsSet.insert( relation );
        }
    }
  }
  BOOST_FOREACH( string s , streamRelationsSet )
  {
    xout << s << endl;
  }


  xout << "}" << endl ;
}

void dumpQFieldsProgram()
{
  cout << endl ;
  cout << "fcnt_ref\tid_ref\ttoken\tvalue" << endl ;

  BOOST_FOREACH ( query q , coreInstance )
  {
    BOOST_FOREACH ( field f , q.lSchema )
    {
      BOOST_FOREACH ( token t , f.lProgram )
      {
        cout << q.id  << "\t" ;
        cout << f.getFieldNameSet()  << "\t" ;
        cout << t.getStrTokenName() << "\t" ;
        cout << t.getValue() ;
        if ( t.getStrTokenName() == "PUSH_ID" )
          {
            cout << " " << t.getCRValue() ;
          }
        cout << endl ;
      }
    }
  }
}

void dumpQFields()
{
  cout << endl ;
  cout << "fcnt\tid_ref\tfName" << endl ;

  BOOST_FOREACH ( query q , coreInstance )
  {
    int loccnt(0);

    BOOST_FOREACH ( field f , q.lSchema )
    {
      cout << ++ loccnt << "\t" ;
      cout << q.id  << "\t" ;
      cout << f.getFieldNameSet() << "\t" ;
      cout << endl ;
    }
  }
}

void dumpQPrograms()
{
  cout << endl ;
  cout << "qcnt\tid_ref\ttoken\tvalue" << endl ;

  BOOST_FOREACH ( query q , coreInstance )
  {
    int loccnt(0);

    BOOST_FOREACH ( token t , q.lProgram )
    {
      cout << ++ loccnt << "\t" ;
      cout << q.id << "\t" ;
      cout << t.getStrTokenName() << "\t" ;
      cout << t.getValue() ;
      cout << endl ;
    }
  }
}

void dumpQSet()
{
  cout << endl ;
  cout << "id\tlen prg\tlen sch\tinterval\tfilename" << endl ;

  BOOST_FOREACH ( query q , coreInstance )
  {
    cout << q.id << "\t" ;
    cout << (int)q.lProgram.size() << "\t" ;
    cout << (int)q.lSchema.size() << "\t" ;
    cout << q.rInterval << "\t" ;
    cout << q.filename << "\t" ;
    cout << endl ;
  }
}

void dumpQHierarchy()
{
  BOOST_FOREACH ( query q , coreInstance )
  {
    cout << q.id << "(" << q.rInterval << ")" << "\t"  ;
    cout << q.filename << "\t" ;
    //cout << " len:" << q.getLen() << endl ;
    cout << endl ;
    BOOST_FOREACH ( token t , q.lProgram )
    {
      cout << "\t:- " << t.getStrTokenName() << "(" << t.getValue() << ")" << endl ;
    }
    BOOST_FOREACH ( field f , q.lSchema )
    {
      cout << "\t" ;
      BOOST_FOREACH ( string s , f.setFieldName )
      {
        cout << s << ":" ;
      }
      cout << endl ;
      BOOST_FOREACH ( token tf , f.lProgram )
          if ( tf.getStrTokenName() == "PUSH_ID" )
      {
        cout << "\t\t"
             << tf.getStrTokenName()
             << "("
             << tf.getValue()
             << " "
             << tf.getCRValue()
             << ")"
             << endl ;
      }
      else
      {
        cout << "\t\t"
             << tf.getStrTokenName()
             << "("
             << tf.getValue()
             << ")"
             << endl ;
      }
    }
  }
}

int main(int argc, char* argv[])
{
  //Intro
  cerr << "Dumper - decoder of qry files.\n";
  cerr << "Tag kompilatora:" + string(COMPILER) +
          ", CTS:" << string( __TIMESTAMP__) <<	//http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx
          std::endl ;
  cerr << "Boost wersja:" << BOOST_LIB_VERSION << std::endl ;

  try {
    namespace po = boost::program_options;

    string sInputFile ;

    po::options_description desc("Obslugiwane opcje programu");
    desc.add_options()
        ("help,h", "opcje programu")
        ("dot,d", "stworz plik dot")
        ("csv,c", "stworz csv")
        ("view,v", "stworz plik dot, a nastepenie wywolaj dot.exe")
        ("fields,f", "pokaz pola w dot")
        ("tags,t", "pokaz tagi")
        ("streamprogs,s", "pokaz programy strumieniowe w dot")
        ("sdump,p", "przyjmij jako plik wejsciowy jako zrzut z executora")
        ("leavedot,e" , "nie usuwaj tymczasowego pliku dot")
        ("showtypes,w" , "pokaz typy pol w pliku dot")
        ("infile,i", po::value<string>(&sInputFile)->default_value("query.qry"), "nazwa pliku z danymi")
        ;

    po::positional_options_description p;       //Assume that infile is the first option
    p.add("infile", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
      {
        cerr << desc << endl ;
        return 1;
      }

    string sPlikDanych = vm["infile"].as< string >() ;

    cout << "Input :" << sPlikDanych << endl ;

    //
    // Main Algorithm body
    //

    std::ifstream ifs( sPlikDanych.c_str() , std::ios::binary);
    boost::archive::text_iarchive ia(ifs);
    ia >> configInstance ;
    ia >> coreInstance ;

    if ( vm.count("sdump") )
      {
        string sPlikDanychDump( sPlikDanych ) ;

        boost::cmatch what;
        boost::regex xprFileNameExtDmp("(\\w*).qry");
        if ( regex_match(sPlikDanych.c_str(),what,xprFileNameExtDmp) )
          {
            const string filename( what[1] );
            sPlikDanychDump = filename + ".dmp" ;
          }

        cerr << "Dump  :" << sPlikDanychDump << endl ;

        dumpStreamData( cout , sPlikDanychDump ) ;
      }
    else if ( vm.count("view") )
      {
        const std::string sTempDotFile( "temp_$$$.dot" );
        //const std::string sTempJpgFile( "temp_$$$.jpg" );
        const std::string sTempPngFile( "temp_$$$.png" );
        {
          std::ofstream ofs(sTempDotFile.c_str());
          dumpGraphiz( ofs ,
                       vm.count("fields") != 0 ,
                       vm.count("streamprogs") != 0 ,
                       vm.count("tags") != 0 ,
                       vm.count("showtypes") != 0
              );
        }
        std::system( std::string( "dot -Tpng " + sTempDotFile + " -o " + sTempPngFile ).c_str() );
        if ( ! vm.count("leavedot") )
          {
            std::remove( sTempDotFile.c_str() );
          }
        cerr << "type: start " << sTempPngFile << endl ;
        //std::system( std::string( "start " +  sTempJpgFile ).c_str() );
      }
    else if (vm.count("dot"))
      {
        dumpGraphiz( cout ,
                     vm.count("fields") != 0 ,
                     vm.count("streamprogs") != 0 ,
                     vm.count("tags") != 0 ,
                     vm.count("showtypes") != 0
            );
      }
    else if (vm.count("csv"))
      {
        cerr << "Core count:" << (int)coreInstance.size() << endl ;
        dumpQSet() ;
        dumpQPrograms() ;
        dumpQFields() ;
        dumpQFieldsProgram() ;
      }
    else
      {
        dumpQHierarchy() ;
      }
  }
  catch(std::exception& e)
  {
    cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
