#include "Buffer.h"
#include <assert.h>                                  // for assert
#include <stddef.h>                                  // for size_t, NULL
#include <boost/crc.hpp>                             // for crc_32_type, crc...
#include <boost/foreach.hpp>                         // for auto_any_base
#include <boost/smart_ptr/scoped_ptr.hpp>            // for scoped_ptr
#include <boost/thread/pthread/recursive_mutex.hpp>  // for recursive_mutex
#include <map>                                       // for map, map<>::iter...
#include <stdexcept>                                 // for out_of_range
#include <utility>                                   // for pair
#include "RandomFile.h"                              // for CRandomFile, string

#include <boost/thread/recursive_mutex.hpp>
using namespace boost ;
#define LOCKBYMUTEX recursive_mutex::scoped_lock scoped_lock_instance(lock_)

#include <boost/foreach.hpp>
#include <boost/crc.hpp>

const int max_packet_size = 1024 ;

/** Mutex declartion */
recursive_mutex lock_ ;

using namespace std ;

using boost::shared_array ;

/** Interface contract for Block class type */
class IBlock {
    void operator= (const IBlock &);  //prevent from copying this way

  public:

    boost::crc_32_type result ;

    long frameSize ;    /**< Size of frame in bytes */
    long frameCount ;   /**< Count of frames in block */
    long firstFrame ;   /**< Pointer to first frame in block (pro forma need to type common for CycBlock) */

    IBlock ( long frameSize = 0 )
        : frameSize(frameSize), frameCount(0), firstFrame(0)
    {}
    IBlock (const IBlock &);          //allow copying
    virtual ~IBlock() {}  ;

    void PutBlock( arrayPointer_t pBlockBuffer, long count ) ;
    long GetBlock( long position, arrayPointer_t pBlockBuffer ) ;
    long GetByteSize() ;
    void Save( std::ostream &strout ) ;
    void Load( std::istream &strin ) ;
    void Trunc() ;
    long SyncToDisk() ;
} ;

/**
     * Template class - atribute is data stream
     * STREAM - can take CBlock<CRandomFile>,CBlock<strstream>
     * Look at cyclic block
     * @see CCycBlock
     */
template <class STREAM>
class CBlock : public IBlock {
    boost::scoped_ptr< STREAM > pImpl ;   /**< Pointer for standard stream ie. (cin,cout,iostream)*/

  public:

    CBlock( const CBlock<STREAM> &x ) ; /**< Copy constructor NOTE! destroying copying - moved data */
    CBlock( long frameSize = 0 ) ;      /**< Constructor - Create on heap stream object - must be w/o mutex */

    virtual ~CBlock() ;                 /**< Destructor - protected by mulithread confilicts with critical section */
    void PutBlock( arrayPointer_t pBlockBuffer, long count ) ;  /**< Put frame in block */
    long GetBlock( long position, arrayPointer_t pBlockBuffer ) ;  /**< Get Frame from block  */
    long GetByteSize() ;                /**< Count of bytes in block */
    void Save( std::ostream &strout ) ; /**< Save block to stream */
    void Load( std::istream &strin ) ;  /**< Load block from stream */
    void Trunc() ;                      /**< Truncate block */
    long SyncToDisk() ;                 /**< Drop block to disk */
};


template < typename BLOCK >
class CRecord : public std::map< long, BLOCK > {
  public:
    typename std::map< long, BLOCK >::iterator lastBlock ;
    typedef std::pair < int, BLOCK > CRec_Pair;
    CRecord() ;
    virtual ~CRecord() ;
    void DefBlock( long blockID, long frameSize ) ;
    void DelBlock( long blockID ) ;
    void PutBlock( long blockID, arrayPointer_t pBlockBuffer, long count ) ;
    long GetBlock( long blockID, long position, arrayPointer_t pBlockBuffer  )  ;
    BLOCK &RefBlock( long blockID )    ;
    typename CRecord::iterator find( long blockID ) ;
    void Save( std::ostream &stream ) ;
    void Load( std::istream &stream ) ;
};

/**
    *   Template class - atribute stream
    *   STREAM - can be CCycBlock<CRandomFile>,CCycBlock<strstream>
    */
template <class STREAM>
class CCycBlock : public IBlock {
    long lastblockCnt ;             /**< JIT Support */

    typedef std::pair < long, CBlock<STREAM>> blockMapPair ;
    typedef std::map<long, CBlock<STREAM>> blockMapT ;
    blockMapT blockMap ;
    typename blockMapT::iterator itLastBlock ;

  public:
    /** Intended no copy consrutor here ! => JIT/Shallow Copy required !*/

    unsigned long slices ;          /**< Slice count for cyclical block  */
    long framesInSlice ;            /**< Count of frames in each subblock */

    /** Konstruktor bloku cyklicznego */
    CCycBlock( long frameSize = 0 );
    CCycBlock( const CCycBlock<STREAM> &x ) ;

    long GetByteSize() ;
    virtual ~CCycBlock() ;
    void PutBlock( arrayPointer_t pBlockBuffer, long count ) ;
    long GetBlock( long position, arrayPointer_t pBlockBuffer ) ;
    void Save( std::ostream &strout ) ;
    void Load( std::istream &strin ) ;
    long SyncToDisk() ;
};

template <class IDTYPE>
class CBufferImpl {
    typedef std::map < long, BlockType > BMap ;

    std::shared_ptr< CRecord<CBlock<CRandomFile>>> recordInFile ;
    std::shared_ptr< CRecord<CBlock<std::stringstream>>> recordInMemory;
    std::shared_ptr< CRecord<CCycBlock<CRandomFile>>> recordInCycleFile ;
    std::shared_ptr< CRecord<CCycBlock<std::stringstream>>> recordInCycleMemory ;
    /** Intended map encapsulation */
    BMap blok  ;

    long eval( long BlockID ) ;     /** Eval functions are type deduction supportes during compilation */
    long eval( const std::string &BlockName ) ;

  public:

    CBufferImpl();

    virtual ~CBufferImpl() ;
    void Load( const char* filename ) ;
    void Save( const char* filename ) ;
    void PutBlock( IDTYPE blockID, arrayPointer_t pBlockBuffer, long count = 1 ) ;
    long GetBlock( IDTYPE blockID, long position, arrayPointer_t pBlockBuffer ) ;
    void DefBlock( IDTYPE blockID, long frameSize, BlockType btType = BF, long aux1 = 0, long aux2 = 0) ;
    void DelBlock( IDTYPE blockID ) ;
    long GetLen( IDTYPE blockID ) ;
    long GetSizeOfFrame( IDTYPE blockID ) ;
    long GetSizeOfBlock( IDTYPE blockID, long firstFrm = 0, long lastFrm = -1 ) ;
    void Clear() ;
    void DecycleBlockType( IDTYPE blockID, arrayPointer_t pBlockBuffer ) ;
    void MemorizeBlockType( IDTYPE blockID, arrayPointer_t pBlockBuffer ) ;
    long syncToDisk(IDTYPE blockID ) ;
    void Dump( std::ostream &os, std::set < string >* pQset ) ;
};

template class CBlock<CRandomFile> ;
template class CBlock<stringstream> ;

template class CCycBlock<CRandomFile> ;
template class CCycBlock<stringstream> ;

template class CBufferImpl<std::string> ;
template class CBufferImpl<long> ;

template class CRecord<CBlock<CRandomFile>> ;
template class CRecord<CBlock<stringstream>>;

template class CRecord<CCycBlock<CRandomFile>> ;
template class CRecord<CCycBlock<stringstream>>;

template <class IDTYPE>
long CBufferImpl<IDTYPE>::eval( long BlockID ) {
    return BlockID ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::eval( const string &BlockName ) {
    long hashKey(0) ;

    for ( long i = 0 ; i < static_cast<long>( BlockName.length() ) ; i ++ ) {
        hashKey += ( static_cast<char>( BlockName[i] ) - 48 ) * ( i * 80 + 1 );
    }

    return hashKey ;
}

template <class IDTYPE>
CBufferImpl<IDTYPE>::CBufferImpl() {
    recordInFile.reset( new CRecord< CBlock<CRandomFile>>() );
    recordInMemory.reset( new CRecord< CBlock<stringstream>>() );
    recordInCycleFile.reset( new CRecord< CCycBlock<CRandomFile>>());
    recordInCycleMemory.reset( new CRecord< CCycBlock<stringstream>>() );
}

template <class IDTYPE>
CBufferImpl<IDTYPE>::~CBufferImpl() {
}

#define MAP_SWICH( __FUNCT__ , __PREFUNCT__ ) \
  switch ( blok[eval(blockID)] ) { \
  case BF:   __PREFUNCT__ (*recordInFile). __FUNCT__  ;  break ; \
  case BM:   __PREFUNCT__ (*recordInMemory). __FUNCT__  ;  break ; \
  default: assert( false ); \
  case BCF:  __PREFUNCT__ (*recordInCycleFile). __FUNCT__  ;  break ; \
  case BCM:  __PREFUNCT__ (*recordInCycleMemory). __FUNCT__  ;  break ;  };

#define MAP_EACH( __FUNCT__ ) \
  (*recordInFile). __FUNCT__  ; \
  (*recordInMemory). __FUNCT__ ; \
  (*recordInCycleFile). __FUNCT__ ; \
  (*recordInCycleMemory). __FUNCT__ ;


template <class IDTYPE>
void CBufferImpl<IDTYPE>::Load( const char* filename ) {
    std::ifstream file(filename, ios::in | ios::binary);

    if ( ! file ) {
        throw std::out_of_range("Thrown/Buffer.h/Load()/Read failed");
    }

    //http://www.cplusplus.com/doc/tutorial/tut5-3.html - exception handling
    long sizeOfmap ;
    file.read(  reinterpret_cast<char*>(&sizeOfmap), sizeof sizeOfmap ) ;

    for ( long i = 0 ; i < sizeOfmap ; i ++ ) {
        long blockID  ;
        BlockType type  ;
        file.read( reinterpret_cast<char*>(&blockID), sizeof blockID ) ;
        file.read( reinterpret_cast<char*>(&type), sizeof type ) ;
        blok[eval(blockID)] = type ;
    }

    MAP_EACH ( Load(file) ) ;
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::Save( const char* filename ) {
    fstream file(filename, ios::out | ios::binary | ios::trunc);

    if ( ! file ) {
        throw std::out_of_range("Thrown/Buffer.h/Save()/Write failed");
    }

    size_t sizeOfmap = blok.size() ;
    file.write(  reinterpret_cast<char*>(&sizeOfmap), sizeof sizeOfmap ) ;

    for ( BMap::iterator it = blok.begin() ; it != blok.end() ; it ++ ) {
        long blockID = (*it).first ;
        BlockType type = (*it).second ;
        file.write(  reinterpret_cast<char*>(&blockID), sizeof blockID ) ;
        file.write(  reinterpret_cast<char*>(&type), sizeof type ) ;
    }

    MAP_EACH ( Save( file ) ) ;
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::PutBlock( IDTYPE blockID, arrayPointer_t pBlockBuffer, long count ) {
    MAP_SWICH ( PutBlock( eval(blockID), pBlockBuffer, count ), ; ) ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetBlock( IDTYPE blockID,
    long position,
    arrayPointer_t pBlockBuffer ) {
    MAP_SWICH ( GetBlock( eval(blockID), position, pBlockBuffer ), return )
    return 0 ; //proforma
}


template <class IDTYPE>
void CBufferImpl<IDTYPE>::DefBlock(
    IDTYPE blockID,
    long frameSize,
    BlockType btType,
    long aux1,
    long aux2 ) {
    blok[eval(blockID)] = btType ;
    MAP_SWICH ( DefBlock( eval(blockID), frameSize ), ; )

    if ( btType == BCF ) {
        (*recordInCycleFile).RefBlock( eval(blockID) ).slices = aux1 ;
        (*recordInCycleFile).RefBlock( eval(blockID) ).framesInSlice = aux2 ;
    }

    if ( btType == BCM ) {
        (*recordInCycleMemory).RefBlock( eval(blockID) ).slices = aux1 ;
        (*recordInCycleMemory).RefBlock( eval(blockID) ).framesInSlice = aux2 ;
    }

    if ( btType == BF || btType == BM ) {
        assert ( aux1 == 0 && aux2 == 0 );
    }
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::DelBlock( IDTYPE blockID ) {
    MAP_SWICH( DelBlock( eval(blockID) ), ; ) ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetLen( IDTYPE blockID ) {
    MAP_SWICH( RefBlock( eval(blockID) ).frameCount, return );
    return 0 ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetSizeOfFrame( IDTYPE blockID ) {
    MAP_SWICH( RefBlock( eval(blockID) ).frameSize, return );
    return 0 ;
}

template <class IDTYPE>
long CBufferImpl<IDTYPE>::GetSizeOfBlock( IDTYPE blockID, long firstFrm, long lastFrm ) {
    MAP_SWICH( RefBlock( eval(blockID) ).GetByteSize(), return );
    return 0 ;
}

template <class IDTYPE>
void CBufferImpl< IDTYPE>::Clear() {
    recordInFile.reset( new CRecord< CBlock<CRandomFile>>() );
    recordInMemory.reset( new CRecord< CBlock<stringstream>>() );
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
void CBufferImpl<IDTYPE>::DecycleBlockType( IDTYPE blockID, arrayPointer_t pBlockBuffer ) {
    if ( blok[eval(blockID)] == BF ) {
        return ;
    }

    if ( blok[eval(blockID)] == BM ) {
        return ;
    }

    META_PROG( BCF, BF, (*recordInCycleFile), (*recordInFile) );
    META_PROG( BCM, BM, (*recordInCycleMemory), (*recordInMemory) );
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::MemorizeBlockType( IDTYPE blockID, arrayPointer_t pBlockBuffer ) {
    if ( blok[eval(blockID)] == BCM ) {
        return ;
    }

    if ( blok[eval(blockID)] == BM ) {
        return ;
    }

    META_PROG( BCF, BCM, (*recordInCycleFile), (*recordInCycleMemory) );
    META_PROG( BF, BM, (*recordInFile), (*recordInMemory) );
}

#undef META_PROG

template < typename IDTYPE >
long CBufferImpl<IDTYPE>::syncToDisk(IDTYPE blockID ) {
    MAP_SWICH( RefBlock( eval(blockID) ).SyncToDisk(), return );
    return 0 ;
}

template <class IDTYPE>
void CBufferImpl<IDTYPE>::Dump( ostream &os,  set < string >* pQset ) {
    os << "Count\tFSize\tLoc\tBlockID" << std::endl ;

    for ( BMap::iterator it = blok.begin() ; it != blok.end() ; it ++ ) {
        switch ( (*it).second ) {
            case BF:
                os << (*recordInFile).RefBlock( (*it).first ).frameCount ;
                os << "\t" ;
                os << (*recordInFile).RefBlock( (*it).first ).frameSize ;
                os << "\t" ;
                os <<  "File" ;
                break ;

            case BM:
                os << (*recordInMemory).RefBlock( (*it).first ).frameCount ;
                os << "\t" ;
                os << (*recordInMemory).RefBlock( (*it).first ).frameSize ;
                os << "\t" ;
                os << "Memory" ;
                break ;

            case BCF:
                os << (*recordInCycleFile).RefBlock( (*it).first ).frameCount ;
                os << "\t" ;
                os << (*recordInCycleFile).RefBlock( (*it).first ).frameSize ;
                os << "\t" ;
                os << "Cycle File" ;
                break ;

            case BCM:
                os << (*recordInCycleMemory).RefBlock( (*it).first ).frameCount ;
                os << "\t" ;
                os << (*recordInCycleMemory).RefBlock( (*it).first ).frameSize ;
                os << "\t" ;
                os << "Cycle Memory" ;
                break ;

            default:
                assert( false );
        }

        os << "\t" ;

        if ( pQset == NULL ) {
            os << (*it).first ;
        } else
            for( auto qid : * pQset ) {
                if ( eval( qid ) ==  (*it).first ) {
                    os << qid ;
                }
            }

        os << std::endl ;
    }
}

#undef MAP_SWICH
#undef MAP_EACH

template <class STREAM>
CBlock<STREAM>::CBlock( const CBlock<STREAM> &x ) {
    //  LOCKBYMUTEX ;
    pImpl.reset( new STREAM );
    frameSize = x.frameSize ;
    frameCount = x.frameCount ;
    firstFrame = x.firstFrame ;
    assert( frameCount == 0 ) ;
    //Stream has always only one owner - copying only <=
    //WARNINNG - THERE IS HAPPEN SOMETHING NASTY
    CBlock<STREAM>* y = const_cast<CBlock<STREAM> * >(&x) ;
    y->pImpl.reset() ;
}

template <class STREAM>
CBlock<STREAM>::CBlock( long frameSize ) :
    pImpl( new STREAM ) {
    IBlock::frameSize = frameSize ;
    frameCount = 0 ;
    firstFrame = 0 ;
}

template <class STREAM>
CBlock<STREAM>::~CBlock() {
    //  LOCKBYMUTEX ;
    frameCount = 0 ;
    firstFrame = 0 ;
}

template <class STREAM>
void CBlock<STREAM>::PutBlock( arrayPointer_t pBlockBuffer, long count ) {
    assert( pBlockBuffer.get() != NULL ) ;
    LOCKBYMUTEX ;
    pImpl->seekp(0, STREAM::end);
    pImpl->write( pBlockBuffer.get(), frameSize * count );
    frameCount ++ ;
}

template <class STREAM>
long CBlock<STREAM>::GetBlock( long position, arrayPointer_t pBlockBuffer ) {
    assert( pBlockBuffer.get() != NULL ) ;
    LOCKBYMUTEX ;
    pImpl->sync();

    if ( position >= 0 ) {
        pImpl->seekg( position * frameSize ) ;
    } else {
        pImpl->seekg( ( frameCount - ( -1 * position ) ) * frameSize ) ;
    }

    pImpl->read( pBlockBuffer.get(), frameSize ) ;
    return ( static_cast<long>(pImpl->gcount()) / frameSize );
}

template <class STREAM>
long CBlock<STREAM>::GetByteSize() {
    return frameSize * frameCount ;
}

template <class STREAM>
void CBlock<STREAM>::Save( ostream &strout ) {
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
        strout.write( reinterpret_cast<char*>(&blok1024), nBlokDysk );
        LeftSizeOfFrames -= nBlokDysk ;
    }

    boost::crc_32_type::value_type crc = result.checksum() ;
    strout.write( reinterpret_cast<char*>(&crc), sizeof crc );
    pImpl->sync();
}

template <class STREAM>
void CBlock<STREAM>::Load( istream &strin ) {
    LOCKBYMUTEX ;
    strin.read( reinterpret_cast<char*>(&frameCount), sizeof(long) );
    strin.read( reinterpret_cast<char*>(&frameSize), sizeof(long) );
    result.reset();
    char blok1024[max_packet_size] ;
    long LeftSizeOfFrames = frameCount * frameSize ;

    while( LeftSizeOfFrames > 0 ) {
        long nBlokDysk = ( LeftSizeOfFrames >= max_packet_size ) ? max_packet_size : LeftSizeOfFrames ;
        strin.read( reinterpret_cast<char*>(&blok1024), nBlokDysk );
        result.process_bytes( reinterpret_cast<void*>(&blok1024), nBlokDysk  );
        pImpl->write( reinterpret_cast<char*>(&blok1024), nBlokDysk );
        LeftSizeOfFrames -= nBlokDysk ;
    }

    boost::crc_32_type::value_type crc ;
    strin.read( reinterpret_cast<char*>(&crc), sizeof crc );
    pImpl->sync();

    if ( crc != result.checksum() ) {
        throw std::out_of_range( "Block.h/broken file/block with data (CRC) - recommend: remove this file" );
    }
}

template <class STREAM>
void CBlock<STREAM>::Trunc() {
    LOCKBYMUTEX ;
    frameCount = 0 ;
    pImpl.reset( new STREAM );
}

template <class STREAM>
long CBlock<STREAM>::SyncToDisk() {
    LOCKBYMUTEX ;
    pImpl->sync();
    return frameCount ;
}


template < typename BLOCK >
CRecord<BLOCK>::CRecord()
    : lastBlock ( this->end() ) {
}

template < typename BLOCK >
CRecord<BLOCK>:: ~CRecord() {
}

template < typename BLOCK >
void CRecord<BLOCK>::DefBlock( long blockID, long frameSize ) {
    BLOCK a( frameSize ) ;
    pair< typename CRecord::iterator, bool > p = this->insert( CRec_Pair( blockID, a ) ) ;
    assert( p.second );
    lastBlock = p.first ;
}

template < typename BLOCK >
void CRecord<BLOCK>::DelBlock( long blockID ) {
    typename CRecord::iterator itBlock = find( blockID ) ;
    assert (itBlock != this->end() );
    this->erase(itBlock);
    lastBlock = this->end() ;
}

template < typename BLOCK >
void CRecord<BLOCK>::PutBlock( long blockID, arrayPointer_t pBlockBuffer, long count ) {
    RefBlock( blockID ).PutBlock( pBlockBuffer, count ) ;
}

template < typename BLOCK >
long CRecord<BLOCK>::GetBlock( long blockID, long position, arrayPointer_t pBlockBuffer  ) {
    return RefBlock( blockID ).GetBlock( position, pBlockBuffer );
}

template < typename BLOCK >
BLOCK &CRecord<BLOCK>::RefBlock( long blockID ) {
    return find( blockID )->second ;
}

template < typename BLOCK >
typename CRecord<BLOCK>::iterator CRecord<BLOCK>::find( long blockID ) {
    if ( lastBlock != this->end() )
        if ( lastBlock->first == blockID ) {
            return lastBlock ;
        }

    return lastBlock = map< long, BLOCK >::find( blockID );
}
template < typename BLOCK >
void CRecord<BLOCK>::Save( ostream &stream ) {
    size_t blockCount = this->size() ;
    stream.write(  reinterpret_cast <char*>(&blockCount), sizeof blockCount ) ;

    for ( typename CRecord::iterator itBlock = this->begin() ; itBlock != this->end() ; itBlock ++ ) {
        long blockID = itBlock->first ;
        stream.write(  reinterpret_cast<char*>(&blockID), sizeof blockID ) ;
        itBlock->second.Save( stream );
    }
}

template < typename BLOCK >
void CRecord<BLOCK>::Load( istream &stream ) {
    this->clear();
    long blockCount ;
    stream.read( reinterpret_cast<char*>(&blockCount), sizeof blockCount );

    for ( int i = 0 ; i < blockCount ; i ++ ) {
        long blockID ;
        stream.read( reinterpret_cast<char*>(&blockID), sizeof blockID );
        BLOCK a( 0 ) ;
        pair< typename CRecord::iterator, bool > p = this->insert( CRec_Pair( blockID, a ) ) ;
        assert( p.second );
        lastBlock = p.first ;
        lastBlock->second.Load( stream );
    }
}


template < typename STREAM >
CCycBlock <STREAM>::CCycBlock( long frameSize ):
    slices(1),
    framesInSlice(1),
    lastblockCnt(0),
    itLastBlock( blockMap.end() ) {
};

template < typename STREAM >
CCycBlock <STREAM>::CCycBlock( const CCycBlock<STREAM> &x ) {
    lastblockCnt = x.lastblockCnt ;
};

template < typename STREAM >
long CCycBlock <STREAM>::GetByteSize() {
    int loc_frameCount( 0 ) ;
    BOOST_FOREACH( const blockMapPair & i, blockMap )
    loc_frameCount += ( i.second.frameCount * i.second.frameSize ) ;
    return loc_frameCount ;
}

template < typename STREAM >
CCycBlock <STREAM>::~CCycBlock() {
    blockMap.clear();
}

template < typename STREAM >
void CCycBlock <STREAM>::PutBlock( arrayPointer_t pBlockBuffer, long count ) {
    assert ( count == 1 );
    LOCKBYMUTEX ;

    if ( lastblockCnt == 0 ) {
        CBlock<STREAM> a( IBlock::frameSize ) ;
        std::pair< typename blockMapT::iterator, bool > p = blockMap.insert( blockMapPair( lastblockCnt ++, a ) ) ;
        assert( p.second );
        itLastBlock = p.first ;
    }

    if ( itLastBlock->second.frameCount >= framesInSlice ) {
        CBlock<STREAM> a( IBlock::frameSize ) ;
        std::pair< typename blockMapT::iterator, bool > p = blockMap.insert( blockMapPair( lastblockCnt ++, a ) ) ;
        assert( p.second );
        itLastBlock = p.first ;

        if ( blockMap.size() >= slices + 1 ) {
            blockMap.erase( blockMap.begin() ) ;
            firstFrame =  firstFrame + framesInSlice ;
        }
    }

    IBlock::frameCount ++ ;
    itLastBlock->second.PutBlock( pBlockBuffer, count );
};

template < typename STREAM >
long CCycBlock <STREAM>::GetBlock( long position, arrayPointer_t pBlockBuffer ) {
    LOCKBYMUTEX ;
    long offsetInSubblock = position % framesInSlice ;
    long offsetInBlockSet = position / framesInSlice ;

    if (  blockMap.find( offsetInBlockSet ) == blockMap.end() ) {
        throw std::out_of_range("Thrown/CycBlock.h/GetBlock()/Call to removed cyclic block");
    }

    return  blockMap.find( offsetInBlockSet )->second.GetBlock( offsetInSubblock, pBlockBuffer );
};

template < typename STREAM >
void CCycBlock <STREAM>::Save( std::ostream &strout ) {
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
    BOOST_FOREACH( blockMapPair i, blockMap ) {
        long blockCntID = i.first ;
        strout.write( reinterpret_cast<char*>( &blockCntID ), sizeof blockCntID );
        i.second.Save( strout );
    };
};

template < typename STREAM >
void CCycBlock <STREAM>::Load( std::istream &strin ) {
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

    for ( long i = 0 ; i < lsize; i ++ ) {
        strin.read( reinterpret_cast<char*>(&lastblockCnt), sizeof lastblockCnt ) ;
        CBlock<STREAM> a( IBlock::frameSize ) ;
        pair< typename blockMapT::iterator, bool > p = blockMap.insert( blockMapPair( lastblockCnt ++, a ) ) ;
        assert( p.second );
        itLastBlock = p.first ;
        itLastBlock->second.Load( strin );
    };

    assert ( static_cast<long>(blockMap.size()) == lsize );
};

template < typename STREAM >
long CCycBlock <STREAM>:: SyncToDisk() {
    LOCKBYMUTEX ;
    long loc_frameCount( 0 );
    BOOST_FOREACH( blockMapPair  i, blockMap ) {
        i.second.SyncToDisk();
        loc_frameCount += i.second.frameCount ;
    }
    return loc_frameCount ;
}

//
// Pimpl implementation - interface class implementation (only proxy)
//

template <class IDTYPE>
CBuffer<IDTYPE>::CBuffer() :
    pImpl( new CBufferImpl<IDTYPE> ) {
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Load( const char* filename ) {
    pImpl->Load( filename );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Save( const char* filename ) {
    pImpl->Save( filename );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::PutBlock (
    IDTYPE blockID,
    arrayPointer_t pBlockBuffer,
    long count
) {
    pImpl->PutBlock( blockID, pBlockBuffer, count );
}

template <class IDTYPE>
long CBuffer<IDTYPE>::GetBlock(
    IDTYPE blockID,
    long position,
    arrayPointer_t pBlockBuffer ) {
    return pImpl->GetBlock( blockID, position, pBlockBuffer );
}


template <class IDTYPE>
void CBuffer<IDTYPE>::DefBlock(
    IDTYPE blockID,
    long frameSize,
    BlockType btType,
    long aux1,
    long aux2 ) {
    pImpl->DefBlock( blockID, frameSize, btType, aux1, aux2 );
}


template <class IDTYPE>
void CBuffer<IDTYPE>::DelBlock( IDTYPE blockID ) {
    pImpl->DelBlock( blockID );
}

template <class IDTYPE>
long CBuffer<IDTYPE>::GetLen( IDTYPE blockID ) {
    return pImpl->GetLen( blockID );
}


template <class IDTYPE>
long CBuffer<IDTYPE>::GetSizeOfFrame( IDTYPE blockID ) {
    return pImpl->GetSizeOfFrame( blockID ) ;
}

template <class IDTYPE>
long CBuffer<IDTYPE>::GetSizeOfBlock(
    IDTYPE blockID,
    long firstFrm,
    long lastFrm ) {
    return pImpl->GetSizeOfBlock( blockID, firstFrm, lastFrm ) ;
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Clear() {
    pImpl->Clear();
}

template <class IDTYPE>
void CBuffer<IDTYPE>::DecycleBlockType(
    IDTYPE blockID,
    arrayPointer_t pBlockBuffer ) {
    pImpl->DecycleBlockType( blockID, pBlockBuffer );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::MemorizeBlockType(
    IDTYPE blockID,
    arrayPointer_t pBlockBuffer ) {
    pImpl->MemorizeBlockType( blockID, pBlockBuffer ) ;
}

template <class IDTYPE>
long CBuffer<IDTYPE>::syncToDisk(IDTYPE blockID ) {
    return pImpl->syncToDisk( blockID );
}

template <class IDTYPE>
void CBuffer<IDTYPE>::Dump(
    std::ostream &os,
    std::set < string >* pQset ) {
    pImpl->Dump( os, pQset );
}

template class CBuffer<std::string> ;
template class CBuffer<long> ;
