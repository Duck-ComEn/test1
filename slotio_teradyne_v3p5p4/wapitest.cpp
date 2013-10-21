/*!
 * @file
 * The file contains the implementation of the Hitachi Interface
 * functions in terms of Teradyne TER_ API functions.
 * 
 * The functions required by Hitachi are prototyped in envApi.h.
 * 
 * Since the functions are not documented in envApi.h, we will do our best 
 * to figure out what they suppose to do and document our 
 * understanding/implementation clearly in this file.
 */


/*!
 * System Include Files
 */
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <assert.h>

/*
 * Teradyne Specific Include Files
 */
#include "common.h"
#include "commonEnums.h"
#include "terSioLib.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "ver.h"

#include "tcTypes.h"
#include "libsi.h"
#include "libu3.h"

#define /*array Element Count*/ ElmCnt(ARY) (sizeof(ARY) / sizeof(*(ARY)))
#define ISCMD(a,b,c) ((cmd[0] == (a)) && (cmd[1] == (b)) && (cmd[2] == (c)))
#define /*"NULL" for NULL*/ N4N(STR) ((STR)? (STR) : "NULL")

#define WAPI_BUFFER_SIZE   	   				65536 + 32

#define WAPI_DRIVE_TIMEOUT_MS  				30000
#define WAPI_PROFILE_START	   				profileTimer(0);
#define WAPI_PROFILE_STOP	   				profileTimer(1);
#define WAPI_DRIVE_TYPE_EC7	   				0
#define WAPI_DRIVE_TYPE_EB7	   				1
#define WAPI_DRIVE_TYPE_CBD					2
#define WAPI_DRIVE_TYPE_INVALID 			100

/* Test09 configs */
#define WAPI_TEST09_ECHO_COUNT 		       	0          /* Number of echos after target temp is reached */
#define WAPI_TEST09_TARGET_TEMP1	       	6000       /* Target temp * 100 */
#define WAPI_TEST09_TARGET_TEMP2	       	3000       /* Target temp * 100 */
#define WAPI_TEST09_POS_RAMP 	           	100        /* Pos ramp * 100 */
#define WAPI_TEST09_NEG_RAMP 	           	100        /* Neg ramp * 100 */
#define WAPI_TEST09_SAFE_TEMP 	           	3000       /* Safe temp * 100 */
#define WAPI_TEST09_TARGET_TEMP_WAIT_SEC   	60*60      /* Max time to reach target temp */
#define WAPI_TEST09_IDLE_WAIT_SEC          	2*3600     /* Time spent while at target temp (if WAPI_TEST09_ECHO_COUNT = 0) */
#define WAPI_TEST09_VOLTAGE_CHECK 		   	0          /* Option to disable voltage check in the beginning (mobile) */
#define WAPI_TEST09_HTEMP_ENABLE 		   	1          /* Option to enable/disable htemp correction */

// --- The argument count (argc) and argument value(s) (argv) given to the
// main method are copied to static items in order to make them available to
// all methods.
static int /*Argument Count*/    argCnt   = 0;
static char* /*Argument String values*/ argStr[] = {
	(char*)0, (char*)0, (char*)0, (char*)0, (char*)0,
	(char*)0, (char*)0, (char*)0, (char*)0, (char*)0
};
static int /*Argument integer Values*/ argInt[] = { // same as argStr
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// --- Argument Copy performs the copy of the main method's argc and argv...
// INPUTS-
// 	ARGC (Argument Count) specifies the number of arguments... If this argument
// 		holds a count greater than the number of available values, it is
// 		treated as if it held the count of values.
// 	ARGV (Argument Values) specifies the argument values...
// RETURNS- int status (0 == OK)
static int ArgCopy(int ARGC, char* ARGV[]) {
	argCnt = (ARGC < (int)ElmCnt(argStr))? ARGC : (int)ElmCnt(argStr);
	for (int /*Index*/ ix = 0; ix < argCnt; ix++) { // Copy argument[ix].
		if (/*not NULL?*/ ARGV[ix]) { // Copy as-is.
			argStr[ix] = ARGV[ix];
		} else { // Refer to "NULL"
			argStr[ix] = "NULL";
		}
		assert(argStr[ix]);
		if ((*argStr[ix] == '0') && (tolower(*(argStr[ix] + 1)) == 'x')) {
			sscanf(argStr[ix], "%X", &argInt[ix]);
		} else {
			argInt[ix] = atoi(argStr[ix]); // Zero (0) if no digits
		}
	}
	return /*OK*/ 0;
}

typedef struct drive_info_t 
{
  	unsigned int  powerupDelay;
  	Dword 	interCharDelay;
  	Byte 	ackTimeout;
  	Byte 	uartProtocol;
	Dword   readAddr;
	Dword   writeAddr;
	Byte    skipWrite;
	Word	uartPullupVoltage;
	Dword	baudRate;
}  
DRIVE_INFO;


static Byte Data[WAPI_BUFFER_SIZE];
static int  verbose;
static int 	quiteMode;
static int 	driveType;
                          /* pdelay	icdelay	acktmo	uartpro	rAddr		wAddr		skipw	uartPullupVoltage	baudRate*/
DRIVE_INFO driveInfo[3] = { {10,	540,    3,     	0,      0x08331000, 0x8331063,  1,		2700,				1843200},	   	/* EC7 	*/
						    {10,	540,    3,     	0,      0x08331000, 0x8331064,  1,		2700,				1843200},		/* EB7	*/
							{10,	0,		0,		0,		0x01800000,	0x0180000,	0,		1800,				115200}			/* CBD	*/
						  };
		

Dword   baudRates[] = {115200,460800,921600,1843200,2778000,3333000,4167000,5556000,8333000,1843200};
int baudRateIndex = 9;

extern void terPrintError(Byte bCellNo, TER_Status status);
extern void terPrintSlotInfo(Byte bCellNo);
 
/**
   * <pre>
   * Description:
   *   Shows API help
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static void showApiHelp() {
	printf("Usage: ./wapitest <CMD> <SLT> [options...]\n");
	printf("   brw <SLT>                             - BulkReadFromDrive & BulkWriteToDrive\n");
	printf("   cce <SLT>                             - clearCellEnvironmentError\n");
	printf("   che <SLT>                             - siClearHaltOnError\n");
	printf("   ctu <SLT>                             - siChangeToUart3\n");
	printf("   ech <SLT> size -d<driveType>          - siEchoDrive(wSize)\n");
	printf("   evt <SLT>                             - getEventTrace\n");
	printf("   gac <SLT>                             - getActualCurrent\n");
	printf("   gat <SLT>                             - getAckTimeout\n");
	printf("   gci <SLT>                             - getCellInventory\n");
	printf("   gcl <SLT>                             - getCurrentLimit\n");
	printf("   gct <SLT>                             - getCurrentTemperature\n");
	printf("   gcv <SLT>                             - getCurrentVoltage\n");
	printf("   gho <SLT>                             - getHeaterOutput\n");
	printf("   gid <SLT>                             - siGetInterCharacterDelay\n");
	printf("   gmb <SLT>                             - siGetImpulseCodeMfgMode\n");
	printf("   gmc <SLT>                             - siGetImpulseCountMfgMode\n");
	printf("   gmd <SLT>                             - siGetIntercharacterDelayMfgMode\n");
	printf("   gmp <SLT>                             - siGetPowerDelayMfgMode\n");
	printf("   gms <SLT>                             - siGetSignatureMfgMode\n");
	printf("   gnr <SLT>                             - getNegativeTemperatureRampRate\n");
	printf("   gpr <SLT>                             - getPositiveTemperatureRampRate\n");
	printf("   grt <SLT>                             - getReferenceTemperature\n");
	printf("   gsp <SLT>                             - getShutterPosition\n");
	printf("   gte <SLT>                             - getTemperatureErrorStatus\n");
	printf("   gtl <SLT>                             - getTemperatureLimit\n");
	printf("   gtn <SLT>                             - getTemperatureEnvelope\n");
	printf("   gtp <SLT> p1                          - TER_GetProperty(p1)\n");
	printf("   gtt <SLT>                             - getTargetTemperature\n");
	printf("   gtv <SLT>                             - getTargetVoltage\n");
	printf("   gub <SLT>                             - siGetUartBaudrate\n");
	printf("   gup <SLT>                             - getUartPullupVoltage\n");
	printf("   guv <SLT>                             - siGetDriveUartVersion\n");
	printf("   gve <SLT>                             - getVoltageErrorStatus\n");
	printf("   gvf <SLT>                             - getVoltageFallTime\n");
	printf("   gvl <SLT>                             - getVoltageLimit\n");
	printf("   gvr <SLT>                             - setVoltageRiseTime\n");
	printf("   ice <SLT>                             - isCellEnvironmentError\n");
	printf("   idp <SLT>                             - isDrivePlugged\n");
	printf("   iot <SLT>                             - isOnTemp\n");
	printf("   ist <SLT>                             - siIsSlotThere\n");
	printf("   qry <SLT>                             - siQueryRequest\n");
	printf("   rdm <SLT> addr size -d<driveType>     - siReadDriveMemory\n");
	printf("   rsd <SLT>                             - siResetDrive\n");
	printf("   sat <SLT> p1                          - siSetAckTimeout\n");
	printf("   sdl <SLT> p1                          - siSetDebugLogLevel\n");
	printf("   sdt <SLT> delay(uSec)                 - siSetDriveDelayTime\n");
	printf("   sfr <SLT> rpm                         - siSetDriveFanRPM\n");
	printf("   sht <SLT> temp(100'ths of deg C)      - setSafeHandlingTemperature\n");
	printf("   sid <SLT> val(bit times)              - siSetInterCharacterDelay\n");
	printf("   sip <SLT> delay(uSec)                 - siSetInterPacketDelay\n");
	printf("   sll <SLT> {0:off, 1: on}              - siEnableLogLevel\n");
	printf("   smb <SLT> [{nnn, 0xXX}]               - siSetImpulseCodeMfgMode\n");
	printf("             Domain: [0x00, 0xFF]; Default: 0x%02X\n", MFGImpulseCodeDEFAULT);
	printf("             Examples: smb 1     or     smb 1 0x45\n");
	printf("   smc <SLT> [nnn]                       - siSetImpulseCountMfgMode\n");
	printf("             Domain: [%u, %u]; Default: %u\n", MFGImpulseCountMINIMUM,
			MFGImpulseCountMAXIMUM, MFGImpulseCountDEFAULT);
	printf("             Examples: smc 18     or     smc 22 35\n");
	printf("   smd <SLT> [nnn]                        - siSetIntercharacterDelayMfgMode\n");
	printf("             Domain: [%u, %u] bits; Default: %u bits\n",
			MFGIntercharacterDelayMINIMUM, MFGIntercharacterDelayMAXIMUM,
			MFGIntercharacterDelayDEFAULT);
	printf("             Examples: smd 5     or     smd 17 9\n");
	printf("   smp <SLT> [nnn]                       - siSetPowerDelayMfgMode\n");
	printf("             Domain: [%u, %u] ms; Default: %u ms\n",
			MFGPowerDelayMINIMUM, MFGPowerDelayMAXIMUM,
			MFGPowerDelayDEFAULT);
	printf("             Examples: smp 4     or     smp 3 91\n");
	printf("   sms <SLT> [XX...XX]                   - siSetSignatureMfgMode\n");
	printf("             Default: %s[%u]\n",
			Common::Sig2Str((Byte*)MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT),
			MFGSignatureLengthDEFAULT);
	printf("             Examples: sms 7     or     sms 6 534F7A9 (do not use 0x)\n");
	printf("             Note: signatures do not persist!\n");
	printf("   smx <SLT> [5V [12V]]                  - siSetPowerEnableMfgMode\n");
	printf("             Defaults: 5000 mV and 12000 mV\n");
	printf("             Examples: smx 8     or     smx 91 4734"
			"     or     smx 123 4999 12999\n");
	printf("   snr <SLT> rate(100'ths of deg C)      - setNegativeTemperatureRampRate\n");
	printf("   spr <SLT> rate(100'ths of deg C)      - setPositiveTemperatureRampRate\n");
	printf("   ssl <SLT> {0:off, 1: on}              - siSetLed\n");
	printf("   ste <SLT> temp(10's of deg C)         - setTemperatureEnvelope\n");
	printf("   stl <SLT> p1 p2 p3 p4                 - setTemperaturelimit\n");
	printf("   stp <SLT> p1 val                      - TER_SetProperty(p1) to val\n");
	printf("   stt <SLT> temp(100'ths of deg C)      - setTargetTemperature\n");
	printf("   stv <SLT> 5V(mV) 12V(mV)              - setTargetVoltage\n");
	printf("   sub <SLT> rate (or . dft)             - siSetUartBaudrate\n");
	printf("   sum <SLT>                             - siSetUART2Mode\n");
	printf("   sup <SLT> volts(mV)                   - setUartPullupVoltage\n");
	printf("   sut <SLT> p1                          - siSetUartProtocol\n");
	printf("   svd <SLT>                             - setUartPullupVoltageDefault\n");
	printf("   svf <SLT> 5V(mV) 12V(mV)              - setVoltageFallTime\n");
	printf("   svi <SLT> delay(mSec)                 - setVoltageInterval\n");
	printf("   svl <SLT> 5VL 12VL 5VH 12VH (mV)      - setVoltageLimit\n");
	printf("   svr <SLT> 5V(mV) 12V(mV)              - getVoltageRiseTime\n");
	printf("   tNN <SLT> -d<driveType>               - test case where NN is a test case number\n");
	printf("   wdm <SLT> addr size -d<driveType>     - siWriteDriveMemory\n");
	printf("---------- Notes ----------\n");
	printf("   1) Slot numbers (<SLT>) are ONE (1) based\n");
	printf("   2) Default SIO IP Address is 10.101.1.2\n");
	printf("   3) To change IP address: export SIOIPADDRESS=\"xx.xx.xx.xx\"\n");
	printf("   4) To unset IP address : unset SIOIPADDRESS\n");
	printf("   5) To enable debug log, append -L1 at the end of the command;\n");
	printf("      to disable, append -L0\n");
	printf("   6) Log file is available at /var/tmp/slotNNNTTT...TT.log\n");
	printf("      where NNN=slot number and TTT...TT is a date and time stamp.\n");
	printf("   7) Valid drive types: ec7, eb7, cbd\n");
	printf("   8) SetWapitestResetSlotDuringInitialize may be forced by the -s{0, 1} option\n");
	printf("      for example: -s0   or   -s1\n");
	printf("   9) Wait points (Press ENTER) may be effected with the -w option (no value)\n");
	printf("Test cases: 01=echo/read/write, 02=baud rates, 03=5V margining, 04=temp control, 05=misc\n");
	printf("            06=bulk read/write (for eng diagnostics), 08=1000 echo, 00=MFG\n");
	printf("Example: ./wapitest t01 3 -L1\n");
	printf("==========\n");
}

/**************************************************************************
 **  Utility functions - Begin
 **************************************************************************/
static void terprintf( const char* format, ... )
{
    if(quiteMode)
		return;

	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
	fflush(stdout);
} 
 
static void printBanner(char *str)
{
	terprintf("************************************************************\n");
	terprintf("** %s\n",str);
	terprintf("************************************************************\n");

}
static void printData(char *str, Byte *Data, Word size)
{
	int 	i;  

    terprintf("%s: Read %d bytes \n", str, size);
	if( verbose ) {
    	for (i = 0; i< size; i++) {
    		terprintf("%02x ", Data[i]);
    		if( (i % 40) == 0 && i) terprintf("\n");
    	}
		terprintf("\n");
	}
}

static void printTemperatureVariables(int id)
{
    Byte 	rc = 0;
	Word    currentTemp, heaterOut, shutterPos, posRampRate, negRampRate, tempError;

	rc = getCurrentTemperature(id, &currentTemp);
	terprintf("getCurrentTemp = %d, ", currentTemp);

    rc = getHeaterOutput(id, &heaterOut);
 	terprintf("getHeaterOutput = %d, ", heaterOut);

    rc = getShutterPosition(id, &shutterPos);
 	terprintf("getShutterPos = %d, ", shutterPos);

	rc = getPositiveTemperatureRampRate(id, &posRampRate);
	terprintf("getPosRampRate = %d, ", posRampRate);

	rc = getNegativeTemperatureRampRate(id, &negRampRate);
	terprintf("getNegRampRate = %d, ", negRampRate);

	rc = isOnTemp(id);
	terprintf("isOnTemp = %s, ", rc?"Yes":"No");
	
	rc = isCellEnvironmentError(id);
	terprintf("isCellErr = %s, ", rc?"Yes":"No");
	
	rc = getTemperatureErrorStatus(id, &tempError);
	terprintf("tempErr = %x ", tempError);

 	rc = isDrivePlugged(id);
	terprintf("isDrivePlugge %d\n", rc);


	return;
}

static Byte initTest(int id, int driveType)
{
    Byte 	rc = 0;
	
	if(!isDrivePlugged(id)) {
		printf("Slot %d, Cannot run this test. Drive is not plugged in. Exiting... \n", id);
		exit(0);
	}

	terprintf("Drive is warming up. Wait %d secs \n", driveInfo[driveType].powerupDelay);
	if(driveType > WAPI_DRIVE_TYPE_CBD ) {
		printf("ERROR: Incorrect drive type [%d] \n", driveType);
		printf("       Usage: -d[cbd|eb7|ec7]\n");
		return 1;
	}

	rc = siSetInterCharacterDelay(id, driveInfo[driveType].interCharDelay);
    rc = siSetUartProtocol(id,driveInfo[driveType].uartProtocol);
	rc = siSetAckTimeout ( id,driveInfo[driveType].ackTimeout);

	rc = siSetUartBaudrate(id, driveInfo[driveType].baudRate);

//	rc = setUartPullupVoltage(id, 0);

	rc = setVoltageRiseTime(id,0,0);

	rc = setTargetVoltage(id, 5000, 12000); 

	rc = setUartPullupVoltage(id, driveInfo[driveType].uartPullupVoltage);

	sleep(driveInfo[driveType].powerupDelay);
	
	return rc;
}

static Byte finishTest(int id, int driveType)
{
    Byte rc = 0;

	rc = setTargetVoltage(id, 0, 0);
	
	return rc;
}
 
 
static void  profileTimer (int cmd)
{
	static 	struct timeval result, x, y;
	static	struct timezone tz;
	static	struct tm tm;

	if( cmd == 0 ) {   /* start profiling */
		gettimeofday(&y, &tz);
		localtime_r((const time_t *)&y.tv_sec, &tm);
	}

	else if ( cmd == 1 ) { /* end profiling */
		gettimeofday(&x, &tz);
		localtime_r((const time_t *)&x.tv_sec, &tm);

  		/* Perform the carry for the later subtraction by updating y. */
  		if (x.tv_usec < y.tv_usec) {
			int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
			y.tv_usec -= 1000000 * nsec;
			y.tv_sec += nsec;
  		}
  		if (x.tv_usec - y.tv_usec > 1000000) {
			int nsec = (x.tv_usec - y.tv_usec) / 1000000;
			y.tv_usec += 1000000 * nsec;
			y.tv_sec -= nsec;
  		}

  		/* Compute the time remaining to wait. tv_usec is certainly positive. */
  		result.tv_sec = x.tv_sec - y.tv_sec;
  		result.tv_usec = x.tv_usec - y.tv_usec;

  		/* Return if result is negative. */
  		if(x.tv_sec < y.tv_sec)
			printf("[Elapsed Time: _._ ] ");
		else
	 		printf("[Elapsed Time:%d.%d] ", (int)result.tv_sec, (int)result.tv_usec);
	}
	return;
}

static Byte  gethtemp (int id)
{
	 static unsigned driveMemTopAddr = 0;
	 static unsigned sensorIOAddress = 0;
	 unsigned offset = 0x1FC;
   	 Byte 	rc = 0, htemp = 0;

	 if( sensorIOAddress == 0 || driveMemTopAddr == 0 ) {
    	 rc = siEchoDrive(id, 512, Data, WAPI_DRIVE_TIMEOUT_MS);
    	 if(rc == 0)  {
	 		 driveMemTopAddr = Data[offset+2] << 24 | Data[offset+3] << 16 | 
	 						   Data[offset+0] << 8  | Data[offset+1];
     		 terprintf("Drive top memory addresss = 0x%08x \n", driveMemTopAddr);
    	 }
		 else
	 		 terprintf("Error %02x: siEchoDrive failed \n", rc);

		 rc = siReadDriveMemory(id, (driveMemTopAddr+0x24), 4, Data, WAPI_DRIVE_TIMEOUT_MS);
    	 if(rc == 0) {
	 		 sensorIOAddress = (Data[2] << 24) | (Data[3] << 16) |  
	 						   (Data[0] << 8)  | Data[1];
     		 terprintf("Sensor IO addresss = 0x%08x \n", sensorIOAddress);
    	 }
		 else {
     		 printf("siReadDriveMemory: Failed \n" );				 
		 }
	 }

   	 rc = siReadDriveMemory(id, sensorIOAddress, 2, Data, WAPI_DRIVE_TIMEOUT_MS);
     if(rc == 0) {
		 htemp =  Data[1];
     }
	 else {
     	 printf("Slot %d, get htemp: Failed \n", id ); 
	 }
	 
	 return(htemp);
}

static void  adjhtemp (int id, FILE *f)
{
#if WAPI_TEST09_HTEMP_ENABLE
   	Byte 	rc = 0;
	Word    currentTemp, htemp, heaterOut, shutterPos, targetTemp, isTempReached; 
    struct  timeval tv;
//	extern 	TER_HtempInfo siHtempInfo;
	static  Byte firstTime = 1;
    static  struct  timeval first_tv;

	/* htemp*/
	htemp = gethtemp(id);
	rc = adjustTemperatureControlByDriveTemperature(id, 100*htemp); 

	rc = getCurrentTemperature(id, &currentTemp);
	rc = getHeaterOutput(id, &heaterOut);
	rc = getShutterPosition(id, &shutterPos);
	isTempReached = isOnTemp(id);
/*	if(isTempReached) {
		 targetTemp = siHtempInfo.targetTemperature;
		 printf("on_t %d, ramp %d, cur_t %d tgt_t %d, tgt_i %d, h_t %d, delta_t %d, adj_tm %d, wait_tm %d, delta_tm %d, delta_in %d \n", 
		 siHtempInfo.targetTemperatureReached,
		 siHtempInfo.positiveRampRate,			
		 siHtempInfo.currentTemperature,
		 siHtempInfo.targetTemperature,
		 siHtempInfo.targetTemperatureInitial,  
		 siHtempInfo.htemp,
		 siHtempInfo.tempDelta,
		 siHtempInfo.htempAdjustmentSetTime,	
		 siHtempInfo.htempWaitTime,			
		 siHtempInfo.timeDelta,
		 siHtempInfo.inletDelta);
	}
	else {*/
		rc = getTargetTemperature(id, &targetTemp);
//	}
	
	if(firstTime) {
		gettimeofday(&first_tv,NULL);
		firstTime = 0;
	}
	
	gettimeofday(&tv,NULL);

	fprintf(f, "%d, %d, %d, %d, %d, %d, %d \n", (int)tv.tv_sec-(int)first_tv.tv_sec, htemp*100, currentTemp, heaterOut, shutterPos, targetTemp, isTempReached);


	fflush(f);
#endif
}

/**************************************************************************
 **  Utility functions - End
 **************************************************************************/

/**************************************************************************
 **  Test functions - Begin
 **************************************************************************/
/** @details Test00 tests the Hitachi MFG "One-Step" methods.
 * @param [in] SID (SLot identity) is the cell number, base one (1)
 * @param [in] DRV (Drive Type)
 * @returns "Were anomalies detected?" {no (0), yes (!0)}
 */
static Byte Test00(int SID, int DRV) { // t00: exercise MFG API...
	int /*Anomaly Count*/             	aC;
	char* /*Arguments*/               	ag[10]; memset(ag, 0, sizeof(ag));
	char /*Argument Values*/          	aV[10240]; (void)memset(aV, 0, sizeof(aV));
	Byte /*Byte Value*/               	bV;
	Byte /*Expected Byte*/            	bX;
	Byte /*Cell Identity (Byte)*/     	cI;
	char /*Error String*/             	eS[MAX_ERROR_STRING_LENGTH] = "";
	Byte /*Result Code*/              	rc;
	Byte /*Expected Result*/          	rX;
	TerSioLib* /*SIO Handle*/	      	sH;
	Byte /*Signature Length*/         	sL;
	Byte /*expected Signature length*/	sN;
	Byte /*Signature Value*/          	sV[MFGSignatureLengthMAXIMUM];
	Byte /*Expected Signature*/       	sX[MFGSignatureLengthMAXIMUM];
	TER_Status /*Teradyne Status*/    	status;
	char /*work Text*/                	tx[2048];
	Word /*Word Value*/               	wV;
	Word /*Expected Word*/            	wX;

    /* --- Prepare local(s)... */
    aC = 0;
    for (bX = 0; bX < (int)ElmCnt(ag); bX++) {
		ag[bX] = aV + (bX * 1024);
	}
    bV = bX = 0;
    cI = (Byte)SID;
    (void)memset(eS, 0, sizeof(eS));
    rc = rX = (Byte)success;
	sH = Common::Instance.GetSIOHandle();
    sL = sN = (Byte)MFGSignatureLengthDEFAULT;
    (void)memset(sV, 0, sizeof(sV));
    (void)memcpy(sV, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
    (void)memset(sX, 0, sizeof(sX));
    (void)memcpy(sX, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
	status = TER_Status_none;
    sprintf(tx, "MFG (\"One-Step\") API Test (%d) - Begin", SID);
    wV = wX = (Word)0;

	printBanner(tx);
	setUartPullupVoltage(SID, 1800);

    /* === Impulse Code ===
     * Valid code values are [0, 255], so there are no tests for out of range
     * code values.
     */
	/* --- Get Impulse Code; get the default */
	printf("Test Get Impulse Code; get the default: 0x%02X (%u)\n",
			MFGImpulseCodeDEFAULT, MFGImpulseCodeDEFAULT);
	bX = (Byte)MFGImpulseCodeDEFAULT;
	rX = (Byte)success;
	rc = siGetImpulseCodeMfgMode(cI, &bV);
	printf("   siGetImpulseCodeMfgMode(%u, %p) == %u", cI, &bV, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)", rX);
	} else {
		printf(" (success, OK); code == 0x%02X (%u)", bV, bV);
		if (/*OK?*/ bV == bX) {
			printf(" (OK)");
		} else {
			aC++;
			printf(" (should be 0x%02X (%u))", bX, bX);
		}
	}
	printf("\n");

	/* --- Get Impulse Code; fail (OK) because of NULL argument */
	printf("Get Impulse Code; fail (OK) because of NULL argument\n");
	rX = (Byte)nullArgumentError;
	rc = siGetImpulseCodeMfgMode(cI, /*NULL*/ (Byte*)0);
	printf("   siGetImpulseCodeMfgMode(%u, /*NULL*/ %p) == %u", cI,
			/*NULL*/ (Byte*)0, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (nullArgumentError, OK)\n");
	}

	/* --- Preset the impulse code to a value other than the first, set
	 * attempt's value. */
	rc = siSetImpulseCodeMfgMode(cI, MFGImpulseCodeMAXIMUM);

	/* --- Set Impulse Code; set the minimum: zero (0) */
	printf("Set Impulse Code; set the minimum: zero (0)\n");
	assert(!MFGImpulseCodeMINIMUM);
	bX = (Byte)MFGImpulseCodeMINIMUM;
	rX = (Byte)success;
	rc = siSetImpulseCodeMfgMode(cI, bX);
	printf("   siSetImpulseCodeMfgMode(%u, 0x%02X (%u)) == %u", cI, bX,
			bX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rc = siGetImpulseCodeMfgMode(cI, &bV);
		printf("\n      siGetImpulseCodeMfgMode(%u, %p) == %u", cI, &bV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); code == 0x%02X (%u)", bV, bV);
			if (/*OK?*/ bV == bX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be 0x%02X (%u))\n", bX, bX);
			}
		}
	}

	/* --- Set Impulse Code; set the maximum: 0xFF (255) */
	assert(MFGImpulseCodeMAXIMUM == 255);
	bX = (Byte)MFGImpulseCodeMAXIMUM;
	rX = (Byte)success;
	printf("Set Impulse Code; set the maximum: 0x%X (%u)\n", bX, bX);
	rc = siSetImpulseCodeMfgMode(cI, bX);
	printf("   siSetImpulseCodeMfgMode(%u, 0x%02X (%u)) == %u", cI, bX, bX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rc = siGetImpulseCodeMfgMode(cI, &bV);
		printf("\n      siGetImpulseCodeMfgMode(%u, %p) == %u", cI, &bV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); code == 0x%02X (%u)", bV, bV);
			if (/*OK?*/ bV == bX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be 0x%02X (%u))\n", bX, bX);
			}
		}
	}

	/* --- Set Impulse Code; set the default */
	bX = (Byte)MFGImpulseCodeDEFAULT;
	rX = (Byte)success;
	printf("Set Impulse Code; set the default: 0x%02X (%u)\n", bX, bX);
	rc = siSetImpulseCodeMfgMode(cI, bX);
	printf("   siSetImpulseCodeMfgMode(%u, 0x%02X (%u)) == %u", cI, bX, bX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rc = siGetImpulseCodeMfgMode(cI, &bV);
		printf("\n      siGetImpulseCodeMfgMode(%u, %p) == %u", cI, &bV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); code == 0x%02X (%u)", bV, bV);
			if (/*OK?*/ bV == bX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be 0x%02X (%u))\n", bX, bX);
			}
		}
	}

    /* === Impulse Count ===
     * Valid code values are [1, 255] but the domain of a Word is [0, 65535]:
     * plenty of room for boundary tests...
     */
#ifndef MFGDefaultBYPASS
	/* --- Get Impulse Count; get the default */
	printf("Get Impulse Count; get the default (%u)\n", MFGImpulseCountDEFAULT);
	wX = (Word)MFGImpulseCountDEFAULT;
	rX = (Byte)success;
	rc = siGetImpulseCountMfgMode(cI, &wV);
	printf("   siGetImpulseCountMfgMode(%u, %p) == %u", cI, &wV, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK); count == %u", wV);
		if (/*OK?*/ wV == wX) {
			printf(" (OK)\n");
		} else {
			aC++;
			printf(" (should be %u)\n", wX);
		}
	}
#endif

	/* --- Get Impulse Count; fail (OK) because of NULL argument */
	printf("Get Impulse Count; fail (OK) because of NULL argument\n");
	rX = (Byte)nullArgumentError;
	rc = siGetImpulseCountMfgMode(cI, /*NULL*/ (Word*)0);
	printf("   siGetImpulseCountMfgMode(%u, /*NULL*/ %p) == %u", cI,
			/*NULL*/ (Word*)0, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (nullArgumentError, OK)\n");
	}

	/* --- Preset the impulse count to a value other than the first, set
	 * attempt's value. */
	rc = siSetImpulseCountMfgMode(cI, MFGImpulseCountMAXIMUM);

	/* --- Set Impulse Count; fail (OK) to set zero (0)... This is, also, a test for
	 * failure setting MFGImpulseCountMINIMUM - 1. */
	printf("Set Impulse Count; fail (OK) to set zero (0)\n");
	wX = (Word)0;
	rX = (Byte)outOfRange;
	rc = siSetImpulseCountMfgMode(cI, wX);
	printf("   siSetImpulseCountMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

	/* --- Set Impulse Count; set MFGImpulseCountMINIMUM... */
	printf("Set Impulse Count; set MFGImpulseCountMINIMUM (%u)\n", MFGImpulseCountMINIMUM);
	wX = (Word)MFGImpulseCountMINIMUM;
	rX = (Byte)success;
	rc = siSetImpulseCountMfgMode(cI, wX);
	printf("   siSetImpulseCountMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rc = siGetImpulseCountMfgMode(cI, &wV);
		printf("\n      siGetImpulseCountMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); count == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Impulse Count; set MFGImpulseCountMAXIMUM... */
	wX = MFGImpulseCountMAXIMUM;
	printf("Set Impulse Count; set MFGImpulseCountMAXIMUM (%u)\n", wX);
	rX = (Byte)success;
	printf("   siSetImpulseCountMfgMode(%u, %u)", cI, wX);
	rc = siSetImpulseCountMfgMode(cI, wX);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		wV = 0;
		rX = (Byte)success;
		printf("\n      siGetImpulseCountMfgMode(%u, %p", cI, &wV);
		rc = siGetImpulseCountMfgMode(cI, &wV);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf("->XXX) == %u (should be %u)\n", rc, rX);
		} else if (/*OK?*/ wV == wX) {
			printf("->%u (OK)) == %u (success, OK)\n", wV, rc);
		} else {
			aC++;
			printf("->%u (should be %u)) == %u (OK, doesn't matter)\n", wV, wX, rc);
		}
	}

	/* --- Set Impulse Count; set MFGImpulseCountDEFAULT... */
	printf("Set Impulse Count; set MFGImpulseCountDEFAULT (%u)\n", MFGImpulseCountDEFAULT);
	wX = MFGImpulseCountDEFAULT;
	rX = (Byte)success;
	rc = siSetImpulseCountMfgMode(cI, wX);
	printf("   siSetImpulseCountMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rX = (Byte)success;
		rc = siGetImpulseCountMfgMode(cI, &wV);
		printf("\n      siGetImpulseCountMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); count == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Impulse Count; fail (OK) to set MFGImpulseCountMAXIMUM + 1... */
	printf("Set Impulse Count; fail (OK) to set MFGImpulseCountMAXIMUM + 1 (%u)\n",
			MFGImpulseCountMAXIMUM + 1);
	wX = (Word)MFGImpulseCountMAXIMUM + 1;
	rX = (Byte)outOfRange;
	rc = siSetImpulseCountMfgMode(cI, wX);
	printf("   siSetImpulseCountMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

	/* --- Set Impulse Count; fail (OK) to set 65535... */
	printf("Set Impulse Count; fail (OK) to set 65535\n");
	wX = (Word)65535;
	rX = (Byte)outOfRange;
	rc = siSetImpulseCountMfgMode(cI, wX);
	printf("   siSetImpulseCountMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

    /* === Inter character Delay ===
     * Valid code values are [0, 31] and the domain of a Word is [0, 65535]:
     * plenty of room for upper boundary tests...
     */
#ifndef MFGDefaultBYPASS
	/* --- Get Inter character Delay; get the default */
	printf("Get Inter character Delay; get the default (%u)\n", MFGIntercharacterDelayDEFAULT);
	wX = (Word)MFGIntercharacterDelayDEFAULT;
	rX = (Byte)success;
	rc = siGetIntercharacterDelayMfgMode(cI, &wV);
	printf("   siGetIntercharacterDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK); delay == %u", wV);
		if (/*OK?*/ wV == wX) {
			printf(" (OK)\n");
		} else {
			aC++;
			printf(" (should be %u)\n", wX);
		}
	}
#endif

	/* --- Get Inter character Delay; fail (OK) because of NULL argument */
	printf("Get Inter character Delay; fail (OK) because of NULL argument\n");
	rX = (Byte)nullArgumentError;
	rc = siGetIntercharacterDelayMfgMode(cI, /*NULL*/ (Word*)0);
	printf("   siGetIntercharacterDelayMfgMode(%u, /*NULL*/ %p) == %u", cI,
			/*NULL*/ (Word*)0, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (nullArgumentError, OK)\n");
	}

	/* --- Preset the inter character delay to a value other than the first,
	 * set attempt's value. */
	rc = siSetIntercharacterDelayMfgMode(cI, MFGIntercharacterDelayMAXIMUM);

	/* --- Set Inter character Delay; set MFGIntercharacterDelayMINIMUM... This
	 * is, also, a test for setting zero (0). */
	printf("Set Inter character Delay; set MFGIntercharacterDelayMAXIMUM"
				" (%u)\n",
			MFGIntercharacterDelayMAXIMUM);
	assert(!MFGIntercharacterDelayMINIMUM);
	wX = (Word)MFGIntercharacterDelayMINIMUM;
	rX = (Byte)success;
	rc = siSetIntercharacterDelayMfgMode(cI, wX);
	printf("   siSetIntercharacterDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rc = siGetIntercharacterDelayMfgMode(cI, &wV);
		printf("\n      siGetIntercharacterDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); delay == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Inter character Delay; set MFGIntercharacterDelayMAXIMUM... */
	printf("Set Inter character Delay; set MFGIntercharacterDelayMAXIMUM (%u)\n", MFGIntercharacterDelayMAXIMUM);
	wX = MFGIntercharacterDelayMAXIMUM;
	rX = (Byte)success;
	rc = siSetIntercharacterDelayMfgMode(cI, wX);
	printf("   siSetIntercharacterDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rX = (Byte)success;
		rc = siGetIntercharacterDelayMfgMode(cI, &wV);
		printf("\n      siGetIntercharacterDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); delay == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Inter character Delay; set MFGIntercharacterDelayDEFAULT... */
	printf("Set Inter character Delay"
			"; set MFGIntercharacterDelayDEFAULT (%u)\n",
			MFGIntercharacterDelayDEFAULT);
	wX = MFGIntercharacterDelayDEFAULT;
	rX = (Byte)success;
	rc = siSetIntercharacterDelayMfgMode(cI, wX);
	printf("   siSetIntercharacterDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rX = (Byte)success;
		rc = siGetIntercharacterDelayMfgMode(cI, &wV);
		printf("\n      siGetIntercharacterDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); delay == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Inter character Delay; fail (OK) to set
	 * MFGIntercharacterDelayMAXIMUM + 1... */
	printf("Set Inter character Delay; fail (OK) to set"
				" MFGIntercharacterDelayMAXIMUM + 1 (%u)\n",
				MFGIntercharacterDelayMAXIMUM + 1);
	wX = (Word)MFGIntercharacterDelayMAXIMUM + 1;
	rX = (Byte)outOfRange;
	rc = siSetIntercharacterDelayMfgMode(cI, wX);
	printf("   siSetIntercharacterDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

	/* --- Set Inter character Delay; fail (OK) to set 65535... */
	printf("Set Inter character Delay; fail (OK) to set 65535\n");
	wX = (Word)65535;
	rX = (Byte)outOfRange;
	rc = siSetIntercharacterDelayMfgMode(cI, wX);
	printf("   siSetIntercharacterDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

    /* === Power Delay ===
     * Valid code values are [5, 100] but the domain of a Word is [0, 65535]:
     * plenty of room for boundary tests...
     */
#ifndef MFGDefaultBYPASS
	/* --- Get Power Delay; get the default */
	printf("Get Power Delay; get the default (%u)\n", MFGPowerDelayDEFAULT);
	wX = (Word)MFGPowerDelayDEFAULT;
	rX = (Byte)success;
	rc = siGetPowerDelayMfgMode(cI, &wV);
	printf("   siGetPowerDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK); count == %u", wV);
		if (/*OK?*/ wV == wX) {
			printf(" (OK)\n");
		} else {
			aC++;
			printf(" (should be %u)\n", wX);
		}
	}
#endif

	/* --- Get Power Delay; fail (OK) because of NULL argument */
	printf("Get Power Delay; fail (OK) because of NULL argument\n");
	rX = (Byte)nullArgumentError;
	rc = siGetPowerDelayMfgMode(cI, /*NULL*/ (Word*)0);
	printf("   siGetPowerDelayMfgMode(%u, /*NULL*/ %p) == %u", cI,
			/*NULL*/ (Word*)0, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (nullArgumentError, OK)\n");
	}

	/* --- Preset the power delay to a value other than the first, set
	 * attempt's value. */
	rc = siSetPowerDelayMfgMode(cI, MFGPowerDelayMAXIMUM);

	/* --- Set Power Delay; fail (OK) to set zero (0)... */
	printf("Set Power Delay; fail (OK) to set zero (0)\n");
	wX = (Word)0;
	rX = (Byte)outOfRange;
	rc = siSetPowerDelayMfgMode(cI, wX);
	printf("   siSetPowerDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

	/* --- Set Power Delay; fail (OK) to set MFGPowerDelayMINIMUM - 1... */
	printf("Set Power Delay; fail (OK) to set MFGPowerDelayMINIMUM - 1 (%u)\n",
			MFGPowerDelayMINIMUM - 1);
	wX = (Word)MFGPowerDelayMINIMUM - 1U;
	rX = (Byte)outOfRange;
	rc = siSetPowerDelayMfgMode(cI, wX);
	printf("   siSetPowerDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

	/* --- Set Power Delay; set MFGPowerDelayMINIMUM... */
	printf("Set Power Delay; set MFGPowerDelayMINIMUM (%u)\n", MFGPowerDelayMINIMUM);
	wX = (Word)MFGPowerDelayMINIMUM;
	rX = (Byte)success;
	rc = siSetPowerDelayMfgMode(cI, wX);
	printf("   siSetPowerDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rc = siGetPowerDelayMfgMode(cI, &wV);
		printf("\n      siGetPowerDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); count == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Power Delay; set MFGPowerDelayMAXIMUM... */
	printf("Set Power Delay; set MFGPowerDelayMAXIMUM (%u)\n",
			MFGPowerDelayMAXIMUM);
	wX = MFGPowerDelayMAXIMUM;
	rX = (Byte)success;
	rc = siSetPowerDelayMfgMode(cI, wX);
	printf("   siSetPowerDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rX = (Byte)success;
		rc = siGetPowerDelayMfgMode(cI, &wV);
		printf("\n      siGetPowerDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); count == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Power Delay; set MFGPowerDelayDEFAULT... */
	printf("Set Power Delay; set MFGPowerDelayDEFAULT (%u)\n",
			MFGPowerDelayDEFAULT);
	wX = MFGPowerDelayDEFAULT;
	rX = (Byte)success;
	rc = siSetPowerDelayMfgMode(cI, wX);
	printf("   siSetPowerDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		rX = (Byte)success;
		rc = siGetPowerDelayMfgMode(cI, &wV);
		printf("\n      siGetPowerDelayMfgMode(%u, %p) == %u", cI, &wV, rc);
		if (/*unexpected result?*/ rc != rX) {
			aC++;
			printf(" (should be %u)\n", rX);
		} else {
			printf(" (success, OK); count == %u", wV);
			if (/*OK?*/ wV == wX) {
				printf(" (OK)\n");
			} else {
				aC++;
				printf(" (should be %u)\n", wX);
			}
		}
	}

	/* --- Set Power Delay; fail (OK) to set MFGPowerDelayMAXIMUM + 1... */
	printf("Set Power Delay; fail (OK) to set MFGPowerDelayMAXIMUM + 1 (%u)\n",
			MFGPowerDelayMAXIMUM + 1);
	wX = (Word)MFGPowerDelayMAXIMUM + 1U;
	rX = (Byte)outOfRange;
	rc = siSetPowerDelayMfgMode(cI, wX);
	printf("   siSetPowerDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

	/* --- Set Power Delay; fail (OK) to set 65535... */
	printf("Set Power Delay; fail (OK) to set 65535\n");
	wX = (Word)65535;
	rX = (Byte)outOfRange;
	rc = siSetPowerDelayMfgMode(cI, wX);
	printf("   siSetPowerDelayMfgMode(%u, %u) == %u", cI, wX, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)\n");
	}

    /* === Signature ===
     * All signature element values are OK. Signature lengths must be in the
     * domain [1, 255].
     */
#ifndef MFGDefaultBYPASS
	/* --- Get Signature; get the default */
	sL = sN = (Byte)MFGSignatureLengthDEFAULT;
	(void)memset(sV, 0, sizeof(sV));
    (void)memcpy(sV, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
	(void)memset(sX, 0, sizeof(sX));
    (void)memcpy(sX, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
	rX = (Byte)success;
	printf("Get Signature; get the default (%s)\n", Common::Sig2Str(sX, sN));
	printf("   siGetSignatureMfgMode(%u, %p", cI, sV);
	rc = siGetSignatureMfgMode(cI, sV, &sL);
	printf("->%s", Common::Sig2Str(sV, sL));
	printf(", %p->%u) == %u", &sL, sN, rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success)");
		if (/*OK?*/ !memcmp(sV, sX, sN)) {
			printf("; (signatures match; OK)");
		} else {
			aC++;
			printf("; (signature should be %s)", Common::Sig2Str(sX, sN));
		}
	}
	printf("\n");
#endif

	/* --- Get Signature; fail (OK) because of NULL argument */
	printf("Get Signature; fail (OK) because of NULL signature argument\n");
	sL = sN = (Byte)MFGSignatureLengthDEFAULT;
	(void)memset(sV, 0, sizeof(sV));
    (void)memcpy(sV, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
    (void)memcpy(sX, sV, sL);
	rX = (Byte)nullArgumentError;
	printf("   siGetSignatureMfgMode(%u, /*NULL*/ %p, %p)", cI, (void*)0, &sL);
	rc = siGetSignatureMfgMode(cI, /*NULL*/ (Byte*)0, &sL);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (nullArgumentError, OK)\n");
	}

	/* --- Preset the signature to a value other than the first, set attempt's
	 * value. */
	(void)memset(sV, 0xFF, sizeof(sV));
	sL = (Byte)sizeof(sV);
	rc = siSetSignatureMfgMode(cI, sV, sL);

	/* --- Set Signature; set MFGSignatureDEFAULT... */
	printf("Set Signature; set MFGSignatureDEFAULT(%s)\n",
			Common::Sig2Str(MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT));
	sL = sN = (Byte)MFGSignatureLengthDEFAULT;
	(void)memset(sV, 0, sizeof(sV));
    (void)memcpy(sV, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
    (void)memcpy(sX, sV, sL);
	rX = (Byte)success;
	printf("   siSetSignatureMfgMode(%u, %p->%s, %u)", cI, MFGSignatureDEFAULT,
			Common::Sig2Str(MFGSignatureDEFAULT, sL), sL);
	rc = siSetSignatureMfgMode(cI, MFGSignatureDEFAULT, sL);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)", rX);
	} else {
		printf(" (success, OK)");
	}
	printf("\n");
	printf("      siGetSignatureMfgMode(%u, %p, %p)", cI, sV, &sL);
	rc = siGetSignatureMfgMode(cI, sV, &sL);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		printf("; signature == %s", Common::Sig2Str(sV, sL));
		if (/*match?*/ (sL == sN) && !memcmp(sV, sX, sL)) {
			printf(" (match, OK)");
		} else {
			aC++;
			printf(" (should be %s)", Common::Sig2Str(sX, sN));
		}
		printf("\n");
	}

	/* --- Set Signature; fail (OK) because of NULL argument... */
	printf("Set Signature; fail (OK) because of NULL argument\n");
	rX = (Byte)nullArgumentError;
	printf("   siSetSignatureMfgMode(%u, /*NULL*/ (void*)0, %u)", cI, sL);
	rc = siSetSignatureMfgMode(cI, /*NULL*/ (void*)0, sL);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (nullArgumentError, OK)");
	}
	printf("\n");

	/* --- Set Signature; fail (OK) because of zero (0) length... */
	printf("Set Signature; fail (OK) because of zero (0) length\n");
	rX = (Byte)outOfRange;
	sL = sN = 0;
	printf("   siSetSignatureMfgMode(%u, %s, %u)", cI,
			Common::Sig2Str(MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT), sL);
	rc = siSetSignatureMfgMode(cI, MFGSignatureDEFAULT, sL);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)", rX);
	} else {	// Reset the code to success and report.
		rc = (Byte)success;
		printf(" (outOfRange, OK)");
	}
	printf("\n");

	/* --- Set Signature; set maximum... */
	printf("Set Signature; set maximum\n");
	sL = sN = (Byte)255;
	(void)memset(sV, 0xFF, sizeof(sV));
    (void)memcpy(sX, sV, sL);
	rX = (Byte)success;
	printf("   siSetSignatureMfgMode(%u, %p->{0x%02X, 0x%02X...}, %u)", cI, sV,
			*sV, sV[1], sL);
	rc = siSetSignatureMfgMode(cI, sV, sL);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)", rX);
	} else {
		printf(" (success, OK)");
	}
	printf("\n");
	printf("      siGetSignatureMfgMode(%u, %p, %p)", cI, sV, &sL);
	rc = siGetSignatureMfgMode(cI, sV, &sL);
	printf(" == %u", rc);
	if (/*unexpected result?*/ rc != rX) {
		aC++;
		printf(" (should be %u)\n", rX);
	} else {
		printf(" (success, OK)");
		printf("; signature == {0x%02X, 0x%02X...}", *sV, sV[1]);
		if (/*match?*/ (sL == sN) && !memcmp(sV, sX, sL)) {
			printf(" (match, OK)\n");
		} else {
			aC++;
			printf("\n         ((expected)%s !=\n", Common::Sig2Str(sX, sN));
			printf("          (acquired)%s\n", Common::Sig2Str(sV, sL));
		}
	}

	// --- Reset attributes to their defaults, including terminating power.
	printf("siSetPowerEnableMfgMode...\n");
	setTargetVoltage(SID, 0, 0);
	siSetImpulseCodeMfgMode(SID, MFGImpulseCodeDEFAULT);
	siSetImpulseCountMfgMode(SID, MFGImpulseCountDEFAULT);
	siSetIntercharacterDelayMfgMode(SID, MFGIntercharacterDelayDEFAULT);
	siSetPowerDelayMfgMode(SID, MFGPowerDelayDEFAULT);
	siSetSignatureMfgMode(SID, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);

	/* --- Exercise siSetPowerEnableMfgMode. */
	Word /*5V*/  v5 = 5000;
	Word /*12V*/ v12 = 12000;

	if (argCnt >= 4) {
		v5 = atoi(argStr[3]);
	}
	if (argCnt >= 5) {
		v12 = atoi(argStr[4]);
	}
	siSetUartBaudrate(SID, 115200);
	printf("   siSetPowerEnableMfgMode(%u, %u, %u)", SID, v5, v12);
	rc = siSetPowerEnableMfgMode(SID, v5, v12);
	printf(" == %u (0x%X)", rc, rc);
	if (/*OK?*/ rc == (Byte)success) {
		printf(" (success, OK)\n");
	} else {
		aC++;
		printf(" (should be %u)\n", (Byte)success);
	}

	// --- Tidy by resetting attributes to their defaults, including terminating
	// power.
	setTargetVoltage(SID, 0, 0);
	siSetImpulseCodeMfgMode(SID, MFGImpulseCodeDEFAULT);
	siSetImpulseCountMfgMode(SID, MFGImpulseCountDEFAULT);
	siSetIntercharacterDelayMfgMode(SID, MFGIntercharacterDelayDEFAULT);
	siSetPowerDelayMfgMode(SID, MFGPowerDelayDEFAULT);
	siSetSignatureMfgMode(SID, MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
	goto TestEND;

TestEND:
	setUartPullupVoltage(SID, 0);
	if (/*OK?*/ !aC) {	// Report results
		sprintf(tx, "MFG (\"One-Step\") API Test"
				" - End with no anomalies detected (OK)!");
	} else {
		sprintf(tx, "MFG (\"One-Step\") API Test - End with %d anomalies", aC);
	}
	printBanner(tx);
	if (/*OK?*/ rc == (Byte)success) {
		// --- If it is not being used to report an anomaly, set the result
		// code (rc) to report the number of detected anomalies.
		rc = (aC <= 255)? (Byte)aC : 255;
	}
	return rc;
}

/**
   * <pre>
   * Description:
   *   Drive tests (Echo, read, Write)
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test01(int id, int driveType)
{
    Byte 	rc = 0;
	Word    uartVersion;
	Dword   size = 512, rAddr, wAddr;
	int 	i, errorCnt = 0;

	initTest(id, driveType);

	rAddr = driveInfo[driveType].readAddr;
	wAddr = driveInfo[driveType].writeAddr;

    rc = siGetDriveUartVersion(id, &uartVersion);
	if( rc == 0 )
		terprintf("siGetDriveUartVersion: 0x%x \n",uartVersion);
	else {
	    ++errorCnt;
		terprintf("ERROR: siGetDriveUartVersion failed \n");
	}

	printBanner("Echo Test - Begin");
	rc = siEchoDrive(id, size, Data, WAPI_DRIVE_TIMEOUT_MS);
    if(rc == 0 ) {
		printData("siEchoDrive", Data, size);
    	for (i = 0; i < (int)size; i++) {
			if(isprint(Data[i]))
				terprintf("%c", Data[i]);
			else
				terprintf(".");
    		if( (i % 60) == 0 && i) terprintf("\n");
    	}
		terprintf("\n");
	}
	else {
	    ++errorCnt;
	    terprintf("Error %02x: siEchoDrive failed \n", rc);
	}
	printBanner("Echo Test - End");
	
    printBanner("Read Drive Test - Begin");
	Dword readSize = (driveType == WAPI_DRIVE_TYPE_CBD) ? 16384 : ((sizeof(Data)/sizeof(Byte))-48);
	for(size = 128; size < readSize; size += 128)
	{
	    memset(Data, 0, WAPI_BUFFER_SIZE);
   		rc = siReadDriveMemory(id, rAddr, (Word)size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0) {
			printData("siReadDriveMemory", Data, size);
		}
		else {
	    	++errorCnt;
			terprintf("Error %02x: siReadDriveMemory failed \n", rc);
		}
	}
    printBanner("Read Drive Test - End");

	if(driveInfo[driveType].skipWrite)
		goto test01Done;
			
    printBanner("Write Drive Test - Begin");
	size = 2;
	for(i = 1; i <= 10; ++i)
	{
  		rc = siReadDriveMemory(id, wAddr, size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0)
			terprintf("Before Write: %02x%02x - ", Data[0], Data[1]);
		else {
	    	++errorCnt;
			terprintf("Error %02x: siReadDriveMemory failed \n", rc);
		}
		
		Data[0] = i; Data[1] = i;
		
		rc = siWriteDriveMemory(id, wAddr, 2, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc != 0) {
	    	++errorCnt;
			terprintf("Error writing from drive \n");
		}
		
  		rc = siReadDriveMemory(id, wAddr, size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0)
			terprintf("After Write: %02x%02x \n", Data[0], Data[1]);
		else {
	    	++errorCnt;
			terprintf("Error %02x: siReadDriveMemory failed \n", rc);
		}
	}
    printBanner("Write Drive Test - End");

test01Done:	
	rc = finishTest(id, driveType);
	printf("Slot %d, errorCnt = %d \n", id, errorCnt);

	return rc;
}

/**
   * <pre>
   * Description:
   *   Drive tests (Echo, read, Write)
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test02(int id, int driveType)
{
    Byte 	rc = 0;
	Dword   retVal;
	Word    i;

	initTest(id, driveType);

//testBaudRates:		
	printBanner("UART Baud Rate Test - Begin");
	for(i=0; i < sizeof(baudRates)/sizeof(Dword); ++i)
	{
	   rc = siSetUartBaudrate(id,baudRates[i]);
	   sleep(1);
	   if(rc == 0) {
    	   rc = siGetUartBaudrate(id, &retVal);
    	   printf("siGetUartBaudrate: %d \n", (int)retVal);
	   }
	   else
		   printf("siGetUartBaudrate: Error\n");
	}

	printBanner("UART Baud Rate Test - End");

	rc = finishTest(id, driveType);
	
	return rc;
}

/**
   * <pre>
   * Description:
   *   Test voltage levels
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test03(int id, int driveType)
{
    Byte 	rc = 0;
	Word    targetVolatge[] = {4500,4600,4700,4800,4900,5000};
	Word   	retVal, retVal1;
	Word    i;

	initTest(id, driveType);

//testTargetVolatge:		
	printBanner("Target Voltage Test - Begin");
	for(i=0; i < sizeof(targetVolatge)/sizeof(Word); ++i)
	{
	   printf("setTargetVoltage:%d mV, ", targetVolatge[i]);
	   rc = setTargetVoltage(id, targetVolatge[i],0);
	   
	   sleep(10);
	   
	   rc = getTargetVoltage(id, &retVal, &retVal1);
       if(rc == 0)
           printf("getTargetVoltage: %d mV, ", retVal);
       else
           printf("getTargetVoltage: Failed, " ); 
		    
	   rc = getCurrentVoltage(id, &retVal, &retVal1);
       if(rc == 0)
           printf("getCurrentVoltage: %d mV ", retVal);
       else
           printf("getCurrentVoltage: Failed " ); 
		   
	   rc = getVoltageErrorStatus(id, &retVal1); 
       if(rc == 0)
           printf("getVoltageErrorStatus: %d \n", retVal1);
       else
           printf("getVoltageErrorStatus: Failed \n" ); 
	}
	printBanner("Target Voltage Test - End");

	rc = finishTest(id, driveType);
	
	return rc;
}

/**
   * <pre>
   * Description:
   *   Temp control test
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test04(int id, int driveType)
{
    Byte 	rc = 0;
	Word    targetTemp, posRampRate, negRampRate, currentTemp, safeTemp, tempEnvelope;
	Word    i;

	rc = getCurrentTemperature(id, &currentTemp);
	printf("Current Temperature (in hundreds) = %d \n\n", currentTemp);

	printf("Enter Target Temperature in deg C -> ");
	scanf("%hd", &targetTemp);

	printf("Enter Positive Ramp Rate in deg C/min -> ");
	scanf("%hd", &posRampRate);

	printf("Enter Negative Ramp Rate in deg C/min -> ");
	scanf("%hd", &negRampRate);

	printf("Enter Temperature Envelope in deg C/min -> ");
	scanf("%hd", &tempEnvelope);
	
	rc = setTargetTemperature(id, targetTemp*100);
	rc = setPositiveTemperatureRampRate(id, posRampRate*100);
	rc = setNegativeTemperatureRampRate(id, negRampRate*100);
	rc = setTemperatureEnvelope(id, tempEnvelope*10);
	
	printf("\n\n");
	rc = getTargetTemperature(id, &targetTemp);
	printf("getTargetTemperature (in hundreds) -> %d\n", targetTemp);
	
	rc = getPositiveTemperatureRampRate(id, &posRampRate);
	printf("getPositiveTemperatureRampRate (in hundreds) -> %d\n", posRampRate);

	rc = getNegativeTemperatureRampRate(id, &negRampRate);
	printf("getNegativeTemperatureRampRate (in hundreds)-> %d\n", negRampRate);
	
	rc = getTemperatureEnvelope(id, &tempEnvelope);
	printf("getTemperatureEnvelope (in tens)-> %d\n\n", tempEnvelope);

	for(i=0; i<25; ++i)
	{
        printTemperatureVariables(id);
		rc = isOnTemp(id);
		if(rc) 
			break;
		sleep(30);
		
	}

	printf("\n\nTarget temperature reached. Wait 10 minutes. Make sure the temperature stays within the target range\n");
	
	for(i=0; i<20; ++i)
	{
        printTemperatureVariables(id);
		sleep(30);
	}
	

	printf("Enter Safe Handling Temperature in deg C -> ");
	scanf("%hd", &safeTemp);
	printf("setSafeHandlingTemperature -> %d deg C\n", safeTemp);
	rc = setSafeHandlingTemperature(id, safeTemp*100);

	for(i=0; i<25; ++i)
	{
        printTemperatureVariables(id);
		sleep(30);
	}
	
	return rc;
}


/**
   * <pre>
   * Description:
   *   misc tests
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test05(int id, int driveType)
{
    Byte 	rc = 0;
	Word    currentLimit5V, currentLimit12V;
    Word 	v5LowerLimit, v12LowerLimit, v5UpperLimit, v12UpperLimit, uartPullup;
    Word 	tempLowerLimit, tempUpperLimit,tempSensorLowerLimit, tempSensorUpperLimit;
	
	printf("setCurrentLimit: 3000 0 \n");
	rc = setCurrentLimit(id, 3000, 0);
	if(rc)
		printf("setCurrentLimit: Error \n");
		
	printf("setCurrentLimit: 4000 0 \n");
	rc = setCurrentLimit(id, 4000, 0);
	if(rc)
		printf("setCurrentLimit: Error (This error is expected) \n");
	rc = getCurrentLimit(id, &currentLimit5V, &currentLimit12V);
	printf("getCurrentLimit %d %d \n", currentLimit5V, currentLimit12V);

	printf("setVoltageLimit: 4500, 0, 5500, 0  \n");
	rc = setVoltageLimit(id, 4500, 0, 5500, 0);
	if(rc)
		printf("setVoltageLimit: Error \n");
	printf("setVoltageLimit: 4800, 0, 5200, 0 \n");
	rc = setVoltageLimit(id, 4800, 0, 5200, 0);
	if(rc)
		printf("setVoltageLimit: Error \n");
	printf("setVoltageLimit: 4499, 0, 5501, 0 \n");
	rc = setVoltageLimit(id, 4499, 0, 5501, 0);
	if(rc)
		printf("setVoltageLimit: Error (This error is expected)\n");
	rc = getVoltageLimit(id, &v5LowerLimit, &v12LowerLimit, &v5UpperLimit, &v12UpperLimit);
	printf("getVoltageLimit: %d %d %d %d \n", v5LowerLimit, v12LowerLimit, v5UpperLimit, v12UpperLimit);
	

	printf("setUartPullupVoltage: 2700 mV \n");
	rc = setUartPullupVoltage(id,2700);
	if(rc)
		printf("setUartPullupVoltage: Error \n");
	rc = getUartPullupVoltage(id, &uartPullup);
	if(rc)
		printf("getUartPullupVoltage: Error \n");
	else
		printf("getUartPullupVoltage: %d mV \n", uartPullup);
		

    rc = setTemperatureLimit(id, 3000, 6000, 3000, 6000);
	printf("setTemperatureLimit: 3000, 6000, 3000, 6000 \n");
    rc = getTemperatureLimit(id, &tempLowerLimit, &tempUpperLimit, &tempSensorLowerLimit, &tempSensorUpperLimit);
	printf("getTemperatureLimit: %d %d %d %d \n", tempLowerLimit, tempUpperLimit,tempSensorLowerLimit, tempSensorUpperLimit);

    printf("siSetSupplyOverVoltageProtectionLevel: 5000, 0 \n");
	rc = siSetSupplyOverVoltageProtectionLevel(id, 5000, 0);
    printf("siSetSupplyOverVoltageProtectionLevel: 5001, 0 \n");
	rc = siSetSupplyOverVoltageProtectionLevel(id, 5001, 0);
	if(rc)
		printf("siSetSupplyOverVoltageProtectionLevel: Error (This error is expected) \n");

	return rc;
}

static Byte test06(int id, int driveType)
{

    Byte 	rc = 0;
	Word    size;
	int     i, value=0;


	initTest(id, driveType);

	printBanner("Bulk Send/Receive - Begin");
	initTest(id, driveType);

	printf("Enter the number of bytes to send: ");
	scanf("%d", &value); size = value;
	
	if(size) {
		for(i = 0; i < size; ++i) {
			printf("Byte%02d: ", i);
			scanf("%x", &value);
			Data[i] = value&0xFF;
		}
		rc = siBulkWriteToDrive(id, size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc != 0) {
    		printf("Error: siBulkWriteToDrive \n");
		}

	}
    rc = siBulkReadFromDrive(id, &size, Data, WAPI_DRIVE_TIMEOUT_MS);
    if(rc == 0)
    {
    	printf("Read %d bytes from drive: ", size);
    	for (i = 0; i< size; i++) {
    		printf("%02x ", Data[i]);
    		if( (i % 40) == 0 && i) printf("\n");
    	}
    }
	printf("\n");
	printBanner("Bulk Send/Receive - End");

	rc = finishTest(id, driveType);
	return rc;
}

/**
   * <pre>
   * Description:
   *   Profile
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test07(int id, int driveType)
{
    Byte 	rc = 0;
	int 	i, errorCnt = 0;
	Dword   size = 64000, rAddr;

	if(driveType > WAPI_DRIVE_TYPE_EB7 )
		return 1;

	initTest(id, driveType);

	rAddr = driveInfo[driveType].readAddr;
	
	WAPI_PROFILE_START;
	
	for(i = 0; i < 100; ++i)
	{
   		rc = siReadDriveMemory(id, rAddr, size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc != 0)
			++errorCnt;
	}

	WAPI_PROFILE_STOP;
	printf(" Slot %d, errorCnt = %d, loopCnt = %d \n", id, errorCnt, i);

	rc = finishTest(id, driveType);
	return rc;
}

/**
   * <pre>
   * Description:
   *   Drive tests (1000 times Echo)
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test08(int id, int driveType)
{
    Byte 	rc = 0;
	Dword   size = 512;
	int 	i, errorCnt = 0;
	int     loopcount=1000;


	initTest(id, driveType);

	WAPI_PROFILE_START;
	for(i=0; i<50; ++i) {
	   rc = siEchoDrive(id, size, Data, WAPI_DRIVE_TIMEOUT_MS);
       if(rc) {
	       ++errorCnt;
	       terprintf("X");
		   // return rc;
	   }
	   else {
	       terprintf(".");
	   }
	}
	WAPI_PROFILE_STOP;
	
	initTest(id, driveType);

	WAPI_PROFILE_START;
	for(i=0; i<50; ++i) {
	   rc = siEchoDrive(id, size, Data, WAPI_DRIVE_TIMEOUT_MS);
       if(rc) {
	       ++errorCnt;
	       terprintf("X");
		   // return rc;
	   }
	   else {
	       terprintf(".");
	   }
	}
	WAPI_PROFILE_STOP;


	rc = finishTest(id, driveType);

	printf("\nSlot %d, %d error(s) out of %d \n", id, errorCnt, loopcount);

	return rc;
}

/**
   * <pre>
   * Description:
   *   Simulate short/long duration test
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test09(int id, int driveType, int tgt1, int tgt2, int stemp, int ramp, int testTime)
{
    Byte 	rc = 0, isDrivePresent;
	Dword   size = 512;
	int 	i, errorCnt = 0;
	Word    currentTemp;
	int     loopcount=WAPI_TEST09_ECHO_COUNT;
    struct  timeval tv,tv_tmp,tv_end;
  	char 	logFileName[100];
	static 	FILE *f = NULL;
	
	
	if(ramp == 0)
		ramp = WAPI_TEST09_POS_RAMP;
	if(tgt1 == 0)
		tgt1 = WAPI_TEST09_TARGET_TEMP1;
	if(tgt2 == 0)
		tgt2 = WAPI_TEST09_TARGET_TEMP2;
	if(stemp == 0)
		stemp = WAPI_TEST09_SAFE_TEMP;
	if(testTime == 0)
		testTime = WAPI_TEST09_IDLE_WAIT_SEC;

	initTest(id, driveType);
	isDrivePresent = isDrivePlugged(id);
	

#if WAPI_TEST09_VOLTAGE_CHECK	
	Word    currentTemp, wV5InMilliVolts, wV12InMilliVolts; 

	rc = getCurrentVoltage(id, &wV5InMilliVolts, &wV12InMilliVolts);
	if(rc || (wV5InMilliVolts < 1000) || (wV12InMilliVolts < 1000) ) {  /* Let us 1000mV for comparison instead of zero */
	   rc = 1; goto test09_exit;
	}
#endif


    sprintf(logFileName, "/var/tmp/htemp%d.csv", id);
	f = fopen(logFileName, "a+b");
	fprintf(f, "time, htemp, currentTemp, heaterOut, shutterPos, targetTemp, isTempReached \n");

	rc = setPositiveTemperatureRampRate(id, ramp);
	rc = setNegativeTemperatureRampRate(id, ramp);

	/* 1) Ramp up to target temp1 */
	terprintf(" 1 - Ramp to target %d \n", tgt1);
	rc = setTargetTemperature(id, tgt1);
	while(1)
	{
        printTemperatureVariables(id);
		rc = isOnTemp(id);
		if(rc) 
			break;
		sleep(15);  /* 15 sec sleep */
		
		if( isDrivePresent ) adjhtemp(id, f);
	}

	/* 2) Do some echos or wait at the target temperature */
	if(loopcount && isDrivePresent) {
		terprintf("\nStarting %d Echo test \n", loopcount);

		for(i=0; i<loopcount; ++i) {
		   rc = siEchoDrive(id, size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	   if(rc) {
	    	   ++errorCnt;
		   }
		   printTemperatureVariables(id);
		}

		terprintf("Slot %d, %d error(s) out of %d echo commands.\n", id, errorCnt, loopcount);
	}
	else {  
		/* 2.1) wait at target temp 1 */
		terprintf(" 2.1 - wait at target for %d  secs \n", testTime);

        memset(&tv,0,sizeof(struct timeval));
        tv.tv_sec = testTime;

        gettimeofday(&tv_tmp,NULL);
        timeradd(&tv,&tv_tmp,&tv_end);

		while(1) {
		   	printTemperatureVariables(id);
			
			sleep(120);  /* 120 sec sleep. Hitachi calls htemp correction once in 2 minutes */
			
			gettimeofday(&tv,NULL);
    		if ( timercmp(&tv, &tv_end, >) ) {
				break;
			}
			
			if( isDrivePresent ) adjhtemp(id, f);
		}


		/* 2.2) Go to temp 2 */
		terprintf(" 1 - Ramp to target %d \n", tgt2);
		rc = setTargetTemperature(id, tgt2);
		while(1)
		{
        	printTemperatureVariables(id);
			rc = isOnTemp(id);
			if(rc) 
				break;
			sleep(15);  /* 15 sec sleep */

			if( isDrivePresent ) adjhtemp(id, f);
		}


		/* 2.2) wait target temp 2 */
 		terprintf(" 2.2 - wait at target for %d secs \n", testTime);

        memset(&tv,0,sizeof(struct timeval));
        tv.tv_sec = testTime;

        gettimeofday(&tv_tmp,NULL);
        timeradd(&tv,&tv_tmp,&tv_end);

		while(1) {
		   	printTemperatureVariables(id);
			
			sleep(120);  /* 120 sec sleep. Hitachi calls htemp correction once in 2 minutes */
			
			gettimeofday(&tv,NULL);
    		if ( timercmp(&tv, &tv_end, >) ) {
				break;
			}
			
			if( isDrivePresent ) adjhtemp(id, f);
		}

	}

	/* 3) Go to safe handling temp */
	terprintf(" 3 - Go to safe handling temp %d \n", stemp);
	rc = setSafeHandlingTemperature(id, stemp);

	while(1)
	{
        printTemperatureVariables(id);
		rc = getCurrentTemperature(id, &currentTemp);
		if(currentTemp <= stemp) {
			break;
		}
		sleep(5);

		if( isDrivePresent ) adjhtemp(id, f);
	}


	fclose(f);
	terprintf("Slot %d, Test09 completed \n", id);
	rc = finishTest(id, driveType);

	return rc;
}

/**
   * <pre>
   * Description:
   *   Temp envelope test
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
static Byte test10(int id, int driveType)
{
    Byte 	rc = 0;
	int     i;

	initTest(id, driveType);
	
	sleep(60);
	
	for(i=0; i < 100; ++i)
	{
		printf("htemp = %d \n", gethtemp(id));
		sleep(1);
	}

	rc = finishTest(id, driveType);
 
	return rc;
}

//This is the test to run all API's to complete
static Byte test25(int id, int driveType)
{
    Byte 	rc = 0;
    Word 	retVal, retVal1, retVal2, retVal3;
	Dword 	retval3;
    int  	i;

    rc = siSetInterCharacterDelay(id, 540);
    if(rc == 0)
    	printf("siSetInterCharacterDelay: Success, Set to %d \n", 540);
    else
    	printf("siSetInterCharacterDelay: Failed \n" );

    rc = siSetAckTimeout(id, 3);
	if(rc == 0)
		printf("siSetAckTimeout: Success, Set to %d \n", 3);
	else
		printf("siSetAckTimeout: Failed \n" );

    rc = siSetUartBaudrate(id, 1843200);
	if(rc == 0)
		printf("siSetUartBaudrate: Success, Set to %d \n", 1843200);
	else
		printf("siSetUartBaudrate: Failed \n" );

	rc = setTargetVoltage(id, 5000, 12000);
	if(rc == 0)
		printf("setTargetVoltage: Success, Set to %d and %d \n", 5000, 12000);
	else
		printf("setTargetVoltage: Failed \n" );

    rc = siSetUartProtocol(id, 0);
	if(rc == 0)
		printf("siSetUartProtocol: Success, Set to %d \n", 0);
	else
		printf("siSetUartProtocol: Failed \n" );

	rc = setTargetTemperature(id, 6000);
	if(rc == 0)
		printf("setTargetTemperature: Success, Set to %d \n", 6000);
	else
		printf("setTargetTemperature: Failed \n" );

	rc = getCurrentTemperature(id, &retVal);
	if(rc == 0)
		printf("getCurrentTemperature: %d \n", retVal);
	else
		printf("getCurrentTemperature: Failed \n" );

	rc = setPositiveTemperatureRampRate(id, 100);
	if(rc == 0)
		printf("setPositiveTemperatureRampRate: %d \n", 100);
	else
		printf("setPositiveTemperatureRampRate: Failed \n" );

	rc = getPositiveTemperatureRampRate(id, &retVal);
	if(rc == 0)
		printf("getPositiveTemperatureRampRate: Success \n");
	else
		printf("getPositiveTemperatureRampRate: Failed \n");

	rc = setNegativeTemperatureRampRate(id, 100);
	if(rc == 0)
		printf("setNegativeTemperatureRampRate: Success \n");
	else
		printf("setNegativeTemperatureRampRate: Failed \n" );

	rc = getNegativeTemperatureRampRate(id, &retVal);
	if(rc == 0)
		printf("getNegativeTemperatureRampRate: Success \n");
	else
		printf("getNegativeTemperatureRampRate: Failed \n" );

	rc = getHeaterOutput(id, &retVal);
	if(rc == 0)
		printf("getHeaterOutput: Success \n");
	else
		printf("getHeaterOutput: Failed \n" );

	rc = getShutterPosition(id, &retVal);
	if(rc == 0)
		printf("getShutterPosition: Success \n");
	else
		printf("getShutterPosition: Failed \n" );

	rc = getTemperatureErrorStatus(id, &retVal);
	if(rc == 0)
		printf("getTemperatureErrorStatus: rc=%d value=%x \n", rc, retVal);
	else
		printf("getTemperatureErrorStatus: Failed \n" );

	rc = getCurrentVoltage(id, &retVal, &retVal1);
	if(rc == 0)
		printf("getCurrentVoltage: 5V=%d mV, 12V=%d mV \n", retVal, retVal1);
	else
	    printf("getCurrentVoltage: Failed \n" );

	rc = getActualCurrent(id, &retVal, &retVal1);
	if(rc == 0)
	    printf("getActualCurrent: 5V=%d mA, 12V=%d mA\n", retVal, retVal1);
	else
	    printf("getActualCurrent: Failed \n" );

	rc = getTargetVoltage(id, &retVal, &retVal1);
	if(rc == 0)
		printf("getTargetVoltage: 5V=%d 12V=%d \n", retVal, retVal1);
	else
		printf("getTargetVoltage: Failed \n" );

	rc = setUartPullupVoltage(id, 2700);
	if(rc == 0)
	    printf("setUartPullupVoltage: Success, Set to %d mV \n", 25);
	else
	    printf("setUartPullupVoltage: Failed \n" );

	rc = isDrivePlugged(id);
	if(rc == 0)
		printf("isDrivePlugged: %s \n", rc ? "Yes":"No");
	else
		printf("isDrivePlugged: Failed \n");

	rc = initTest(id, driveType);
	WAPI_PROFILE_START;
	rc = siReadDriveMemory(id, (Dword)0x08331000, (Word)128, Data, WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0)
	{
		WAPI_PROFILE_STOP;
	    printf("Slot %d: siReadDriveMemory, Read %d bytes from Address 0x%08x \n", id, 0x08331000, 128);
		if(verbose) {
	    	for (i = 0; i< 128; i++) {
	    		printf("%02x ", Data[i]);
	    		if( (i % 40) == 0 && i) printf("\n");
	    	}
		}
	}
	else
		printf("siReadDriveMemeory: Failed \n");
	rc = finishTest(id, driveType);

	rc = siGetDriveUartVersion(id, &retVal);
	if(rc == 0)
		printf("siGetDriveUartVersion: %d \n", retVal);
	else
		printf("siGetDriveUartVersion: Failed \n");

	rc = siGetUartBaudrate(id, &retval3);
	if(rc == 0)
		printf("siGetUartBaudrate: Success \n");
	else
		printf("siGetUartBaudrate: Failed \n");

	rc = siSetUartBaudrate(id,1843200);
	if(rc == 0) {
	    rc = siGetUartBaudrate(id, &retval3);
	    printf("siGetUartBaudrate: %d \n", (int)retval3);
	}
	else
		printf("Valid baud rates: 115200,460800,921600,1843200,2778000,3333000,4167000,5556000,8333000 \n");

	rc = getUartPullupVoltage(id, &retVal);
	if(rc == 0)
		printf("getUartPullupVoltage: %d mV \n", retVal);
	else
	    printf("getUartPullupVoltage: Failed \n" );


	rc = initTest(id, driveType);
	rc = siEchoDrive(id, 512, Data, WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0)
	{
		printf("siEchoDrive: %d bytes were received in the response \n", 25);
	    for (i = 0; i< 512; i++) {
			if(25) {
				if(isprint(Data[i]))
					terprintf("%c", Data[i]);
				else
					terprintf(".");
			}
			else
	    		terprintf("%02x ", Data[i]);
	    		if( (i % 40) == 0 && i) terprintf("\n");
	    }
	}
	else
		printf("Error %02x: siEchoDrive failed \n", rc);
	rc = finishTest(id, driveType);
	terprintf("\n");

	rc = isCellEnvironmentError(id);
	if(rc == 0)
	    printf("isCellEnvironmentError: No \n");
	else
	    printf("isCellEnvironmentError: Yes, Error=%d \n", rc);

	rc = clearCellEnvironmentError(id);
	if(rc == 0)
	    printf("clearCellEnvironmentError: Success \n" );
	else
	    printf("clearCellEnvironmentError: Failed \n" );

	rc = setSafeHandlingTemperature(id,3500);
	if(rc == 0)
		printf("setSafeHandlingTemperature: Success \n" );
	else
		printf("setSafeHandlingTemperature: Failed \n" );


	rc = siWriteDriveMemory(id, (Dword)0x08331000, (Word)128, Data, WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0)
		printf("siWriteDriveMemory: Success \n");
	else
		printf("siWriteDriveMemory: Failed \n");

	rc = siSetDriveFanRPM(id, (Byte)25);
	if(rc == 0) {
		printf("siSetDriveFanRPM: Success \n");
	}
	else {
	    printf("siSetDriveFanRPM: Failed \n" );
	}

	rc = isSlotThere(id);
	if(rc == 0)
		printf("isSlotThere: Yes \n");
	else
		printf("isSlotThere: No \n");

	rc = siGetInterCharacterDelay ( id, &retval3 );
	if(rc == 0)
		printf("siGetInterCharacterDelay: %lx \n", retval3);
	else
		printf("siGetInterCharacterDelay: Failed \n");

	rc = siSetInterPacketDelay ( id, WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0)
		printf("siSetInterPacketDelay: Success \n");
	else
		printf("siSetInterPacketDelay: Failed \n");

	rc = siSetDriveDelayTime(id, WAPI_DRIVE_TIMEOUT_MS, WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0)
		printf("siSetDriveDelayTime: Success \n");
	else
		printf("siSetDriveDelayTime: Failed \n");

	rc = siSetDebugLogLevel(id, 1);
	if(rc == 0)
		printf("siEnableLogLevel: Success \n");
	else
		printf("siEnableLogLevel: Failed \n");

	rc = siClearHaltOnError(id);
	if(rc == 0)
		printf("siClearHaltOnError: Success \n");
	else
		printf("siClearHaltOnError: Failed \n");

	rc = setTemperatureEnvelope(id, 60);
	if(rc == 0)
	    printf("setTemperatureEnvelope: Success, Set to %d \n", 60);
	else
	    printf("setTemperatureEnvelope: Failed \n" );

	rc = getTemperatureEnvelope(id, &retVal);
	if(rc == 0)
		printf("getTemperatureEnvelope: %d \n", retVal);
	else
		printf("getTemperatureEnvelope: Failed \n");

	rc = setVoltageRiseTime(id, 50, 50);
	if(rc == 0)
	    printf("setVoltageRiseTime: 5V=%d, 12V=%d \n", 50, 50);
	else
	    printf("setVoltageRiseTime: Failed \n" );

	rc = getVoltageRiseTime(id, &retVal, &retVal1);
	if(rc == 0)
	    printf("setVoltageRiseTime: 5V=%d, 12V=%d \n", retVal, retVal1);
	else
	    printf("setVoltageRiseTime: Failed \n" );

	rc = setVoltageFallTime(id, 50, 50);
	if(rc == 0)
	    printf("setVoltageFallTime: 5V=%d, 12V=%d \n", 50, 50);
	else
	    printf("setVoltageFallTime: Failed \n" );

	rc = getVoltageFallTime(id, &retVal, &retVal1);
	if(rc == 0)
	    printf("getVoltageFallTime: 5V=%d, 12V=%d \n", retVal, retVal1);
	else
	    printf("getVoltageFallTime: Failed \n" );

	rc = getVoltageErrorStatus(id, &retVal);
	if(rc == 0)
		printf("getVoltageErrorStatus: rc=%d, value=%x \n", rc, retVal);
	else
		printf("getVoltageErrorStatus: Failed \n");

	rc = setVoltageInterval(id, 10);
	if(rc == 0)
		printf("setVoltageInterval: Success");
	else
		printf("setVoltageInterval: Failed \n");

	rc = adjustTemperatureControlByDriveTemperature(id, 3500);
	if(rc == 0)
		printf("adjustTemperatureControlByDriveTemperature: Success \n");
	else
		printf("adjustTemperatureControlByDriveTemperature: Failed \n");

	//rc = siSetSupplyOverVoltageProtectionLevel(id, WAPI_MOBILE_VOLT_STD,WAPI_ENTERPRISE_VOLT_STD);
	//if(rc == 0)
	//	printf("siSetSupplyOverVoltageProtectionLevel: Success \n");
	//else
	//	printf("siSetSupplyOverVoltageProtectionLevel: Failed \n");

	rc = siSetFanRPMDefault(id);
	if(rc == 0)
		printf("siSetFanRPMDefault: Success \n");
	else
		printf("siSetFanRPMDefault: Failed \n");

	rc = siSetElectronicsFanRPM(id, 25);
	if(rc == 0)
		printf("siSetElectronicsFanRPM: Success \n");
	else
		printf("siSetElectronicFanRPM: Failed \n");

	Dword tempDword;
	rc = siGetAckTimeout(id, &tempDword);
	if(rc == 0)
		printf("siGetAckTimeout: Success \n");
	else
		printf("siGetAckTimeout: Failed \n");

	Byte tempByte = 12;
	rc = siBulkWriteToDrive(id, 2, &tempByte , WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0)
		printf("siBulkWriteToDrive: Success \n");
	else
		printf("siBulkWriteToDrive: Failed \n");

	rc = siBulkReadFromDrive(id, &retVal, &tempByte, 2);
	if(rc == 0)
		printf("siBulkReadFromDrive: Success \n");
	else
		printf("siBulkReadFromDrive: Failed \n");

	rc = setVoltageLimit(id, 4500, 11500, 5500, 12700);
	if(rc == 0)
		printf("setVoltageLimit: Success \n");
	else
		printf("setVoltageLimit: Failed \n");

	//This currently is not supported so we wont actually use it
	/*rc = setVoltageCalibration(id, 5, 5, 12, 12);
	if(rc == 0)
		printf("setVoltageCalibration: Success \n");
	else
		printf("setVoltageCalibration: Failed \n");
*/
	rc = setUartPullupVoltageDefault(id);
	if(rc == 0)
		printf("setUartPullupVoltageDefault: Success \n");
	else
		printf("setUartPullupVoltageDefault: Failed \n");

	//This currently doesn't seem to be implemented so it's commented out
	/*rc = setTemperatureSensorCalibrationData(id, 25);
	if(rc == 0)
		printf("setTemperatureSensorCalibrationData: Success \n");
	else
		printf("setTemperatureSensorCalibrationData: Failed \n");
*/
	rc = setTemperatureLimit(id, 3000,6000,3000,6000);
	if(rc == 0)
		printf("setTemperatureLimit: Success \n");
	else
		printf("setTemperatureLimit: Failed \n");

	//This is currently not supported so it is commented out
	/*rc = setShutterPosition(id, 25);
	if(rc == 0)
		printf("setShutterPosition: Success \n");
	else
		printf("setShutterPosition: Failed \n");*/

	//This is currently not supported so it is commented out
	/*rc = setHeaterOutput(id, 25);
	if(rc == 0)
		printf("setHeaterOutput: Success \n");
	else
		printf("setHeaterOutput: Failed \n");*/

	rc = setCurrentLimit(id, 3000, 3000);
	if(rc == 0)
		printf("setCurrentLimit: Success \n");
	else
		printf("setCurrentLimit: Failed \n");

	rc = isOnTemp(id);
	if(rc == 0)
		printf("isOnTemp: Success \n");
	else
		printf("IsOnTemp: Failed \n");

	rc = getVoltageLimit(id, &retVal, &retVal1, &retVal2, &retVal3);
	if(rc == 0)
		printf("getVoltageLimit: Success \n");
	else
		printf("getVoltageLimit: Failed \n");

	rc = getVoltageInterval(id, &retVal);
	if(rc == 0)
		printf("getVoltageInterval: Success \n");
	else
		printf("getVoltageInterval: Failed \n");

	rc = getTemperatureLimit(id, &retVal, &retVal1, &retVal2, &retVal3);
	if(rc == 0)
		printf("getTemperatureLimit: Success \n");
	else
		printf("getTemperatureLimit: Failed \n");

	rc = getTargetTemperature(id, &retVal);
	if(rc == 0)
		printf("getTargetTemperature: Success \n");
	else
		printf("getTargetTemperature: Failed \n");

	rc = getSafeHandlingTemperature(id, &retVal);
	if(rc == 0)
		printf("getSafeHandlingTemperature: Success \n");
	else
		printf("getSafeHandlingTemperature: Failed \n");

	rc = getReferenceTemperature(id, &retVal);
	if(rc == 0)
		printf("getReferenceTemperature: Success \n");
	else
		printf("getReferenceTemperature: Failed \n");

	rc = getCurrentLimit(id, &retVal, &retVal1);
	if(rc == 0)
		printf("getCurrentLimit: Success \n");
	else
		printf("getCurrentLimit: Failed \n");

	CELL_CARD_INVENTORY_BLOCK *temp;
	rc = getCellInventory(id, &temp);
	if(rc == 0)
		printf("getCellInventory: Success \n");
	else
		printf("getCellInventory: Failed \n");

	rc = siChangeToUart3(id, 0x0000, WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0)
		printf("siChangeToUart3: Success \n");
	else
		printf("siChangeToUart3: Failed \n");

	rc = siResetDrive(id, 0x00, 25, 25);
	if(rc == 0)
		printf("siResetDrive: Success \n");
	else
		printf("siResetDrive: Failed \n");

	return rc;
}

/**
   * <pre>
   * Description:
   *  
   * Arguments:
   *   ----
   * Return:
   *   ----
   * Note:
   *   ----
   * </pre>
   *****************************************************************************/
#define  WAPI_TEST98_READ_SIZE  (2048*5)+2
static Byte Data1[WAPI_BUFFER_SIZE];

static Byte test98(int id, int driveType)
{
    Byte 	rc = 0;
	Word    size, totalSize= WAPI_TEST98_READ_SIZE;
	Dword   rAddr;
	Byte    *ptrData;


	initTest(id, driveType);
	
	memset(Data, 0, WAPI_BUFFER_SIZE);
	rAddr = driveInfo[driveType].readAddr;
	ptrData = Data;
	
	while(totalSize > 0) {
		if(totalSize > 2048) {
		    size = 2048;
			totalSize -= size;
		}
		else {
			size = totalSize;
			totalSize = 0;
		}
		
		rc = siReadDriveMemory(id, rAddr, size, ptrData, WAPI_DRIVE_TIMEOUT_MS);
		if(rc == 0) {
   			terprintf("Slot %d: siReadDriveMemory, Read %d bytes from Address 0x%08x \n", id, size, rAddr);
			rAddr += (size/2);
			ptrData += size;
		}
		else {
	    	terprintf("Slot %d: siReadDriveMemory,failed \n");
 		}
	}
	
	sleep(1);
	
	/* Read using 64K FIFO */
	memset(Data1, 0, WAPI_BUFFER_SIZE);
	rAddr = driveInfo[driveType].readAddr;
	totalSize = WAPI_TEST98_READ_SIZE;
	
	rc = siReadDriveMemory(id, rAddr, totalSize, Data1, WAPI_DRIVE_TIMEOUT_MS);
	if(rc == 0) {
   		terprintf("Slot %d: siReadDriveMemory, Read %d bytes from Address 0x%08x \n", id, totalSize, rAddr);
	}
	else {
		terprintf("Slot %d: siReadDriveMemory,failed \n");
 	}
	
	rc = finishTest(id, driveType);
	
	for(int i=0; i<totalSize; ++i) {
		if(Data[i] != Data1[i]) {
			terprintf("Slot %d: Reads did not match \n");
		}	
	}
	

	return rc;
}


static Byte test99(int id, int driveType)
{

    Byte 	rc = 0;
	Word    size = 0;
	int     i, errorCnt = 0, loopCount = 5;
	Byte    echArray[] = {0xaa,0x55,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x10,0xff,0xf0}; 
//	Byte    echArray[] = {0xaa,0x55,0x02,0x02,0x00,0x04,0xff,0xfc}; 

	printBanner("Bulk Send/Receive - Begin");
	initTest(id, driveType);

    printf("Press Control-C to exit \n");
 
	while(--loopCount) {
		size = sizeof(echArray)/sizeof(Byte);
		for(i = 0; i < size; ++i) {
			rc = siBulkWriteToDrive(id, 1, &echArray[i], WAPI_DRIVE_TIMEOUT_MS);
    		if(rc != 0) {
    			printf("Error: siBulkWriteToDrive \n");
			}
			usleep(15000); /* At lease 12 ms */
		}
        
		memset(Data, 0, WAPI_BUFFER_SIZE);
		size = 100;
    	rc = siBulkReadFromDrive(id, &size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0)
    	{
    		printf("\nRead %d bytes: ", size);
    		for (i = 0; i < size; i++) {
    			printf("%02x", Data[i]);
    		}
    	}
		else {
			++errorCnt;
			break;
		}
	}

	rc = finishTest(id, driveType);
	printf("\nBulk Send/Receive - End %d errors \n", errorCnt);

	return rc;
}

/**************************************************************************
 **  Test functions - End
 **************************************************************************/

static Byte testApi(char *cmd, Byte id, int *p)
{
    Byte 	rc = 0;
    Word 	retVal = 0, retVal1 = 0, retVal2 = 0, retVal4 = 0;
	Dword 	retval3 = 0;
    int  	i;

    memset(Data, 0, WAPI_BUFFER_SIZE);
	
    if (ISCMD('g','c','t')) {
    	rc = getCurrentTemperature(id, &retVal);
    	if(rc == 0)
    		printf("getCurrentTemperature: %d \n", retVal);
    	else
    		printf("getCurrentTemperature: Failed \n" );				
    }
    else if (ISCMD('g','e','c'))
    {

    }
    else if (ISCMD('e','v','t')) {
        	UartEventTrace eventTrace[MAX_TRANSACTION_TRACE];
        	printf ("Last %d events\n", MAX_TRANSACTION_TRACE);
        	if ( (rc = getEventTrace(id, &(eventTrace[0]))) == 0 )
    			for (int ii = 0; ii < MAX_TRANSACTION_TRACE; ii++) {
    				long int seconds = 0;
    				long int microsecs = 0;
    				// calculate elapsed time since last transaction
    				if (ii) {
    					seconds = eventTrace[ii].tv_start_sec - eventTrace[ii-1].tv_end_sec;
    					microsecs = eventTrace[ii].tv_start_usec - eventTrace[ii-1].tv_end_usec;
    						if (microsecs <  0) {
    							seconds --;
    							microsecs += 1000000;
    						}
    				}
    				char description [GET_DIAG_INFO_LEN_64];
    				sprintf (description, "(%s),",eventTrace[ii].eventCodeDescription);
    				printf(" 0x%0x %-18s %d bytes,\ttimeStart: %ld.%06ld, timeEnd: %ld.%06ld, time elapsed since last op: %ld.%06ld sec\n",
    						eventTrace[ii].eventCode, description, eventTrace[ii].transferredBytes,
    						eventTrace[ii].tv_start_sec, eventTrace[ii].tv_start_usec,
    						eventTrace[ii].tv_end_sec, eventTrace[ii].tv_end_usec, seconds, microsecs);
    			}
    }

    else if (ISCMD('s','t','t')) {
    	rc = setTargetTemperature(id, p[1]);
    	if(rc == 0)
    		printf("setTargetTemperature: Success, Set to %d \n", p[1]);
    	else
    		printf("setTargetTemperature: Failed \n" ); 		   
    }

    else if (ISCMD('g','t','t')) {
    	rc = getTargetTemperature(id, &retVal);
    	printf("getTargetTemperature: %d \n", retVal);
    }

    else if (ISCMD('s','p','r')) {
    	rc = setPositiveTemperatureRampRate(id, p[1]);
   	}
    	
    else if (ISCMD('g','p','r')) {
    	rc = getPositiveTemperatureRampRate(id, &retVal);
    	printf("getPositiveTemperatureRampRate: %d \n", retVal);
    }
    	
    else if (ISCMD('s','n','r')) {
    	rc = setNegativeTemperatureRampRate(id, p[1]);
    }
    	
    else if (ISCMD('g','n','r')) {
    	rc = getNegativeTemperatureRampRate(id, &retVal);
    	printf("getNegativeTemperatureRampRate: %d \n", retVal);
    }

	else if (ISCMD('g','h','o')) {
    	rc = getHeaterOutput(id, &retVal);
    	printf("getHeaterOutput: %d \n", retVal);
    }
	
	else if (ISCMD('g','s','p')) {
    	rc = getShutterPosition(id, &retVal);
    	printf("getShutterPosition: %d \n", retVal);
    }

	else if (ISCMD('g','t','e')) {
    	rc = getTemperatureErrorStatus(id, &retVal);
    	printf("getTemperatureErrorStatus: rc=%d value=%x \n", rc, retVal);
    }
    
	else if (ISCMD('g','c','v')) {	
    	rc = getCurrentVoltage(id, &retVal, &retVal1);
    	if(rc == 0)
    		printf("getCurrentVoltage: 5V=%d mV, 12V=%d mV \n", retVal, retVal1);
    	else
    		printf("getCurrentVoltage: Failed \n" );  
    }
	else if (ISCMD('g','a','c')) {	
    	rc = getActualCurrent(id, &retVal, &retVal1);
    	if(rc == 0)
    		printf("getActualCurrent: 5V=%d mA, 12V=%d mA\n", retVal, retVal1);
    	else
    		printf("getActualCurrent: Failed \n" );  
    }
	
	else if (ISCMD('s','t','v')) {	
    	rc = setTargetVoltage(id, p[1], p[2]);
    }
    
	else if (ISCMD('g','t','v')) {	
    	rc = getTargetVoltage(id, &retVal, &retVal1);
    	printf("getTargetVoltage: 5V=%d 12V=%d \n", retVal, retVal1);
    }
    
	else if (ISCMD('s','u','p')) {	
    	rc = setUartPullupVoltage(id, p[1]);
    	if(rc == 0)
    		printf("setUartPullupVoltage: Success, Set to %d mV \n", p[1]);
    	else
    		printf("setUartPullupVoltage: Failed \n" ); 				 
    }
    
	else if (ISCMD('i','d','p')) {	
    	rc = isDrivePlugged(id);
    	printf("isDrivePlugged: %s \n", rc ? "Yes":"No");			
    }
	else if (ISCMD('v','e','r')) {	
		terPrintSlotInfo(id);
    }
    
	else if (ISCMD('r','d','m')) {	
		rc = initTest(id, driveType);
		
		WAPI_PROFILE_START;
   		rc = siReadDriveMemory(id, (Dword)p[1], (Word)p[2], Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0)
    	{
			WAPI_PROFILE_STOP;
    		printf("Slot %d: siReadDriveMemory, Read %d bytes from Address 0x%08x \n", id, p[2], p[1]);

			if(verbose) {
    			for (i = 0; i< p[2]; i++) {
    				printf("%02x ", Data[i]);
    				if( (i % 40) == 0 && i) printf("\n");
    			}
			}
    	}
		rc = finishTest(id, driveType);
    }
    
	else if (ISCMD('g','u','v')) {	
    	rc = siGetDriveUartVersion(id, &retVal);
    	if(rc == 0)
    		printf("siGetDriveUartVersion: %d \n", retVal); 
    }
    
	else if (ISCMD('g','u','b')) {	
    	rc = siGetUartBaudrate(id, &retval3);
    	printf("siGetUartBaudrate: %d \n", (int)retval3);
    }
    
	else if (ISCMD('s','u','b')) {	
    	rc = siSetUartBaudrate(id,(Dword)p[1]);
		if(rc == 0) {
    		rc = siGetUartBaudrate(id, &retval3);
    		printf("siGetUartBaudrate: %d \n", (int)retval3);
		}
		else
			printf("Valid baud rates: 115200,460800,921600,1843200,2778000,3333000,4167000,5556000,8333000 \n");
    }
    
	
	else if (ISCMD('g','u','p')) {	
    	rc = getUartPullupVoltage(id, &retVal);
    	if(rc == 0)
    		printf("getUartPullupVoltage: %d mV \n", retVal);
    	else
    		printf("getUartPullupVoltage: Failed \n" );  

    }
    
	else if (ISCMD('e','c','h')) {	
	    if(p[1] == 0) {
			showApiHelp();
			return 1;
		}

	    rc = initTest(id, driveType);
		
    	rc = siEchoDrive(id, (Word)p[1], Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0)
    	{
    		printf("siEchoDrive: %d bytes were received in the response \n", p[1]);
    		for (i = 0; i< p[1]; i++) {
				if(p[2]) {
					if(isprint(Data[i]))
						terprintf("%c", Data[i]);
					else
						terprintf(".");
				}
				else
    				terprintf("%02x ", Data[i]);
    			if( (i % 40) == 0 && i) terprintf("\n");
    		}
    	}
		else
		   printf("Error %02x: siEchoDrive failed \n", rc);
		rc = finishTest(id, driveType);
		terprintf("\n");
    }
    
	else if (ISCMD('i','c','e')) {	
    	rc = isCellEnvironmentError(id);
    	if(rc == 0)
    		printf("isCellEnvironmentError: No \n");
    	else
    		printf("isCellEnvironmentError: Yes, Error=%d \n", rc); 			
    }
    
	else if (ISCMD('c','c','e')) {	
    	rc = clearCellEnvironmentError(id);
    	if(rc == 0)
    		printf("clearCellEnvironmentError: Success \n" );
    	else
    		printf("clearCellEnvironmentError: Failed \n" );
    }
    
	else if (ISCMD('s','h','t')) {	
    	rc = setSafeHandlingTemperature(id,(Word)p[1]);
    }
    
	else if (ISCMD('w','d','m')) {	
    	for(i=0; i < p[2]; ++i) {
    		Data[i] = (i % 2) ? 0x33: 0xcc;
    	}

    	rc = siWriteDriveMemory(id, (Dword)p[1], (Word)p[2], Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc != 0) {
		    printf("siWriteDriveMemory: Failed \n");
    	}
    }
	else if (ISCMD('g','f','t')) {	
		unsigned driveMemTopAddr = 0;
		unsigned sensorIOAddress = 0;
		unsigned offset = 0x1FC;
		static FILE *f = NULL;
	 	
		
		if( 0 == isDrivePlugged(id) ) {
			printf("Slot %d, Drive not present \n", id);
			exit(0);
		}

	    rc = initTest(id, driveType);
		
		f = fopen("/var/tmp/htemp%d.txt", "a+b");

    	rc = siEchoDrive(id, 512, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0)
    	{
			driveMemTopAddr = Data[offset+2] << 24 | Data[offset+3] << 16 | 
							  Data[offset+0] << 8  | Data[offset+1];
    		terprintf("Drive top memory addresss = 0x%08x \n", driveMemTopAddr);

    	}
		else
		   	terprintf("Error %02x: siEchoDrive failed \n", rc);

		sleep(60);
		
		rc = siReadDriveMemory(id, (driveMemTopAddr+0x24), 4, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0) {
			sensorIOAddress = (Data[2] << 24) | (Data[3] << 16) |  
			                  (Data[0] << 8)  | Data[1];
    		terprintf("Sensor IO addresss = 0x%08x \n", sensorIOAddress);
    	}
		else {
    		printf("siReadDriveMemory: Failed \n" );				
		}


		for(i=0; i<10; ++i) {
			sleep(1);
			
   			rc = siReadDriveMemory(id, sensorIOAddress, 2, Data, WAPI_DRIVE_TIMEOUT_MS);

    		if(rc == 0) {
		    	fprintf(f, "FTemp in Degree C: %d \n", Data[1]);
    		}
			else {
    			printf("Slot %d, getftemp: Failed \n", id );				
			}
		}

		fclose(f);
		rc = finishTest(id, driveType);

	}
	else if (ISCMD('s','f','r')) {	
   		rc = siSetDriveFanRPM(id, (Byte)p[1]);
    	if(rc == 0) {
		    printf("siSetDriveFanRPM: Success \n");
    	}
		else {
    		printf("siSetDriveFanRPM: Failed \n" );				
		}
	}

	else if (ISCMD('i','s','t')) {	
   		rc = isSlotThere(id);
    	if(rc == 0) {
		    printf("isSlotThere: Yes \n");
    	}
		else {
		    printf("isSlotThere: ");
			terPrintError(id, (TER_Status)rc);
		}
	}

	else if (ISCMD('g','i','d')) {	
 		rc = siGetInterCharacterDelay ( id, &retval3 );
    	if(rc == 0) {
		    printf("siGetInterCharacterDelay: %lx \n", retval3);
    	}
	}

	else if (ISCMD('s','i','d')) {	
	    rc = siSetInterCharacterDelay ( id, p[1] );
    	if(rc != 0) {
		    printf("siSetInterCharacterDelay: Failed");
    	}
	}
	else if (ISCMD('s','i','p')) {	
	    rc = siSetInterPacketDelay ( id, p[1] );
    	if(rc != 0) {
		    printf("siSetInterPacketDelay: Failed");
    	}
	}
	else if (ISCMD('s','s','l')) {	
	    rc = siSetLed(id, p[1]);
    	if(rc != 0) {
		    printf("siSetLed: Failed");
    	}
	}
	else if (ISCMD('s','d','t')) {	
		rc = siSetDriveDelayTime(id, (Word)p[1], WAPI_DRIVE_TIMEOUT_MS);
    	if(rc != 0) {
		    printf("siSetDriveDelayTime: Failed");
    	}
	}
	else if (ISCMD('s','l','l')) {	
	    rc = siSetDebugLogLevel(id, p[1]);
    	if(rc != 0) {
		    printf("siEnableLogLevel: Failed");
    	}
	}
	else if (ISCMD('c','h','e')) {	
	    rc = siClearHaltOnError(id);
    	if(rc != 0) {
		    printf("siClearHaltOnError: Failed \n");
    	}
	}
	else if (ISCMD('s','t','e')) {	
    	rc = setTemperatureEnvelope(id, p[1]);
    	if(rc == 0)
    		printf("setTemperatureEnvelope: Success, Set to %d \n", p[1]);
    	else
    		printf("setTemperatureEnvelope: Failed \n" ); 		   
    }
    else if (ISCMD('g','t','n')) {
    	rc = getTemperatureEnvelope(id, &retVal);
    	printf("getTemperatureEnvelope: %d \n", retVal);
    }
	else if (ISCMD('s','v','r')) {	
    	rc = setVoltageRiseTime(id, p[1], p[2]);
    	if(rc == 0)
    		printf("setVoltageRiseTime: 5V=%d, 12V=%d \n", p[1], p[2]);
    	else
    		printf("setVoltageRiseTime: Failed \n" ); 		   
    }
	else if (ISCMD('g','v','r')) {	
    	rc = getVoltageRiseTime(id, &retVal, &retVal1);
    	if(rc == 0)
    		printf("setVoltageRiseTime: 5V=%d, 12V=%d \n", retVal, retVal1);
    	else
    		printf("setVoltageRiseTime: Failed \n" ); 		   
    }
	else if (ISCMD('s','v','f')) {	
    	rc = setVoltageFallTime(id, p[1], p[2]);
    	if(rc == 0)
    		printf("setVoltageFallTime: 5V=%d, 12V=%d \n", p[1], p[2]);
    	else
    		printf("setVoltageFallTime: Failed \n" ); 		   
    }
	else if (ISCMD('g','v','f')) {	
    	rc = getVoltageFallTime(id, &retVal, &retVal1);
    	if(rc == 0)
    		printf("getVoltageFallTime: 5V=%d, 12V=%d \n", retVal, retVal1);
    	else
    		printf("getVoltageFallTime: Failed \n" ); 		   
    }
	
	else if (ISCMD('g','v','e')) {
    	rc = getVoltageErrorStatus(id, &retVal); 
    	printf("getVoltageErrorStatus: rc=%d, value=%x \n", rc, retVal);
    }
	else if (ISCMD('s','v','i')) {
    	rc = setVoltageInterval(id, p[1]); 
    	printf("setVoltageInterval: rc=%d \n", rc);
    }
	else if (ISCMD('c','t','u')){
		rc = siChangeToUart3(id, 0x0000, WAPI_DRIVE_TIMEOUT_MS);
		if (rc != success)
			printf("siChangeToUart3: Failed\n");
	}
	else if (ISCMD('q', 'r', 'y')) {
		rc = siQueryRequest(id, 0x00, WAPI_DRIVE_TIMEOUT_MS);
		printf("siQueryRequest: status = 0x%02x\n", rc);
	}
	else if (ISCMD('g','c','i'))
	{
		CELL_CARD_INVENTORY_BLOCK *temp;
		rc = getCellInventory(id, &temp);
		if(rc == 0)
			printf("getCellInventory: Success \n");
		else
			printf("getCellInventory: Failed \n");
	}
	else if(ISCMD('g','c','l'))
	{
		rc = getCurrentLimit(id, &retVal, &retVal1);
		if(rc == 0)
			printf("getCurrentLimit: Success 5V=%d, 12V=%d\n", retVal, retVal1);
		else
			printf("getCurrentLimit: Failed \n");
	}
	else if(ISCMD('g','r','t'))
	{
		rc = getReferenceTemperature(id, &retVal);
		if(rc == 0)
			printf("getReferenceTemperature: Success Reference Temp: %d\n", retVal);
		else
			printf("getReferenceTemperature: Failed \n");
	}
	else if(ISCMD('g','t','l'))
	{
		rc = getTemperatureLimit(id, &retVal, &retVal1, &retVal2, &retVal4);
		if(rc == 0)
			printf("getTemperatureLimit: Success Lower: %d, Upper: %d, Lower: %d, Upper: %d\n", retVal, retVal1, retVal2, retVal4);
		else
			printf("getTemperatureLimit: Failed \n");
	}
	else if(ISCMD('g','v','l'))
	{
		rc = getVoltageLimit(id, &retVal, &retVal1, &retVal2, &retVal4);
		if(rc == 0)
			printf("getVoltageLimit: %d %d %d %d \n", retVal, retVal1, retVal2, retVal4);
		else
			printf("getVoltageLimit: Failed\n");
	}
	else if(ISCMD('i','o','t'))
	{
		rc = isOnTemp(id);
		terprintf("isOnTemp = %s \n", rc?"Yes":"No");
	}
	else if(ISCMD('s','t','l'))
	{
	    rc = setTemperatureLimit(id, p[1], p[2], p[3], p[4]);
	    if(rc == 0)
	    	printf("setTemperatureLimit: %d, %d, %d, %d \n", p[1],p[2],p[3],p[4]);
	    else
	    	printf("setTemperatureLimit: Failed \n");
	}
	else if(ISCMD('s','v','d'))
	{
		rc = setUartPullupVoltageDefault(id);
		if(rc == 0)
			printf("setUartPullupVoltageDefault: Success \n");
		else
			printf("setUartPullupVoltageDefault: Failed \n");
	}
	else if(ISCMD('s','v','l'))
	{
		rc = setVoltageLimit(id, p[1], p[2], p[3], p[4]);
		if(rc == 0)
			printf("setVoltageLimit: %d, %d, %d, %d \n", p[1],p[2],p[3],p[4]);
		else
			printf("setVoltageLimit: Failed, rc = %d\n",rc);
	}
	else if(ISCMD('b','r','w')) //This reuses a test that was already created.
	{
		rc = test06(id,driveType);
		if(rc == 0)
			printf("Bulk Read/Write: Success \n");
		else
			printf("Bulk Read/Write: Failed \n");
	}
	else if(ISCMD('g','a','t'))
	{
		Dword tempDword;
		rc = siGetAckTimeout(id, &tempDword);
		if(rc == 0)
			printf("siGetAckTimeout: Success Timeout=%lu\n", tempDword);
		else
			printf("siGetAckTimeout: Failed \n");
	}
	else if(ISCMD('r','s','d'))
	{
		rc = siResetDrive(id, p[1], p[2], p[3]);
		if(rc == 0)
			printf("siResetDrive: Success, Type:%d, Reset:%d, Timeout:%d\n", p[1],p[2],p[3]);
		else
			printf("siResetDrive: Failed \n");
	}
	else if(ISCMD('s','a','t'))
	{
		rc = siSetAckTimeout(id, p[1]);
		if(rc == 0)
			printf("siSetAckTimeout: Success, Set to %d \n", p[1]);
		else
			printf("siSetAckTimeout: Failed \n" );
	}
	else if(ISCMD('s','d','l'))
	{
		rc = siSetDebugLogLevel(id, p[1]);
		if(rc == 0)
			printf("siEnableLogLevel: Success, Set to %d\n", p[1]);
		else
			printf("siEnableLogLevel: Failed \n");
	}
	else if(ISCMD('s','u','t'))
	{
		 rc = siSetUartProtocol(id, p[1]);
		if(rc == 0)
			printf("siSetUartProtocol: Success, Set to %d \n", p[1]);
		else
			printf("siSetUartProtocol: Failed \n" );
	}
	/*else if (ISCMD('s','u','m')) {
		rc = siSetUart2ModeRequest(id, WAPI_DRIVE_TIMEOUT_MS);
		printf("siSetUART2ModeRequest: status = 0x%02x\n", rc);
	}*/

	else if (ISCMD('g','m','b')) { // gmb: siGetImpulseCodeMfgMode
		Byte /*Byte Value*/ bV;

		printf("siGetImpulseCodeMfgMode(%u, %p", id, &bV);
		rc = siGetImpulseCodeMfgMode(id, &bV);
		printf("->0x%02X (%u", bV, bV);
		if (/*printable?*/ isprint(bV)) { // show character
			printf(", '%c'", bV);
		}
		printf(")) == %u (0x%X)\n", rc, rc);
	}
	else if (ISCMD('g','m','c')) { // gmc: siGetImpulseCountMfgMode
		Word /*Word Value*/ wV;

		printf("siGetImpulseCountMfgMode(%u, %p", id, &wV);
		rc = siGetImpulseCountMfgMode(id, &wV);
		printf("->%u) == %u (0x%X)\n", wV, rc, rc);
	}
	else if (ISCMD('g','m','d')) { // gmd: siGetIntercharacterDelayMfgMode
		Word /*Word Value*/ wV;

		printf("siGetIntercharacterDelayMfgMode(%u, %p", id, &wV);
		rc = siGetIntercharacterDelayMfgMode(id, &wV);
		printf("->%u) == %u (0x%X)\n", wV, rc, rc);
	}
	else if (ISCMD('g','m','p')) { // gmp: siGetPowerDelayMfgMode
		Word /*Word Value*/ wV;

		printf("siGetPowerDelayMfgMode(%u, %p", id, &wV);
		rc = siGetPowerDelayMfgMode(id, &wV);
		printf("->%u) == %u (0x%X)\n", wV, rc, rc);
	}
	else if (ISCMD('g','m','s')) { // gms: siGetSignatureMfgMode
		Byte /*Byte Vector*/ bV[256];
		Byte /*Length*/      ln = 0;

		memset(bV, 0, sizeof(bV));
		printf("siGetSignatureMfgMode(%u, %p", id, bV);
		rc = siGetSignatureMfgMode(id, bV, &ln);
		printf("->%s, %p->%u) == %u (0x%X)\n", Common::Sig2Str(bV, ln), &ln,
				ln, rc, rc);
	}
	else if (ISCMD('s','m','b')) { // smb: siSetImpulseCodeMfgMode
		if (argCnt < 4) { // default value
			argInt[3] = (int)MFGImpulseCodeDEFAULT;
			printf("   Using 0x%X as value...\n", argInt[3]);
		}
		argInt[3] = (int)((Byte)(0xFFU & (unsigned int)argInt[3]));
		printf("siSetImpulseCodeMfgMode(%u, 0x%02X)", id, argInt[3]);
		rc = siSetImpulseCodeMfgMode(id, (Byte)argInt[3]);
		printf(" == %u (0x%X)", rc, rc);
		if (/*OK?*/ rc == (Byte)success) { // Check by Get
			Byte /*Byte Value*/ bV = 0xFF;

			printf("; Checking  siGetImpulseCodeMfgMode(%u, %p", id, &bV);
			siGetImpulseCodeMfgMode(id, &bV);
			printf("->%u (0x%02X", bV, bV);
			if (/*printable?*/ isprint(bV))
				printf(", '%c'", bV);
			printf(")) == %u (0x%X)", rc, rc);
		}
		printf("\n");
	}
	else if (ISCMD('s','m','c')) { // smc: siSetImpulseCountMfgMode
		if (argCnt < 4) { // default value
			argInt[3] = (int)MFGImpulseCountDEFAULT;
			printf("   Using %d (0x%X) as value...\n", argInt[3],
					(unsigned int)argInt[3]);
		}
		printf("siSetImpulseCountMfgMode(%u, %u)", id, (Word)argInt[3]);
		rc = siSetImpulseCountMfgMode(id, (Word)argInt[3]);
		printf(" == %u (0x%X)", rc, rc);
		if (/*OK?*/ rc == (Word)success) { // Check by Get
			Word /*Word Value*/ wV = 0xFFFF;

			printf("; Checking  siGetImpulseCountMfgMode(%u, %p", id, &wV);
			siGetImpulseCountMfgMode(id, &wV);
			printf("->%u", wV);
			printf(")) == %u (0x%X)", rc, rc);
		}
		printf("\n");
	}
	else if (ISCMD('s','m','d')) { // smd: siSetIntercharacterDelayMfgMode
		if (argCnt < 4) { // default value
			argInt[3] = (int)MFGIntercharacterDelayDEFAULT;
			printf("   Using %d (0x%X) as value...\n", argInt[3],
					(unsigned int)argInt[3]);
		}
		printf("siSetIntercharacterDelayMfgMode(%u, %u)", id, (Word)argInt[3]);
		rc = siSetIntercharacterDelayMfgMode(id, (Word)argInt[3]);
		printf(" == %u (0x%X)", rc, rc);
		if (/*OK?*/ rc == (Word)success) { // Check by Get
			Word /*Word Value*/ wV = 0xFFFF;

			printf("; Checking  siGetIntercharacterDelayMfgMode(%u, %p", id, &wV);
			siGetIntercharacterDelayMfgMode(id, &wV);
			printf("->%u", wV);
			printf(")) == %u (0x%X)", rc, rc);
		}
		printf("\n");
	}
	else if (ISCMD('s','m','p')) { // smp: siSetPowerDelayMfgMode
		if (argCnt < 4) { // default value
			argInt[3] = (int)MFGPowerDelayDEFAULT;
			printf("   Using %d (0x%X) as value...\n", argInt[3],
					(unsigned int)argInt[3]);
		}
		printf("siSetPowerDelayMfgMode(%u, %u)", id, (Word)argInt[3]);
		rc = siSetPowerDelayMfgMode(id, (Word)argInt[3]);
		printf(" == %u (0x%X)", rc, rc);
		if (/*OK?*/ rc == (Word)success) { // Check by Get
			Word /*Word Value*/ wV = 0xFFFF;

			printf("; Checking  siGetPowerDelayMfgMode(%u, %p", id, &wV);
			siGetPowerDelayMfgMode(id, &wV);
			printf("->%u", wV);
			printf(")) == %u (0x%X)", rc, rc);
		}
		printf("\n");
	}
	else if (ISCMD('s','m','s')) { // sms: siSetSignatureMfgMode
		Byte /*Byte Vector*/ bV[256];
		Byte /*Length*/      ln = 0;

		memset(bV, 0, sizeof(bV));
		if (argCnt < 4) { // default value
			memcpy(bV, (Byte*)MFGSignatureDEFAULT, MFGSignatureLengthDEFAULT);
			ln = MFGSignatureLengthDEFAULT;
			printf("   Using %s[%u] as value...\n", Common::Sig2Str(bV, ln),\
					ln);
		} else { // decode value
			ln = Common::Str2Sig(bV, argStr[3]);
		}
		printf("siSetSignatureMfgMode(%u, %s, %u)", id,
				Common::Sig2Str(bV, ln), ln);
		rc = siSetSignatureMfgMode(id, bV, ln);
		printf(" == %u (0x%X)", rc, rc);
		if (/*OK?*/ rc == (Byte)success) { // Check by Get
			printf("; Checking siGetSignatureMfgMode(%u, %p", id, bV);
			memset(bV, 0xFF, sizeof(bV));
			rc = siGetSignatureMfgMode(id, bV, &ln);
			printf("->%s, %p->%u)", Common::Sig2Str(bV, ln), &ln, ln);
			printf(" == %u (0x%X)", rc, rc);
		}
		printf("\n");
	}
	else if (ISCMD('s','m','x')) { // smx: siSetPowerEnableMfgMode
		Word /*5V*/  v5 = 5000;
		Word /*12V*/ v12 = 12000;

		if (argCnt >= 4) {
			v5 = atoi(argStr[3]);
		}
		if (argCnt >= 5) {
			v12 = atoi(argStr[4]);
		}
		siSetUartBaudrate(id, 115200);
		printf("siSetPowerEnableMfgMode(%u, %u, %u)", id, v5, v12);
		rc = siSetPowerEnableMfgMode(id, v5, v12);
		printf(" == %u (0x%X)\n", rc, rc);
	}

	else if (ISCMD('h','t','p')) {
    	rc = adjustTemperatureControlByDriveTemperature(id, p[1]); 
    }
	else if (ISCMD('t', '0', '0')) { // t00: Test MFG (one-step)
		rc = Test00(id, driveType);
	}
	else if (ISCMD('t','0','1')) {	 /* Test echo/read/write */
		rc = test01(id,driveType);
	}
	else if (ISCMD('t','0','2')) {	 /* Test baud rates */
		rc = test02(id,driveType);
	}
	else if (ISCMD('t','0','3')) {	 /* Test 5V margining */
		rc = test03(id,driveType);
	}
	else if (ISCMD('t','0','4')) {	 /* Test temp control */
		rc = test04(id,driveType);
	}
	else if (ISCMD('t','0','5')) {	 /* Misc. tests */
		rc = test05(id,driveType);
	}
	else if (ISCMD('t','0','6')) {	 /* Misc. tests */
		rc = test06(id,driveType);
	}
	else if (ISCMD('t','0','7')) {	 /* Misc. tests */
		rc = test07(id,driveType);
	}
	else if (ISCMD('t','0','8')) {	 /* Misc. tests */
		rc = test08(id,driveType);
	}
	else if (ISCMD('t','0','9')) {	 /* Misc. tests */
		rc = test09(id,driveType, p[1], p[2], p[3], p[4], p[5]);
	}
	else if (ISCMD('t','1','0')) {	 /* Misc. tests */
		rc = test10(id,driveType);
	}
	else if (ISCMD('t','2','5')) { 	 /* Testing the siGetLastErrorCause */
		rc = test25(id,driveType);
	}
	else if (ISCMD('t','9','8')) {	 /* Misc. tests */
		rc = test98(id,driveType);
	}
	else if (ISCMD('t','9','9')) {	 /* Misc. tests */
		rc = test99(id,driveType);
	}

    else{
    	printf("Unknown command \n"); 
    }

   	char errorString[1024];
   	Byte errorCode = success;
	rc = siGetLastErrorCause(id, errorString, &errorCode);
	if(rc == 0)
		printf("siGetLastErrorCause: %s, 0x%x \n", errorString, errorCode);
	else
		printf("siGetLastErrorCause: Failed \n");

   	return(rc);  		
}

char *strSiInitError[] = {"Success", "TerError", "DriveError", "SIO_Not_Reachable", 
                         "SIO_Socket_Error","DIO_Non_Responsive", "DIO_App_Error"};

/*!
 * The main function that would call the unit test.
   \param argc
         Number of arguments
   \param argv
   	   	 An array of argument strings
   \return Status code
 */
int main(int argc, char *argv[])
{
	Byte rc = 0;
	int  id = 0;
	int  i, j, p[10];
    char *cmd;
	
    printf("Version: %s\n", __DATE__);
    ArgCopy(argc, argv);
	
    memset(p, 0, 10*sizeof(int));  /* clear parameter list */
	verbose = 0;
	quiteMode = 0;
	driveType = WAPI_DRIVE_TYPE_INVALID;   /* initialize it to incorrect drive type */
	
	if (argc <= 2) {
		showApiHelp();
		return 1;
	}
    
    cmd = argv[1];        /* argv[1] = command */
	id = atoi(argv[2]);   /* argv[2] = slotid */

    /* copy argv[3]... to an array */
	for(i = 3, j = 1; i < argc; ++i) {
       if ( argv[i][0] == '-' && tolower(argv[i][1]) == 'l' ) {
	   		if(argv[i][2] == '0') {
	   			siSetDebugLogLevel(id, 0);
			}
			else {
				siSetDebugLogLevel(id, argv[i][2]-'0');
			}
			continue;
	   }

       if ( argv[i][0] == '-' && tolower(argv[i][1]) == 'v' ) {
	   		verbose = 1;
			continue;
	   }

       if ( argv[i][0] == '-' && tolower(argv[i][1]) == 'q' ) {
	   		quiteMode = 1;
			continue;
	   }
       if ( argv[i][0] == '-' && tolower(argv[i][1]) == 'b' ) {
	   		baudRateIndex = (argv[i][2])-'0';
			if( baudRateIndex >= (int)(sizeof(baudRates)/sizeof(Dword)) ) {
				printf("ERROR: Invalid baudrate \n");
				baudRateIndex = 9;
			}
			continue;
	   }
	   
	   if ( argv[i][0] == '-' && tolower(argv[i][1]) == 'd' ) {
			if ( strcmp ( argv[i], "-dcbd" ) == 0 ) {
				driveType = WAPI_DRIVE_TYPE_CBD;
			}
			else if ( strcmp ( argv[i], "-deb7" ) == 0 ) {
			    driveType = WAPI_DRIVE_TYPE_EB7;
			}
			else if ( strcmp ( argv[i], "-dec7" ) == 0 ) {
			    driveType = WAPI_DRIVE_TYPE_EC7;
			}
			continue;
	   }

	   
       if ( argv[i][0] == '0' || tolower(argv[i][1]) == 'x' ) 
           sscanf(argv[i],"%x",&p[j]);
       else 
	       p[j] = atoi(argv[i]);
	   j++;
	}
	
	// If the command is cce or if it is a test case then we should reset slot otherwise
	// for all other commands do not reset a slot
	if ((cmd[0] == 't') || (cmd[0] == 'c' && cmd[1] == 'c' && cmd[2] == 'e'))
		Common::Instance.SetWapitestResetSlotDuringInitialize(1);
	else
		Common::Instance.SetWapitestResetSlotDuringInitialize(0);
	
 	rc = siInitialize(id);

	if( rc != 0 ) {
	   	char errorString[1024];
	   	Byte errorCode = success;
		rc = siGetLastErrorCause(id, errorString, &errorCode);
		if(rc == 0)
			printf("siGetLastErrorCause: %s, 0x%x \n", errorString, errorCode);
		else
			printf("siGetLastErrorCause: Failed \n");

		printf("ERROR: siInitialize failed [rc=%d]. Continuing... \n", rc);
	}
   
//	printf("Press any key to continue\n");
//	getchar();

	int count = 0;
	while (count < 1){
		rc = testApi(cmd, (Byte)id, p);
		count++;
	}
	
	siFinalize(id);

	return rc;
}


#ifdef __cplusplus
};
#endif

