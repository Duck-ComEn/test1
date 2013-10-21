#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/fcntl.h>
//#include <sys/file.h>
#include <unistd.h>
#include "../testcase/tcTypes.h"
#include "../testscript/ts.h"

//#define HIPIPE_DEBUG

//#ifdef HIPIPE_DEBUG
//#define PIPE_DIR "../"
//#else
#define PIPE_DIR "/tmp/"
//#endif

#define MAX_FILE_NAME_LENGTH 64



/**
 * <pre>
 * Description:
 *   Open pipe.
 *    Input : Port :  9000/9001/9100/9101
 *               Type :  'c' as TestCase => Host
 *                            's' as Host => TestCase
 *               cellNum : Cell Number
 *    Output : File descripter. If error, return -1.
 * Example:
 *    Pipe name : PIPE_DIR + port + "_" + type + "_" + cellNum(%03d) + ".pipe"
 *    Type :  'c' as TestCase => Host
 *                 's' as Host => TestCase
 * </pre>
 *****************************************************************************/
int hiOpenPipe(int port, char type, int cellNum)
{
  int file_descripter = -1;
  int access_mode;
  int error_flag = 0;
  char *target_file_name;

  target_file_name = (char *)malloc(MAX_FILE_NAME_LENGTH);
  memset(target_file_name, 0, MAX_FILE_NAME_LENGTH);
//     snprintf(target_file_name, MAX_FILE_NAME_LENGTH,  "%s%d_%c_%03d.pipe", PIPE_DIR, port, type, cellNum);

  if (type == 's') {
    snprintf(target_file_name, MAX_FILE_NAME_LENGTH,  "%sfifo_%d_%c_%03d", PIPE_DIR, port, type, cellNum);
  } else if (type == 'c') {
    snprintf(target_file_name, MAX_FILE_NAME_LENGTH,  "%sfifo_%d_%c", PIPE_DIR, port, type);
  } else {
    TCPRINTF("Unsupported type [%c].", port);
    snprintf(target_file_name, MAX_FILE_NAME_LENGTH,  "com_error");
    error_flag = 0;
  }

  if (type == 'c') {
    access_mode = O_WRONLY ;
  } else if (type == 's') {
    access_mode = O_RDONLY;
  } else {
    TCPRINTF("Unsupported type [%c]. Then open as O_RDONLY.", type);
    access_mode = O_RDONLY ;
    error_flag = 0;
  }

  //Confirm pipe exists.
  if (access(target_file_name, F_OK)) {
    TCPRINTF("File NOT exists error.[%s]", target_file_name);
  } else {
    if (access(target_file_name, W_OK | R_OK)) {
      TCPRINTF("R/W NOT acceptable error.[%s]", target_file_name);
    } else {
      file_descripter = open(target_file_name, access_mode );
      if (file_descripter == -1) {
        TCPRINTF("File NOT open error.[%s]", target_file_name);
      }
    }
  }

  free(target_file_name);

  if (error_flag) {
    return -1;
  }
  return file_descripter;
}

#ifdef HIPIPE_DEBUG
int main()
{
  printf("Try to Open FIFO\n");
  int fd_header = hiOpenPipe(9101, 'c', 0);
  flock(fd_header, LOCK_EX);
// int fd_data = hiOpenPipe(9100, 'c', 0);
  printf("FIFO Opened[%d]\n", fd_header);
  printf("Sleep 30sec\n");
  sleep(30);
  printf("Exit sleep\n");
  close(fd_header);
// close(fd_data);
  printf("Close FIFO\n");
// int file_descripter = hiOpenPipe(9101, 'c', 1);
// char *buf;
// buf = malloc(5);
// memset(buf, 0, 5);
// long len = hiReadPipe(file_descripter, buf, 5);
// close(file_descripter);
// free(buf);
  return 0;
}
#endif

