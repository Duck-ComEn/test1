/*
 * neptune_hitachi_comm.c
 */
#include <string.h>
#include "neptune_hitachi_comm.h"
#include "neptune_hitachi_interface.h"

#define LITTLE_ENDIAN_PC
#define SYNC  0xAA55


/*!
 * swapBytes: Swaps the bytes in a 16-bit Word.
 */
static Word swapBytes(Word in)
{
#ifdef LITTLE_ENDIAN_PC
	return ((in & 0x00FF) << 8) | ((in & 0xFF00) >> 8);
#else
	return in;
#endif
}

/*!
 * computeChecksum: Computes checksum of a buffer.
 */
Word computeChecksum(Byte *buffer, Word len)
{
	Word j = 4, checkSum = 0;	
	
    /* Calculate checksum */
	
    while(j<len)
    {
        checkSum += (buffer[j] << 8) | buffer[j+1];
        j += 2;
    }
    checkSum = (0-checkSum);
	
	return(checkSum);
}

