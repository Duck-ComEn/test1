//#define HI_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#include <time.h>
//#include <pthread.h>
#include "./hiMain.h"
#include "./hiPipe.h"
#include "../testcase/tcTypes.h"
#include "../testscript/ts.h"

#define HEADER_LENGTH 1024
#define MAX_DATA_LENGTH 64000

/**
 * <pre>
 * Description:
 *   This method is called by test case to send CMD or FILE packet to Host Program.
 *   This method checks ZQUE->bTypes and select hiSendCMDPkt(ZQUE) or send_FILE_pkt(ZDAT).
 * Input
 *   void*ptr (Shuld be ZQUE or ZDATA)
 * Example:
 *
 * </pre>
 *****************************************************************************/
void hi_qput(void* ptr)
{
  TCPRINTF("hi_qput 0x%08lx", (unsigned long)ptr);

//    if( !hi_is_initialize_mutex){   //Initialize mutex only once.
//        hi_is_initialize_mutex = 1;
//        pthread_mutex_init(&hi_qput_mutex, &fast);
//    }

//    printf("Try to get Mutex\n");
////    int ret = pthread_mutex_lock(&hi_qput_mutex);
//    printf("RET:%d\n", ret);
//    if(ret == EINVAL){
//        printf("ENIVAL\n");
//    }else if(ret = EDEADLK){
//        printf("EDEADLK\n");
//    }
  //printf("Get Mutex\n");

  ZQUE *zque = (ZQUE *)ptr;
  if (zque->bType == ZCmd) {
    hiSendCMDPkt(zque);
    free(zque);
  } else if (zque->bType == ZUpload) {
    ZDAT *zdat = (ZDAT *)ptr;
    hiSendFILEPkt(zdat);
    free(zdat);
  } else {
    TCPRINTF("Invalid bType[%02X]", zque->bType);
    free(zque);
  }

}

/**
 * <pre>
 * Description:
 *   This method is called by test case to receive data from Host PGM.
 * Input
 *   cell_num
 * Output
 *   void *  ZQUE or ZDAT pointer.
 * Example:
 *
 * </pre>
 *****************************************************************************/
void* hi_qget(Byte cell_num)
{
  TCPRINTF("hi_qget %d", cell_num);

  ZQUE *zque ;
  Byte *header = (Byte *)malloc(HEADER_LENGTH);
  memset(header, 0, HEADER_LENGTH);
  hiReceiveHeader(cell_num, header);
  zque = analysisHeader(cell_num,  header);
  if ( zque->wLgth != 0) {
    if (zque->bType == ZCmd) {
      hiReceiveData(zque->bAdrs, &(zque->bData[0]), zque->wLgth);
    } else if (zque->bType == ZData) {
      ZDAT *zdat = (ZDAT *) zque;
      hiReceiveData(zque->bAdrs, &(zdat->bData[0]), zdat->dwLgth);
    } else {
      TCPRINTF("Invalid bType[%02X]", zque->bType);
    }
  }
  free(header);
  return zque;
}

/**
 * <pre>
 * Description:
 *   This method is called by test case to confirm fifo has data.
 * Input
 *   none
 * Output
 *   bool
 * Example:
 *
 * </pre>
 *****************************************************************************/
Byte hi_qis()
{
  TCPRINTF("hi_qis - just wait 10 msec for Java Host Processing...");

  usleep(10 * 1000);

//#ifdef HI_DEBUG
//    char *fileName =  (char *)malloc(64);
//    snprintf(fileName, 64,  "/tmp/fifo_9001_s_%03d", cell_num);
//    printf("File Name [%s]\n", fileName);
//    FILE *fd = fopen(fileName, "rb");
//    printf("File Open[%p]\n", fd);
////    int fd = hiOpenPipe(9001, 's', cell_num);
//    fseek( fd, 0, SEEK_END );
//    printf("Seek\n");
//    long sz = ftell( fd );
//    printf( "%s File Size:%ld bytes\n", fileName, sz );
//#endif

  return 1; // Return 1 always.
}


// *****************************************************************************/
// From here, use by only hiMain.c. (private method)
//*****************************************************************************/
/**
 * <pre>
 * Description:
 *   Send ZQUE(CMD packet) to host PC PGM.
 *   This method is called by hiPut(void *ptr).
 * Input
 *   ZQUE
 * Example:
 *
 * </pre>
 *****************************************************************************/
void hiSendCMDPkt(ZQUE *zque)
{
  Byte cell_num = zque->bAdrs;
  Byte *type = "CMD ";
  Byte *fileName = NULL;
  Word dataLen = zque->wLgth;
  Byte *dataLenStr = (Byte*)malloc(12);
  memset(dataLenStr, 0, 12);
  sprintf(dataLenStr, "%d", dataLen);

  Byte *header = (Byte *)malloc(HEADER_LENGTH);
  memset(header, 0, HEADER_LENGTH);
  hiCreateHeader(header, type, fileName, cell_num, dataLenStr);

  hiSend(header, &(zque->bData[0]), zque->wLgth);

  free(header);
  free(dataLenStr);
}

/**
 * <pre>
 * Description:
 *   Send ZDATA(FILE packet) to host PC PGM.
 *   This method is called by hiPut(void *ptr).
 * Input
 *   ZDATA
 * Example:
 *
 * </pre>
 *****************************************************************************/
void hiSendFILEPkt(ZDAT *zdata)
{
  ZQUE *zque = &(zdata->zque);
  Byte cell_num = zque->bAdrs;
  Byte *type = "FILE";
  Byte *fileName = zdata->pName;
  Dword dataLen = zdata->dwLgth;
  Byte *dataLenStr = (Byte*)malloc(12);
  memset(dataLenStr, 0, 12);
  sprintf(dataLenStr, "%ld", dataLen);

  Byte *header = (Byte *)malloc(HEADER_LENGTH);
  memset(header, 0, HEADER_LENGTH);
  hiCreateHeader(header, type, fileName, cell_num, dataLenStr);

  hiSend(header, zdata->pData, dataLen);
#ifdef HI_DEBUG
  printf("Dump[%s]\n", zdata->pData);
#endif
  free(header);
  free(dataLenStr);
}

/**
 * <pre>
 * Description:
 *   Create header.
 * Example:
 *      Header length 4Bytes ASCII
 *      Type 4 Bytes ASCII
        Parameter   4 Bytes Binary
        Cell Address    N Bytes (Max 512Bytes)  ASCII
        Data Length N Bytes (Max 12Bytes)   ASCII
        Filename    N Bytes (Max. 256 Bytes)    ASCII
 *
 *         When FILE Packet is sent to Cell #1 and #10,
    FILE[00][????FF00][00]010A[00]1000[00]FILENAME.TXT[00]
   When there is no filename and CMD_ Packet is sent to Cell #1,
    CMD [00][0000FF00][00]01[00]10[00][00]
   When PC sends command to CONT,
    CNTL[00][0000FF00][00]79[00]10[00][00]

 * </pre>
 *****************************************************************************/
void hiCreateHeader(Byte *header, Byte *type, Byte *fileName, Byte cell_num, Byte* dataLenStr)
{
  Byte partationChar = 0x00;
  unsigned int tmpInt = 0;
  Byte *offset = header ;
  unsigned int headerLength = 0;

  //Type : ASCII. 4byte. ex) CMD_, FILE
// printf("TYPE\n");
  memcpy(offset, type, 4);
  offset = offset + 4;
  headerLength = headerLength + 4;

  //Partation 0x00
  memcpy(offset, &partationChar, 1);
  offset = offset + 1;
  headerLength = headerLength + 1;

  //Parameter upper 2bytes.
  //File Name Length : Binary, 2bytes. If fileName == NULL, 0x0000
// printf("File Name\n");
  if (fileName != NULL) {
    unsigned int fileNameLength = strlen(fileName);
    memcpy(offset, &fileNameLength, 2);
  } else {
    tmpInt = 0x0000;
    memcpy(offset, &tmpInt, 2);
  }
  offset = offset + 2;

  //Parameter lower 2bytes.
  //0x00FF fix.
  tmpInt = 0x00FF;
  memcpy(offset, &tmpInt, 2);
  offset = offset + 2;
  headerLength = headerLength + 2;

  //Partation 0x00
  memcpy(offset, &partationChar, 1);
  offset = offset + 1;
  headerLength = headerLength + 1;

  //Cell Num. ASCII 2bytes. Hex.
  Byte *cellnum_str_hex = (Byte *)malloc(2);
  memset(cellnum_str_hex, 0, 2);
  sprintf(cellnum_str_hex, "%02X", cell_num);
  memcpy(offset, cellnum_str_hex, 2);
  free(cellnum_str_hex);
  offset = offset + 2;
  headerLength = headerLength + 2;

  //Partation 0x00
  memcpy(offset, &partationChar, 1);
  offset = offset + 1;
  headerLength = headerLength + 1;

  //Data length. ASCII <12bytes.
  sprintf(dataLenStr, "%s", dataLenStr);
  memcpy(offset, dataLenStr, strlen(dataLenStr));
  offset = offset + strlen(dataLenStr);
  headerLength = headerLength + strlen(dataLenStr);

  //Partation 0x00
  memcpy(offset, &partationChar, 1);
  offset = offset + 1;
  headerLength = headerLength + 1;

  //File Name : ASCII, < 512bytes.
  if (fileName != NULL) {
    memcpy(offset, fileName, strlen(fileName));
    offset = offset + strlen(fileName);
    headerLength = headerLength + strlen(fileName);
  }

  //Partation 0x00
  memcpy(offset, &partationChar, 1);
  offset = offset + 1;
  headerLength = headerLength + 1;

  /*    // Finally input header length offset 0-3
      offset = header;
      Byte* header_length_str = (Byte *)malloc(4);
      sprintf(header_length_str, "%04d", headerLength);
      memcpy(offset,header_length_str, 4);
      free(header_length_str);
  */
}

/**
 * <pre>
 * Description:
 *   This method write header and data to named pipe. This method support exclusion control by flock().
 * Input:
 * Byte* header : Header by createHeader
 * Byte* data : Data by ZQUE or ZDAT
 * Dword wLgth : data length.
 * </pre>
 *****************************************************************************/
void hiSend(Byte* header, Byte* data, Dword wLgth)
{
  int fd_header = hiOpenPipe(9101, 'c', 0);
  flock(fd_header, LOCK_EX);    //get lock
  write(fd_header, header, HEADER_LENGTH);
  int fd_data =   hiOpenPipe(9100, 'c', 0);
  write(fd_data, data, wLgth);
  close(fd_data);
  close(fd_header);     //release lock
}

/**
 * <pre>
 * Description:
 *   Write data to pipe for header. (pipe_9101_c)
 * Input :
 *      cellNum
 *      Byte*header. Buffer for data receive.
 * </pre>
 *****************************************************************************/
int hiReceiveHeader(int cellNum, Byte *header)
{
  int fd = hiOpenPipe(9001, 's', cellNum);
  read(fd, header, HEADER_LENGTH);
  close(fd);
  return 0;
}

/**
 * <pre>
 * Description:
 *   Write data to pipe for data. (pipe_9100_c)
 * Input :
 *      cellNum
 *      Byte*header. Buffer for data receive.
 *      unsigned int dataLen. Data length is written in header.
 * </pre>
 *****************************************************************************/
int hiReceiveData (int cellNum, Byte *data, Word dataLen)
{
  int fd = hiOpenPipe(9000, 's', cellNum);
  read(fd, data, dataLen);
  close(fd);
  return 0;
}

/**
 * <pre>
 * Description:
 *   Get data length from header.
 * Input :
 *      header
 * </pre>
 *****************************************************************************/
void* analysisHeader(int cell_num, Byte *header)
{
  int i = 0;
  int offset = 0;

  char type[4];
  for ( i = 0; i < 4; i++, offset++) {
    type[i] = header[offset];
  }
  offset++; //To skip separater [00];

  Byte param[4];
  for ( i = 0; i < 4; i++, offset++) {
    param[i] = header[offset];
  }
  offset++; //To skip separater [00];

  char cell_address_str[2];
  for ( i = 0; i < 2; i++, offset++) {
    cell_address_str[i] = header[offset];
  }
  offset++; //To skip separater [00];

  char data_length_str[12];
  for ( i = 0; i < 13; i++, offset++) {  // If data length is 12bytes, loop will break at 13 times and skip separater.
    if (header[offset] == 0x00) break;
    data_length_str[i] = header[offset];
  }

  char file_name[256];
  for ( i = 0; i < 13; i++, offset++) {  // If data length is 256bytes, loop will break at 257 times and skip separater.
    if (header[offset] == 0x00) break;
    file_name[i] = header[offset];
  }
  int file_name_length = strlen(file_name);

  Dword data_length = atol(data_length_str);
  if (strcmp(type, "CMD ") == 0) {
    ZQUE *zque =   (ZQUE *)malloc(sizeof(ZQUE) + data_length);
    zque->more = NULL;
    zque->less = NULL;       // Library  link list pointer
    zque->wDID = cell_num;
    zque->wSID = 241;         // User     destination, source ID
    zque->wLgth = data_length;              // User     bData size in bytes. < 64KB.
    zque->bType = ZCmd;              // User     enum ZCmd (= 0x0a) fixed value
    zque->bAdrs = cell_num;              // User     cell/slot address
    memcpy(zque->bP, &file_name_length, 2);
    zque->bP[2] = 0xFF;
    zque->bP[3] = 0x00;
    zque->wDID0 = 0;
    zque->wSID0 = 0;       // User     reserved
    return zque;
  } else {
    ZDAT *zdat = (ZDAT *)malloc(sizeof(ZDAT) + file_name_length + data_length);
    ZQUE *zque = (ZQUE *)malloc(sizeof(ZQUE) );
    zque->more = NULL;
    zque->less = NULL;       // Library  link list pointer
    zque->wDID = cell_num;
    zque->wSID = 241;         // User     destination, source ID
    zque->wLgth = data_length;              // In case of FILE packet, 0;
    zque->bType = ZData;              // User     enum ZCmd (= 0x0a) fixed value
    zque->bAdrs = cell_num;              // User     cell/slot address
    zque->bP[0] = 0x00;
    zque->bP[1] = 0x00;
    zque->bP[2] = 0xFF;
    zque->bP[3] = 0x00;
    zque->wDID0 = 0;
    zque->wSID0 = 0;       // User     reserved

    zdat->zque = *zque;               // User     see ZQUE definition. 'bType' shall be enum ZData (= 0x04)
    zdat->dwLgth = data_length;             // User     *pData size in bytes. < 64KB
    zdat->que = NULL;               // User     reserved
    zdat->bReturn = 0;
    for ( i = 0; i < 2; i++) {
      zdat->wAdrs[i] = 0;            // User     reserved
    }
    memset(&(zdat->bData[0]), 0, file_name_length + data_length);
    memcpy(&(zdat->bData[0]), file_name, file_name_length);
    zdat->pName = &(zdat->bData[0]);             // User     pointer to file name string (data instance is allocated in bData[])
    zdat->pData = zdat->pName + file_name_length + 1;
    return zdat;
  }
}



