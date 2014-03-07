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

//
//Wlaczenie kodu odpowiedzialnego za wielodostep do strumieni danych
//

#define BOOST_LOCK

#include "Buffer.h"
#include "RandomFile.h"

#ifdef BOOST_LOCK
#include <boost/thread/recursive_mutex.hpp>
using namespace boost ;
#define LOCKBYMUTEX recursive_mutex::scoped_lock scoped_lock_instance(lock_)
#define DECLARMUTEX recursive_mutex lock_
#endif

#ifdef ZTHREAD_LOCK
#include <zthread/Mutex.h>
#include <zthread/Guard.h>
#define LOCKBYMUTEX ZThread::Guard<ZThread::Mutex> g(lock)
#define DECLARMUTEX ZThread::Mutex lock
#endif

#ifndef LOCKBYMUTEX
#define LOCKBYMUTEX
#endif

#ifndef DECLARMUTEX
#define DECLARMUTEX
#endif

const int max_packet_size=1024 ;

/** Macrodefinicja - deklaracja mutexu - obiektu na ktorym bedzie odchodzilo blokowanie */
DECLARMUTEX ;

#ifdef WIN32
#pragma warning(disable:4355)       //warning C4355: 'this' : used in base member initializer list
#endif

using namespace std ;

using boost::shared_array ;

/** Interface contract for Block class type */
class IBlock
{
  void operator= (const IBlock&);   //prevent from copying this way

public:

  boost::crc_32_type result ;

  long frameSize ;    /**< Szerokosc ramki w bajtach */
  long frameCount ;   /**< Ilosc ramek w bloku */
  long firstFrame ;   /**< Wskaznik pierwszej ramki w bloku (pro forma w celu uzgodnienia typow z CycBlock) */

  IBlock ( long frameSize = 0 )
    : frameSize(frameSize) , frameCount(0) , firstFrame(0)
  {}
  IBlock (const IBlock&);           //allow copying
  virtual ~IBlock() {}  ;

  void PutBlock( arrayPointer_t pBlockBuffer , long count ) ;
  long GetBlock( long position , arrayPointer_t pBlockBuffer ) ;
  long GetByteSize() ;
  void Save( std::ostream& strout ) ;
  void Load( std::istream& strin ) ;
  void Trunc() ;
  long SyncToDisk() ;
} ;

/**
	 * Klasa wzorcowa - atrybutem jest strumien danych
	 * STREAM - moze przyjac wartosc CBlock<CRandomFile>,CBlock<strstream>
	 * Patrz tez klasa cyklicznych blokow
	 * @see CCycBlock
	 * @short Klasa obslugujaca bloki - pojedynczy strumien w murze
	 */
template <class STREAM>
class CBlock : public IBlock
{
  boost::scoped_ptr< STREAM > pImpl ;   /**< Wskaznik na zmienna typu strumieniowego (cin,cout,iostream, itp)*/

public:

  CBlock( const CBlock<STREAM> &x ) ; /**< Konstruktor kopiujacy Uwaga! Z uwagi na operacje kopiowania niszczacego pwartosc - nastepuje konwersja wskaznikow i zniszczenie wartosci przekazywanej. parametr to obiekt do skopiowania */
  CBlock( long frameSize = 0 ) ;      /**< Konstruktor - Tworzy na stercie obiekt typu strzam. - musi byc bez mutexu! bo siadzie parametr frameSize dlugosc ramki */

  virtual ~CBlock() ;                 /**< Destruktor klasy - zabezpieczony przed konflikatmi wielowatkowymi jedna duza sekcja krytyczna */
  void PutBlock( arrayPointer_t pBlockBuffer , long count ) ; /**< Umieszczenie ramki w bloku (wymiar (ilosc bajtow) ustawia sie w konstruktorze) jesli w buforze pBlockBuffer jest wiecej ramek count okresla ta ilosc @param pBlockBuffer wskaznik do bufora @param count ilosc ramek w buforze - w Buffer.h default parametr = 1 */
  long GetBlock( long position , arrayPointer_t pBlockBuffer ) ; /**< Pobranie ramki od dlugosci framesize z bloku (wymiar (ilosc bajtow) ustawia sie w konstruktorze) @param position pozycja do pobrania ze strumienia @param pBlockBuffer wskaznik do bufora @return ilosc odczytanych danych */
  long GetByteSize() ;                /**< Funkcja zwraca ilosc bajtow w bloku @return ilosc bajtow w strumieniu */
  void Save( std::ostream& strout ) ; /**< Funkcja zapisuje blok do wskazanego strumienia (tzn. pliku) - zapisanie powoduje utrwalnie stanu @param strout strumien wyjsciowy */
  void Load( std::istream& strin ) ;  /**< Funkcja odczytuje blok ze wskazanego strumieni (tzn. pliku) @param strin strumien wejsciowy */
  void Trunc() ;                      /**< Obciecie bloku (zerowanie) */
  long SyncToDisk() ;                 /**< Funkcja zrzuca blok na dysk i zwraca ile bylo ramek w trakcie zrzutu @return ilosc ramek w strumieniu w trakcie zrzutu */
};


template < typename BLOCK >
class CRecord : public std::map< long, BLOCK >
{
public:
  typename std::map< long, BLOCK >::iterator lastBlock ;
  typedef std::pair < int, BLOCK > CRec_Pair;
  CRecord() ;
  virtual ~CRecord() ;
  void DefBlock( long blockID , long frameSize ) ;
  void DelBlock( long blockID ) ;
  void PutBlock( long blockID, arrayPointer_t pBlockBuffer , long count ) ;
  long GetBlock( long blockID, long position , arrayPointer_t pBlockBuffer  )	;
  BLOCK & RefBlock( long blockID )	;
  typename CRecord::iterator find( long blockID ) ;
  void Save( std::ostream& stream ) ;
  void Load( std::istream &stream ) ;
};

/**
	*	Klasa wzorcowa - atrybutem jest strumien danych
	*	STREAM - moze przyjac wartosc CCycBlock<CRandomFile>,CCycBlock<strstream>
	*   @short Klasa obslugujaca bloki cykliczne
	*/
template <class STREAM>
class CCycBlock : public IBlock
{
  long lastblockCnt ;			    /**< JIT Support - jesli 0 to znaczy ze trzeba utworzyc, z kazdym insertem do mapy zwiekszany o jeden*/

  typedef std::pair < long,CBlock<STREAM> > blockMapPair ;
  typedef std::map<long,CBlock<STREAM> > blockMapT ;
  blockMapT blockMap ;
  typename blockMapT::iterator itLastBlock ;

public:
  /**	Swiadomy i przemyslany brak konstruktora kopiujacego ! => JIT/Shallow Copy required !*/

  unsigned long slices ;	        /**< Ilosc czesci na jaki jest podzielony blok cykliczny */
  long framesInSlice ;	        /**< Ilosc ramek w kazdym z podblokow */

  /** Konstruktor bloku cyklicznego */
  CCycBlock( long frameSize = 0 );
  CCycBlock( const CCycBlock<STREAM> &x ) ;

  long GetByteSize();             /**< Funkcja zwraca ilosc bajtow w bloku @return ilosc bajtow w strumieniu */
  virtual ~CCycBlock() ;          /**< Destruktor - czysci mape */
  void PutBlock( arrayPointer_t pBlockBuffer , long count ) ;
  long GetBlock( long position , arrayPointer_t pBlockBuffer ) ;
  void Save( std::ostream& strout ) ;
  void Load( std::istream& strin ) ;
  long SyncToDisk() ;             /**< Funkcja zrzuca blok na dysk i zwraca ile bylo ramek w trakcie zrzutu @return ilosc ramek w strumieniu w trakcie zrzutu */
};

/**
	 *  Podstawowa klasa obslugujaca strumienie.
	 *  CBufferImpl zawiera mape blokow
	 *  Bloki identyfikowane sa unikalnymi identyfikatorami (identyfikatory nadaje programista)
	 *  Kazdy z blokow moze byc Plikiem w pamieci, strumieniem dyskowy, Plikiem cyklicznym w pamieci, strumieniem cyklicznym
	 *
	 */
template <class IDTYPE>
class CBufferImpl
{
  typedef std::map < long , BlockType > BMap ;

  /** Zmienne przechowujace wskazniki na CRecord */
  boost::shared_ptr< CRecord<CBlock<CRandomFile> > > recordInFile ;
  boost::shared_ptr< CRecord<CBlock<std::stringstream> > > recordInMemory;
  boost::shared_ptr< CRecord<CCycBlock<CRandomFile> > > recordInCycleFile ;
  boost::shared_ptr< CRecord<CCycBlock<std::stringstream> > > recordInCycleMemory ;
  /** Swiadoma i przemyslana decyzja enkapsulacji mapy */
  BMap blok  ;

  long eval( long BlockID ) ;     /** Funkcje EVAL sluza do dedukowania typu w trakcie kompilacji ta funkcjonje przezroczyscie - w przypadku typu long zwracanay jest ten sam typ */
  long eval( const std::string &BlockName ) ;/**< Funkcje EVAL sluza do dedukowania typu w trakcie kompilacji ta funkcjonje jak funkcja haszujaca	- w systemie nie moga sie pojawic dwa strumienie o identycznych nazwach nie moga sie rowniez pojawic dwa sturmienie o tym samym kluczu haszujacym Klucz haszujacy wykorzystywanyjest do okreslania BlockID w CBufferImpl @retval unikalny dla strumienia klucz haszujacy */

public:

  CBufferImpl();                      /**< Konstruktor - tworzy z kazdego po jednym (tworzenie jest opoznione - tzn. dopoki nie uzyte nic sie nie tworzy*/

  virtual ~CBufferImpl() ;                    /**< Destruktor - assert zbedny/shared_ptr? */
  void Load( const char * filename ) ;    /**< Odczytanie CBufferImpl z pliku */
  void Save( const char * filename ) ;    /**< Zapisanie CBufferImpl do pliku */
  void PutBlock( IDTYPE blockID, arrayPointer_t pBlockBuffer , long count = 1 ) ;  /**< Umieszczenie ramki w zadanym Bloku (przed ta operacja pamietac ze raz trzeba DefBlock lub Load) */
  long GetBlock( IDTYPE blockID, long position, arrayPointer_t pBlockBuffer ) ; /**< Pobranie ramki */
  void DefBlock( IDTYPE blockID, long frameSize , BlockType btType = BF , long aux1 = 0 , long aux2 = 0) ; /**< Zdefiniowanie bloku Jesli BCF - Cykliczny binarny plik to aux1 - ilosc podplikow, aux2 - ilosc ramek w podpliku	Jesli BCM - j.w. else aux1,aux2 musi byc 0 ! */
  void DelBlock( IDTYPE blockID )  ;      /**< Usuniecie bloku */
  long GetLen( IDTYPE blockID )  ;        /**< Zwrocenie ilosci ramek */
  long GetSizeOfFrame( IDTYPE blockID ) ; /**< Zwrocenie dlugosci pojedynczej ramki */
  long GetSizeOfBlock( IDTYPE blockID , long firstFrm=0 , long lastFrm=-1 ) ; /**< Zwrocenie dlugosci w bajtach calego bloku */
  void Clear() ;                          /**< Obciecie - wyczyszczenie - Truncate */
  void DecycleBlockType( IDTYPE blockID , arrayPointer_t pBlockBuffer ) ; /**< Rozpiecie cyklu - zmiana typu - z typu Cyklicznego na typ plaski */
  void MemorizeBlockType( IDTYPE blockID , arrayPointer_t pBlockBuffer ) ; /**< Przpisanie bloku do pamieci */
  long syncToDisk(IDTYPE blockID ) ;      /** Funkcja zatwierdza strumien na dysku @return zwraca dlugosc strumienia w ramkach */
  void Dump( std::ostream &os , std::set < string > * pQset ) ;         /** Do celow testowych i raportowych */
};

//Specjalizacja dla CRandomFile i stringstream
template class CBlock<CRandomFile> ;
template class CBlock<stringstream> ;

template class CCycBlock<CRandomFile> ;
template class CCycBlock<stringstream> ;
//Metaszablon - wybor sposobu blokowania w zaleznosci od wybranej biblioteki

//Specjalizacja dla stringa i long
template class CBufferImpl<std::string> ;
template class CBufferImpl<long> ;

//Specjalizacja dla CBlock<CRandomFile> i CBlock<stringstream>
template class CRecord<CBlock<CRandomFile> > ;
template class CRecord<CBlock<stringstream> >;
//dla cycbloka dalsze specjalizacje
template class CRecord<CCycBlock<CRandomFile> > ;
template class CRecord<CCycBlock<stringstream> >;

template <class IDTYPE>
long CBufferImpl<IDTYPE>::eval( long BlockID )
{
  return BlockID ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::eval( const string &BlockName )
{
  long hashKey(0) ;
  for ( long i = 0 ; i < static_cast<long>( BlockName.length() ) ; i ++ )
    hashKey += ( static_cast<char>( BlockName[i] ) - 48 ) * ( i * 80 + 1 );
  return hashKey ;
}

template <class IDTYPE>
CBufferImpl<IDTYPE>::CBufferImpl()
{
  recordInFile.reset( new CRecord< CBlock<CRandomFile> >() );
  recordInMemory.reset( new CRecord< CBlock<stringstream> >() );

  recordInCycleFile.reset( new CRecord< CCycBlock<CRandomFile> >());
  recordInCycleMemory.reset( new CRecord< CCycBlock<stringstream> >() );
}

/** Destruktor - assert zbedny/shared_ptr?
        */

template <class IDTYPE>
CBufferImpl<IDTYPE>::~CBufferImpl()
{
}

/** META-programowanie */
#define MAP_SWICH( __FUNCT__ , __PREFUNCT__ ) \
  switch ( blok[eval(blockID)] ) { \
  case BF: 	 __PREFUNCT__ (*recordInFile). __FUNCT__  ;  break ; \
  case BM:	 __PREFUNCT__ (*recordInMemory). __FUNCT__  ;  break ; \
  default: assert( false ); \
  case BCF:	 __PREFUNCT__ (*recordInCycleFile). __FUNCT__  ;  break ; \
  case BCM:	 __PREFUNCT__ (*recordInCycleMemory). __FUNCT__  ;  break ;  };

/** META-programowanie */
#define MAP_EACH( __FUNCT__ ) \
  (*recordInFile). __FUNCT__  ; \
  (*recordInMemory). __FUNCT__ ; \
  (*recordInCycleFile). __FUNCT__ ; \
  (*recordInCycleMemory). __FUNCT__ ;

/** Odczytanie CBufferImpl z pliku */
template <class IDTYPE>
void CBufferImpl<IDTYPE>::Load( const char * filename )
{
  std::ifstream file(filename, ios::in|ios::binary);
  if ( ! file ) throw std::out_of_range("Thrown/Buffer.h/Load()/Odczyt sie nie udal");
  //http://www.cplusplus.com/doc/tutorial/tut5-3.html - exception handling

  long sizeOfmap ; //Odczyt mapy
  file.read(  reinterpret_cast<char*>(&sizeOfmap), sizeof sizeOfmap ) ;
  for ( long i = 0 ; i < sizeOfmap ; i ++ )
    {
      long blockID  ;
      BlockType type  ;
      file.read( reinterpret_cast<char*>(&blockID), sizeof blockID ) ;
      file.read( reinterpret_cast<char*>(&type), sizeof type ) ;
      blok[eval(blockID)] = type ;
    }
  MAP_EACH ( Load(file) ) ;
}

/** Zapisanie CBufferImpl do pliku */
template <class IDTYPE>
void CBufferImpl<IDTYPE>::Save( const char * filename )
{
  fstream file(filename, ios::out|ios::binary|ios::trunc);
  if ( ! file ) throw std::out_of_range("Thrown/Buffer.h/Save()/Zapis sie nie powiodl");

  size_t sizeOfmap = blok.size() ; //Nagrywanie mapy
  file.write(  reinterpret_cast<char*>(&sizeOfmap), sizeof sizeOfmap ) ;
  for ( BMap::iterator it = blok.begin() ; it != blok.end() ; it ++ )
    {
      long blockID = (*it).first ;
      BlockType type = (*it).second ;
      file.write(  reinterpret_cast<char*>(&blockID), sizeof blockID ) ;
      file.write(  reinterpret_cast<char*>(&type), sizeof type ) ;
    }

  MAP_EACH ( Save( file ) ) ;
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::PutBlock( IDTYPE blockID, arrayPointer_t pBlockBuffer , long count )
{
  MAP_SWICH ( PutBlock( eval(blockID) , pBlockBuffer , count ) , ; ) ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetBlock( IDTYPE blockID,
                                    long position,
                                    arrayPointer_t pBlockBuffer )
{
  MAP_SWICH ( GetBlock( eval(blockID)  , position, pBlockBuffer ) , return )
      return 0 ; //proforma
}


template <class IDTYPE>
void CBufferImpl<IDTYPE>::DefBlock(
    IDTYPE blockID,
    long frameSize ,
    BlockType btType ,
    long aux1 ,
    long aux2 )
{
  blok[eval(blockID)] = btType ;
  MAP_SWICH ( DefBlock( eval(blockID) , frameSize ) , ; )

      if ( btType == BCF )
  {
    (*recordInCycleFile).RefBlock( eval(blockID) ).slices = aux1 ;
    (*recordInCycleFile).RefBlock( eval(blockID) ).framesInSlice = aux2 ;
  }
  if ( btType == BCM )
    {
      (*recordInCycleMemory).RefBlock( eval(blockID) ).slices = aux1 ;
      (*recordInCycleMemory).RefBlock( eval(blockID) ).framesInSlice = aux2 ;
    }
  if ( btType == BF || btType == BM ) assert ( aux1 == 0 && aux2 == 0 );	//Zwroc uwage na zgodnosc typow
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::DelBlock( IDTYPE blockID )
{
  MAP_SWICH( DelBlock( eval(blockID) ) , ; ) ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetLen( IDTYPE blockID )
{
  MAP_SWICH( RefBlock( eval(blockID) ).frameCount , return );
  return 0 ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetSizeOfFrame( IDTYPE blockID )
{
  MAP_SWICH( RefBlock( eval(blockID) ).frameSize , return );
  return 0 ;
}

/** Zwrocenie dlugosci w bajtach calego bloku */
template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetSizeOfBlock( IDTYPE blockID , long firstFrm , long lastFrm )
{
  MAP_SWICH( RefBlock( eval(blockID) ).GetByteSize() , return );
  return 0 ;
}

template <class IDTYPE>
void CBufferImpl< IDTYPE>::Clear()
{
  recordInFile.reset( new CRecord< CBlock<CRandomFile> >() );
  recordInMemory.reset( new CRecord< CBlock<stringstream> >() );
  blok.clear();
}

#define META_PROG( TYPE1 , TYPE2 ,  FIRST , SECOND ) \
  if ( blok[eval(blockID)] == TYPE1 ) {\
  SECOND.DefBlock( eval(blockID) , FIRST.RefBlock( eval(blockID) ).frameSize ) ;\
  for ( int i = FIRST.RefBlock( eval(blockID) ).firstFrame ; \
  i < FIRST.RefBlock( eval(blockID) ).frameCount ; \
  i ++ ) {\
  FIRST.GetBlock( eval(blockID) , i , pBlockBuffer  );\
  SECOND.PutBlock( eval(blockID) , pBlockBuffer , 1 );\
  }\
  FIRST.DelBlock( eval(blockID) );\
  blok[eval(blockID)] = TYPE2 ;\
  }

template <class IDTYPE>
void CBufferImpl<IDTYPE>::DecycleBlockType( IDTYPE blockID , arrayPointer_t pBlockBuffer )
{
  if ( blok[eval(blockID)] == BF ) return ;
  if ( blok[eval(blockID)] == BM ) return ;
  META_PROG( BCF , BF , (*recordInCycleFile) , (*recordInFile) );
  META_PROG( BCM , BM , (*recordInCycleMemory) , (*recordInMemory) );
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::MemorizeBlockType( IDTYPE blockID , arrayPointer_t pBlockBuffer )
{
  if ( blok[eval(blockID)] == BCM ) return ;
  if ( blok[eval(blockID)] == BM ) return ;
  META_PROG( BCF , BCM , (*recordInCycleFile) , (*recordInCycleMemory) );
  META_PROG( BF  , BM , (*recordInFile) , (*recordInMemory) );
}

#undef META_PROG

template < typename IDTYPE >
long CBufferImpl<IDTYPE>::syncToDisk(IDTYPE blockID )
{
  MAP_SWICH( RefBlock( eval(blockID) ).SyncToDisk() , return );
  return 0 ;
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::Dump( ostream &os ,  set < string > * pQset )
{
  os << "Count\tFSize\tLoc\tBlockID" << std::endl ;
  for ( BMap::iterator it = blok.begin() ; it != blok.end() ; it ++ )
    {
      switch ( (*it).second )
        {
        case BF: 	os << (*recordInFile).RefBlock( (*it).first ).frameCount ;
          os << "\t" ;
          os << (*recordInFile).RefBlock( (*it).first ).frameSize ;
          os << "\t" ;
          os <<  "File" ;
          break ;
        case BM:	os << (*recordInMemory).RefBlock( (*it).first ).frameCount ;
          os << "\t" ;
          os << (*recordInMemory).RefBlock( (*it).first ).frameSize ;
          os << "\t" ;
          os << "Memory" ;
          break ;
        case BCF:	os << (*recordInCycleFile).RefBlock( (*it).first ).frameCount ;
          os << "\t" ;
          os << (*recordInCycleFile).RefBlock( (*it).first ).frameSize ;
          os << "\t" ;
          os << "Cycle File" ;
          break ;
        case BCM:	os << (*recordInCycleMemory).RefBlock( (*it).first ).frameCount ;
          os << "\t" ;
          os << (*recordInCycleMemory).RefBlock( (*it).first ).frameSize ;
          os << "\t" ;
          os << "Cycle Memory" ;
          break ;
        default: assert( false );
        }

      os << "\t" ;
      if ( pQset == NULL )
        os << (*it).first ;
      else
        BOOST_FOREACH( string qid , * pQset )
            if ( eval( qid ) ==  (*it).first )
            os << qid ;

      os << std::endl ;
    }
}

#undef MAP_SWICH
#undef MAP_EACH

template <class STREAM>
CBlock<STREAM>::CBlock( const CBlock<STREAM> &x )
{
  //	LOCKBYMUTEX ;

  pImpl.reset( new STREAM );
  frameSize = x.frameSize ;
  frameCount = x.frameCount ;
  firstFrame = x.firstFrame ;

  assert( frameCount == 0 ) ;
  //Jak to padnie to znaczy ze proba kopiowania wlaczonego bloku

  //Strumien ma zawsze tylko jednego wlascicela - kopiowanie tylko <=
  //WARNINNG - THERE IS HAPPEN SOMETHING NASTY
  CBlock<STREAM> * y = const_cast<CBlock<STREAM> * >(&x) ;
  y->pImpl.reset() ;
}

template <class STREAM>
CBlock<STREAM>::CBlock( long frameSize ) :
  pImpl( new STREAM )
{
  IBlock::frameSize = frameSize ;
  frameCount = 0 ;
  firstFrame = 0 ;
}

template <class STREAM>
CBlock<STREAM>::~CBlock()
{
  //	LOCKBYMUTEX ;
  frameCount = 0 ;
  firstFrame = 0 ;
}

template <class STREAM>
void CBlock<STREAM>::PutBlock( arrayPointer_t pBlockBuffer , long count )
{
  assert( pBlockBuffer.get() != NULL ) ;

  LOCKBYMUTEX ;

  pImpl->seekp(0,STREAM::end);
  pImpl->write( pBlockBuffer.get() , frameSize * count );

  frameCount ++ ;
}

template <class STREAM>
long CBlock<STREAM>::GetBlock( long position , arrayPointer_t pBlockBuffer )
{
  assert( pBlockBuffer.get() != NULL ) ;

  LOCKBYMUTEX ;

  pImpl->sync();
  if ( position >= 0 )
    pImpl->seekg( position * frameSize ) ;
  else //wartosci ujemne oznaczaja odczyt z konca
    pImpl->seekg( ( frameCount - ( -1 * position ) ) * frameSize ) ;

  pImpl->read( pBlockBuffer.get() , frameSize ) ;
  return ( static_cast<long>(pImpl->gcount())/frameSize );
}

template <class STREAM>
long CBlock<STREAM>::GetByteSize()
{
  return frameSize*frameCount ;
}

template <class STREAM>
void CBlock<STREAM>::Save( ostream& strout )
{
  LOCKBYMUTEX ;

  pImpl->sync();
  pImpl->seekg( 0 );

  strout.write( reinterpret_cast<char*>(&frameCount), sizeof(long) );
  strout.write( reinterpret_cast<char*>(&frameSize), sizeof(long) );

  result.reset();
  char blok1024[max_packet_size] ;

  long LeftSizeOfFrames = frameSize * frameCount ;
  while( LeftSizeOfFrames > 0 ) {
      long nBlokDysk = ( LeftSizeOfFrames >= max_packet_size ) ? max_packet_size : LeftSizeOfFrames ;
      pImpl->read( reinterpret_cast<char*>(&blok1024), nBlokDysk );
      result.process_bytes( reinterpret_cast<void*>(&blok1024), nBlokDysk  ) ;
      strout.write( reinterpret_cast<char*>(&blok1024) , nBlokDysk );
      LeftSizeOfFrames -= nBlokDysk ;
    }

  boost::crc_32_type::value_type crc = result.checksum() ;

  strout.write( reinterpret_cast<char*>(&crc), sizeof crc );

  pImpl->sync();

}

template <class STREAM>
void CBlock<STREAM>::Load( istream& strin )
{
  LOCKBYMUTEX ;

  strin.read( reinterpret_cast<char*>(&frameCount), sizeof(long) );
  strin.read( reinterpret_cast<char*>(&frameSize), sizeof(long) );

  result.reset();
  char blok1024[max_packet_size] ;

  long LeftSizeOfFrames = frameCount * frameSize ;

  while( LeftSizeOfFrames > 0 )
    {
      long nBlokDysk = ( LeftSizeOfFrames >= max_packet_size ) ? max_packet_size : LeftSizeOfFrames ;
      strin.read( reinterpret_cast<char*>(&blok1024) , nBlokDysk );
      result.process_bytes( reinterpret_cast<void*>(&blok1024), nBlokDysk  );
      pImpl->write( reinterpret_cast<char*>(&blok1024) , nBlokDysk );
      LeftSizeOfFrames -= nBlokDysk ;
    }

  boost::crc_32_type::value_type crc ;
  strin.read( reinterpret_cast<char*>(&crc), sizeof crc );

  pImpl->sync();

  if ( crc != result.checksum() )
    throw std::out_of_range( "Block.h/Uszkodzony plik/blok z danymi (CRC) - zalecam usuniecie pliku." );
}

template <class STREAM>
void CBlock<STREAM>::Trunc()
{
  LOCKBYMUTEX ;

  frameCount = 0 ;
  pImpl.reset( new STREAM );
}

template <class STREAM>
long CBlock<STREAM>::SyncToDisk()
{
  LOCKBYMUTEX ;

  pImpl->sync();
  return frameCount ;
}


template < typename BLOCK >
CRecord<BLOCK>::CRecord()
  : lastBlock ( this->end() )
{
}

template < typename BLOCK >
CRecord<BLOCK>:: ~CRecord()
{
}

template < typename BLOCK >
void CRecord<BLOCK>::DefBlock( long blockID , long frameSize )
{
  BLOCK a( frameSize ) ;
  pair< typename CRecord::iterator , bool > p = this->insert( CRec_Pair( blockID, a ) ) ;
  assert( p.second );
  lastBlock = p.first ;
}

template < typename BLOCK >
void CRecord<BLOCK>::DelBlock( long blockID )
{
  typename CRecord::iterator itBlock = find( blockID ) ;
  assert (itBlock != this->end() );
  this->erase(itBlock);
  lastBlock = this->end() ;
}

template < typename BLOCK >
void CRecord<BLOCK>::PutBlock( long blockID, arrayPointer_t pBlockBuffer , long count )
{
  RefBlock( blockID ).PutBlock( pBlockBuffer , count ) ;
}

template < typename BLOCK >
long CRecord<BLOCK>::GetBlock( long blockID, long position , arrayPointer_t pBlockBuffer  )
{
  return RefBlock( blockID ).GetBlock( position , pBlockBuffer );
}

template < typename BLOCK >
BLOCK & CRecord<BLOCK>::RefBlock( long blockID )
{
  return find( blockID )->second ;
}

template < typename BLOCK >
typename CRecord<BLOCK>::iterator CRecord<BLOCK>::find( long blockID )
{
  if ( lastBlock != this->end() )
    if ( lastBlock->first == blockID )
      return lastBlock ;
  return lastBlock = map< long, BLOCK >::find( blockID );
}
template < typename BLOCK >
void CRecord<BLOCK>::Save( ostream& stream )
{
  size_t blockCount = this->size() ;
  stream.write(  reinterpret_cast <char*>(&blockCount), sizeof blockCount ) ;

  for ( typename CRecord::iterator itBlock = this->begin() ; itBlock != this->end() ; itBlock ++ )
    {
      long blockID = itBlock->first ;
      //	long size = itBlock->second.frameSize * itBlock->second.frameCount ;
      stream.write(  reinterpret_cast<char*>(&blockID), sizeof blockID ) ;
      itBlock->second.Save( stream );
    }
}

template < typename BLOCK >
void CRecord<BLOCK>::Load( istream &stream )
{
  this->clear();
  long blockCount ;
  stream.read( reinterpret_cast<char*>(&blockCount), sizeof blockCount );
  for ( int i = 0 ; i < blockCount ; i ++ )
    {
      long blockID ;
      stream.read( reinterpret_cast<char*>(&blockID), sizeof blockID );
      BLOCK a( 0 ) ;
      pair< typename CRecord::iterator , bool > p = this->insert( CRec_Pair( blockID, a ) ) ;
      assert( p.second );
      lastBlock = p.first ;
      lastBlock->second.Load( stream );
    }
}


/** Konstruktor bloku cyklicznego */
template < typename STREAM >
CCycBlock <STREAM>::CCycBlock( long frameSize /* = 0 */ ):
  slices(1),
  framesInSlice(1),
  lastblockCnt(0) ,
  itLastBlock( blockMap.end() )
{
};

template < typename STREAM >
CCycBlock <STREAM>::CCycBlock( const CCycBlock<STREAM> &x )
{
  lastblockCnt = x.lastblockCnt ;
};

/** Funkcja zwraca ilosc bajtow w bloku
	*  @return ilosc bajtow w strumieniu
	*/
template < typename STREAM >
long CCycBlock <STREAM>::GetByteSize()
{
  int loc_frameCount( 0 ) ;

  //for ( blockMapT::iterator i = blockMap.begin() ; i != blockMap.end() ; i ++ )
  BOOST_FOREACH( const blockMapPair & i , blockMap )
      loc_frameCount += ( i.second.frameCount * i.second.frameSize ) ;

  return loc_frameCount ;
}

/** Destruktor - czysci mape */
template < typename STREAM >
CCycBlock <STREAM>::~CCycBlock()
{
  blockMap.clear();
}

/*	Swiadomy i przemyslany brak konstruktora kopiujacego ! => JIT/Shallow Copy required !*/

template < typename STREAM >
void CCycBlock <STREAM>::PutBlock( arrayPointer_t pBlockBuffer , long count )
{
  assert ( count == 1 );	/** Trzeba przemyslec i napisac dla przypadku > 1 */

  LOCKBYMUTEX ;

  if ( lastblockCnt == 0 )
    { //Just In Time activation - (lub size() == 0)
      CBlock<STREAM> a( IBlock::frameSize ) ;
      std::pair< typename blockMapT::iterator  , bool > p = blockMap.insert( blockMapPair( lastblockCnt ++ , a ) ) ;
      assert( p.second );
      itLastBlock = p.first ;
    }

  if ( itLastBlock->second.frameCount >= framesInSlice )
    {
      CBlock<STREAM> a( IBlock::frameSize ) ;
      std::pair< typename blockMapT::iterator , bool > p = blockMap.insert( blockMapPair( lastblockCnt ++  , a ) ) ;
      assert( p.second );
      itLastBlock = p.first ;
      if ( blockMap.size() >= slices + 1 ) 	{
          blockMap.erase( blockMap.begin() ) ;			//pop_front
          firstFrame =  firstFrame + framesInSlice ;
          //					frameCount = frameCount - framesInSlice ;		//Kwestia filozoficzna ... pomyslec
        }
    }
  IBlock::frameCount ++ ;
  itLastBlock->second.PutBlock( pBlockBuffer , count );
};

template < typename STREAM >
long CCycBlock <STREAM>::GetBlock( long position , arrayPointer_t pBlockBuffer )
{
  LOCKBYMUTEX ;

  long offsetInSubblock = position % framesInSlice ;
  long offsetInBlockSet = position / framesInSlice ;

  if (  blockMap.find( offsetInBlockSet ) == blockMap.end() )
    throw std::out_of_range("Thrown/CycBlock.h/GetBlock()/Odwolanie do usunietego bloku cyklicznego");

  return  blockMap.find( offsetInBlockSet )->second.GetBlock( offsetInSubblock , pBlockBuffer );
};

template < typename STREAM >
void CCycBlock <STREAM>::Save( std::ostream& strout )
{
  LOCKBYMUTEX ;

  size_t lsize = blockMap.size() ;

  unsigned long frameSize = IBlock::frameSize ;
  unsigned long frameCount = IBlock::frameCount ;

  strout.write( reinterpret_cast<char*>(&lsize), sizeof(long) );
  strout.write( reinterpret_cast<char*>(&slices), sizeof(long) );
  strout.write( reinterpret_cast<char*>(&frameSize), sizeof(long) );
  strout.write( reinterpret_cast<char*>(&framesInSlice), sizeof(long) );
  strout.write( reinterpret_cast<char*>(&frameCount), sizeof(long) );
  strout.write( reinterpret_cast<char*>(&firstFrame), sizeof(long) );

  //for ( blockMapT::iterator i = blockMap.begin() ; i != blockMap.end() ; i ++ )
  BOOST_FOREACH( blockMapPair i , blockMap )
  {
    long blockCntID = i.first ;
    strout.write( reinterpret_cast<char*>( &blockCntID ) , sizeof blockCntID );
    i.second.Save( strout );
  };
};

template < typename STREAM >
void CCycBlock <STREAM>::Load( std::istream& strin )
{
  LOCKBYMUTEX ;

  long lsize ;

  unsigned long frameSize(0) ;
  unsigned long frameCount(0) ;

  strin.read( reinterpret_cast<char*>(&lsize), sizeof(long) );
  strin.read( reinterpret_cast<char*>(&slices), sizeof(long) );
  strin.read( reinterpret_cast<char*>(&frameSize), sizeof(long) );
  strin.read( reinterpret_cast<char*>(&framesInSlice), sizeof(long) );
  strin.read( reinterpret_cast<char*>(&frameCount), sizeof(long) );
  strin.read( reinterpret_cast<char*>(&firstFrame), sizeof(long) );

  IBlock::frameSize = frameSize ;
  IBlock::frameCount = frameCount  ;

  blockMap.clear();

  for ( long i = 0 ; i < lsize; i ++ )
    {
      strin.read( reinterpret_cast<char*>(&lastblockCnt), sizeof lastblockCnt ) ;

      CBlock<STREAM> a( IBlock::frameSize ) ;
      pair< typename blockMapT::iterator , bool > p = blockMap.insert( blockMapPair( lastblockCnt ++ , a ) ) ;
      assert( p.second );
      itLastBlock = p.first ;

      itLastBlock->second.Load( strin );
    };
  assert ( static_cast<long>(blockMap.size()) == lsize );
};

/** Funkcja zrzuca blok na dysk i zwraca ile bylo ramek w trakcie zrzutu
	*  @return ilosc ramek w strumieniu w trakcie zrzutu
	*/
template < typename STREAM >
long CCycBlock <STREAM>:: SyncToDisk()
{
  LOCKBYMUTEX ;

  long loc_frameCount( 0 );

  //for ( blockMapT::iterator i = blockMap.begin() ; i != blockMap.end() ; i ++ )
  BOOST_FOREACH( blockMapPair  i , blockMap )
  {
    i.second.SyncToDisk();
    loc_frameCount += i.second.frameCount ;
  }

  return loc_frameCount ;
}

//
//Pimpl implementation - interface class implementation (only proxy)
//

template <class IDTYPE>
CBuffer<IDTYPE>::CBuffer() :
  pImpl( new CBufferImpl<IDTYPE> )
{
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Load( const char * filename )
{
  pImpl->Load( filename );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Save( const char * filename )
{
  pImpl->Save( filename );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::PutBlock (
    IDTYPE blockID,
    arrayPointer_t pBlockBuffer ,
    long count
    )
{
  pImpl->PutBlock( blockID , pBlockBuffer , count );
}

template <class IDTYPE>
long CBuffer<IDTYPE>::GetBlock(
    IDTYPE blockID,
    long position,
    arrayPointer_t pBlockBuffer )
{
  return pImpl->GetBlock( blockID , position , pBlockBuffer );
}


template <class IDTYPE>
void CBuffer<IDTYPE>::DefBlock(
    IDTYPE blockID,
    long frameSize ,
    BlockType btType ,
    long aux1  ,
    long aux2 )
{
  pImpl->DefBlock( blockID , frameSize , btType , aux1, aux2 );
}


template <class IDTYPE>
void CBuffer<IDTYPE>::DelBlock( IDTYPE blockID )
{
  pImpl->DelBlock( blockID );
}

template <class IDTYPE>
long CBuffer<IDTYPE>::GetLen( IDTYPE blockID )
{
  return pImpl->GetLen( blockID );
}


template <class IDTYPE>
long CBuffer<IDTYPE>::GetSizeOfFrame( IDTYPE blockID )
{
  return pImpl->GetSizeOfFrame( blockID ) ;
}

template <class IDTYPE>
long CBuffer<IDTYPE>::GetSizeOfBlock(
    IDTYPE blockID ,
    long firstFrm ,
    long lastFrm )
{
  return pImpl->GetSizeOfBlock( blockID , firstFrm , lastFrm ) ;
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Clear()
{
  pImpl->Clear();
}

template <class IDTYPE>
void CBuffer<IDTYPE>::DecycleBlockType(
    IDTYPE blockID ,
    arrayPointer_t pBlockBuffer )
{
  pImpl->DecycleBlockType( blockID , pBlockBuffer );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::MemorizeBlockType(
    IDTYPE blockID ,
    arrayPointer_t pBlockBuffer )
{
  pImpl->MemorizeBlockType( blockID , pBlockBuffer ) ;
}

template <class IDTYPE>
long CBuffer<IDTYPE>::syncToDisk(IDTYPE blockID )
{
  return pImpl->syncToDisk( blockID );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Dump(
    std::ostream &os ,
    std::set < string > * pQset )
{
  pImpl->Dump( os , pQset );
}

//Specjalizacja dla stringa i long - tylko pimpl
template class CBuffer<std::string> ;
template class CBuffer<long> ;

