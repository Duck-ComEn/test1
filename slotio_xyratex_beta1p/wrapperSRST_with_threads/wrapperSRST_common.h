#if defined XCALIBRE
	#define APP_NAME  		"wrapperSRST_xcal" // needed by printlog
#elif defined OPTIMUS
	#define APP_NAME  		"wrapperSRST_optimus" // needed by printlog
#endif	

#include "xytypes.h"

void Testcase_handle_xyrc_failure( int xyrc); // Envfuncs callback to us on error
 

extern int  InfoUpdateFrequencySeconds; // -u option
extern bool PerformanceTestMode; // / -perf option, affects amount of logging, housekeeping, CPU/USB 'niceness' etc.
extern bool LeavePowerOnAfterTest; // -ne 

extern bool DontTerminateOnBadXYRCs; // -xyrcignore

extern bool clean_exit_requested;

extern bool WantPlugCheck;
extern bool WantPowerDrive;
extern bool WantSerialComms;

extern bool WantNEWPROTOCOL; // declared by main, used by drivethread (NOT from library)

extern int LoopCount;

extern uint16 u16nBytesOfDriveDataToBeRead;



// local functions
void myhandler(int sig);
void Usage(void);
bool ParseCmdLineArguments( int argc, char *argv[]);
void EndTestcaseSafely( int ExitCode );  // WARNING - THIS FUNCTION NEVER RETURNS
void WaitForOnTempIndication( Word AwaitedTempInHundredths, int max_wait_secs, bool PrintTempsWhileWaiting); 


// A testcase's process exit code MUST  be in the range 0 to 255
//  (to allow it to be picked up by TestManager across all supported Operating Systems)
// The convention is 0 for 'drive testing completely passed', non-zero (1 to 255) for 'some kind of problem testing drive' 
//---------------------------------------------------------------------------------

#define EXITCODE_DRIVETEST_COMPLETE_PASSED  	0  // pass

// These indicate problems with the Drive Under Test ITSELF (ie BAD-DRIVE)   
#define EXITCODE_DRIVETEST_FAILED_AT_STAGE_1     1
#define EXITCODE_DRIVETEST_FAILED_AT_STAGE_2     2     
// etc.
  
// These indicate problems that could be the Testing hardware, the testing process, or possibly still the Drive itself (Cell, OperatingSystem, Configuration etc.)    
#define EXITCODE_TESTCASE_FAILED_CELL_NOT_ONLINE  	81 // could be bad cell, bad networking, or bad 'factory process' (ie really not online)
#define EXITCODE_TESTCASE_FAILED_NO_DRIVE_PLUGGED  	82 // could be bad cell, bad drive, or bad factory process (ie really no drive)
#define EXITCODE_TESTCASE_FAILED_ENV_STATUS_ERROR_BEFORE_TEST  	83 // could be bad cell, bad drive, or bad factory process (ie really no drive)
#define EXITCODE_TESTCASE_FAILED_ENV_STATUS_ERROR_DURING_TEST_SETUP  	84 // could be bad cell, bad drive, or bad factory process (ie really no drive)
#define EXITCODE_TESTCASE_FAILED_ENV_STATUS_ERROR_DURING_TEST_LOOP  	85 // could be bad cell, bad drive, or bad factory process (ie really no drive)

#define EXITCODE_TESTCASE_FAILED_UNDEFINED  				91  // catch-all - try not to rely on this!
#define EXITCODE_TESTCASE_FAILED_CMDLINE_ERROR  		92  
#define EXITCODE_TESTCASE_FAILED_XYAPI_ERROR  			93  
#define EXITCODE_TESTCASE_FAILED_SYSTEM_ERROR  			94  
#define EXITCODE_TESTCASE_FAILED_PROCEDURAL_ERROR  	95  
#define EXITCODE_TESTCASE_FAILED_RECEIVED_SIG_TERM 	96 // received SIG-TERM signal (RequestTestCaseTerminate, or kill -15) 


#ifdef WANT_DEFINE_CONVERSION // only one file should do this

char *ExitCodeToString( int u8ExitCode)
{
	char * p = "xx";
	switch (u8ExitCode)
	{
		case EXITCODE_DRIVETEST_COMPLETE_PASSED:   p="DRIVETEST_COMPLETE_PASSED"; break;
		case EXITCODE_DRIVETEST_FAILED_AT_STAGE_1: p="DRIVETEST_FAILED_AT_STAGE_1"; break;
		case EXITCODE_DRIVETEST_FAILED_AT_STAGE_2: p="DRIVETEST_FAILED_AT_STAGE_2"; break;

		case EXITCODE_TESTCASE_FAILED_CELL_NOT_ONLINE: p="FAILED_CELL_NOT_ONLINE"; break;
		case EXITCODE_TESTCASE_FAILED_NO_DRIVE_PLUGGED: p="FAILED_NO_DRIVE_PLUGGED"; break;
		case EXITCODE_TESTCASE_FAILED_ENV_STATUS_ERROR_BEFORE_TEST: p="FAILED_ENV_STATUS_ERROR_BEFORE_TEST"; break;
		case EXITCODE_TESTCASE_FAILED_ENV_STATUS_ERROR_DURING_TEST_SETUP: p="FAILED_ENV_STATUS_ERROR_DURING_TEST_SETUP"; break;
		case EXITCODE_TESTCASE_FAILED_ENV_STATUS_ERROR_DURING_TEST_LOOP: p="FAILED_ENV_STATUS_ERROR_DURING_TEST_LOOP"; break;

		case EXITCODE_TESTCASE_FAILED_UNDEFINED: p="FAILED_UNDEFINED"; break;
		case EXITCODE_TESTCASE_FAILED_CMDLINE_ERROR: p="FAILED_CMDLINE_ERROR"; break;
		case EXITCODE_TESTCASE_FAILED_XYAPI_ERROR: p="FAILED_XYAPI_ERROR"; break;
		case EXITCODE_TESTCASE_FAILED_SYSTEM_ERROR: p="FAILED_SYSTEM_ERROR"; break;
		case EXITCODE_TESTCASE_FAILED_PROCEDURAL_ERROR: p="FAILED_PROCEDURAL_ERROR"; break;
		case EXITCODE_TESTCASE_FAILED_RECEIVED_SIG_TERM: p="FAILED_RECEIVED_SIG_TERM"; break;

		default: p="coding error mixup"; break;
	}
	return p;
}
#else
	char *ExitCodeToString( int u8ExitCode);
#endif







