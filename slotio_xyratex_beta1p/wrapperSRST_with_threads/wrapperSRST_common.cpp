#define APP_VERSION 	"v8.0b"   // 25May10 fix bCellNo calculations (to match Taka's change to library for 3DP, and also change lib for Optimus)
                                //          also, support micro-Secs resolution on statistics
                                
//#define APP_VERSION 	"v8.0a"   // first  WRAPPER version using multi-threading

//#define APP_VERSION 	"v9.0a"   // first version using WRAPPER

	// created from simpleSRST "v0.0b"   // 21Apr10:

//=========================================================================
// Simple, sample testcase USING HGST-generic WRAPPER API to access the cell !!!!!!!!!!!!
// Demonstrating:
//   Parsing cmdline arguments 
//      (in particular extraction of CellId and TrayIndex)
//   Initialising wrapper
//   Checking Drive-Plugged
//   Controlling Temperature
//   Powering Drive
//   Talking Serial to Drive 
//   Sending test-progress information back to TestManger
//      (using the TMFifoInterface class)
//   Sending a test result to Test Manger via our exit code
//=========================================================================


// OS-independent system headers
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <pthread.h>
#include <sys/types.h>


#define OS_IS_LINUX


#include "libsi.h"  // the hgst generic slot-interface to different suppliers (defined and supplied by HGST)
                    // NOTE: we also get the HGST-preferred typedefs Byte, Word, Dword etc from here (via its inclusion of tcTypes.h)

#include "libsi_xyexterns.h" // // for lib's externs of Tracing flags, pstats etc



#define WANT_DEFINE_CONVERSION
#include "wrapperSRST_common.h"  // own header (testcase #defines etc)

#include "miscfuncs.h" // interface to our lower-level functions (printlog etc)


#include "CellTypes.h" // OPTIONAL (for xyrc enum...

// prototype functions AT END OF THIS FILE
void tc_printf(char *bFileName, unsigned long dwNumOfLine, char *description, ...); // HGST dictated name/args format IMPLEMENTATION NEEDS TO BE PROVIDED BY ANY USER OF libsi !!!!!!!!!
void WaitForOnTempIndication( Word AwaitedTempInHundredths, int max_wait_secs, bool PrintTempsWhileWaiting);
bool CheckForAndReportAnyCellStatusErrors( bool InitialCall);  //(returns true if there IS a problem) 
void EnvAttemptToMakeCellSafe( void);


#define MATCH 0  // makes use of strncmp() more readable

#define printf printf_NOT_ALLOWED  // catch any stray printfs - note NO CONSOLE for testcases started by TestManager!!!!

#define TCLOG  tc_printf(__FILE__, __LINE__,  


// globals - generally set by cmd line args
//-----------------------------------------
int CellIndex = -1; // initialise to 'invalid' - for benefit/protection of miscfuncs/UpdateTMTestInfoLine() 
int TrayIndex = -1; // for 3DP typically 0=lower or 1=upper or 2=both  
bool PowerFailRecoveryFlag = false;

static Byte bCellNo = 0;


// On a 3DP cell, UART  ChannelID is 0,1 (lower,upper) - exactly the same as TrayIndex
int	UART_ChannelId = -1;  // Set in code from TrayIndex  TODO: !!!!!!!!!!!!!!!!!!!! (extend to both drives at once -!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
bool WantUpperTray = false;// (these are now set by anyalysing the TrayIndex parameter)
bool WantLowerTray = false;


int LoopCount = 1;   
int InfoUpdateFrequencySeconds = 1; // -u option

bool WantAPITrace = false;   //  -t option 
bool JustGoGreenAndQuiet = false;  // -g option

bool PerformanceTestMode = false; // if set by -perf option, affects amount of logging, housekeeping, CPU/USB 'niceness' etc.

bool NoFansMode = false; // if set by -nf option, FPGA is set to testmode to allow use of supplies etc. on bench without fans

Word u16nBytesOfDriveDataToBeRead = 508; // can be overridden via -s<nnn>


bool WantPlugCheck = true;
bool WantRampTemp = true;
bool WantPowerDrive = true;
bool WantSerialComms = true; // use -ns to clear

int EnvironmentId = -1;  // valid range for 3DP is 0 ONLY

bool LeavePowerOnAfterTest = false;
int i16RampTempInTenths = 350; // 35 Degrees C            
int i16SafeHandlingTempInTenths = 400; // 40 Degrees C

Word u16TestRampRateInTenthsPerMinute = 60;  // 1 tenth per second !  - changeable via -rr cmdline parameter 
 
bool clean_exit_requested = false;

bool WantQuietFans = false; // -quietfans
  Word slowDriveRPM = 1700;
  Word slowElecRPM = 1900;

bool DontTerminateOnBadXYRCs = false;

Byte MyExitCode = EXITCODE_TESTCASE_FAILED_UNDEFINED; // initialise to something other than PASS

bool flSignalProcessed = false; // set by signal handler - affects out exit code at (premature) end of test

bool SpawnedByTestManager = false; // set by examining our process' ENVIRONMENT (variables)

bool WantNEWPROTOCOL = false; // set via -NEWPROTOCOL (now conveyed via Taka's new API in silib)

bool WantPin11Low = false; // set via -pin11_low

bool Want_HANDLE_STATS_IN_MICRO_SECS = true;
time_t TimeAppStarted = time( NULL); // used to endure TC always runs for at least 3 secs (requirement of TestManager - to be re-evaluated one day!!!)

bool siInitialise_successful = false;


bool WantFirstDrive = false;  // easiest way to help out with statitics printing code in loop below
bool WantSecondDriveInXcalibre = false;  // easiest way to help out with statitics printing code in loop below


//-----------------------------------------------------------------------------
void Usage(void)
//-----------------------------------------------------------------------------
{
	printlog("\n");
	printlog("usage: %s [options] <PowerFailRecoveryFlag> <CellIndex> <TrayIndex>\n", APP_NAME);
	printlog("\n");
	printlog("  PowerFailRecoveryFlag: 0 or 1\n");
	printlog("      0 = normal start\n");                                      
	printlog("      1 = RE-start following power-fail during previous run\n");
	printlog("\n");
	printlog("  CellIndex:\n");
	printlog("      Linux   - CellIndex (eg 1-72)\n");                                      
	printlog("      Windows - XYDevNum  (eg 0-255)\n");
	printlog("      significance: = what we need to pass to constructor of Cell Interface Classes\n");
	printlog("\n");
	printlog("  TrayIndex: (0=lower,1=upper (also allow 2=both) idenifies which tray in the 3DP Cell\n");                                      
	printlog("      significance: eg.\n");
	printlog("         Relates to Drive, PSUs, UART ChannelId etc...\n");
	printlog("\n");
	printlog("\n");
	printlog("  optional switches:\n");                                      
	printlog("\n");
	printlog("   -nc             dont Check for drive-plugged \n");
	printlog("   -np             dont Power drive(s) on at start");
	printlog("   -nr             dont Ramp temp\n");
	printlog("   -ne             dont power drive(s) off at End\n");
	printlog("\n");
	printlog("   -rt<RampTempInTenths>          (default=%d)\n", i16RampTempInTenths);
	printlog("   -rr<RampRateInTenthsPerMinute> (default=%d)\n", u16TestRampRateInTenthsPerMinute);
	printlog("   -S<SafeHandlingTempInTenths>   (default=%d)\n", i16SafeHandlingTempInTenths);
	printlog("\n");
	printlog("   -l<loopcount>  number of test loops (default=1)\n");
	printlog("   -u<seconds>    update frequency (secs) (if using TestInfoLine) (default=%d)\n", InfoUpdateFrequencySeconds); // new 25Mar04
	printlog("\n");
	printlog("   -t             enable debug Trace (in Xyratex Classes)\n");
	printlog("   -i             don't use TestInfoLine attribute (TMFifoInterface)\n");
	printlog("   -p             mirror printlog() to stdout (in addtion to logfiles)\n");
	printlog("   -g             just GoGreenAndQuiet and exit \n");
	printlog("   -xyrcignore    Ignore bad xyrcs (WHEN IN MAIN LOOP) (ie dont figure them a reason to terminate testcase)\n"); // new 4Nov09
	printlog("\n");
	printlog("   -perf          performance-measuring mode (much less print-logging)\n");
	printlog("   -s             size of actual drive data to be read (default=%u)\n", u16nBytesOfDriveDataToBeRead);
	printlog("   -nf            nofans mode (override some FPGA interlocks (for Bench debug only!!!))\n");
	printlog("   -nli           no logging of infolines (smaller logs - easier to spot error details)\n");
	printlog("   -ns            No serial (Dont attempt any serial comms to the drive)\n");
	printlog("   -quietfans     Put fans into PRM control Mode, and set slow (legal) RPMs - do not use for Dynamic or serious thermal testing!\n");
	printlog("   -NEWPROTOCOL   (affects checksum, odd-byte padding etc...) - default = old protocol\n");
	printlog("   -pin11_low     Drive pin11 low before powering drive  (default is to float it, which suits most newer SRST drives)\n");

}



//-----------------------------------------------------------------------------
int main( int argc, char *argv[] )
//-----------------------------------------------------------------------------
{ 
	
	//-------------------------------------------------------------
	// MAIN THREAD = Chamber-thread
	//
	//   spawns 1 slot/drive thread for Optimus
	//   spawns 1 or 2 slot/drive threads for Xcalibre (two when TrayIndex 2 (=both) specified on cmdline)
	//-------------------------------------------------------------
	
	WantCommonLogfile = false;  // new default (change via -slf)  (declared in misc.c)

	// Need to do this FIRST (since it affects if and where prints and logs go to etc.)
	// (Note this is a relatively recent idea - supported by TestManagers in the 4.x series onwards)
	#ifdef OS_IS_LINUX
		SpawnedByTestManager = (getenv( "SPAWNED_BY_TESTMANGER") == NULL) ? false: true;
	#else
		SpawnedByTestManager = (GetEnvironmentVariable( "SPAWNED_BY_TESTMANGER", NULL, 0) > 0) ? true : false;
	#endif
	if (SpawnedByTestManager)
	{
		setworkingdirectory(argv[0]); // vip to do this first (can affect log file location, .ini access etc)
		WantPrintLogToMirrorToStdout = false; // miscfuncs.h (just in case default was true)
		WantToUseTraceInfoLine = true; // miscfuncs.h
		WantToLogInfoLines = false; //
		WantLogging = true; // choose
	}
	else
	{
		WantPrintLogToMirrorToStdout = true; // miscfuncs.h (just in case default was false)
		WantToUseTraceInfoLine = false; // miscfuncs.h
		WantToLogInfoLines = true;
		WantLogging = true;  // choose
	}
	
	
	
// Note: any calls to UpdateTMTestInfoLine before ParseCmdLineArguments() will now just get rejected (since our cellIndex/TrayIndex are initialised to invalid)
	//  - since it relies on parsed CellId and TrayIndex to construct the pipe name back to TestManager!
	bool parse_rc = ParseCmdLineArguments( argc, argv);
	if (parse_rc == false)
	{
		EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_CMDLINE_ERROR );  // NO RETURN !!!!!
	}   

	printlog("Testcase %s %s\n", APP_NAME, APP_VERSION);
	printlog("=================================================\n\n");

	if (PowerFailRecoveryFlag == true)
	{
		printlog("POWER FAIL RECOVERY FLAG WAS supplied as SET !");  // of no consequence to this simple testcase
	}

 
	
	// these are externed from wrapper lib (see end of libsi.h)
	WantLogAPIInfo = true; // true=default anyway
	if (PerformanceTestMode)
	{
		WantLogAPITrace = false;	
		WantLogAPIEntryExit = false;
	}
	else
	{
		WantLogAPITrace = true;	
		WantLogAPIEntryExit = true;
	}
 
 

	// We need to catch signals so we can exit gracefully
	// ie leaving the cell in a safe/sensible state
	// It is advisable to avoid, where possible, having our process killed off whilst
	//   within a Xyratex Cell Method (this can leave the cell firmware in an
	//   indeterminate state, which must be recovered next time you try to use the cell.
	//
	// myhandler will set a flag and continue
	//  (main loop will test flag at suitable times and exit gracefully)
	signal(SIGINT,  myhandler);
	signal(SIGTERM, myhandler);
	#ifdef OS_IS_LINUX
		signal(SIGQUIT, myhandler);
		signal(SIGHUP,  SIG_IGN);
	#endif



	//================================================================================================
	//================================================================================================
	// THREADING MODEL
	//
	// This (main) thread handles Thermals etc (equiv of hgst Chamber thread)
	//
	// seperate thread(s) are created to handle each DRIVE/SLOT (voltage, and serial etc)
	//================================================================================================
	//================================================================================================
	/*
	  hgst cell id is one start (1, 2, ...). 
	  It is created here from supplied supplier cell/slot id 
	  (and then converted back to supplier values in the library)
	
	  xc3dp: bCellNo = (cellIndex - 1) x 2  + (trayIndex)     + 1
	  op:    bCellNo = (cellIndex - 1) x 12 + (trayIndex - 1) + 1
	
	  note - only xc3dp trayindex is zero-base value
	*/

	#if defined OPTIMUS //----------------------------------------
	
		// Main thread and single drive thread both use same bCellNo (since they both refer to same slot/drive)
	
		bCellNo = ((CellIndex-1)*12) + (TrayIndex-1) + 1;
		
		// Doesn't matter if they both call siInitialise, as long as someone does.
		Byte sirc = siInitialize( bCellNo );
		if (sirc != 0)
		{
			if (sirc == xyrcCellNotPlugged)
			{
		    printlog("rc from siInitialize shows xyrcCellNotPlugged");
			  EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_CELL_NOT_ONLINE );  // NO RETURN !!!!!
			}
			else
			{
				char Reason[80];
				siGetReasonStringForReturnCode( sirc, Reason, sizeof(Reason));
		    printlog("bad rc of %u from siInitialize shows %s", sirc, Reason);
			  EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_XYAPI_ERROR );  // NO RETURN !!!!!
			}
		}	
		else
		{
			siInitialise_successful = true;
		}
		

		// Start the single Drive thread (same bCellNo as main)	
		void * DriveThreadFunc( void * arg);  // in drivethread.cpp

		pthread_t thd;
	  int ret = pthread_create( &thd, NULL, DriveThreadFunc, (void *)bCellNo);
	  if (ret != 0)
	  {
	  	printlog("Main: pthread_create( bCellNo %d) failed ret=%d - ABORT\n", bCellNo, ret);
	  	exit(0);
	  } 
  	printlog("Main: pthread_create( bCellNo %d) OK\n",  bCellNo);

	  WantFirstDrive = true;  // easiest way to help out with statitics printing code in loop below
	
	
	#elif defined XCALIBRE //----------------------------------------
 
		// Main thread and drive threads both use different bCellNo's (since they both refer to differnt slots/drives)
 		
 		bCellNo = ((CellIndex-1)*2) + 0 + 1; // main on Tray 0 (by convention)
 		
 		
		// Doesn't matter if they both call siInitialise, as long as someone does.
    printlog("xcal main thread: calling siInitialize");
		Byte sirc = siInitialize( bCellNo );
		if (sirc != 0)
		{
			if (sirc == xyrcCellNotPlugged)
			{
		    printlog("rc from siInitialize shows xyrcCellNotPlugged");
			  EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_CELL_NOT_ONLINE );  // NO RETURN !!!!!
			}
			else
			{
				char Reason[80];
				siGetReasonStringForReturnCode( sirc, Reason, sizeof(Reason));
		    printlog("bad rc of %u from siInitialize shows %s", sirc, Reason);
			  EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_XYAPI_ERROR );  // NO RETURN
			}
		}	
		else
		{
			siInitialise_successful = true;
		}


		// Start one or two Drive threads (based on supplied TrayIndex)
		// NOTE: one of them may have DIFFERENT bCellNo from main !!!!!!!!!!!!!!!!!!!!	
		//----------------------------------------------------------------------------
		void * DriveThreadFunc( void * arg);  // in drivethread.cpp
		
 		if ((TrayIndex == 0) || (TrayIndex == 2))
 		{
 			Byte bCellNo_drive_0 = ((CellIndex-1)*2) + 0 +  1;  // Tray 0
	    printlog("xcal main thread: starting drivethread tray 0");
			pthread_t drivethread_0;
		  int ret = pthread_create( &drivethread_0, NULL, DriveThreadFunc, (void *)bCellNo_drive_0);
		  if (ret != 0)
		  {
		  	printlog("Main: pthread_create DriveThreadFunc_0( bCellNo %d) failed ret=%d - ABORT\n", bCellNo_drive_0, ret);
		  	exit(0);
		  } 
		  printlog("Main: pthread_create DriveThreadFunc_0( bCellNo %d) OK\n", bCellNo_drive_0);
 		  WantFirstDrive = true;  // easiest way to help out with statitics printing code in loop below

 		}

 		if ((TrayIndex == 1) || (TrayIndex == 2))
 		{
 			Byte bCellNo_drive_1 = ((CellIndex-1)*2) + 1 + 1; // Tray 1
 			
 			// NOTE: SOMEONE needs to call siInitialize on this DIFFERENT bCellNo !!!!!!!!!!!!!!!!!!
 			//       (to configure PSU bits, drive-plugged, stats etc etc
 			//
 			// The drive-thread could do it itself  (and/or we can do it here)
 			// - seems easiest to just do it here
	    printlog("xcal main thread: siInitialize on bCellNo %d ( For drive/tray 1)", bCellNo_drive_1);
	 		Byte sirc = siInitialize( bCellNo_drive_1 );
			if (sirc != 0)
			{
				char Reason[80];
				siGetReasonStringForReturnCode( sirc, Reason, sizeof(Reason));
		    printlog("bad rc of %u from siInitialize on Drive1 (bCellNo_drive_1=%d) shows %s", sirc, bCellNo_drive_1, Reason);
			  EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_XYAPI_ERROR );  // NO RETURN 
			}	
			
 			
	    printlog("xcal main thread: starting drivethread tray 1");
			pthread_t drivethread_1;
		  int ret = pthread_create( &drivethread_1, NULL, DriveThreadFunc, (void *)bCellNo_drive_1);
		  if (ret != 0)
		  {
		  	printlog("Main: pthread_create DriveThreadFunc_1( bCellNo %d) failed ret=%d - ABORT\n", bCellNo_drive_1, ret);
		  	exit(0);
		  } 
		  printlog("Main: pthread_create DriveThreadFunc_1( bCellNo %d) OK\n", bCellNo_drive_1);
		  
		  WantSecondDriveInXcalibre = true;  // easiest way to help out with statitics printing code in loop below
 		}
	#endif	


	//--------------------------------------------------------------------------------------------------
	// This main thread now proceeds with thermal and general monitoring things........................
	//--------------------------------------------------------------------------------------------------

	// just test this new API
	sirc = siSetDriveFanRPM( bCellNo, 4200);
	if (sirc!=0)
	{
	  printlog("siSetDriveFanRPM() FAILED - see wrapper-lib-log");
	}


	if (JustGoGreenAndQuiet)
	{
		// may as well turn off drives (and LEDs) (drive would go off anyway (due to interlocks))
		//TurnOffDrive(0);
		sirc = setTargetVoltage( bCellNo, 0, 0);

		//TurnOffDrive(1); 
		sirc = setTargetVoltage( bCellNo, 0, 0);
		
		//GoGreenAndQuiet();
		sirc = setSafeHandlingTemperature( bCellNo, 4000);
		
		EndTestcaseSafely( EXITCODE_DRIVETEST_COMPLETE_PASSED );  // NO RETURN 
	}


  printlog("xcal main thread: contunuing with thermal/chamber things...");
				
	
	// initial check of CellStatusFlags (also continually monitored in main loop)
	//-------------------------------------------------------------------------------
	if (NoFansMode == true)
	{
		printlog("Skipping Initial CheckForAndReportAnyCellStatusErrors - due to NOFANSMode\n");
		printlog("Setting FPGA into TestMode to defeat fan/psu interlocks (due to NOFANSMode)\n");
		sirc = SetFPGATestMode( bCellNo); // POTENTIALLY DANGEROUS XYRATEX/BENCHTOP TEST USE ONLY!!!!!!!!!!!!!!!!!!!!!
	}
	else
	{
		printlog("Initial CheckForAndReportAnyCellStatusErrors...\n");
		bool brc = CheckForAndReportAnyCellStatusErrors( true);  // InitialCall = try to clear any errors (eg csfSupplyInterlockTrip)
		if (brc == true)
		{
			printlog("Exiting Testcase due to CellStatusErrors reported above\n");
			EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_SYSTEM_ERROR );  // NO RETURN !!!!!
		}
		printlog("...ok\n");
	} 


	// drive-plugged check handled by DriveThread

	// safehandling
	//----------------
	if (NoFansMode == false)
	{
		//NEW for Thermal stability (oscillator) testing etc.
		printlog("We now want to turn on drive BEFORE ramping temperature");
		printlog(" BUT we must come out of GreenAndQuiet Mode to be able to use the power supplies");
		printlog(" SO the simplest Temperature Control Mode is GoToSafeHandlingTemperature");
		
		if (WantQuietFans)
		{
			//printlog("Setting fans to RPM mode at QUIET RPMs of (drive=%u, elec=%u) \n", slowDriveRPM, slowElecRPM);
			//SetFansToRPMControl( slowDriveRPM, slowElecRPM);
		}
			
		//StartGoToSafeHandlingTemperature( i16SafeHandlingTempInTenths);
		sirc = setSafeHandlingTemperature( bCellNo, 4000);
	}
	
	if (WantRampTemp)
	{ 
		printlog(" Takahashi-san wants to wait here, until till down to SafeHandlingTemp, before proceeding with power-on, ramps etc.");
		UpdateTMTestInfoLine(" waiting until till down to SafeHandlingTemp, before proceeding with power-on, ramps etc."); 
		WaitForOnTempIndication( 4000,  100 * 60, true );  // Secs timeout, true=print temps while waiting (with infoline)
		printlog(" SafeHandlingTemp achieved");
	}
	else
	{
		printlog(" skiping initial wait for SafeHandlingTemp (due to -nr option)");
		
		//printlog(" just wait 3 secs for fans to stabilise - before using supplies (to prevent SupplyInterlockTrip?)");
		//sleep(3); // SECONDS (SUSPECT THIS SHOULD NOT BE NECESSARY ???)
	}
	


	// ramp to temperature
	//-------------------------------------------------------------------------------
	if (WantRampTemp)
	{
		printlog(" \n"); // (after 'secs to go' line re-use)
		UpdateTMTestInfoLine("StartRampToTemperature of %u TENTHS (at rate of %u TenthsPerMinute)\n", i16RampTempInTenths, u16TestRampRateInTenthsPerMinute); 
		
		//StartRampToTemperature( i16RampTempInTenths, u16TestRampRateInTenthsPerMinute );
		sirc = setPositiveTemperatureRampRate( bCellNo, u16TestRampRateInTenthsPerMinute*10);
		sirc = setNegativeTemperatureRampRate( bCellNo, u16TestRampRateInTenthsPerMinute*10);
		sirc = setTargetTemperature( bCellNo, i16RampTempInTenths*10);

		// note: new firmware 1.83-> will only allow positive rates of 10 less (default=10, so default of this application is now 10)
		// 10 means 1 degreeC per Minute - so a ramp from 20 to 70 could take 50 Minutes or more....(lets just blindly allow up to 100 minutes then)
		#define WAIT_FOR_ONTEMP_TIMEOUT_SECS  ( 100 * 60 ) // lets allow more than double - to allow for slower ramps etc. 
		printlog("Waiting For on-temp (timeout %d secs)\n", WAIT_FOR_ONTEMP_TIMEOUT_SECS);
		WaitForOnTempIndication( i16RampTempInTenths*10, WAIT_FOR_ONTEMP_TIMEOUT_SECS, true );  // Secs timeout , print temps while waiting
	}
	else
	{
		printlog("SKIPPING RampToTemp\n");

		// (Drive now powered BEFORE any ramp - so TempMode to get fans on is handled earlier)
	}

	//---------------------------------------------------------------------------------------------
	// NOW "do the testing"
	// (in our case that is just loop doing simple 1-wire SRST serial comms wth the drive  (updating our testprogress)
	//   until its time to declare a test result and exit
	//---------------------------------------------------------------------------------------------
	
	// main test loop
	//----------------
	time_t TimeOfLastInfoLineUpdate = time(NULL);  
 
	// want to stay in this loop until the drive 'testing' is finished!
	while ( true )
	{
		// Are (both) drive threads finished yet ?
		// TODO: consider detecting thead ending via pthread techniques???
		//-----------------------------------------
		bool first_busy = false;
		if (( WantFirstDrive) && (pstats[bCellNo].TotalCommands < LoopCount))
			first_busy=true;
			
			
		Byte bCellNo_drive_1 = ((CellIndex-1)*2) + 1 + 1; // Tray 1
		bool second_busy = false;
		if (( WantSecondDriveInXcalibre) && (pstats[bCellNo_drive_1].TotalCommands < LoopCount))
			second_busy=true;
		
		if ((first_busy == false) && (second_busy == false))
		{
			printlog("*** Breaking out of test/monitoring loop due (all) drive thread(s) finished***\n");
			break; // out of while not finished loop - so we get nicer cleanup
		}	
		
		
		
		// dont overload the CPU (or USB/ethernet)
		sleep (1 ); // Secs
		
		if (PerformanceTestMode == false)
		{	
			// Regularly monitor Cell/Environment Health
			if (NoFansMode == false)
			{
				bool brc = CheckForAndReportAnyCellStatusErrors( false);  // NOT InitialCall = DONT try to clear any errors (eg csfSupplyInterlockTrip)
				if (brc == true)
				{
					printlog("Exiting Testcase due to CellStatusErrors reported above\n");
					EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_SYSTEM_ERROR );  // NO RETURN !!!!!
				}
			} 
		}
		
		
		// calculate and display serial performance statistics
		//-----------------------------------------------------
		#define PERIOD_FOR_MID_TERM_AVERAGE_SECS  30 // must be a multiple of InfoUpdateFrequencySeconds!
		time_t TimeNow = time(NULL);
		if  ( (TimeNow - TimeOfLastInfoLineUpdate) > InfoUpdateFrequencySeconds)  
		{
			if (WantFirstDrive && (pstats[bCellNo].TotalCommands > 0)) // (using main bCellNo) // dont output stats until its had one loop (else the spin-up countdown gets mixed in and messy)
			{
				static time_t timeNextMinute = time(NULL) + PERIOD_FOR_MID_TERM_AVERAGE_SECS;
				static double prev_TotalDataBytesReceived = 0;
				static double prev_TotalTimeTransferring_mS = 0;
				static double prev_TotalTimeReceiving_mS = 0; 
				static double prev_TotalTimeTransferring_uS = 0;
				static double prev_TotalTimeReceiving_uS = 0; 
				
				static double minute_prev_TotalDataBytesReceived = 0;
				static double minute_prev_TotalTimeTransferring_mS = 0;
				static double minute_prev_TotalTimeReceiving_mS = 0;  
				static double minute_prev_TotalTimeTransferring_uS = 0;
				static double minute_prev_TotalTimeReceiving_uS = 0;  
	 
				if (TimeNow >= timeNextMinute)
				{
					minute_prev_TotalDataBytesReceived = pstats[bCellNo].TotalDataBytesReceived - minute_prev_TotalDataBytesReceived;       
					minute_prev_TotalTimeTransferring_mS = pstats[bCellNo].TotalTimeTransferring_mS - minute_prev_TotalTimeTransferring_mS;       
					minute_prev_TotalTimeReceiving_mS = pstats[bCellNo].TotalTimeReceiving_mS - minute_prev_TotalTimeReceiving_mS;  
					minute_prev_TotalTimeTransferring_uS = pstats[bCellNo].TotalTimeTransferring_uS - minute_prev_TotalTimeTransferring_uS;       
					minute_prev_TotalTimeReceiving_uS = pstats[bCellNo].TotalTimeReceiving_uS - minute_prev_TotalTimeReceiving_uS;  
								
					timeNextMinute+= PERIOD_FOR_MID_TERM_AVERAGE_SECS;
				}
	
	//trans@size,  ShortAvg KB/s,(,rcvburst,),  MedAvg KB/s,(,rcvburst,),  TotAvg KB/s,(,rcvburst,), RESP:oruns, nrcvs:Hi,/,avg,  CMD:bads, nrcvs:Hi,/,avg,       
				UpdateTMTestInfoLine("MainTray:%.6d xfers@size=%d+8>>, %.0f,(,%.0f,), %.0f,(,%.0f,), %.0f,(,%.0f,), "
																	 "%d, %d,/,%d,  %d, %d,/,%d "
																	 
											,pstats[bCellNo].TotalCommands 
											,u16nBytesOfDriveDataToBeRead
											
											/*
											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000) /  (pstats[bCellNo].TotalTimeTransferring_mS - prev_TotalTimeTransferring_mS)
											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000) /  (pstats[bCellNo].TotalTimeReceiving_mS - prev_TotalTimeReceiving_mS)
	
											,((minute_prev_TotalDataBytesReceived) * 1000) /  (minute_prev_TotalTimeTransferring_mS)
											,((minute_prev_TotalDataBytesReceived) * 1000) /  (minute_prev_TotalTimeReceiving_mS)
	
											,(pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeTransferring_mS
											,(pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeReceiving_mS
											*/

											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000 * 1000) /  (pstats[bCellNo].TotalTimeTransferring_uS - prev_TotalTimeTransferring_uS)
											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000 * 1000) /  (pstats[bCellNo].TotalTimeReceiving_uS - prev_TotalTimeReceiving_uS)
											
											,((minute_prev_TotalDataBytesReceived) * 1000 * 1000) /  (minute_prev_TotalTimeTransferring_uS)
											,((minute_prev_TotalDataBytesReceived) * 1000 * 1000) /  (minute_prev_TotalTimeReceiving_uS)
	
											,((pstats[bCellNo].TotalDataBytesReceived * 1000 * 1000) /  pstats[bCellNo].TotalTimeTransferring_uS) 
											,((pstats[bCellNo].TotalDataBytesReceived * 1000 * 1000) /  pstats[bCellNo].TotalTimeReceiving_uS) 

	
											,pstats[bCellNo].TotalResponseOverruns
	
											,pstats[bCellNo].HighestReceivesForOneResponse
											,pstats[bCellNo].AvgReceivesForOneResponse
	
											,pstats[bCellNo].TotalBadCommands
											
											,pstats[bCellNo].HighestReceivesForOneAck
											,pstats[bCellNo].AvgReceivesForOneAck
											
									 ); 
												
				prev_TotalDataBytesReceived = pstats[bCellNo].TotalDataBytesReceived;
				prev_TotalTimeTransferring_mS = pstats[bCellNo].TotalTimeTransferring_mS;
				prev_TotalTimeReceiving_mS = pstats[bCellNo].TotalTimeReceiving_mS ;
				prev_TotalTimeTransferring_uS = pstats[bCellNo].TotalTimeTransferring_uS;
				prev_TotalTimeReceiving_uS = pstats[bCellNo].TotalTimeReceiving_uS ;
				
				TimeOfLastInfoLineUpdate = TimeNow;
			}
		


	 		Byte bCellNo_drive_1 = ((CellIndex-1)*2) + 1 + 1; // Tray 1
			if (WantSecondDriveInXcalibre && (pstats[bCellNo_drive_1].TotalCommands > 0)) // (using special bCellNo) // dont output stats until its had one loop (else the spin-up countdown gets mixed in and messy)
			{
				
	 			Byte bCellNo = bCellNo_drive_1; // HACK HACK HACK !!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				
				static time_t timeNextMinute = time(NULL) + PERIOD_FOR_MID_TERM_AVERAGE_SECS;
				static double prev_TotalDataBytesReceived = 0;
				static double prev_TotalTimeTransferring_mS = 0;
				static double prev_TotalTimeReceiving_mS = 0; 
				static double prev_TotalTimeTransferring_uS = 0;
				static double prev_TotalTimeReceiving_uS = 0; 
				
				static double minute_prev_TotalDataBytesReceived = 0;
				static double minute_prev_TotalTimeTransferring_mS = 0;
				static double minute_prev_TotalTimeReceiving_mS = 0;  
				static double minute_prev_TotalTimeTransferring_uS = 0;
				static double minute_prev_TotalTimeReceiving_uS = 0;  
	 
				if (TimeNow >= timeNextMinute)
				{
					minute_prev_TotalDataBytesReceived = pstats[bCellNo].TotalDataBytesReceived - minute_prev_TotalDataBytesReceived;       
					minute_prev_TotalTimeTransferring_mS = pstats[bCellNo].TotalTimeTransferring_mS - minute_prev_TotalTimeTransferring_mS;       
					minute_prev_TotalTimeReceiving_mS = pstats[bCellNo].TotalTimeReceiving_mS - minute_prev_TotalTimeReceiving_mS;  
					minute_prev_TotalTimeTransferring_uS = pstats[bCellNo].TotalTimeTransferring_uS - minute_prev_TotalTimeTransferring_uS;       
					minute_prev_TotalTimeReceiving_uS = pstats[bCellNo].TotalTimeReceiving_uS - minute_prev_TotalTimeReceiving_uS;  
								
					timeNextMinute+= PERIOD_FOR_MID_TERM_AVERAGE_SECS;
	 
				}
	//trans@size,  ShortAvg KB/s,(,rcvburst,),  MedAvg KB/s,(,rcvburst,),  TotAvg KB/s,(,rcvburst,), RESP:oruns, nrcvs:Hi,/,avg,  CMD:bads, nrcvs:Hi,/,avg,       
				UpdateTMTestInfoLine("UpperTray:%.6d xfers@size=%d+8>>, %.0f,(,%.0f,), %.0f,(,%.0f,), %.0f,(,%.0f,), "
																	 "%d, %d,/,%d,  %d, %d,/,%d "
											,pstats[bCellNo].TotalCommands 
											,u16nBytesOfDriveDataToBeRead
											
											/*
											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000) /  (pstats[bCellNo].TotalTimeTransferring_mS - prev_TotalTimeTransferring_mS)
											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000) /  (pstats[bCellNo].TotalTimeReceiving_mS - prev_TotalTimeReceiving_mS)
	
											,((minute_prev_TotalDataBytesReceived) * 1000) /  (minute_prev_TotalTimeTransferring_mS)
											,((minute_prev_TotalDataBytesReceived) * 1000) /  (minute_prev_TotalTimeReceiving_mS)
	
											,(pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeTransferring_mS
											,(pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeReceiving_mS
											*/

											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000 * 1000) /  (pstats[bCellNo].TotalTimeTransferring_uS - prev_TotalTimeTransferring_uS)
											,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000 * 1000) /  (pstats[bCellNo].TotalTimeReceiving_uS - prev_TotalTimeReceiving_uS)
											
											,((minute_prev_TotalDataBytesReceived) * 1000 * 1000) /  (minute_prev_TotalTimeTransferring_uS)
											,((minute_prev_TotalDataBytesReceived) * 1000 * 1000) /  (minute_prev_TotalTimeReceiving_uS)
	
											,((pstats[bCellNo].TotalDataBytesReceived * 1000 * 1000) /  pstats[bCellNo].TotalTimeTransferring_uS) 
											,((pstats[bCellNo].TotalDataBytesReceived * 1000 * 1000) /  pstats[bCellNo].TotalTimeReceiving_uS) 
											
	
											,pstats[bCellNo].TotalResponseOverruns
	
											,pstats[bCellNo].HighestReceivesForOneResponse
											,pstats[bCellNo].AvgReceivesForOneResponse
	
											,pstats[bCellNo].TotalBadCommands
											
											,pstats[bCellNo].HighestReceivesForOneAck
											,pstats[bCellNo].AvgReceivesForOneAck
											
									 ); 
												
				prev_TotalDataBytesReceived = pstats[bCellNo].TotalDataBytesReceived;
				prev_TotalTimeTransferring_mS = pstats[bCellNo].TotalTimeTransferring_mS;
				prev_TotalTimeReceiving_mS = pstats[bCellNo].TotalTimeReceiving_mS ;
				prev_TotalTimeTransferring_uS = pstats[bCellNo].TotalTimeTransferring_uS;
				prev_TotalTimeReceiving_uS = pstats[bCellNo].TotalTimeReceiving_uS ;
				
				TimeOfLastInfoLineUpdate = TimeNow;
			}
		}


		
		if (clean_exit_requested)
		{
			printlog("*** Breaking out of test/monitoring loop due to clean_exit_requested ***\n");
			break; // out of while not finished loop - so we get nicer cleanup
		}

	} // while (test/monitoring loop)


	//----------------------------------------------------------------
	// testing finished
	// - turn off drive and start cooling down to SafeHandlingTemp
	//----------------------------------------------------------------
	printlog("Testing Finished\n" );

	if (WantSerialComms)
	{
		printlog("TotalDataBytesReceived=%.0f", pstats[bCellNo].TotalDataBytesReceived);
		printlog("TotalTimeTransferring_mS=%.0f", pstats[bCellNo].TotalTimeTransferring_mS);
		printlog("TotalTimeReceiving_mS=%.0f", pstats[bCellNo].TotalTimeReceiving_mS);
	
		if (pstats[bCellNo].TotalTimeTransferring_mS == 0) { printlog ("WARNING: avoiding divide by zero by adding 1 to TotalTimeTransferring_mS"); pstats[bCellNo].TotalTimeTransferring_mS++; } 
		if (pstats[bCellNo].TotalTimeReceiving_mS == 0) { printlog ("WARNING: avoiding divide by zero by adding 1 to TotalTimeReceiving_mS"); pstats[bCellNo].TotalTimeReceiving_mS++; } 
		
		printlog("SO overall Average rate = %.0f Bytes Per Second  ", ( pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeTransferring_mS );
		printlog("  (average receiving-only rate = %.0f)", ( pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeReceiving_mS );
	}


	if (LeavePowerOnAfterTest == true)
	{
		printlog("Leaving drive powered (due to -ne option)\n");
	}
	else
	{
		printlog("Turning off power to drive %d\n", 0 );
		sirc = setTargetVoltage( bCellNo, 0, 0);

		#if defined XCALIBRE		
	 		if ((TrayIndex == 1) || (TrayIndex == 2))
	 		{
	 			Byte bCellNo_drive_1 = ((CellIndex-1)*2) + 1; // Tray 1
				sirc = setTargetVoltage( bCellNo_drive_1, 0, 0);
	 		}
		#endif
	}


	if (WantRampTemp)  // new 25Mar04
	{
		printlog("Going to SafeHandling Temperature (%d tenths)\n", i16SafeHandlingTempInTenths );
		//StartGoToSafeHandlingTemperature( i16SafeHandlingTempInTenths );
		sirc = setSafeHandlingTemperature( bCellNo, 4000);

		printlog("Waiting for 'on-temp' indication\n" );

		clean_exit_requested = false; // in case we used a signal to break out of test/monitoring loop
																	// we dont want to also break out of WaitForOnTemp
																	// (but if we do - a second signal will get us out of that)

		WaitForOnTempIndication( 4000, 10 * 60, true );  // Secs timeout , print temps while waiting
	}


	/* GreenAndQuiet not in favour for various reasons...
	if (LeavePowerOnAfterTest == false)
	{
		printlog("Going GreenAndQUIET\n" );
		GoGreenAndQuiet();
	}
	else
	{
		printlog("NOT Going GreenAndQuiet - because you have asked me to leave DrivePowerOn\n" );
	}
	*/

	printlog("\nAll done \n" );
	Byte ExitCode = (flSignalProcessed) ? EXITCODE_TESTCASE_FAILED_RECEIVED_SIG_TERM : EXITCODE_DRIVETEST_COMPLETE_PASSED;
	
	EndTestcaseSafely( ExitCode ); 
}





//---------------------------------------------------------------------------
bool ParseCmdLineArguments( int argc, char *argv[] )
//---------------------------------------------------------------------------
{
	printlog("parsing cmdline of:\n");
	for (int i = 0; i < argc; i++)
	{
		printlog("%s ", argv[i]);
	}
	printlog("\n");


	bool brc = true;
	if (argc < 3)
	{
		printlog("Insufficient arguments (must have at least PowerFailRecoveryFlag, CellId and TrayIndex)\n");
		Usage();
		brc = false;
	}
	else
	{    
		
		TrayIndex = atoi( argv[argc-1] ); // LAST arg must always be TrayIndex
		#if defined XCALIBRE
		if ( (TrayIndex < 0) || (TrayIndex > 2) )  // 3DP CELL HAS 2 trays,  index 0 to 1 (but some customers use a convention of 2=BOTH)
		#elif defined OPTIMUS
		if ( (TrayIndex < 1) || (TrayIndex > 12) )  // Optimus cell has 12 trays,  index 1 to 12
		#endif
		{ 
			printlog("Invalid TrayIndex supplied (%d)", TrayIndex); 
			Usage();
			return false;
		}
				
		CellIndex = atoi( argv[argc-2] ); // next-to-LAST arg must always be CellId
		#if defined OS_IS_LINUX
			if ( (CellIndex < 1) || (CellIndex > 128) )
			{
				printlog("Invalid CellId supplied (as CellIndex %d)\n", CellIndex);
				Usage();
				return false;
			}
		#elif defined OS_IS_WINDOWS 
			// NOTE:
			//  we cannot sanity-check the CellId (= 0-based usb dev-num) YET 
			// - because we must make an allowance for TestManager and TestCase being in NoCellMode
			//   (in which case we will be supplied a CellId of -1 (invalid) - since TestManager does not HAVE a DevNum for unpowered cells!!!!!!
			//  SO we delay the sanity-check until we have parsed the rest of the args,
			// - and temper the check if we are in NoCell mode
		#endif
			 
			 
		int n = atoi( argv[argc-3] ); // next-to-next-to-LAST arg must always be PowerFailRecoveryFlag
		if ( (n < 0) || (n > 1) )
		{ 
			//printlog("Invalid PowerFailRecoveryFlag supplied (%d)\n", n);
			UpdateTMTestInfoLine("Invalid PowerFailRecoveryFlag supplied (%d)", n); // also does printlog()
			Usage();
			return false;
		}   
		else
		{
			PowerFailRecoveryFlag = (n > 0) ? true : false;
		}
			 
			 
		// parse any options (working backwards through remaining args)
		//---------------------------------------------------------------
		for (int i = 1; i < argc-3; i++)
		{
			if (strncmp( argv[i], "-?", 2) == MATCH)
			{ 
				Usage();
				brc = false;
				break; 
			}


			else if (strncmp( argv[i], "-pin11_low", 5) == MATCH)
			{
				WantPin11Low = true;
				printlog(" Cmdline specifes -pin11_low\n");
			}
			
			else if (strncmp( argv[i], "-nc", 3) == MATCH)
			{
				WantPlugCheck = false;
			}
			else if (strncmp( argv[i], "-np", 3) == MATCH)
			{
				WantPowerDrive = false;
			}
			else if (strncmp( argv[i], "-nr", 3) == MATCH)
			{
				WantRampTemp = false;
			}
			else if (strncmp( argv[i], "-ne", 3) == MATCH)
			{
				LeavePowerOnAfterTest = true;
			}
	
	
			else if (strncmp( argv[i], "-rt", 3) == MATCH)
			{
				i16RampTempInTenths = (Word) atoi( argv[i]+3 );
				printlog(" Cmdline specifes i16RampTempInTenths of %d\n", i16RampTempInTenths);
			}
			else if (strncmp( argv[i], "-rr", 3) == MATCH)
			{ 
				u16TestRampRateInTenthsPerMinute = atoi( argv[i]+3 );
				printlog(" Cmdline specifes u16TestRampRateInTenthsPerMinute of %d\n", u16TestRampRateInTenthsPerMinute);
			}  

			else if (strncmp( argv[i], "-S", 2) == MATCH)
			{
				i16SafeHandlingTempInTenths = (Word) atoi( argv[i]+2 );
				printlog(" Cmdline specifes i16SafeHandlingTempInTenths of %d\n", i16SafeHandlingTempInTenths);
			}
	
			else if (strncmp( argv[i], "-l", 2) == MATCH)
			{ 
				LoopCount = atoi( argv[i]+2 );
				printlog(" Cmdline specifes LoopCount of %d\n", LoopCount);
			}
				
			else if (strncmp( argv[i], "-u", 2) == MATCH)
			{ 
				InfoUpdateFrequencySeconds = atoi( argv[i]+2 );
				printlog(" Cmdline specifes InfoUpdateFrequencySeconds of %d\n", InfoUpdateFrequencySeconds);
			}  


			else if (strncmp( argv[i], "-t", 2) == MATCH)
			{
				printlog(" Requesting Trace (Xyratex Classes)\n");
				WantAPITrace = true;
			}
			else if (strncmp( argv[i], "-i", 2) == MATCH)
			{
				printlog(" not using TraceInfoLine (due to -i)\n");
				WantToUseTraceInfoLine = false;
			}

			else if (strncmp( argv[i], "-noprintlog", 9) == MATCH)
			{
				printlog(" not using printlog at all\n");
				WantLogging = false;
			}

			else if (strncmp( argv[i], "-perf", 4) == MATCH) // VIP check for -perf BEFORE -p !!!!
			{
				PerformanceTestMode = true;
				printlog(" PerformanceTestMode set (due to -perf) - minimal logging etc.\n");
			}
			else if (strncmp( argv[i], "-p", 2) == MATCH)
			{
				WantPrintLogToMirrorToStdout = true;
				printlog(" mirroring printlog() to stdout (due to -p)\n");
			}
			
			else if (strncmp( argv[i], "-s", 2) == MATCH)
			{
				u16nBytesOfDriveDataToBeRead = atoi( argv[i]+2 );
				printlog(" Cmdline specifes u16nBytesOfDriveDataToBeRead of %u\n", u16nBytesOfDriveDataToBeRead);
			}
			
			else if (strncmp( argv[i], "-g", 2) == MATCH)
			{
				JustGoGreenAndQuiet = true;
				printlog(" JUST going GreenAndQuiet (due to -g)\n");
			}

			else if (strncmp( argv[i], "-nf", 3) == MATCH)
			{
				NoFansMode = true;
				printlog("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				printlog(" NOFANS MODE - FPGA-TESTMODE- WARNING - SOME INTERLOCKS DISABLED  (due to -nf)\n");
				printlog("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			}
			else if (strncmp( argv[i], "-nli", 4) == MATCH)
			{
				WantToLogInfoLines = false;
				printlog(" WantToLogInfoLines=false  (due to -nli)\n");
			}
			else if (strncmp( argv[i], "-ns", 3) == MATCH)
			{
				WantSerialComms = false;
				printlog(" WantSerialComms=false  (due to -ns)\n");
			}
			else if (strncmp( argv[i], "-xyrcignore", 5) == MATCH)
			{
			  DontTerminateOnBadXYRCs = true;
			  printlog("will NOT terminate on bad xyrc! (due to -xyrcignore)\n");
			}
			else if (strncmp( argv[i], "-quietfans", 5) == MATCH)
			{
			  WantQuietFans = true;
			  printlog("WantQuietFans (due to -quietfans)\n");
			}
			else if (strncmp( argv[i], "-NEWPROTOCOL", 5) == MATCH)
			{
			  WantNEWPROTOCOL = true;
			  printlog("WantNEWPROTOCOL (due to -NEWPROTOCOL)\n");
			}



			else
			{
				printlog(" ignoring unrecognised option %s\n", argv[i]);
			}  
				
		}  
	}

	return brc;
}


//---------------------------------------------------------------------------
void myhandler(int sig)
//---------------------------------------------------------------------------
{
	switch (sig)
	{
		case SIGINT:
			printlog("\nCaught SIGINT signal\n" );
			printlog(" - will exit at earliest safe opportunity!!!!!!!!!!!!!!!\n" );
			clean_exit_requested = true;  // global flag
			flSignalProcessed = true; // global (affects our exit code)
			break;
		case SIGTERM:
			printlog("\nCaught SIGTERM signal\n" );
			 printlog(" - will exit at earliest safe opportunity!!!!!!!!!!!!!!!\n" );
			clean_exit_requested = true;  // global flag
			flSignalProcessed = true; // global (affects our exit code)
			break;
			
	 #ifdef OS_IS_LINUX
		case SIGQUIT:
			printlog("\nCaught SIGQUIT signal\n" );
			printlog(" - will exit at earliest safe opportunity!!!!!!!!!!!!!!!\n" );
			clean_exit_requested = true;  // global flag
			flSignalProcessed = true; // global (affects our exit code)
			break;
		case SIGHUP:
			printlog("\nCaught SIGHUP signal\n" );
			printlog(" - will exit at earliest safe opportunity!!!!!!!!!!!!!!!\n" );
			clean_exit_requested = true;  // global flag
			flSignalProcessed = true; // global (affects our exit code)
			break;
	 #endif

		default:
			printlog("\nCaught unknown signal %d - ignoring!!!!!!!!!!!!!!!\n", sig);
			break;
	}
}


//---------------------------------------------------------------------------
void EndTestcaseSafely( int ExitCode )  // WARNING - THIS FUNCTION NEVER RETURNS
//---------------------------------------------------------------------------
{
	// WARNING - THIS FUNCTION NEVER RETURNS - it exits the entire testcase process !!!!
		
	if (ExitCode != 0)
	{	
		UpdateTMTestInfoLine( "ExitCode=%d=%s", ExitCode, ExitCodeToString(ExitCode) );
	}
	else
	{
		// for 'good' exit, better to leave last operation on info-text
		// so just printlog 
		printlog( "ExitCode=%d=%s", ExitCode, ExitCodeToString(ExitCode) );
	}
	

	if (ExitCode != 0)
	{
		EnvAttemptToMakeCellSafe();
	}
	
	//DeleteCellEnv();    
	if (bCellNo != 0)
	{
		siFinalize(bCellNo);
	}

	if (ExitCode > 255)
	{
		printlog("EndTestcaseSafely: ***NOTE*** ExitCodes above 255 cannot be picked up by TestManger on some platforms");
		ExitCode = 255;
		printlog("             SO I HAVE FORCIBLY CHANGED YOUR EXIT CODE TO %d", ExitCode);
	} 
	time_t TimeNow = time(NULL);
	if (TimeNow < (TimeAppStarted+4))
	{
		printlog("We ran for very short time - so sleep 3 secs before exiting - to avoid confusing TestManager (to be re-evaluated one day!)");
		sleep(3);
	}
	
	printlog("EndTestcaseSafely: calling OS exit ( %d) - GOODBYE", ExitCode);
	exit ( ExitCode );  
}

/*
//---------------------------------------------------------------------------
void Testcase_handle_xyrc_failure( enum _xyrc xyrc) // Envfuncs callback to us on error
//---------------------------------------------------------------------------
{
	printlog("Testcase_handle_xyrc_failure: called with xyrc=%d", (int)xyrc);  // (no way to call ConvertToString from here)
	EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_XYAPI_ERROR );
}
*/
//---------------------------------------------------------------------------
void Testcase_handle_xyrc_failure( Byte xyrc) // Envfuncs callback to us on error
//---------------------------------------------------------------------------
{
	printlog("Testcase_handle_xyrc_failure: called with xyrc=%d", (int)xyrc);  // (no way to call ConvertToString from here)
	if (DontTerminateOnBadXYRCs)
	{
		static bool firsttime = true;  // STATIC
		if (firsttime)
		{
			firsttime = false;
			UpdateTMTestInfoLine( "xyrc=%d ignored due to -xyrcignore", (int)xyrc );
		}
		printlog("Testcase_handle_xyrc_failure: NOT ending testcase (due to -xyrcignore)");  // 
	}
	else
	{
		EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_XYAPI_ERROR );
	}
}




/**
 * <pre>
 * Description:
 *   Puts given strings into system log file. This function inserts current time
 *   in secondes and my cell ID automatically at top of line. Also, it append
 *   'line-feed(0x0A)' at last of description if it is not in description.
 * Arguments:
 *   Variable arguments like 'printf'
 * </pre>
 *****************************************************************************/

pthread_mutex_t mutex_tc_printf;
 
#define EXIT__LOG_MUTEX_FAILURE 98
#define EXIT__LOG_FILE_FAILURE 99

bool optionLogMirrorToStdout = true;

#include <stdarg.h>   // for varargs



void tc_printf(char *bFileName, unsigned long dwNumOfLine, char *description, ...) 
//-----------------------------------------------------------------------------------
{
  char lineBuf[1024];
  int lineSizeNow = 0;
  int lineSizeMax = sizeof(lineBuf);

  // get time
  time_t timep;
  struct tm tm;
  time(&timep);
  localtime_r(&timep, &tm);

  // get pid & create log file
  static int isFirstVisit = 1;
  static char logFileName[128];
  static FILE *logFilePtr = NULL;
  int ret = 0;

  ret = pthread_mutex_lock(&mutex_tc_printf);
  if (ret) {
    exit(EXIT__LOG_MUTEX_FAILURE);
  }

  if (isFirstVisit) {
    isFirstVisit = 0;
    pid_t pid = getpid();
    snprintf(&logFileName[0],
             sizeof(logFileName),
             "tclogger.%04d.%02d.%02d.%02d.%02d.%02d.%ld.log",
             tm.tm_year + 1900,
             tm.tm_mon + 1,
             tm.tm_mday,
             tm.tm_hour,
             tm.tm_min,
             tm.tm_sec,
             (unsigned long)pid
            );
    logFilePtr = fopen(logFileName, "w+b");
    if (logFilePtr==NULL) {
      exit(EXIT__LOG_FILE_FAILURE);
    }
  }

  ret = pthread_mutex_unlock(&mutex_tc_printf);
  if (ret) {
    exit(EXIT__LOG_MUTEX_FAILURE);
  }

  // time stamp
  lineSizeNow += snprintf(&lineBuf[lineSizeNow],
                          lineSizeMax - lineSizeNow,
                          "%04d/%02d/%02d,%02d:%02d:%02d,",
                          tm.tm_year + 1900,
                          tm.tm_mon + 1,
                          tm.tm_mday,
                          tm.tm_hour,
                          tm.tm_min,
                          tm.tm_sec
                         );

  // file name and line #
  lineSizeNow += snprintf(&lineBuf[lineSizeNow],
                          lineSizeMax - lineSizeNow,
                          "%s,%ld,",
                          bFileName,
                          dwNumOfLine
                         );

  // description
  va_list ap;
  va_start(ap, description);
  lineSizeNow += vsnprintf(&lineBuf[lineSizeNow],
                           lineSizeMax - lineSizeNow,
                           description,
                           ap
                          );
  va_end(ap);

  // add a newline only if not one already
  if (lineBuf[lineSizeNow - 1] != '\n') {
    lineSizeNow += snprintf(&lineBuf[lineSizeNow],
                            lineSizeMax - lineSizeNow,
                            "\n"
                           );
  }

	// write to file
  fwrite(&lineBuf[0], 1, lineSizeNow, logFilePtr);
  fflush(logFilePtr);
  
  // Possibly alos write to console/stdout
  if (optionLogMirrorToStdout)
  {
  	fprintf(stdout, "thread %.8X:", (unsigned int) pthread_self() );
	  fprintf(stdout, "%s", &lineBuf[0]); // avoid direct use of printf here (so we can macro-catch any other (illegal) uses)
    fflush(stdout);
  }
}




void WaitForOnTempIndication( Word AwaitedTempInHundredths, int max_wait_secs, bool PrintTempsWhileWaiting)
//-------------------------------------------------------------------------------------------------------------
{
	float nsecs_waited = 0;
	Word wTempInHundredth = 0;
	Byte OnTemp = 0;
	Byte sirc =0;
	
	sleep(2); // enough to allow PID in cell  to start to process any prior request
	

	#define POLLING_MS  5*1000  // 5 secs seems reasonable (can make much shorter for 'stress-testing')
	
	printlog("WaitForOnTempIndication: polling every %d mS !", POLLING_MS);
	
	//while ( ( wTempInHundredth < (AwaitedTempInHundredths - 50)) ||  ( wTempInHundredth > (AwaitedTempInHundredths + 50)) )
	while ( OnTemp == 0 )
	{

		sirc = getCurrentTemperature( bCellNo, &wTempInHundredth);
		if (sirc != 0)
		{
			printlog("WaitForOnTempIndication: error %d from getCurrentTemperature - aborting wait", sirc);
			break;
		}
		
		Byte flags = isCellEnvironmentError( bCellNo);
		
		UpdateTMTestInfoLine( "Awaiting temp %d (now %d, %f secs max more wait envFlags=0x%.2X)", AwaitedTempInHundredths, wTempInHundredth, max_wait_secs-nsecs_waited, flags);


		OnTemp = isOnTemp( bCellNo);
		if (OnTemp > 1)
		{
			UpdateTMTestInfoLine("WaitForOnTempIndication: error %d from isOnTemp - aborting wait", OnTemp);
			break;
		}
		if (OnTemp==1)
		{
			UpdateTMTestInfoLine( "Now on-temp");
			break; // now ontemp - get out immediatley
		}
		
		
		usleep( POLLING_MS * 1000);
		nsecs_waited += (POLLING_MS / 1000);
		
		
		if (nsecs_waited > max_wait_secs)
		{
			UpdateTMTestInfoLine("WaitForOnTempIndication: timed out waiting for ontemp (after %f secs)!", nsecs_waited);
			break;
		}
		
		if (clean_exit_requested)
		{
			printlog("WaitForOnTempIndication: breaking (after %f secs) due to clean_exit_requested !", nsecs_waited);
			break;
		}
		
	}
	
}



bool CheckForAndReportAnyCellStatusErrors( bool InitialCall)  //(returns true if there IS a problem) 
//-------------------------------------------------------------------------------------------------------------
{
	bool mybrc = false; // init to no-error
	
	Byte CellStatusFlags = isCellEnvironmentError( bCellNo);
	
	if (CellStatusFlags)
	{
		// NOTE: above does all its own logging to tc_printf (including full bit-decodes, and 'drilling-down' for any Interlock reasons etc._
		//  - no point repeating any of it here !!!
	
		printlog("CheckForAndReportAnyCellStatusErrors: non-zero status flags (0x%.2X) from isCellEnvironmentError()", CellStatusFlags);
		printlog("    See tc_printf  logfile for full details, bit decodes etc.");
		printlog("'    including drilling-down' for any Interlock reasons etc.");
	}
	
	mybrc = (CellStatusFlags > 0) ? true : false;
	return mybrc;
}


//---------------------------------------------------------------------------
void EnvAttemptToMakeCellSafe( void)
//---------------------------------------------------------------------------
{
	// DO NOT USE CALLBACKS TO Testcase IN THIS FUNCTION
	// - would likely lead to recursion !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	printlog("EnvAttemptToMakeCellSafe called \n");
	
	if (siInitialise_successful)
	{
		if (LeavePowerOnAfterTest)
		{
			printlog("EnvAttemptToMakeCellSafe: NOT turning off drives (- due to LeavePowerOnAfterTest cmdline option) \n");
		}
		else
		{
			printlog("EnvAttemptToMakeCellSafe: attempting to turn off drives\n");
			setTargetVoltage( bCellNo, 0, 0);
			#if defined XCALIBRE		
		 		if ((TrayIndex == 1) || (TrayIndex == 2))
		 		{
		 			Byte bCellNo_drive_1 = ((CellIndex-1)*2) + 1; // Tray 1
					setTargetVoltage( bCellNo_drive_1, 0, 0);
		 		}
			#endif
			//pCellEnv->SetLEDA( lsOff ); // use LED A to indicate upper drive power state
			//pCellEnv->SetLEDB( lsOff ); // use LED B to indicate lower drive power state
		}
		
		printlog("EnvAttemptToMakeCellSafe: before attempting GoSafeHandlingTemp....\n");
	
		printlog("EnvAttemptToMakeCellSafe: NEW 23Aug06 - set a moderate ramp rate first!!!\n");
		printlog("EnvAttemptToMakeCellSafe:  (in case we were on an agressive (positive) one!)\n");
		printlog("EnvAttemptToMakeCellSafe:  (to prevent 5Deg-over-temp trips)\n");
	
		#define MODERATE_NEGATIVE_RAMP_RATE (10*10) // tenths DegC per minute -> HGST Hundredths !
		printlog("EnvAttemptToMakeCellSafe:  setting negative rate to a moderate %d TenthsPerMinute\n", MODERATE_NEGATIVE_RAMP_RATE);
		setNegativeTemperatureRampRate( bCellNo, MODERATE_NEGATIVE_RAMP_RATE);
		
		#define DEFAULT_SAFE_HANDLING_TEMP  (400*10) // 40 Degrees C (the default) -> HGST Hundredths !
		printlog("EnvAttemptToMakeCellSafe: NOW starting GoSafeHandlingTemp\n");
		setSafeHandlingTemperature( bCellNo, DEFAULT_SAFE_HANDLING_TEMP);
	}
	else
	{
		printlog("EnvAttemptToMakeCellSafe: not currently siInitialised - do cannot do anything\n");
	}
	
	printlog("EnvAttemptToMakeCellSafe: exiting\n");
}







