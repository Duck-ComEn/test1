//********************************************************************************************
// multithread.cpp:  application to test
//  behaviour of mmulltiple threads driving into a single (shared) instance of the Env Class

#define APP_NAME  "multithread" 

#define VERSION  "v0.0a"  // 23Apr10 -


  #include <unistd.h>
  #include <errno.h>
  #include <sys/time.h> // for gettimeofday()
  #define GET_TIME( x )  	gettimeofday( x, NULL);
  #define TIME_TYPE   struct timeval
  
  #define SLEEP_MS( mS)  usleep ( ms * 1000 )
	


unsigned long ElapsedTimeInMs ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd);


//********************************************************************************************

#include <stdio.h>
#include <time.h>
#include <string.h>
	#define MATCH 0  // makes use of strcmp() etc more readable
#include <stdlib.h>
#include <errno.h>


#include <signal.h>
#include <pthread.h>
#include <unistd.h>  // for sleep

// These xyratex headers need to be found via -I option to compiler in makefile                

#include XCAL_COMPAT_INCLUDE_FILENAME	// compat. values/defines (Xcalibre vs Optimus)

#include <sys/time.h> // for gettimeofday()
#define GET_TIME( x )  	gettimeofday( x, NULL);
#define TIME_TYPE   struct timeval


CELLENV * pCellEnv=NULL; // Global (set/constructed by main thread(eg Chamber), accessed by slave threads (eg slots/drives)


uint8 u8CellIndex = 0;
uint8 u8EnvId=DEF_ENVIRONMENT_ID;  // default ?


void * EnvThreadFunc( void * arg); // prototype

//--------------------------------------------------------------------------------------------

void Usage( void)
{
	printf("Usage:\n");
	printf(" %s  <PowerFailRecoveryFlag> <CellIndex> <TrayIndex>\n", APP_NAME);
	printf("\n");
	printf("      Note: <PowerFailRecoveryFlag> must be present, but is ignored\n");
	printf("\n");
}


int main( int argc, char *argv[] )
//--------------------------------------------------------------------------------------------
{
	xyrc xyrc = xyrcOK;  
  
	setbuf(stdout, NULL); // makes sure those printfs get straight out!

	printf("\n%s %s\n\n", APP_NAME, VERSION);


	// parse args
	//-----------------------------------------------------------------------
	if ( argc != 4) 
	{
		Usage();
		exit(1);
	}  

	u8CellIndex = (uint8) atoi (argv[2]);
  u8EnvId = (uint8) atoi (argv[3]);

	// Initialse
  //-----------------------------------------------------------------------
	printf("main: Constructing an instance of CELLENV CellIndex=%d, EnvId=%d\n", u8CellIndex, u8EnvId);
	pCellEnv = CELLENV::CreateInstance( (uint32) u8CellIndex,u8EnvId );

	
	// make sure cell is on-line 
	if ( pCellEnv->IsCellOnlineOnUSB() != xyrcOK)  
	{ 
		printf("main: Cell NOT ONLINE on USB\n");
		exit(1);
	}  
	
	
	int16 i16CurrentTemp = 0;
	xyrc = pCellEnv->GetCurrentTemp(  i16CurrentTemp ); 
	if (xyrc != xyrcOK)
	{
		printf("main:GetCurrentTemp() FAILED - xyrc= %s (%d)\n", CELLENV::ConvertXyrcToPsz(xyrc), xyrc);
		exit(1); // bail-out now !!!
	}  
	printf("Main: CellOnline - Current Temp=%d\n", i16CurrentTemp);
	
	
	
	// Now ready to test, 
	// by creating two threads, each of which then calls methods in the SINGLE shared insance of pCellEnv class created above 
	// (ie both threads use the same global pCellEnv) 
	#define NUM_THREADS 2	
	pthread_t thd[NUM_THREADS+1];
	
	for (int threadnum = 0; threadnum < NUM_THREADS; threadnum++)
	{
	  int ret = pthread_create( &thd[threadnum], NULL, EnvThreadFunc, (void *)threadnum);
	  if (ret != 0)
	  {
	  	printf("Main: pthread_create(%d) failed ret=%d - ABORT\n", threadnum, ret);
	  	exit(0);
	  } 
	}
	

	//printf("main: sleeping 20 secs before ending\n");
	//sleep(20);
	
	// wait for signal to die
	printf("main: waiting for signal to die (Ctr-C when done)\n");
	while (true)
	{
		sleep(1);
	}
	
	
	
   
	printf("main: exiting\n");
	return 0;
}



unsigned long ElapsedTimeInMs ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd)
//---------------------------------------------------------------------------
{
	unsigned long ElapsedMs;
	unsigned long ElapsedWholeSecs;
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
	return ElapsedMs;
}



//--------------------------------------------------------------------------------------------

void * EnvThreadFunc( void * arg)
{
	// arg is an interger (our number)
	int id = (int) arg;
	printf("  EnvThreadFunc_%d: started \n", id);
	
	for (int i=1; i<10000; i++)
	{
		TIME_TYPE x;
		GET_TIME (&x);
		
		int16 i16CurrentTemp;
		xyrc xyrc = pCellEnv->GetCurrentTemp( i16CurrentTemp);
		
		TIME_TYPE y;
		GET_TIME (&y);
		
		if (xyrc!=xyrcOK)
		{
			printf("  EnvThreadFunc_%d: FAILURE!!!: loop %.6u, GetCurrentTemp failed xyrc= %s (%d)\n", id, i, CELLENV::ConvertXyrcToPsz(xyrc), xyrc);
		}
		else
		{
			printf("  EnvThreadFunc_%d: loop %.6u, CurrentTemp=%d (took %lumS)\n", id, i, i16CurrentTemp, ElapsedTimeInMs( &x, &y) );
		}	
		//sleep(1);
	}	
	
	printf("  EnvThreadFunc_%d: ending \n", id);
	return NULL;
}


