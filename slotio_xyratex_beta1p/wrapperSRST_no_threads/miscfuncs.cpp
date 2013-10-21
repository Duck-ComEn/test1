#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>     // for time, ctime etc
#include <stdarg.h>   // for varargs
#include <sys/stat.h>  // for getpid

//#include "xytypes.h"  // sets OS_IS_*
#define OS_IS_LINUX
#if defined  OS_IS_WINDOWS
	#include <process.h>  // for getpid
	
#elif defined OS_IS_LINUX
	#include <unistd.h>  // for getpid

#endif  


#include "libsi.h"  // the hgst generic slot-interface to different suppliers (defined and supplied by HGST)
                    // NOTE: we also get the HGST-preffered typedefs Byte, Word, Dword etc from here (via its inclusion of tcTypes.h)

#include "wrapperSRST_common.h"  // for APP_NAME

#include "miscfuncs.h"  // own header (for function prototypes)


bool WantToUseTraceInfoLine = true;
bool WantToLogInfoLines = false;
bool WantLogging = true;
bool WantPrintLogToMirrorToStdout = true;  // application should set to false if SpawnedByTestManager
bool WantCommonLogfile = true; //  true: filename=<appname>.log    (but entries auto-prefixed by CellIndex&Tray)
															 // false: filename=<appname><CellIndex><TrayIndex>.log
															 // eterned - so main can override this default



#include "TMFifoInterface.h" // simple class allowing us to feedback attributename:value to TestManager
															//   (eg for tescase progress via TestInfoLine: attribute)

//---------------------------------------------------------------------------
void UpdateTMTestInfoLine( char * message, ... )
//---------------------------------------------------------------------------
{
	char szBuf[1024];
	va_list ap;
	va_start(ap, message);
	vsprintf(szBuf, message, ap);
	va_end(ap);

	// might as well log it too
	// NOW CONTROLLABE via -nli
	if (WantToLogInfoLines)
	{
		printlog( szBuf);
	}
				
	if (WantToUseTraceInfoLine)
	{
		// NOTE THESE ARE STATICS (persistant)!!!!!!!!!!!!!!!!!!!!!!!!!!!
		static bool FirstCall = true;  // STATIC
		static CTMFifoInterface * pTMFifo = NULL; // STATIC

		if (FirstCall)
		{
			if ((CellIndex==-1) || (TrayIndex==-1))
			{
				printlog("UpdateTMTestInfoLine: WARNING: did not try to open FifoInterface to TestManager - since CellIndex, TrayIndex not yet set/parsed!!!\n");
			}
			else
			{
				pTMFifo = new CTMFifoInterface( CellIndex, TrayIndex);
				if (pTMFifo->CheckInstantiationOK() == false)
				{
					printlog("UpdateTMTestInfoLine: WARNING: Could not open FifoInterface to TestManager\n");
					printlog(" - so cannot feedback testcase progress via TestInfoLine: attribute\n");
					pTMFifo = NULL;
				}
				FirstCall = false;
			}	
		}


		if (pTMFifo != NULL)
		{
			// remove any newlines in attribute value (XYRMI cannot handle them as it is 'line-based' itself
			char  * p = strchr( szBuf, '\n');
			while (p != NULL)
			{
				*p = '~';
				p = strchr( szBuf, '\n');
			}

			// now send it
			pTMFifo->WriteToFifo( "TestInfoLine", szBuf);
		}
	}
	else
	{
		// dont want to use TraceInfoLine
		// - so just leave it at the printlog above
	}
}





//---------------------------------------------------------------------------
void printlog ( const char* description, ...)
//---------------------------------------------------------------------------
{
	if (WantLogging)
	{
		if (WantPrintLogToMirrorToStdout)
		{
			// also do a normal printf first
			// - but note that TestCases strarted by TestManager may not have a stdout or console!!
			va_list ap;
			va_start(ap, description);
			vfprintf(stdout, description, ap);
			va_end(ap);
			if (strchr( description, '\n') == NULL) // add a newline only if not one already
			{ 
				fprintf(stdout, "\n");
			} 
		}


		// now write the same information to a logfile
		// (the only way to see it in the 'real' world)
		char szFilename[255];
		if (WantCommonLogfile)
		{
			// COMMON logfile (for this testcase - all cells and trays) 
			sprintf( szFilename, "%s/%s.log", LOG_FILE_DIRECTORY, APP_NAME);
			FILE * fp = fopen ( szFilename, "a");     // a=open for append (or create if doesnt exist)
			if (fp != NULL)
			{             
				time_t time_now;
				time( &time_now);
				fprintf(fp, "%20.20s %.3d %.1d (%5u) ",
										ctime( &time_now ),  // limit this to 20 chars to remove year and newline
										CellIndex,
										TrayIndex, 
										getpid()
							);     
										
				va_list ap;
				va_start(ap, description);
				vfprintf(fp, description, ap);
				va_end(ap);

				if (strchr( description, '\n') == NULL) // add a newline only if not one already
				{ 
					fprintf(fp, "\n");
				} 
				
				fclose(fp);
			}   
			
		}
		else
		{
			// SEPARATE LOGFILES per testcase+cell+tray
			sprintf( szFilename, "%s/%s%.3d%.1d.log", LOG_FILE_DIRECTORY, APP_NAME, CellIndex, TrayIndex);
			FILE * fp = fopen ( szFilename, "a");     // a=open for append (or create if doesnt exist)
			if (fp != NULL)
			{             
				time_t time_now = time(NULL);
				fprintf(fp, "%20.20s  ",
										ctime( &time_now )  // limit this to 20 chars to remove year and newline
							);     
										
				va_list ap;
				va_start(ap, description);
				vfprintf(fp, description, ap);
				va_end(ap); 
					
				if (strchr( description, '\n') == NULL) // add a newline only if not one already
				{ 
					fprintf(fp, "\n");
				} 
				
				fclose(fp);
			}   
		}

		// ALWAYS check for filesize exceeding our limit
		//-----------------------------------------------
		// if now > MAX size then rename to .previous 
		struct stat st;
		if ( stat( szFilename, &st)  == 0)
		{
			if (st.st_size >= LOG_FILE_MAX_KBYTES * 1024)  
			{
				char szCmd[256];
				#ifdef  OS_IS_WINDOWS
					sprintf(szCmd, "move /Y  %s  %s.previous", szFilename, szFilename);
				#else 
					sprintf(szCmd, "mv -f  %s  %s.previous", szFilename, szFilename);
				#endif
				system( szCmd );    // 'system' blocks till complete
			}   
		}  

	} // if WantLogging

}                                                           




unsigned long ElapsedTimeInMs ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd)
//---------------------------------------------------------------------------
{
	unsigned long ElapsedMs;
	unsigned long ElapsedWholeSecs;
	
	#if defined OS_IS_WINDOWS
	
		unsigned long Elapsed_ms_part;
		ElapsedWholeSecs = (unsigned long) (ptvEnd->time - ptvStart->time);
		if (ptvEnd->millitm >= ptvStart->millitm)
		{
			Elapsed_ms_part = ptvEnd->millitm - ptvStart->millitm;
		}
		else
		{
			Elapsed_ms_part = (ptvEnd->millitm + 1000) - ptvStart->millitm;
			ElapsedWholeSecs--;
		}
		ElapsedMs = (ElapsedWholeSecs * 1000) + Elapsed_ms_part;
		
	#elif defined OS_IS_LINUX
		unsigned long Elapsed_us_part;
		ElapsedWholeSecs = ptvEnd->tv_sec - ptvStart->tv_sec;
		if (ptvEnd->tv_usec >= ptvStart->tv_usec)
		{
			Elapsed_us_part = ptvEnd->tv_usec - ptvStart->tv_usec;
		}
		else
		{
			Elapsed_us_part = (ptvEnd->tv_usec + 1000000) - ptvStart->tv_usec;
			ElapsedWholeSecs--;
		}
		ElapsedMs = (ElapsedWholeSecs * 1000) + (Elapsed_us_part/1000);
		
	#else
		#error "not Windows or Linux ??"
	#endif

	return ElapsedMs;
}



void setworkingdirectory( char *FromFullyQualifiedExeName)
//---------------------------------------------------------------------------
{
	// SET WORKING DIRECTORY
	// NOTE: Currently, Testcases started via TestManager inherit TestManager's Current Working Directory ( /usr/xcal/run )
	// so we SET our current working directory to be the same as the path to our exe (derived from argv[0])
	char *p = FromFullyQualifiedExeName;
	for (size_t i = strlen(p); i > 0; i--)   // search backwards for first slash
	{
		if ( (p[i] == '/') || (p[i] == '\\') )
		{
			p[i] = '\0';
			break;
		}
	}
	int chrc = chdir( p);
	if (chrc)
	{
		fprintf( stderr, "error changing working directory to %s, (errno=%d=%s)\n", p, errno, strerror(errno) );
	}
	else
	{
		//fprintf( stderr, "changed working directory to %s\n", p );
	}
}
