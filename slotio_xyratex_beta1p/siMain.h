
#include "xytypes.h" // Xyratex general datatypes (uint8 etc...)

#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/time.h> // for gettimeofday()
#define GET_TIME( x )  	gettimeofday( x, NULL);
#define TIME_TYPE   struct timeval

unsigned long ElapsedTimeIn_mS ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd);
unsigned long ElapsedTimeIn_uS ( TIME_TYPE *ptvStart, TIME_TYPE *ptvEnd);

// (ChrisB: 04Jun10  NOTE: the user-facing 'externs' for the Tracecontrol bools and pstats are now in new file libsi_xyexterns.h)

#if defined XCALIBRE

	#include "CellEnv.h" // Xyratex Environment class etc.
	#include "CellRIMHGSTSerial3DP.h" // Xyratex RIM (serial) class etc. 

  #define CCELLENV CCellEnv
  #define CCELLRIM CCellRIMHGSTSerial3DP
	
#elif defined OPTIMUS  

	#include "CellEnvHGSTOptimus.h" // Xyratex Environment class etc.
	#include "CellRIMHGSTSerialOptimus.h" // Xyratex RIM (serial) class etc.
	#include "CellEnvHGSTOptimus_params.h"    // new Optimus-only super-Parameter system (for FPGA test mode etc)

  #define CCELLENV CCellEnvHGSTOptimus
  #define CCELLRIM CCellRIMHGSTSerialOptimus
  
#endif


#define printf print_NOT_ALLOWED  // catch any stray printfs - note NO CONSOLE for testcases started by TestManager!!!!


#define LOG_ERROR                             tcPrintf(__FILE__, __LINE__, __func__, "ERROR:"
#define LOG_WARNING                           tcPrintf(__FILE__, __LINE__, __func__, "Warning:"
#define LOG_INFO    if (WantLogAPIInfo)      tcPrintf(__FILE__, __LINE__, __func__, "Info: "
#define LOG_TRACE   if (WantLogAPITrace)     tcPrintf(__FILE__, __LINE__, __func__, "trace:"


#define LOG_ENTRY   if (WantLogAPIEntryExit) tcPrintf(__FILE__, __LINE__, __func__, "Entry:"
#define LOG_EXIT    if (WantLogAPIEntryExit) tcPrintf(__FILE__, __LINE__, __func__, "Exit: "



#define CELLENV_XYRC_TO_STRING(xyrc)    CCELLENV::ConvertXyrcToPsz(xyrc) 
#define CELLRIM_XYRC_TO_STRING(xyrc)    CCELLRIM::ConvertXyrcToPsz(xyrc)

	
#define ASSERT_INITIALIZED        if (!Initialize_success) { LOG_ERROR "not siInitialized"); return 255; }
#define ASSERT_INITIALIZED_VOID   if (!Initialize_success) { LOG_ERROR "not siInitialized"); return; }
#define ASSERT_INITIALIZED_BOOL_TRUE   if (!Initialize_success) { LOG_ERROR "not siInitialized"); return true; }


struct psd // per_slot_data
{
	Byte  bCellNo; // this is the key - but no harm storing it here (eg for use in Logging)
	
	uint8 CellIndex;
	uint8 TrayIndex;
	uint8 EnvironmentId;
	
	uint8 UARTChannelId;
	
	uint16 PlugStatusBit;  // for this 'slot'
	
	uint16 PSU_sid_mask;  // supply(s) for this 'slot'
	uint16 PSU_sid_5V;   // 5V supply id for this 'slot'
	uint16 PSU_sid_12V;  // 12V supply id for this 'slot'
	
	//struct _protocol_stats pstat; // seperate, to allow safer read-access from TestCase
};



extern struct psd psda[255];  // per_slot_data_indexed_by_bCellNo



// note only ever one instance of Cell Classes per-testcase (regardless of number of threads or slots) !!
#if defined XCALIBRE
	extern CCellEnv              *pCellEnv;
	extern CCellRIMHGSTSerial3DP *pCellRIM;
#elif defined OPTIMUS
	extern CCellEnvHGSTOptimus *pCellEnv;
	extern CCellRIMHGSTSerialOptimus *pCellRIM;
#endif	
	

extern bool Initialize_success;



/*
//----------------------------------------------------------------------
int cellIdFrombCellNo(Byte bCellNo)
//----------------------------------------------------------------------
{
	int CellId = 0;
#if defined (OPTIMUS)
  CellId = (bCellNo - 1) / 12 + 1;
#elif defined(XCAL)
  CellId = (bCellNo - 1) / 2 + 1;
#else
  CellId = -1;
#endif
  return CellId;
}
//----------------------------------------------------------------------
int trayIndexFrombCellNo(Byte bCellNo)
//----------------------------------------------------------------------
{
	int TrayIndex=0; 
#if defined(OPTIMUS)
  TrayIndex = (bCellNo - 1) % 12 + 1;
#elif defined(XCAL)
  TrayIndex = (bCellNo - 1) % 2;
#else
  TrayIndex = -1;
#endif
  return TrayIndex;
}
extern int EnvironmentId;  // [DEBUG] valid range for 3DP is 0 ONLY
//----------------------------------------------------------------------
int environmentIdFrombCellNo(Byte bCellNo)
//----------------------------------------------------------------------
{
#if defined(OPTIMUS)
  EnvironmentId = (bCellNo - 1) % 12 + 1;
#elif defined(XCAL)
  EnvironmentId = 0;
#else
  EnvironmentId = -1;
#endif
  return EnvironmentId;
}

*/




