#pragma once

#include <iostream>                                  // for ostream
#include <set>                                       // for set
#include <string>                                    // for string

#include <memory>
#include <boost/shared_array.hpp>                    // IWYU pragma: keep

typedef boost::shared_array<char> arrayPointer_t ;

enum BlockType {
    BF,     /** Block in File */
    BM,     /** Block in Memory */
    BCF,    /** Block in Cyclic File */
    BCM     /** Block in Cyclic Memory */
} ;

template <class IDTYPE>
class CBufferImpl ;

template <class IDTYPE>
class CBuffer {
    std::shared_ptr< CBufferImpl< IDTYPE >> pImpl ;

  public:

    CBuffer() ;

    void Load(const char* filename) ;       /**< Read CBufferImpl from file */
    void Save(const char* filename) ;       /**< Save CBufferImpl to file */
    void PutBlock(IDTYPE blockID, arrayPointer_t pBlockBuffer, long count = 1) ;     /**< Put Frame in given block */
    long GetBlock(IDTYPE blockID, long position, arrayPointer_t pBlockBuffer) ;    /**< Get Block  */
    void DefBlock(IDTYPE blockID, long frameSize, BlockType btType = BF, long aux1 = 0, long aux2 = 0) ;     /**< Define block. */
    void DelBlock(IDTYPE blockID)  ;        /**< Remove block */
    long GetLen(IDTYPE blockID)  ;          /**< Get frames count */
    long GetSizeOfFrame(IDTYPE blockID) ;    /**< Get length of single frame */
    long GetSizeOfBlock(IDTYPE blockID, long firstFrm = 0, long lastFrm = -1) ;    /**< Get size of block in bytes */
    void Clear() ;                          /**< Clean and Trucate */
    void DecycleBlockType(IDTYPE blockID, arrayPointer_t pBlockBuffer) ;    /**< Cyclic -> Flat type conversion */
    void MemorizeBlockType(IDTYPE blockID, arrayPointer_t pBlockBuffer) ;    /**< Move block to memory */
    long syncToDisk(IDTYPE blockID) ;       /** Store and materialize stream on disk */
    void Dump(std::ostream &os, std::set < std::string >* pQset) ;    /** Testing and reporting */
};
