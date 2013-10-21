
#define LOG_FILE_DIRECTORY   "."   
#define LOG_FILE_MAX_KBYTES  1000

extern bool WantToUseTraceInfoLine;
extern bool WantToLogInfoLines;
extern bool WantLogging;
extern bool WantPrintLogToMirrorToStdout;
extern bool WantCommonLogfile;



#define PRINTF_STYLE(fmt_paramnum,firstarg_paramnum)	__attribute__ ((format (printf,fmt_paramnum,firstarg_paramnum)))

void UpdateTMTestInfoLine( char * message, ... )  PRINTF_STYLE(1,2);  // ChrisB - this gives benefits of compiler arg-type-match checking !!!!

void printlog ( const char* description, ...)  PRINTF_STYLE(1,2);  // ChrisB - this gives benefits of compiler arg-type-match checking !!!!

void setworkingdirectory( char *FromFullyQualifiedExeName);





#if defined OS_IS_WINDOWS

  #include <sys/timeb.h> // for ftime (similar to linux gettimeofday() )
  #define GET_TIME( x )  	ftime( x );
  #define TIME_TYPE   struct timeb
  
  #define SLEEP_MS( mS)  Sleep ( ms )
	
#elif defined OS_IS_LINUX

  #include <unistd.h>
  #include <errno.h>
  #include <sys/time.h> // for gettimeofday()
  #define GET_TIME( x )  	gettimeofday( x, NULL);
  #define TIME_TYPE   struct timeval
  
  #define SLEEP_MS( mS)  usleep ( ms * 1000 )
	
#endif


unsigned long ElapsedTimeInMs ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd);






















