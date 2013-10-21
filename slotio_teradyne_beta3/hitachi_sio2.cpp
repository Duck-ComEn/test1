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
#include <alloca.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sched.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Teradyne Specific Include Files
 */
#include "neptune_sio2.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "ver.h"

#include "tcTypes.h"
#include "envApi.h"
#include "libsi.h"
#include "neptune_hitachi_interface.h"
#include "neptune_hitachi_comm.h"

#ifdef SIMULATE_HARDWARE_FOR_TESTING
#include "neptune_sio2_sim.h"
#endif

#define ISCMD(a,b,c) ((cmd[0] == (a)) && (cmd[1] == (b)) && (cmd[2] == (c)))


#define WAPI_BUFFER_SIZE   	   65536 + 32

#define WAPI_DRIVE_TIMEOUT_MS  30000
#define WAPI_PROFILE_START	   profileTimer(0);
#define WAPI_PROFILE_STOP	   profileTimer(1);
#define WAPI_DRIVE_TYPE_EC7	   0
#define WAPI_DRIVE_TYPE_PTB	   1
#define WAPI_DRIVE_TYPE_EB7	   2
#define WAPI_DRIVE_TYPE_INVALID 100

/* Test01 configs */
#define WAPI_TEST01_ENABLE_64K 		1
#define WAPI_TEST01_EXIT_ON_ERROR 	0
#define WAPI_TEST01_CONTINUE_READ 	0
#define WAPI_TEST01_TEST_UART_VER 	0

/* Test09 configs */
#define WAPI_TEST09_ECHO_COUNT 		1000
#define WAPI_TEST09_TARGET_TEMP 	6000
#define WAPI_TEST09_POS_RAMP 	    300
#define WAPI_TEST09_NEG_RAMP 	    300
#define WAPI_TEST09_SAFE_TEMP 	    3500
#define WAPI_TEST09_TEST_TIME_MIN   10  

typedef struct drive_info_t 
{
  	unsigned int  powerupDelay;
  	Dword 	interCharDelay;
  	Byte 	ackTimeout;
  	Byte 	uartProtocol;
	Dword   readAddr;
	Dword   writeAddr;
	Byte    skipWrite;
}  
DRIVE_INFO;


static Byte Data[WAPI_BUFFER_SIZE];
static int  verbose;
static int 	quiteMode;
static int 	driveType;
                          /* pdelay  icdelay  acktmo uartpro rAddr       wAddr        skipw*/
DRIVE_INFO driveInfo[3] = { {12,     540,     3,     0,      0x08331000, 0x8331063,   1},	   /* EC7 */
                            {90,     100,     3,     0,      0x08331000, 0x8331063,   1},	   /* PTB */
						    {12,     540,     3,     0,      0x08331000, 0x8331063,   1} };    /* EB7 */
		

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
static void showApiHelp(void)
{
	printf("gct slotid                             - getCurrentTemperature \n");
    printf("stt slotid temp(deg C*100)             - setTargetTemperature \n");
    printf("gtt slotid                             - getTargetTemperature \n");
    printf("spr slotid rate(deg C*100)             - setPositiveTemperatureRampRate \n");
    printf("gpr slotid                             - getPositiveTemperatureRampRate \n");
    printf("snr slotid rate(deg C*100)             - setNegativeTemperatureRampRate \n");
    printf("gnr slotid                             - getNegativeTemperatureRampRate \n");
    printf("gho slotid                             - getHeaterOutput \n");
    printf("gte slotid                             - getTemperatureErrorStatus \n");
    printf("gve slotid                             - getVoltageErrorStatus \n");
    printf("gcv slotid                             - getCurrentVoltage \n");
    printf("gac slotid                             - getActualCurrent \n");
    printf("stv slotid 5v(mV) 12v(mV)              - setTargetVoltage \n");
    printf("gtv slotid                             - getTargetVoltage \n");
    printf("sup slotid volts(mV)                   - setUartPullupVoltage \n");
	printf("idp slotid                             - isDrivePlugged \n");
    printf("rdm slotid addr size -d[driveType]     - siReadDriveMemory\n");
	printf("guv slotid                             - siGetDriveUartVersion \n");
	printf("gub slotid                             - siGetUartBaudrate\n");
	printf("sub slotid                             - siSetUartBaudrate\n");
	printf("gup slotid                             - getUartPullupVoltage \n");
	printf("ech slotid size -d<driveType>          - siEchoDrive(wSize)\n");
	printf("ice slotid                             - isCellEnvironmentError \n");
	printf("cce slotid                             - clearCellEnvironmentError \n");
	printf("sht slotid temp(deg C*100)             - setSafeHandlingTemperature \n");
    printf("wdm slotid addr size -d[driveType]     - siWriteDriveMemory \n");
	printf("sfr slotid rpm                         - siSetDriveFanRPM \n");
	printf("gsp slotid                             - getShutterPosition \n");
	printf("ist slotid                             - siIsSlotThere\n");
	printf("gid slotid                             - siGetInterCharacterDelay \n");
	printf("sid slotid val(bit times)              - siSetInterCharacterDelay \n");
	printf("gle slotid                     	       - siGetLastErrors \n");
	printf("sdt slotid delay(uSec)                 - siSetDriveDelayTime \n");
	printf("ssl slotid <0:off|1:on>        	       - siSetLed \n");
	printf("sll slotid <0:off|1:on>        	       - siEnableLogLevel \n");
	printf("che slotid                             - siClearHaltOnError \n");
	printf("ste slotid temp(deg C*10)              - setTemperatureEnvelope \n");
	printf("gtn slotid                             - getTemperatureEnvelope \n");
	printf("svr slotid 5v(mV) 12v(mV)              - getVoltageRiseTime \n");
	printf("svf slotid 5v(mV) 12v(mV)              - setVoltageFallTime \n");
	printf("gvr slotid                             - setVoltageRiseTime \n");
	printf("gvf slotid                             - getVoltageFallTime \n");
	printf("sip slotid delay(uSec)                 - siSetInterPacketDelay \n");
	printf("tNN slotid -d[driveType]               - test case where NN is a test case number \n");
	printf("____________________________________________________________________________________________________\n");
	printf("Note: 1) Slot numbers are ONE based\n");
	printf("      2) Default SIO IP Address is 10.101.1.2\n");
	printf("      3) To change ip address: export SIOIPADDRESS=\"xx.xx.xx.xx\"\n");
	printf("      4) To unset ip address : unset SIOIPADDRESS\n");
	printf("      5) To enable debug log, pass -L1 at the end of the cmd. To disable, use -L0 option\n");
	printf("      6) Log file is available at /var/tmp/slotN.log, where N=slot number\n");
	printf("      7) Valid drive types: ptb,eb7,ec7 \n");
	printf("Test cases: 01=echo/read/write, 02=baud rates, 03=5V margining, 04=temp control, 05=misc\n");
	printf("            06=bulk read/write, 08=1000 echo\n");
	printf("Example Command: ./wapitest t01 3 -L1\n");
	printf("____________________________________________________________________________________________________\n");

	

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


static Byte initDriveParameters(int id, int driveType)
{
    Byte 	rc = 0;

	if(driveType > WAPI_DRIVE_TYPE_EB7 ) {
		printf("ERROR: Incorrect drive type [%d] \n", driveType);
		printf("       Usage: -d[ptb|eb7|ec7]\n");
		return 1;
	}

	rc = siSetInterCharacterDelay(id, driveInfo[driveType].interCharDelay);	    
    rc = siSetUartProtocol(id,driveInfo[driveType].uartProtocol);
	rc = siSetAckTimeout ( id,driveInfo[driveType].ackTimeout);
	return rc;
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

	terprintf("Drive is warming up. Wait %d secs \n", driveInfo[driveType].powerupDelay);
	rc = initDriveParameters(id, driveType); 
	rc = setUartPullupVoltage(id,0);

//	sleep(10);

	rc = setTargetVoltage(id, 5000, 0); 
//	sleep(2);
	rc = setUartPullupVoltage(id,2700);

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


/**************************************************************************
 **  Utility functions - End
 **************************************************************************/

/**************************************************************************
 **  Test functions - Begin
 **************************************************************************/
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
#if WAPI_TEST01_TEST_UART_VER
	Word    uartVersion;
#endif
	Dword   size = 512, rAddr, wAddr;
	int 	i, errorCnt = 0;

	initTest(id, driveType);

	rAddr = driveInfo[driveType].readAddr;
	wAddr = driveInfo[driveType].writeAddr;

	if( driveType == WAPI_DRIVE_TYPE_PTB )
		goto testEcho;


//getUartVersion:
#if WAPI_TEST01_TEST_UART_VER
    rc = siGetDriveUartVersion(id, &uartVersion);
	if( rc == 0 )
		terprintf("siGetDriveUartVersion: %d \n",uartVersion);
	else {
	    ++errorCnt;
		terprintf("ERROR: siGetDriveUartVersion failed \n");
	}
	
	sleep(2); 
#endif 
	
testEcho:
	printBanner("Echo Test - Begin");
	sleep(2);
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
	sleep(2);
	printBanner("Echo Test - End");
	

//testReadDrive:	
    printBanner("Read Drive Test - Begin");
#if WAPI_TEST01_ENABLE_64K
	for(size = 128; size < ((sizeof(Data)/sizeof(Byte))-48); size += 128)
#else
	for(size = 128; size <=  2048; size += 128)
#endif
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
#if WAPI_TEST01_CONTINUE_READ
		if(size == 2048) size = 0;
#endif
#if WAPI_TEST01_EXIT_ON_ERROR
		if(errorCnt) return 1;
#endif
	}
    printBanner("Read Drive Test - End");

	if(driveInfo[driveType].skipWrite)
		goto test01Done;
			
//testWriteDrive:
    printBanner("Write Drive Test - Begin");
	size = 2;
	for(i = 1; i <= 10; ++i)
	{
  		rc = siReadDriveMemory(id, wAddr-1, size, Data, WAPI_DRIVE_TIMEOUT_MS); sleep(4);
    	if(rc == 0)
			terprintf("Before Write: %02x%02x - ", Data[0], Data[1]);
		else {
	    	++errorCnt;
			terprintf("Error %02x: siReadDriveMemory failed \n", rc);
		}
		
		Data[0] = i; Data[1] = i;
		
		rc = siWriteDriveMemory(id, wAddr, 1, Data, WAPI_DRIVE_TIMEOUT_MS); sleep(4);
    	if(rc != 0) {
	    	++errorCnt;
			terprintf("Error writing from drive \n");
		}
		
  		rc = siReadDriveMemory(id, wAddr-1, size, Data, WAPI_DRIVE_TIMEOUT_MS); sleep(4);
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
	Dword   baudRates[] = {115200,460800,921600,1843200,2778000,3333000,4167000,5556000,8333000,1843200};
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
	   rc = setTargetVoltage(id, targetVolatge[i],0); sleep(10);
	   
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
	initDriveParameters(id, driveType);

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
	for(i=0; i<loopcount; ++i) {
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
static Byte test09(int id, int driveType)
{
    Byte 	rc = 0;
	Dword   size = 512;
	int 	i, errorCnt = 0;
	int     loopcount=WAPI_TEST09_ECHO_COUNT;


	initTest(id, driveType);

	rc = setPositiveTemperatureRampRate(id, WAPI_TEST09_POS_RAMP);
	rc = setNegativeTemperatureRampRate(id, WAPI_TEST09_NEG_RAMP);
	rc = setTargetTemperature(id, WAPI_TEST09_TARGET_TEMP);

	for(i=0; i<WAPI_TEST09_TEST_TIME_MIN*2; ++i)
	{
        printTemperatureVariables(id);
		rc = isOnTemp(id);
		if(rc) 
			break;
		sleep(30);  /* 30 sec sleep */
		
	}

	if(loopcount) {
		terprintf("\nStarting %d Echo test \n", loopcount);

		for(i=0; i<loopcount; ++i) {
		   rc = siEchoDrive(id, size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	   if(rc) {
	    	   ++errorCnt;
		   }
		}

		printf("Slot %d, %d error(s) out of %d echo commands.\n", id, errorCnt, loopcount);
	}

	printf("Slot %d, Going to safe handling temp. Wait.... \n", id);
	rc = setSafeHandlingTemperature(id, WAPI_TEST09_SAFE_TEMP);

	for(i=0; i<100; ++i)
	{
        printTemperatureVariables(id);
		rc = isOnTemp(id);
		if(rc) {
		    printf("Slot %d, Safe handling temp reached \n", id);
			break;
		}
		sleep(5);
		
	}
	
	printf("Slot %d, Test09 completed \n", id);
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
	Word    envelope;

    printTemperatureVariables(id);
	for( i = 20; i <= 70; ++i ) {
	   envelope = (Word)i;
       rc = setTemperatureEnvelope(id, envelope);
       rc = getTemperatureEnvelope(id, &envelope);
       printf("getTemperatureEnvelope: %d \n", envelope);

       printTemperatureVariables(id);
	}
 
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
static Byte test98(int id, int driveType)
{
    Byte 	rc = 0;
//	int 	i;

	initTest(id, driveType);

//	for(i=0; i<1000000; ++i)
    while(1)
	{
		rc += setPositiveTemperatureRampRate(id, WAPI_TEST09_POS_RAMP);
		rc += setNegativeTemperatureRampRate(id, WAPI_TEST09_NEG_RAMP);
		rc += setTargetTemperature(id, WAPI_TEST09_TARGET_TEMP);
        printTemperatureVariables(id);
		rc += isOnTemp(id);
		if(rc) {
			printf("Error: Exiting.. \n");
			break;
		}
	}

	rc = finishTest(id, driveType);

	return rc;
}


static Byte test99(int id, int driveType)
{

    Byte 	rc = 0;
	Word    size;
	int     i, errorCnt = 0;
	Byte    echArray[] = {0xaa,0x55,0x04,0x04,0x00,0x00,0x00,0x00,0x02,0x00,0xfe,0x00};


	printBanner("Bulk Send/Receive - Begin");
	initTest(id, driveType);


    printf("Press Control-C to exit \n");
    
	rc = siBulkReadFromDrive(id, &size, Data, WAPI_DRIVE_TIMEOUT_MS);
	
	while(1) {
		size = sizeof(echArray)/sizeof(Byte);
		for(i = 0; i < size; ++i) {
			rc = siBulkWriteToDrive(id, 1, &echArray[i], WAPI_DRIVE_TIMEOUT_MS);
    		if(rc != 0) {
    			printf("Error: siBulkWriteToDrive \n");
			}
			usleep(15000); /* At lease 12 ms */
		}
        
		memset(Data, 0, WAPI_BUFFER_SIZE);
    	rc = siBulkReadFromDrive(id, &size, Data, WAPI_DRIVE_TIMEOUT_MS);
    	if(rc == 0)
    	{
    		printf("\nRead %d bytes: ", size);
    		for (i = 0; i < 40; i++) {
    			printf("%02x ", Data[i]);
    		}
    	}
		else {
			++errorCnt;
			break;
		}
	}
	printBanner("Bulk Send/Receive - End");

	rc = finishTest(id, driveType);
	return rc;
}

/**************************************************************************
 **  Test functions - End
 **************************************************************************/

static Byte testApi(char *cmd, Byte id, int *p)
{
    Byte 	rc = 0;
    Word 	retVal, retVal1;
	Dword 	retval3;
    int  	i;

    memset(Data, 0, WAPI_BUFFER_SIZE);
	
    if (ISCMD('g','c','t')) {
    	rc = getCurrentTemperature(id, &retVal);
    	if(rc == 0)
    		printf("getCurrentTemperature: %d \n", retVal);
    	else
    		printf("getCurrentTemperature: Failed \n" );				
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
    	printf("getTemperatureErrorStatus: %x \n", retVal);
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
   		rc = siReadDriveMemory(id, 0x39EF0, 2, Data, WAPI_DRIVE_TIMEOUT_MS);
		
    	if(rc == 0) {
		    printf("FTemp in Degree C: %d \n", Data[0]);
    	}
		else {
    		printf("getftemp: Failed \n" );				
		}
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
			terPrintError(id, rc);
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
	else if (ISCMD('g','l','e')) {	
	    rc = siGetLastErrors (id);
    	if(rc != 0) {
		    printf("siGetLastErrors: Failed");
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
    	printf("getVoltageErrorStatus: %x \n", retVal);
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
		rc = test09(id,driveType);
	}
	else if (ISCMD('t','1','0')) {	 /* Misc. tests */
		rc = test10(id,driveType);
	}
	else if (ISCMD('t','9','8')) {	 /* Misc. tests */
		rc = test98(id,driveType);
	}
	else if (ISCMD('t','9','9')) {	 /* Misc. tests */
		rc = test99(id,driveType);
	}
    else {
    	printf("Unknown command \n"); 
    }
   
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
	
	
    memset(p, 0, 10);  /* clear parameter list */
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
				siSetDebugLogLevel(id, 1);
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
	   
	   if ( argv[i][0] == '-' && tolower(argv[i][1]) == 'd' ) {
			if ( strcmp ( argv[i], "-dptb" ) == 0 ) {
				driveType = WAPI_DRIVE_TYPE_PTB;
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
	
	if(cmd[0] != 't') {   /* Let us reserve first letter t for test cases. If it is not a test case, do not call slot reset inside siInitialize */
		terSetWapiTestMode(id, 1);
	}
	
 	rc = siInitialize(id);
	if( rc != 0 ) {
		printf("ERROR: siInitialize failed [rc=%s] \n", strSiInitError[rc]);
		return 1;
	}
   
	rc = testApi(cmd, (Byte)id, p);
	
	siFinalize(id);

	return rc;
}


#ifdef __cplusplus
};
#endif

