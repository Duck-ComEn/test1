/*
 * neptune_hitachi_comm.c
 */
#include <string.h>
#include "neptune_hitachi_comm.h"


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

