#define APP_VERSION 	"v9.1a"   // 15Jun10 - siSetDriveFanRPM(4200) , and setUartPullupVoltage(xx)

//#define APP_VERSION 	"v9.0b"   // 25May10 fix bCellNo calculations (to match Taka's change to library for 3DP, and also change lib for Optimus)

//#define APP_VERSION 	"v9.0a"   // first version using WRAPPER

	// created from simpleSRST "v0.0b"   // 21Apr10:

//=========================================================================
// Simple, sample testcase USING HGST-generic WRAPPER API to access the cell !!!!!!!!!!!!
// Demonstrating:
//   Parsing cmdline arguments 
//      (in particular extraction of CellIndex and TrayIndex)
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

#define OS_IS_LINUX


#include "libsi.h"  // the hgst generic slot-interface to different suppliers (defined and supplied by HGST)
                    // NOTE: we also get the HGST-preffered typedefs Byte, Word, Dword etc from here (via its inclusion of tcTypes.h)


#include "libsi_xyexterns.h" // // for lib's externs of Tracing flags, pstats etc





#define WANT_DEFINE_CONVERSION
#include "wrapperSRST_common.h"  // own header (testcase #defines etc)

#include "miscfuncs.h" // interface to our lower-level functions (printlog etc)


#include "CellTypes.h" // OPTIONAL (for xyrc enum...

// prototype functions AT END OF THIS FILE
void tc_printf(char *bFileName, unsigned long dwNumOfLine, char *description, ...); // HGST dictated name/args format IMPLEMENTATION NEEDS TO BE PROVIDED BY ANY USER OF libsi !!!!!!!!!
void WaitForOnTempIndication( Word AwaitedTempInHundredths, int max_wait_secs, bool PrintTempsWhileWaiting);
bool isDriveAlreadyPowered( int TrayIndex);
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
	//  - since it relies on parsed CellIndex and TrayIndex to construct the pipe name back to TestManager!
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

/*
  hgst cell id is one start (1, 2, ...). 
  It is created here from supplied supplier cell/slot id 
  (and then converted back to supplier values in the library)

  xc3dp: bCellNo = (cellIndex - 1) x 2  + (trayIndex)     + 1
  op:    bCellNo = (cellIndex - 1) x 12 + (trayIndex - 1) + 1

  note - only xc3dp trayindex is zero-base value
*/


	#if defined OPTIMUS
	
		bCellNo = ((CellIndex-1)*12) + (TrayIndex-1) + 1;
	
	#elif defined XCALIBRE
 		
 		// Note we do not support TrayIndex=2 convention at this level !!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// If the TC itself gets 2 as a parameter from TestControl/TestManager, it must convert down per-thread before creating separate bCellNo per-slot
		//
 		if (TrayIndex > 1)
 		{
 			printlog("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
 			printlog("This version of the sample testcase has NOT been upgraded to handle TrayIndex=2 from cmdline args/TestManager!!!!!!!");
 			printlog("Look at the multi-threaded version if you want to work that way...");
 			printlog("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
 		  EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_CMDLINE_ERROR );  // NO RETURN !!!!!
 		}
 		
		bCellNo = ((CellIndex-1)*2) + TrayIndex + 1; 

	#endif	

	
	
	//ConstructCellEnvAndCheckCellOnline( CellIndex, EnvironmentId,  WantAPITrace);  // DEFAULT (sole) EnvironmentId 0
	//ObtainValidateAndDisplayEnvClassVersionInfo();
	//ObtainAndLogCellFirmwareLimitsAndVersions(); // new 5Oct04
	
	Byte sirc = siInitialize( bCellNo);
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
		  EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_CELL_NOT_ONLINE );  // NO RETURN !!!!!
		}
	}	
	else
	{
		siInitialise_successful = true;
	}
	

	//----------------------------------------------------------------------------------------------------------
	// just test this new API
	sirc = siSetDriveFanRPM( bCellNo, 4200);
	if (sirc!=0)
	{
	  printlog("siSetDriveFanRPM() FAILED - see wrapper-lib-log");
	}

	#if defined OPTIMUS
		// and test this new API
		sirc = setUartPullupVoltage( bCellNo, 2500); // optimus default is 1800 (supported= 1200, 1500, 1800, 2500, 3300)  
		if (sirc!=0)
		{
		  printlog("setUartPullupVoltage() FAILED - see wrapper-lib-log");
		}
	#elif defined XCALIBRE
		// and test this new API
		sirc = setUartPullupVoltage( bCellNo, 2000); // xcalibre default is 3300 (supported=2000, 3300)
		if (sirc!=0)
		{
		  printlog("setUartPullupVoltage() FAILED - see wrapper-lib-log");
		}
	#endif
	//----------------------------------------------------------------------------------------------------------



	if (JustGoGreenAndQuiet)
	{
		// may as well turn off drives (and LEDs) (drive would go off anyway (due to interlocks))
		//TurnOffDrive(0);
		sirc = setTargetVoltage( bCellNo, 0, 0);

		//TurnOffDrive(1); 
		sirc = setTargetVoltage( bCellNo, 0, 0);
		
		//GoGreenAndQuiet();
		sirc = setSafeHandlingTemperature( bCellNo, 4000);
		
		EndTestcaseSafely( EXITCODE_DRIVETEST_COMPLETE_PASSED );  // NO RETURN !!!!!
	}
				
	
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


	// check that the Environment shows drive(s)-plugged
	//-------------------------------------------------------------------------------
	if (WantPlugCheck)
	{
		//if ( ConfirmDrivePlugged( TrayIndex ) == false)
		sirc = isDrivePlugged( bCellNo );
		if (sirc == 0)
		{
			UpdateTMTestInfoLine("No Drive plugged in tray %d - aborting",TrayIndex); 
			EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_NO_DRIVE_PLUGGED );  // NO RETURN !!!!!
		}
		else if (sirc == 1)
		{
			printlog("Confirmed Drive plugged in tray %d\n", TrayIndex);
		}
		else
		{
			printlog("isDrivePlugged FAILED sirc=%d\n", sirc);
		}
	}
	else
	{
		printlog("SKIPPING test for drive-plugged in tray %d\n",TrayIndex);
	}


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
	

	// Set Voltage Levels and power on drive
	//-------------------------------------------------------------------------------
	if (WantPowerDrive)
	{
		if (isDriveAlreadyPowered( TrayIndex) == true)  
		{
			printlog("Drive was already powered (so skipping our power-up and delay) \n");
		}
		else
		{
			#if defined XCALIBRE
				printlog("Setting nominal voltage levels on BOTH drive supplies (both 5V & both 12V\n");
				 
				// NOTE: riser is now turned on automatically in siInitialize 
				//       since no suitable API in wrapper !!!!
	
				
				// NOTE: Configure_SATA_P11_as_Float_To_PREVENT_Autospinup now done automatically in siInitialize
				//       since no suitable API in wrapper !!!!
	

				printlog("Turning on Drive supplies for Tray %d\n", TrayIndex);
				sirc = setTargetVoltage( bCellNo, 5000, 12000);
				
			#elif defined OPTIMUS
			
				printlog("Setting nominal voltage level on single drive supply\n");
				 
				// NOTE: Configure_SATA_P11_as_Float_To_PREVENT_Autospinup now done automatically in siInitialize
				//       since no suitable API in wrapper !!!!

				printlog("Turning on Drive supply for Tray %d\n", TrayIndex);
				sirc = setTargetVoltage( bCellNo, 5000, 0);
				
			#endif
	
			// Allow drive power to come-up (typical 100 - 200 mS before supplies switch on (PWM pre-charge + soft-start)
			sleep (1);  // seconds

			if (WantSerialComms)
			{
	
				#define INITIAL_DELAY_SECS  (30 + 60)  // power-on-ready + srst startup time  
				//#define INITIAL_DELAY_SECS  (20)  // early testing on CHAV !!!  
				printlog("SRST drives need some time to power up and settle");
				printlog(" before they can respond to serial commands ");
				printlog("sleeping %d seconds", INITIAL_DELAY_SECS);
				for (int i = INITIAL_DELAY_SECS; i>0; i--)
				{
					sleep( 1 );
					fprintf(stdout, "\r%d secs to go", i);
					fflush(stdout);
					
					if (i % InfoUpdateFrequencySeconds == 0)
					{
						//UpdateTMTestInfoLine("%d secs delay for Drive spin-up/SerialReady", i);
					}
					
					if (clean_exit_requested)
					{
						EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_RECEIVED_SIG_TERM );  // NO RETURN !!!!!
					}
				}
			}
			
		} // drive already powered

	}
	else
	{
		printlog("SKIPPING Powering-on of the drive (due to -np cmdline option)\n");
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
	if (WantSerialComms)
	{
		#define DESIRED_SRST_BAUDRATE  1843200    // 921600

		//SerialInitialise(  UART_ChannelId, DESIRED_SRST_BAUDRATE ); // activates UART drivers (requires drive-plugged and powered-up), sets one-wire-mode, TxInterCharacterDayay, BaudRate etc.
		sirc = siSetUartBaudrate(  bCellNo, DESIRED_SRST_BAUDRATE ); // activates UART drivers (requires drive-plugged and powered-up), sets one-wire-mode, TxInterCharacterDayay, BaudRate etc.

		// 25May - we now have an API to set protocol type
		//  bProtocol 0: A00 protocol (PTB), 1: ARM protocol (JPK)
		Byte bProtocol = (WantNEWPROTOCOL)?1:0;
		sirc = siSetUartProtocol( bCellNo, bProtocol); 

	}
	
	
	// main test loop
	//----------------
	time_t TimeOfLastInfoLineUpdate = time(NULL);  
	uint32 u32nLoops = 0;
	if (LoopCount > 0) printlog("\nEntering test/monitoring LOOP...\n\n");
	
 
	// want to stay in this loop until the drive 'testing' is finished!
	while ( LoopCount > 0 )
	{

		if (WantSerialComms)
		{
			// have a serial transaction with the drive 
			//----------------------------------------------------------------------
			
			//SerialSimpleReadMemory( UART_ChannelId, u16nBytesOfDriveDataToBeRead );  // (nbytes from  -s option)  (Serial already 'initialised' outside loop)
			
			#if defined XCALIBRE
				Dword dwAddress = 0x009e6c00; // 3.5 JPK srst sequence
			#elif defined OPTIMUS
				//Dword dwAddress = 0x00060200; // for drive model = MPB  =garbage on 2.5 EB4 !
				Dword dwAddress = 0x00070200; //  =garbage on 2.5 EB4 !
			#endif
			
			Word wSize = u16nBytesOfDriveDataToBeRead; // (changeable via -p cmdline option)
			Byte bData[0xffff+1]; // handle up-to 64K ish
			Word wTimeoutInMillSec = 2500; // NOTE THAT 2550 is max that Xyratex ReceiveWithWait can handle ! (warning log in libsi if higher value specified)
			sirc = siReadDriveMemory( bCellNo, dwAddress, wSize, bData, wTimeoutInMillSec);

		}
		

		// House-keeping
		//----------------
		u32nLoops++; // (wrap acceptable)
		LoopCount--;
		
		if (PerformanceTestMode == false)
		{
			// dont overload the CPU (or USB/ethernet)
			sleep (1 ); // Secs
			
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
		static time_t timeNextMinute = time(NULL) + PERIOD_FOR_MID_TERM_AVERAGE_SECS;
		time_t TimeNow = time(NULL);
		if ( ( (TimeNow - TimeOfLastInfoLineUpdate) > InfoUpdateFrequencySeconds) || (LoopCount == 0) ) // neater to always print one at the end
		{
			static double prev_TotalDataBytesReceived = 0;
			static double prev_TotalTimeTransferring_mS = 0;
			static double prev_TotalTimeReceiving_mS = 0; 
			
			static double minute_prev_TotalDataBytesReceived = 0;
			static double minute_prev_TotalTimeTransferring_mS = 0;
			static double minute_prev_TotalTimeReceiving_mS = 0;  
 
			if (TimeNow >= timeNextMinute)
			{
				minute_prev_TotalDataBytesReceived = pstats[bCellNo].TotalDataBytesReceived - minute_prev_TotalDataBytesReceived;       
				minute_prev_TotalTimeTransferring_mS = pstats[bCellNo].TotalTimeTransferring_mS - minute_prev_TotalTimeTransferring_mS;       
				minute_prev_TotalTimeReceiving_mS = pstats[bCellNo].TotalTimeReceiving_mS - minute_prev_TotalTimeReceiving_mS;  
							
				timeNextMinute+= PERIOD_FOR_MID_TERM_AVERAGE_SECS;
			}

//trans@size,  ShortAvg KB/s,(,rcvburst,),  MedAvg KB/s,(,rcvburst,),  TotAvg KB/s,(,rcvburst,), RESP:oruns, nrcvs:Hi,/,avg,  CMD:bads, nrcvs:Hi,/,avg,       
			UpdateTMTestInfoLine("%.6ld xfers@size=%d+8>>, %.0f,(,%.0f,), %.0f,(,%.0f,), %.0f,(,%.0f,), "
																 "%d, %d,/,%d,  %d, %d,/,%d "
										,u32nLoops
										,u16nBytesOfDriveDataToBeRead
										
										//,InfoUpdateFrequencySeconds
										
										,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000) /  (pstats[bCellNo].TotalTimeTransferring_mS - prev_TotalTimeTransferring_mS)
										,((pstats[bCellNo].TotalDataBytesReceived-prev_TotalDataBytesReceived) * 1000) /  (pstats[bCellNo].TotalTimeReceiving_mS - prev_TotalTimeReceiving_mS)
										
										//,PERIOD_FOR_MID_TERM_AVERAGE_SECS

										,((minute_prev_TotalDataBytesReceived) * 1000) /  (minute_prev_TotalTimeTransferring_mS)
										,((minute_prev_TotalDataBytesReceived) * 1000) /  (minute_prev_TotalTimeReceiving_mS)

										,(pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeTransferring_mS
										,(pstats[bCellNo].TotalDataBytesReceived * 1000) /  pstats[bCellNo].TotalTimeReceiving_mS
										

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
			
			TimeOfLastInfoLineUpdate = TimeNow;
		}

		if (clean_exit_requested)
		{
			printlog("*** Breaking out of test/monitoring loop (%lu) due to clean_exit_requested ***\n", u32nLoops);
			break; // out of while not finished loop - so we get nicer cleanup
		}

	} // while (test/monitoring loop)


	//----------------------------------------------------------------
	// testing finished
	// - turn off drive and start cooling down to SafeHandlingTemp
	//----------------------------------------------------------------
	printlog("\nTesting Finished\n" );

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
		printlog("Turning off power to drive %d\n", TrayIndex );
		//TurnOffDrive( TrayIndex );
		sirc = setTargetVoltage( bCellNo, 0, 0);
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
		printlog("Insufficient arguments (must have at least PowerFailRecoveryFlag, CellIndex and TrayIndex)\n");
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
				
		CellIndex = atoi( argv[argc-2] ); // next-to-LAST arg must always be CellIndex
		#if defined OS_IS_LINUX
			if ( (CellIndex < 1) || (CellIndex > 128) )
			{
				printlog("Invalid CellIndex supplied (as CellIndex %d)\n", CellIndex);
				Usage();
				return false;
			}
		#elif defined OS_IS_WINDOWS 
			// NOTE:
			//  we cannot sanity-check the CellIndex (= 0-based usb dev-num) YET 
			// - because we must make an allowance for TestManager and TestCase being in NoCellMode
			//   (in which case we will be supplied a CellIndex of -1 (invalid) - since TestManager does not HAVE a DevNum for unpowered cells!!!!!!
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
				break; // ot of for
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

	#ifdef OS_IS_WINDOWS
		// NOTE we must make an allowance here for TestManager and TestCase being in NoCellMode
		// (in which case we will be supplied a CellIndex of -1 (invalid) - since TestManager does not HAVE a DevNum for unpowered cells!!!!!!
		if ( (CellIndex < 0) || (CellIndex > 127 ) )
		{
			UpdateTMTestInfoLine("Invalid CellIndex supplied (as XyDevNum %d)", CellIndex); // also does printlog()
			Usage();
			brc = false;
		}
	#endif

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
	int nsecs_waited = 0;
	Word wTempInHundredth = 0;
	Byte OnTemp = 0;
	Byte sirc =0;
	
	sleep(2); // enough to allow PID in cell  to start to process any prior request
	
	
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
		
		UpdateTMTestInfoLine( "Awaiting temp %d (now %d, %d secs max more wait envFlags=0x%.2X)", AwaitedTempInHundredths, wTempInHundredth, max_wait_secs-nsecs_waited, flags);


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
		
		
		sleep(5);
		nsecs_waited+=5;
		if (nsecs_waited > max_wait_secs)
		{
			UpdateTMTestInfoLine("WaitForOnTempIndication: timed out waiting for ontemp (after %d secs)!", nsecs_waited);
			break;
		}
		
		if (clean_exit_requested)
		{
			printlog("WaitForOnTempIndication: breaking (after %d secs) due to clean_exit_requested !", nsecs_waited);
			break;
		}
		
	}
	
}


bool isDriveAlreadyPowered( int TrayIndex)
//-------------------------------------------------------------------------------------------------------------
{
	bool brc = false;
	Word w5, w12;
	Byte sirc = getTargetVoltage( bCellNo, &w5, &w12);
	if (sirc == 0)
	{
		brc = ((w5>0) || (w12 > 0)) ? true : false;
	}
	else
	{
		printlog("WaitForOnTempIndication: error %d from getCurrentTemperature - aborting wait", sirc);
	}
	return brc;
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







