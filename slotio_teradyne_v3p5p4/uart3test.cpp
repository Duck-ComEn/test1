#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "tcTypes.h"
#include "common.h"
#include "commonEnums.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libu3.h"
#include "libsi.h"

void PrintHelp();

void siUnitTestAssert(char* testcase, bool expected, Dword &dwTestPassed,
		Dword &dwTestFailed, struct timeval TimeAtCheckPoint,
		struct timeval TimeAtStart) {
	static char bTraceMessage[512];
	struct timeval TimeAtNow;
	gettimeofday(&TimeAtNow, NULL);

	if (expected) {
		dwTestPassed++;
	} else {
		dwTestFailed++;
	}

	sprintf(
			bTraceMessage,
			"Test#%ld,%s,%s,L:%04d,%ldms,%ldms,`%s`",
			(dwTestPassed + dwTestFailed),
			expected ? "Passed" : "Failed",
			__FILE__,
			__LINE__,
			Common::Instance.GetElapsedTimeInMillSec(TimeAtCheckPoint, TimeAtNow),
			Common::Instance.GetElapsedTimeInMillSec(TimeAtStart, TimeAtNow),
			testcase
			);
	printf("%s\n", bTraceMessage);

	if (!expected) {
		printf("Press any key to continue ...\n");
		getchar();
	}
}

void siUnitTestUpdateLenghFieldInCDB(Byte* cdb, Dword length) {
	cdb[6] = (length >> 16) & 0xff;
	cdb[7] = (length >> 8) & 0xff;
	cdb[8] = length & 0xff;
}

Byte siUnitTestResetDriveState(bool doPowerOff, bool doPowerOn,
		bool doChangeToUart3, int slotid, Word v5Supply, Word v12Supply,
		Dword& dwTestFailed, CCB_DEBUG ccb_obj_prototype, Byte* ccb_data,
		Dword uartBaudRate, Dword uartLineSped, Dword uart3BaudRate,
		Word uartPullUpVoltage) {
	Byte byte_rc = success;
	long long_rc = 0;
	CCB_DEBUG ccb_obj;
	printf("Reset Drive State\n");
	if (doPowerOff) {
		printf("Turning off drive power\n");
		if ((byte_rc = setTargetVoltage(slotid, 0, 0)) != success) {
			printf("Error: while turning off drive 0x%x\n", byte_rc);
			dwTestFailed = 0xFFFFFFFF;
			return byte_rc;
		}
		printf("Sleeping for 10 seconds\n");
		sleep(10);
		byte_rc = clearCellEnvironmentError(slotid);
	}
	if (doPowerOn) {
		printf("Turning on drive power\n");
		if ((byte_rc = siSetUartBaudrate(slotid, uartBaudRate)) != success) {
			printf("Error: while setting baudrate 0x%x\n", byte_rc);
			dwTestFailed = 0xFFFFFFFF;
			return byte_rc;
		}
		if ((byte_rc = setUartPullupVoltage(slotid, uartPullUpVoltage)) != success) {
			printf("Error: while setting uart pullup voltage 0x%x\n", byte_rc);
			dwTestFailed = 0xFFFFFFFF;
			return byte_rc;
		}
		if ((byte_rc = setTargetVoltage(slotid, v5Supply, v12Supply))
				!= success) {
			printf("Error: while turning on drive 0x%x\n", byte_rc);
			dwTestFailed = 0xFFFFFFFF;
			return byte_rc;
		}
		printf("Sleeping for 10 seconds\n");
		sleep(10);
	}

	if (doChangeToUart3) {
		printf("Changing drive to UART 3 mode\n");
		// Change the drive mode to be UART 3
		if ((byte_rc = siChangeToUart3(slotid, uartLineSped, 1 * 1000))
				!= success) {
			printf("Error: Change to UART3 0x%x\n", byte_rc);
			dwTestFailed = 0xFFFFFFFF;
			return byte_rc;
		}
		// Change the Baudrate to be uart3BaudRate
		if ((byte_rc = siChangeToUart3_debug(slotid, uart3BaudRate, 1 * 1000)) != uart3AckReady){
			printf("Error: Change to UART3 (Debug) 0x%x\n", byte_rc);
			dwTestFailed = 0xFFFFFFFF;
			return byte_rc;
		}
	}

	printf("Setting Inter-Segment delay of 0us\n");
	if ((byte_rc = siSetUartInterSegmentDelay(slotid, 0)) != uart3AckReady) {
		printf("Error: Setting Inter-segment delay 0x%x\n", byte_rc);
		dwTestFailed = 0xFFFFFFFF;
		return byte_rc;
	}

	printf("execCCBCommand to read buffer - 512 bytes\n");
	ccb_obj = ccb_obj_prototype;
	ccb_obj.read = 512;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	long_rc = execCCBCommand(&ccb_obj, ccb_data);
	if (long_rc != uart3AckSCSICommandComplete) {
		printf("error\n");
		dwTestFailed = 0xFFFFFFFF;
		return long_rc;
	}

	printf("execCCBCommand to read buffer - 512 bytes\n");
	ccb_obj = ccb_obj_prototype;
	ccb_obj.read = 512;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	long_rc = execCCBCommand(&ccb_obj, ccb_data);
	if (long_rc != uart3AckSCSICommandComplete) {
		printf("error\n");
		dwTestFailed = 0xFFFFFFFF;
		return long_rc;
	}
	printf("Exit: %s\n", __func__);
	return success;
}

Byte Uart3BatchTest(int slotid, int v5Supply, int v12Supply, Dword uartBaudRate,
		Dword uart3BaudRate, Word uartPullupVoltage) {
#define siUnitTestAssertSkip(testcase, expected)			\
  if (1) {								\
    dwTestSkipped++;							\
									\
    sprintf(bTraceMessage, "Test#xxx,Skipped,%s,L:%04d,`%s`",	\
	    __FILE__,							\
	    __LINE__,							\
	    testcase							\
	    );								\
									\
    printf("%s\n", bTraceMessage);				\
  }
  
#define siUnitTestResetReqParameters()		\
  if (1) {					\
    byte_rc = 1;				\
    DriveStateData = DriveStateData_prototype;	\
    dwTimeoutInMillSec = 1000;			\
  }

#define siUnitTestResetLinkParameters()		\
  if (1) {					\
    byte_rc = 1;				\
    bReqRespOpcode = uart3OpcodeQueryCmd;	\
    bFlagByte = 0;				\
    wPtrDataLength = 0;				\
    wDataSequence = 0;				\
    DataBuffer = DataBuffer_Save;		\
    Timeout_mS = 1000;				\
  }

	printf("Entry:%s\n", __func__);
	// variables for global
	Dword i = 0;
	Dword x = 0, y = 0, z = 0, w = 0, t = 0;
	Dword dwTestPassed = 0;
	Dword dwTestFailed = 0;
	Dword dwTestSkipped = 0;
	struct timeval TimeAtStart;
	struct timeval TimeAtCheckPoint;
	struct timeval TimeAtNow;
	char bTraceMessage[512];

	Word uartLineSpeed = 0;
	switch(uartBaudRate) {
	case 115200:
		uartLineSpeed = lineSpeed115200;
		break;
	case 1843200:
		uartLineSpeed = lineSpeed1843200;
		break;
	default:
		uartLineSpeed = lineSpeed115200;
		break;
	}
//	Dword uart3BaudRate = 8333000;

	// variables for execCCBCommand()
	long long_rc = 0;
	CCB_DEBUG ccb_obj_prototype_requestsense = { 6, 0, 0, { 0x03, 0x00, 0x00,
			0x00, MAXSENSELENGTH, 0x00 }, { 0 }, { 0 }, 0, MAXSENSELENGTH, 0,
			480 * 1000, 0 };
	CCB_DEBUG ccb_obj_prototype_startstopunit = { 6, 0, 0, { 0x1B, 0x00, 0x00,
			0x00, 0x00, 0x00 }, { 0 }, { 0 }, 0, 0, 0, 480 * 1000, 0 };
	CCB_DEBUG ccb_obj_prototype_writebuffer = { 10, 0, 0, { 0x3B, 0x02, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, { 0 }, { 0 }, 0, 0, 0,
			480 * 1000, 0 };
	CCB_DEBUG ccb_obj_prototype_readbuffer = { 10, 0, 0, { 0x3C, 0x02, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, { 0 }, { 0 }, 0, 0, 0,
			480 * 1000, 0 };
	CCB_DEBUG ccb_obj_prototype = { 10, 0, 0, { 0 }, { 0 }, { 0 }, 0, 0, 0, 480
			* 1000, 0 };
	CCB_DEBUG ccb_obj;
	Dword ccb_data_maxsize = 2 * 1024 * 1024;
	Byte *ccb_data = new Byte[2 * 1024 * 1024];

	// variables for siReq
	Byte byte_rc = 1;
	GET_DRIVE_STATE_DATA DriveStateData_prototype = { 0x1234, 0x1234, 0x1234,
			0x1234, 0x1234, 0x1234, 0x1234, 0x1234, 0x12345678 };
	GET_DRIVE_STATE_DATA DriveStateData;
	Word dwTimeoutInMillSec = 0;

	// variables for siLink
	Byte bReqRespOpcode = 0;
	Byte bFlagByte = 0;
	Word wPtrDataLength = 0;
	Word wDataSequence = 0;
	Byte *DataBuffer_Save = new Byte[0xffff];
	Byte *DataBuffer = DataBuffer_Save;
	Dword Timeout_mS = 0;

	/* start timer */
	gettimeofday(&TimeAtStart, NULL);

	/* start test for execCCBCommand() */
	ccb_obj_prototype_requestsense.bCellNo = slotid;
	ccb_obj_prototype_startstopunit.bCellNo = slotid;
	ccb_obj_prototype_writebuffer.bCellNo = slotid;
	ccb_obj_prototype_readbuffer.bCellNo = slotid;
	ccb_obj_prototype.bCellNo = slotid;

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 1
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(NULL, &ccb_data[0]);
	siUnitTestAssert("INPUT: cb == NULL, OUTPUT: rc == nullArgumentError",
			long_rc == nullArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 2
	ccb_obj = ccb_obj_prototype;
	ccb_obj.read = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssert(
			"INPUT: (data == NULL) && (read == 1), OUTPUT: rc == nullArgumentError",
			long_rc == nullArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 3
	ccb_obj = ccb_obj_prototype;
	ccb_obj.write = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssert(
			"INPUT: (data == NULL) && (write == 1), OUTPUT: rc == nullArgumentError",
			long_rc == nullArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 4
	ccb_obj = ccb_obj_prototype;
	ccb_obj.read = 1;
	ccb_obj.write = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: (read == 1) && (write == 1), OUTPUT: rc == invalidArgumentError",
			long_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 5
	ccb_obj = ccb_obj_prototype_startstopunit;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssert(
			"INPUT: (read == 0) && (write == 0), OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed,
			TimeAtCheckPoint, TimeAtStart);

	// Test # 6
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = 1;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: read == 1, OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed,
			TimeAtCheckPoint, TimeAtStart);

	// Test # 7
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = ccb_data_maxsize;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert("INPUT: read == ccb_data_maxsize, OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 8
	ccb_obj = ccb_obj_prototype;
	ccb_obj.read = ccb_data_maxsize + 1;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: read == ccb_data_maxsize + 1, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 9
	ccb_obj = ccb_obj_prototype;
	ccb_obj.read = 0xFFFFFFFF;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: read == 0xFFFFFFFF, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 10
	ccb_obj = ccb_obj_prototype_writebuffer;
	ccb_obj.write = 1;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert("INPUT: write == 1, OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 11
	ccb_obj = ccb_obj_prototype_writebuffer;
	ccb_obj.write = ccb_data_maxsize;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert("INPUT: write == ccb_data_maxsize, OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 12
	ccb_obj = ccb_obj_prototype;
	ccb_obj.write = ccb_data_maxsize + 1;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: write == ccb_data_maxsize + 1, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 13
	ccb_obj = ccb_obj_prototype;
	ccb_obj.write = 0xFFFFFFFF;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: write == 0xFFFFFFFF, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	gettimeofday(&TimeAtCheckPoint, NULL);
	if (ccb_data_maxsize % 4) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 14
	ccb_obj = ccb_obj_prototype_writebuffer;
	ccb_obj.write = ccb_data_maxsize;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
	for (i = 0, x = 123456789, y = 362436069, z = 521288629, w = 88675123, t = 0;
			i < ccb_data_maxsize; i += 4) {
		t = (x ^ (x << 11));
		x = y;
		y = z;
		z = w;
		w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
		*((Dword *) &ccb_data[i]) = w;
		if ((i % 4096) == 0) {
			sleep(1);
		}
	}
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	if (long_rc != uart3AckSCSICommandComplete) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}

	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = ccb_data_maxsize;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	for (i = 0; i < ccb_data_maxsize; i++) {
		ccb_data[i] = 0x00;
		if ((i % 4096) == 0) {
			usleep(1*1000);
		}
	}

	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	if (long_rc != uart3AckSCSICommandComplete) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}

	for (i = 0, x = 123456789, y = 362436069, z = 521288629, w = 88675123, t = 0;
			i < ccb_data_maxsize; i += 4) {
		t = (x ^ (x << 11));
		x = y;
		y = z;
		z = w;
		w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
		if (*((Dword *) &ccb_data[i]) != w) {
			break;
		}
		if ((i % 4096) == 0) {
			usleep(1*1000);
		}
	}
	siUnitTestAssert(
			"INPUT: write = read = ccb_data_maxsize, OUTPUT: data compare match",
			i == ccb_data_maxsize, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 15
	ccb_obj = ccb_obj_prototype;
	ccb_obj.cdblength = 0;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: cdblength == 0, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 16
	ccb_obj = ccb_obj_prototype_startstopunit;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssert("INPUT: cdblength == 6, OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	siUnitTestAssertSkip(
			"INPUT: cdblength == MAXCDB(32), OUTPUT: rc == success [DEBUG] wait for implementation",
			0);

	// Test # 17
	ccb_obj = ccb_obj_prototype;
	ccb_obj.cdblength = MAXCDBLENGTH + 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: cdblength = MAXCDB + 1, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 18
	ccb_obj = ccb_obj_prototype;
	ccb_obj.cdblength = 0xFF;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: cdblength = 0xFF, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 19
	ccb_obj = ccb_obj_prototype_startstopunit;
	ccb_obj.senselength = 0;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssert(
			"INPUT: senselength == 0, OUTPUT: (rc == uart3AckSCSICommandComplete) && (senselength == 0)",
			(long_rc == uart3AckSCSICommandComplete)
					&& (ccb_obj.senselength == 0), dwTestPassed, dwTestFailed,
			TimeAtCheckPoint, TimeAtStart);

	// Test # 20
	ccb_obj = ccb_obj_prototype_requestsense;
	ccb_obj.read = 0xff;
	ccb_obj.cdb[4] = 0xff;
	ccb_obj.senselength = 0;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssertSkip(
			"INPUT: senselength == 0, OUTPUT: (rc == rc_FailWithSense) && (senselength == MAXSENSELENGTH) [DEBUG] wait for implementation",
			(long_rc == rc_FailWithSense) && (ccb_obj.senselength == MAXSENSELENGTH));

	// Test # 21
	ccb_obj = ccb_obj_prototype_startstopunit;
	ccb_obj.senselength = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssertSkip(
			"INPUT: senselength == 1, OUTPUT: (rc == success) && (senselength == 1) [DEBUG] wait for implementation",
			(long_rc == success) && (ccb_obj.senselength == 1));

	// Test # 22
	ccb_obj = ccb_obj_prototype_requestsense;
	ccb_obj.read = 0xff;
	ccb_obj.cdb[4] = 0xff;
	ccb_obj.senselength = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssertSkip(
			"INPUT: senselength == 1, OUTPUT: (rc == rc_FailWithSense) && (senselength == 1) [DEBUG] wait for implementation",
			(long_rc == rc_FailWithSense) && (ccb_obj.senselength == 1));

	// Test # 23
	ccb_obj = ccb_obj_prototype_startstopunit;
	ccb_obj.senselength = MAXSENSELENGTH;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssertSkip(
			"INPUT: senselength == MAXSENSELENGTH, OUTPUT: (rc == rc_SuccessWithSense) && (senselength == MAXSENSELENGTH) [DEBUG] wait for implementation",
			(long_rc == success) && (ccb_obj.senselength == MAXSENSELENGTH));

	// Test # 24
	ccb_obj = ccb_obj_prototype_requestsense;
	ccb_obj.read = 0xff;
	ccb_obj.cdb[4] = 0xff;
	ccb_obj.senselength = MAXSENSELENGTH;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, NULL);
	siUnitTestAssertSkip(
			"INPUT: senselength == MAXSENSELENGTH, OUTPUT: (rc == rc_FailWithSense) && (senselength == MAXSENSELENGTH) [DEBUG] wait for implementation",
			(long_rc == rc_FailWithSense) && (ccb_obj.senselength == MAXSENSELENGTH));

	// Test # 25
	ccb_obj = ccb_obj_prototype;
	ccb_obj.senselength = MAXSENSELENGTH + 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: senselength = MAXSENSELENGTH + 1, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 26
	ccb_obj = ccb_obj_prototype;
	ccb_obj.senselength = 0xFF;
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert("INPUT: senselength = 0xFF, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	siUnitTestAssertSkip(
			"INPUT: senselength == 1, OUTPUT: (rc == rc_FailWithSense) && (senselength == 0(error on requestsense)) [DEBUG] wait for implementation",
			0);

	// Test # 27
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = ccb_data_maxsize / 32;
	ccb_obj.timeout = 0 * 1000;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert("INPUT: timeout == 0 * 1000, OUTPUT: rc == outOfRange",
			long_rc == outOfRange, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 28
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = ccb_data_maxsize / 32;
	ccb_obj.timeout = 1 * 1000;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert(
			"INPUT: timeout == 1 * 1000, OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed,
			TimeAtCheckPoint, TimeAtStart);

	// Test # 29
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = ccb_data_maxsize / 32;
	ccb_obj.timeout = 10 * 1000;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	gettimeofday(&TimeAtCheckPoint, NULL);
	long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
	siUnitTestAssert("INPUT: timeout == 10 * 1000, OUTPUT: rc == uart3AckSCSICommandComplete",
			long_rc == uart3AckSCSICommandComplete, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	siUnitTestAssertSkip(
			"INPUT: timeout == 0xFFFFFFFF, OUTPUT: rc == success(command takes 3600,000ms) [DEBUG] wait for implementation",
			0);


  /* start test for execCCBCommand (Performance) */
#define READ_SIZE_FOR_TEST      (6 * 1024 * 1024)
#define WRITE_SIZE_FOR_TEST     (16 * 1024 * 1024)
#define CHUNK_SIZE_FOR_TEST     (64 * 1024)

	// Test # 30
	gettimeofday(&TimeAtCheckPoint, NULL);
	for (i = 0; i < WRITE_SIZE_FOR_TEST / CHUNK_SIZE_FOR_TEST; i++) {
		ccb_obj = ccb_obj_prototype_writebuffer;
		ccb_obj.write = CHUNK_SIZE_FOR_TEST;
		siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
		long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
		if (long_rc != uart3AckSCSICommandComplete) {
			break;
		}
	}
	if (long_rc == uart3AckSCSICommandComplete) {
		for (i = 0; i < READ_SIZE_FOR_TEST / CHUNK_SIZE_FOR_TEST; i++) {
			ccb_obj = ccb_obj_prototype_readbuffer;
			ccb_obj.read = CHUNK_SIZE_FOR_TEST;
			siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
			long_rc = execCCBCommand(&ccb_obj, &ccb_data[0]);
			if (long_rc != uart3AckSCSICommandComplete) {
				break;
			}
		}
	}
	gettimeofday(&TimeAtNow, NULL);
	siUnitTestAssert(
			"INPUT: performance (22MBytes), OUTPUT: rc == uart3AckSCSICommandComplete, elapsed < 90 sec",
			(long_rc == uart3AckSCSICommandComplete)
					&& (Common::Instance.GetElapsedTimeInMillSec(
							TimeAtCheckPoint, TimeAtNow) < (90 * 1000)),
			dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);
  /* start test for si***Request() */

	// Test # 31
	siUnitTestResetReqParameters();
	byte_rc = siGetDriveStateRequest(slotid, &DriveStateData, 0);
	siUnitTestAssert(
			"INPUT: siGetDriveStateRequest timeout = 0, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 32
	siUnitTestResetReqParameters();
	byte_rc = siGetDriveStateRequest(slotid, NULL, dwTimeoutInMillSec);
	siUnitTestAssert(
			"INPUT: siGetDriveStateRequest timeout = 0, OUTPUT: rc == nullArgumentError",
			byte_rc == nullArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 33
	siUnitTestResetReqParameters();
	byte_rc = siGetDriveStateRequest(slotid, &DriveStateData,
			dwTimeoutInMillSec);
	siUnitTestAssert(
			"INPUT: siGetDriveStateRequest, OUTPUT: rc == uart3AckDataAvailable_DriveRetainsControl, version == 0x0001",
			(byte_rc == uart3AckDataAvailable_DriveRetainsControl)
					&& (DriveStateData.wDriveStateVersion == 0x0001),
			dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	/* start test for siUart3LinkControl() */
	// Test # 34
	siUnitTestResetLinkParameters();
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(0, bReqRespOpcode, bFlagByte,
			&wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert("INPUT: bCellNo = 0, OUTPUT: rc == invalidSlotIndex",
			byte_rc == invalidSlotIndex, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 35
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeQueryCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert("INPUT: bCellNo == (this), OUTPUT: rc == uart3AckReady",
			byte_rc == uart3AckReady, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 36
	siUnitTestResetLinkParameters();
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(241, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert("INPUT: bCellNo = 241, OUTPUT: rc == invalidSlotIndex",
			byte_rc == invalidSlotIndex, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 37
	siUnitTestResetLinkParameters();
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(255, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert("INPUT: bCellNo = 255, OUTPUT: rc == invalidSlotIndex",
			byte_rc == invalidSlotIndex, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 38
	siUnitTestResetLinkParameters();
	bReqRespOpcode = 0x00;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode = 0x00, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 39
	siUnitTestResetLinkParameters();
	bReqRespOpcode = 0x3F;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode = 0x3F, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 40
	siUnitTestResetLinkParameters();
	bReqRespOpcode = 0x4B;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode = 0x4B, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 41
	siUnitTestResetLinkParameters();
	bReqRespOpcode = 0x4C;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode = 0x4C, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 42
	siUnitTestResetLinkParameters();
	bReqRespOpcode = 0x4D;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode = 0x4D, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 43
	siUnitTestResetLinkParameters();
	bReqRespOpcode = 0x50;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode = 0x50, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 44
	siUnitTestResetLinkParameters();
	bReqRespOpcode = 0xFF;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode = 0xFF, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 45
	siUnitTestResetLinkParameters();
	bFlagByte = 0;
	bReqRespOpcode = uart3OpcodeQueryCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (bFlagByte == 0) && (bReqRespOpcode == uart3OpcodeQueryCmd), OUTPUT: rc == uart3AckReady",
			byte_rc == uart3AckReady, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 46
	siUnitTestResetLinkParameters();
	bFlagByte = 1;
	bReqRespOpcode = uart3OpcodeQueryCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (bFlagByte == 1) && (bReqRespOpcode == uart3OpcodeQueryCmd), OUTPUT: rc == uart3AckReady",
			byte_rc == uart3AckReady, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 47
	siUnitTestResetLinkParameters();
	bFlagByte = 2;
	bReqRespOpcode = uart3OpcodeQueryCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (bFlagByte == 2) && (bReqRespOpcode == uart3OpcodeQueryCmd), OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 48
	siUnitTestResetLinkParameters();
	bFlagByte = 255;
	bReqRespOpcode = uart3OpcodeQueryCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (bFlagByte == 255) && (bReqRespOpcode == uart3OpcodeQueryCmd), OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 49
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeReadDataCmd;
	wPtrDataLength = 16;
	DataBuffer = NULL;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (wPtrDataLength == 16) && (DataBuffer == NULL), OUTPUT: rc == nullArgumentError",
			byte_rc == nullArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 50
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeReadDataCmd;
	wDataSequence = 0;
	wPtrDataLength = 16;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (wDataSequence == 0) && (wPtrDataLength = 16), OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	if ((byte_rc = siUnitTestResetDriveState(0, 0, 0, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 51
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = 512;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSCSICmd;
	wPtrDataLength = ccb_obj.cdblength;
	DataBuffer = &ccb_obj.cdb[0];
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	if (byte_rc != uart3DriveBusyWithDataReceived) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 52
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeReadDataCmd;
	wPtrDataLength = ccb_obj.read;
	wDataSequence = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (wDataSequence == 1) && (wPtrDataLength == 512), OUTPUT: (rc == uart3AckDataAvailable_DriveRetainsControl) && (wPtrDataLength == 512)",
			(byte_rc == uart3AckDataAvailable_DriveRetainsControl)
					&& (wPtrDataLength == 512), dwTestPassed, dwTestFailed,
			TimeAtCheckPoint, TimeAtStart);

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 53
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = 16 * 1024;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSCSICmd;
	wPtrDataLength = ccb_obj.cdblength;
	DataBuffer = &ccb_obj.cdb[0];
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	if (byte_rc != uart3DriveBusyWithDataReceived) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 54
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeReadDataCmd;
	wPtrDataLength = ccb_obj.read;
	wDataSequence = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: (wDataSequence == 1) && (wPtrDataLength == 16 * 1024), OUTPUT: (rc == uart3AckDataAvailable_DriveRetainsControl) && (wPtrDataLength == 16 * 1024)",
			(byte_rc == uart3AckDataAvailable_DriveRetainsControl)
					&& (wPtrDataLength == 16 * 1024), dwTestPassed,
			dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 55
	siUnitTestResetLinkParameters();
	wPtrDataLength = 16 * 1024 + 4;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssertSkip(
			"INPUT: wPtrDataLength = 16 * 1024 + 4, OUTPUT: rc == invalidArgumentError [DEBUG] wait for implementation",
			byte_rc == invalidArgumentError);

	// Test # 56
	siUnitTestResetLinkParameters();
	wPtrDataLength = 0xffff;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssertSkip(
			"INPUT: wPtrDataLength = 0xffff, OUTPUT: rc == invalidArgumentError [DEBUG] wait for implementation",
			byte_rc == invalidArgumentError);

	// Test # 57
	siUnitTestResetLinkParameters();
	Timeout_mS = 0;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: Timeout_mS == 0, OUTPUT: rc == invalidArgumentError",
			byte_rc == invalidArgumentError, dwTestPassed, dwTestFailed,
			TimeAtCheckPoint, TimeAtStart);

	// Test # 58
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSCSICmd;
	wPtrDataLength = ccb_obj_prototype_readbuffer.cdblength;
	DataBuffer = &ccb_obj_prototype_readbuffer.cdb[0];
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssertSkip(
			"INPUT: bReqRespOpcode == opcodeSCSICmdRequest, OUTPUT: rc == uart3DriveBusyWithDataReceived [DEBUG] wait for implementation",
			byte_rc == uart3DriveBusyWithDataReceived);

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 59
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeQueryCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode == uart3OpcodeQueryCmd, OUTPUT: rc == uart3AckReady",
			byte_rc == uart3AckReady, dwTestPassed, dwTestFailed,
			TimeAtCheckPoint, TimeAtStart);

	if ((byte_rc = siUnitTestResetDriveState(0, 0, 0, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 60
	ccb_obj = ccb_obj_prototype_readbuffer;
	ccb_obj.read = 512;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.read);
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSCSICmd;
	wPtrDataLength = ccb_obj.cdblength;
	DataBuffer = &ccb_obj.cdb[0];
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	if (byte_rc != uart3DriveBusyWithDataReceived) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeReadDataCmd;
	wPtrDataLength = ccb_obj.read;
	wDataSequence = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode == opcodeReadDataRequest, OUTPUT: rc == uart3AckDataAvailable_DriveRetainsControl",
			byte_rc == uart3AckDataAvailable_DriveRetainsControl, dwTestPassed,
			dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 62
	ccb_obj = ccb_obj_prototype_writebuffer;
	ccb_obj.write = 512;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSCSICmd;
	wPtrDataLength = ccb_obj.cdblength;
	DataBuffer = &ccb_obj.cdb[0];
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	if (byte_rc != uart3DriveBusyWithDataReceived) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeWriteDataCmd;
	wPtrDataLength = ccb_obj.write;
	wDataSequence = 1;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode == opcodeWriteDataRequest, OUTPUT: rc == uart3DriveBusyWithDataReceived",
			byte_rc == uart3DriveBusyWithDataReceived, dwTestPassed,
			dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 63
	ccb_obj = ccb_obj_prototype_writebuffer;
	ccb_obj.write = 512;
	siUnitTestUpdateLenghFieldInCDB(ccb_obj.cdb, ccb_obj.write);
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSCSICmd;
	wPtrDataLength = ccb_obj.cdblength;
	DataBuffer = &ccb_obj.cdb[0];
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	if (byte_rc != uart3DriveBusyWithDataReceived) {
		printf("error\n");
		dwTestFailed = LONG_MAX;
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 64
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeAbortSCSICmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssertSkip(
			"INPUT: bReqRespOpcode == opcodeAbortSCSICmdRequest, OUTPUT: rc == ??????? [DEBUG] need to define expected output",
			0);

	// Test # 65
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeGetUARTParametersCmd;
	wPtrDataLength = 24;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode == opcodeGetUARTParametersRequest, OUTPUT: rc == uart3AckDataAvailable_DriveRetainsControl",
			byte_rc == uart3AckDataAvailable_DriveRetainsControl, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 66
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSetUARTParametersCmd;
	wPtrDataLength = 24;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode == opcodeSetUARTParametersRequest, OUTPUT: rc == uart3AckReady",
			byte_rc == uart3AckReady, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 67
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeSetUART2ModeCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssertSkip(
			"INPUT: bReqRespOpcode == opcodeSetUART2Mode, OUTPUT: rc == uart3AckReady [DEBUG] wait for implementation",
			byte_rc == uart3AckReady);

	if ((byte_rc = siUnitTestResetDriveState(1, 1, 1, slotid, v5Supply,
			v12Supply, dwTestFailed, ccb_obj_prototype_readbuffer, ccb_data,
			uartBaudRate, uartLineSpeed, uart3BaudRate, uartPullupVoltage))
			!= success) {
		goto SI_UNIT_TEST_MAIN_END;
	}

	// Test # 65
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeMemoryReadCmd;
	wPtrDataLength = 512;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssertSkip(
			"INPUT: bReqRespOpcode == opcodeMemoryRead, OUTPUT: rc == uart3AckDataAvailable_DriveRetainsControl [DEBUG] wait for implementation",
			byte_rc == uart3AckDataAvailable_DriveRetainsControl);

	if (0) {
		siUnitTestResetLinkParameters();
		bReqRespOpcode = uart3OpcodeMemoryWriteCmd;
		wPtrDataLength = 512;
		gettimeofday(&TimeAtCheckPoint, NULL);
		byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
				bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer,
				Timeout_mS);
		siUnitTestAssert(
				"INPUT: bReqRespOpcode == opcodeMemoryWrite, OUTPUT: rc == uart3AckReady",
				byte_rc == uart3AckReady, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);
	} else {
		siUnitTestAssertSkip(
				"INPUT: bReqRespOpcode == opcodeMemoryWrite, OUTPUT: rc == uart3AckReady [DEBUG] need special drive",
				0);
	}

	// Test # 66
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeEchoCmd;
	wPtrDataLength = 56;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssertSkip(
			"INPUT: bReqRespOpcode == opcodeEcho, OUTPUT: rc == uart3AckDataAvailable_DriveRetainsControl [DEBUG] wait for implementation",
			byte_rc == uart3AckDataAvailable_DriveRetainsControl);

	// Test # 67
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeGetDriveStateCmd;
	wPtrDataLength = sizeof(GET_DRIVE_STATE_DATA);
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode == opcodeGetDriveStateRequest, OUTPUT: rc == uart3AckDataAvailable_DriveRetainsControl",
			byte_rc == uart3AckDataAvailable_DriveRetainsControl, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	// Test # 68
	siUnitTestResetLinkParameters();
	bReqRespOpcode = uart3OpcodeDriveResetCmd;
	gettimeofday(&TimeAtCheckPoint, NULL);
	byte_rc = Common::Instance.Uart3LinkControl(slotid, bReqRespOpcode,
			bFlagByte, &wPtrDataLength, wDataSequence, DataBuffer, Timeout_mS);
	siUnitTestAssert(
			"INPUT: bReqRespOpcode == opcodeDriveResetRequest, OUTPUT: rc == uart3AckReady",
			byte_rc == uart3AckReady, dwTestPassed, dwTestFailed, TimeAtCheckPoint, TimeAtStart);

	/* finalize */
SI_UNIT_TEST_MAIN_END:

	setTargetVoltage(slotid, 0, 0);
	gettimeofday(&TimeAtNow, NULL);
	printf(
			"Summary : Total = %ld, Pass = %ld, Fail = %ld, Skip = %ld Elapsed = %ldms\n",
			dwTestPassed + dwTestFailed, dwTestPassed, dwTestFailed,
			dwTestSkipped, Common::Instance.GetElapsedTimeInMillSec(TimeAtStart, TimeAtNow));

	printf("Exit:%s\n", __func__);

	if (ccb_data) {
		delete[] ccb_data;
	}
	if (DataBuffer_Save) {
		delete[] DataBuffer_Save;
	}

	if (dwTestFailed > 0xff) {
		return 0xff;
	}
	return 0xff & dwTestFailed;
}
void PrintUartParams (GETSETU3PARAM *parameterData)
{                			
	printf("UART Parameters Data: \n");
        printf("UART Version: 0x%x\n", (parameterData->bUartProtocolVersionH << 8) + parameterData->bUartProtocolVersionL);
        printf("UART Line Speed: 0x%x\n", (parameterData->bUartLineSpeedH << 8) + parameterData->bUartLineSpeedL);
        printf("UART Ack Delay: %d usec\n",
		(parameterData->bAckDelayTimeInMicroSecondsH << 8) + parameterData->bAckDelayTimeInMicroSecondsL);
	printf("Max Write Block Size: %d bytes\n",
		(parameterData->bMaxBlockSizeForWriteHH << 24) + (parameterData->bMaxBlockSizeForWriteHL << 16) +
		(parameterData->bMaxBlockSizeForWriteLH << 8) + parameterData->bMaxBlockSizeForWriteLL);
	printf("Max Read Block Size: %d bytes\n",
		(parameterData->bMaxBlockSizeForReadHH << 24) + (parameterData->bMaxBlockSizeForReadHL << 16) +
		(parameterData->bMaxBlockSizeForReadLH << 8) + parameterData->bMaxBlockSizeForReadLL);
        printf("DMA Timeout: %d msec\n",
		(parameterData->bDMAtimeoutInMilliSecondsH << 8) + parameterData->bDMAtimeoutInMilliSecondsL);
	printf("FIFO Timeout: %d msec\n",
		(parameterData->bFIFOtimeoutInMilliSecondsH << 8) + parameterData->bFIFOtimeoutInMilliSecondsL);
        printf("Transmit Segment Size: %d bytes\n",
		(parameterData->bTransmitSegmentSizeHH << 24) + (parameterData->bTransmitSegmentSizeHL << 16) +
		(parameterData->bTransmitSegmentSizeLH << 8) + parameterData->bTransmitSegmentSizeLL);
	printf("Inter-Segment Delay: %d usec\n",
		(parameterData->bInterSegmentDelayInMicroSecondsH << 8) + parameterData->bInterSegmentDelayInMicroSecondsL);
}

Byte Uart3PerformanceTest(Byte bCellNo, Dword uartBaudRate,
		Dword uart3BaudRate, int dmaTimeout, bool writeVerify) {

	Byte rc = success;
	struct timeval start, end;
	Dword dwMainLoopCount = 1;
	CCB_DEBUG dcb = { 6, 0, 0, { 0x03, 0x00, 0x00,
						0x00, MAXSENSELENGTH, 0x00 }, { 0 }, { 0 }, 0, MAXSENSELENGTH, 0,
						480 * 1000, 0 },
			  ucb = { 6, 0, 0, { 0x03, 0x00, 0x00,
						0x00, MAXSENSELENGTH, 0x00 }, { 0 }, { 0 }, 0, MAXSENSELENGTH, 0,
						480 * 1000, 0 };
	const Dword dwDownloadSize = 6 * 1024 * 1024;
	const Dword dwUploadSize   = 16 * 1024 * 1024;
	Dword dwTimeout = 480 * 1000;
	Byte bufDownload[MAX_DATA_BUFFER_LENGTH];
	Byte bufCompare[MAX_DATA_BUFFER_LENGTH];

	GETSETU3PARAM parameterData;
	Word size = 0;


	Word uartLineSpeed = 0;
	switch(uartBaudRate) {
	case 115200:
		uartLineSpeed = lineSpeed115200;
		break;
	case 1843200:
		uartLineSpeed = lineSpeed1843200;
		break;
	default:
		uartLineSpeed = lineSpeed115200;
		break;
	}

	if ((rc = siChangeToUart3(bCellNo, uartLineSpeed, 10 * 1000)) != success) {
		printf("Error: Change to UART3 0x%x Retrying... \n", rc);
		sleep (1);
		if ((rc = siChangeToUart3(bCellNo, uartLineSpeed, 10 * 1000)) != success) {
			printf("Error: Change to UART3 0x%x Retries exhausted \n", rc);
			return rc;
		}
	}
	printf("siChangeToUart3 Passed\n");
/*
	// Set new ack timeout
	if ((rc = siSetDriveResponseDelayTimeForAck(bCellNo, 1000)) != uart3AckReady){
		printf("Error: Ack Timeout change 0x%x  Retrying... \n", rc);
		sleep (1);
		if ((rc = siSetDriveResponseDelayTimeForAck(bCellNo, 1000)) != uart3AckReady){
			printf("Error: Ack Timeout change 0x%x  Retries exhausted... \n", rc);
			return rc;
		}
	}
	printf("Ack timeout change Passed\n");
*/

	// Change the Baudrate to be uart3BaudRate
	if ((rc = siChangeToUart3_debug(bCellNo, uart3BaudRate, 10 * 1000)) != uart3AckReady){
		printf("Error: Change to UART3 (Debug) 0x%x  Retrying... \n", rc);
		sleep (1);
		if ((rc = siChangeToUart3_debug(bCellNo, uart3BaudRate, 10 * 1000)) != uart3AckReady){
			printf("Error: Change to UART3 (Debug) 0x%x  Retries exhausted... \n", rc);
			return rc;
		}
	}
	printf("siChangeToUart3_debug Passed\n");

	if ((rc = siSetUartInterSegmentDelay(bCellNo, 0)) != uart3AckReady) {
		printf("Error: Setting Inter-segment delay 0x%x  Retrying... \n", rc);
		sleep (1);
		if ((rc = siSetUartInterSegmentDelay(bCellNo, 0)) != uart3AckReady) {
			printf("Error: Setting Inter-segment delay 0x%x Retries exhausted... \n", rc);
			return rc;
		}
	}
	printf("siSetUartInterSegmentDelay Passed\n");

	/*======================== */
	size = sizeof(parameterData);
	rc = siGetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
	if (rc == uart3AckDataAvailable_DriveRetainsControl) {
		printf("Perf test current DMA Timeout: %d, requested %d\n", (parameterData.bDMAtimeoutInMilliSecondsH << 8) + parameterData.bDMAtimeoutInMilliSecondsL, dmaTimeout);
	} else {
		printf("siGetUARTParametersRequest Failed with error code 0x%x\n", rc);
	}
	parameterData.bDMAtimeoutInMilliSecondsH = (dmaTimeout >> 8) & 0xFF;
	parameterData.bDMAtimeoutInMilliSecondsL = dmaTimeout & 0xFF;

	rc = siSetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siSetUARTParametersRequest Passed\n");
		rc = siGetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
		if (rc == uart3AckDataAvailable_DriveRetainsControl) {
			printf("Perf test new DMA Timeout: %d msec\n", (parameterData.bDMAtimeoutInMilliSecondsH << 8) + parameterData.bDMAtimeoutInMilliSecondsL);
		} else {
			printf("siGetUARTParametersRequest Failed with error code 0x%x\n", rc);
		}
	} else {
		printf("siSetUARTParametersRequest Failed with error code 0x%x  Retrying... \n", rc);
		sleep (2);
		rc = siSetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
		if (rc != uart3AckReady) {
			printf("siSetUARTParametersRequest Failed with error code 0x%x  Retries exhausted... \n", rc);
			return rc;
		}
	}
/* ======================== */

	memset(bufDownload, 0xca, MAX_DATA_BUFFER_LENGTH);

	gettimeofday(&start, NULL);
	  // Test Loop
	for (Dword i = 0; i < dwMainLoopCount; i++) {

		// WRITE BUFFER Command Block Setup
		dcb.cdb[0] = 0x3B; // OPERATION CODE = WRITE BUFFER
		dcb.cdb[1] = 0x02; // MODE = Data
		dcb.cdb[6] = (Byte) ((MAX_DATA_BUFFER_LENGTH >> 16) & 0x0000ff);
		dcb.cdb[7] = (Byte) ((MAX_DATA_BUFFER_LENGTH >> 8) & 0x0000ff);
		dcb.cdb[8] = (Byte) ((MAX_DATA_BUFFER_LENGTH) & 0x0000ff);
		dcb.write = MAX_DATA_BUFFER_LENGTH;
		dcb.read = 0;
		dcb.timeout = dwTimeout;
		dcb.cdblength = 10;
		dcb.bCellNo = bCellNo;
		dcb.senselength = MAXSENSELENGTH;
		Dword dwDownloadIdx = 0;

		// READ BUFFER
		ucb.cdb[0] = 0x3C; // OPERATION CODE = READ BUFFER
		ucb.cdb[1] = 0x02; // MODE = Data
		ucb.cdb[6] = (Byte) ((MAX_DATA_BUFFER_LENGTH >> 16) & 0x0000ff);
		ucb.cdb[7] = (Byte) ((MAX_DATA_BUFFER_LENGTH >> 8) & 0x0000ff);
		ucb.cdb[8] = (Byte) ((MAX_DATA_BUFFER_LENGTH) & 0x0000ff);
		ucb.write = 0;
		ucb.read = MAX_DATA_BUFFER_LENGTH;
		ucb.timeout = dwTimeout;
		ucb.cdblength = 10;
		ucb.bCellNo = bCellNo;
		ucb.senselength = MAXSENSELENGTH;

		// WRITE BUFFER loop
		printf("Write Start\n");
		for (unsigned long ii=0;;ii++) {
			dcb.bytesTransferred = 0;
			if ((rc = execCCBCommand(&dcb, bufDownload)) == 0x26) {
		                printf ("rc from write execCCBCommand = 0x%x !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", rc);
				printf ("during %ldth of %ld loops, Press ENTER to continue\n", 
					ii, dwDownloadSize/MAX_DATA_BUFFER_LENGTH);
				rc = siGetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
		        	if (rc == uart3AckDataAvailable_DriveRetainsControl) {
		        		PrintUartParams (&parameterData);
	        		} else {
		           		printf("siGetUARTParametersRequest Failed with error code 0x%x\n", rc);
		        	}
			    	getchar();
        		}
			//  Verify the data we just wrote
			else if (writeVerify) {
				if (dcb.bytesTransferred != dcb.write)
					continue;
				if ( (rc != uart3AckSenseAvailable) && (rc != uart3AckQueueFull) ) {
					Byte rrc = success;
					memset(bufCompare, 0x0, MAX_DATA_BUFFER_LENGTH);
					ucb.bytesTransferred = 0;
					if ((rrc = execCCBCommand(&ucb, bufCompare)) != 0xf0) {
						printf ("read compare rc is 0x%x\n",rrc);getchar();
					}
					else {
						if (memcmp (bufCompare, bufDownload, MAX_DATA_BUFFER_LENGTH) != 0) {
							printf ("Write return code was 0x%x, Written/Read data mismatch, during %ldth of %ld loops, first byte 0x%x vs 0x%x, Press ENTER to continue\n",
							rc, ii, dwDownloadSize/MAX_DATA_BUFFER_LENGTH, bufCompare[0], bufDownload[0]);
			    				getchar();
						}
					}
				}
			}
			dwDownloadIdx += MAX_DATA_BUFFER_LENGTH;
			if (dwDownloadIdx < dwDownloadSize) {
				Dword dwRemainSize = dwDownloadSize - dwDownloadIdx;
				if (dwRemainSize < MAX_DATA_BUFFER_LENGTH) {
					dcb.cdb[6] = (Byte) ((dwRemainSize >> 16) & 0x0000ff);
					dcb.cdb[7] = (Byte) ((dwRemainSize >> 8) & 0x0000ff);
					dcb.cdb[8] = (Byte) ((dwRemainSize) & 0x0000ff);
					dcb.write = dwRemainSize;

					ucb.cdb[6] = (Byte) ((dwRemainSize >> 16) & 0x0000ff);
					ucb.cdb[7] = (Byte) ((dwRemainSize >> 8) & 0x0000ff);
					ucb.cdb[8] = (Byte) ((dwRemainSize) & 0x0000ff);
					ucb.read = dwRemainSize;

				}
			} else {
				break;
			}
		}

	gettimeofday(&end, NULL);

	Dword elapsedTime = Common::Instance.GetElapsedTimeInMillSec(start, end);
	printf("Write Elapsed time (msec): %ld\n", elapsedTime);

	gettimeofday(&start, NULL);
		// Setup Command Block
		ucb.cdb[0] = 0x3C; // OPERATION CODE = READ BUFFER
		ucb.cdb[1] = 0x02; // MODE = Data
		ucb.cdb[6] = (Byte) ((MAX_DATA_BUFFER_LENGTH >> 16) & 0x0000ff);
		ucb.cdb[7] = (Byte) ((MAX_DATA_BUFFER_LENGTH >> 8) & 0x0000ff);
		ucb.cdb[8] = (Byte) ((MAX_DATA_BUFFER_LENGTH) & 0x0000ff);
		ucb.write = 0;
		ucb.read = MAX_DATA_BUFFER_LENGTH;
		ucb.timeout = dwTimeout;
		ucb.cdblength = 10;
		ucb.bCellNo = bCellNo;
		ucb.senselength = MAXSENSELENGTH;
		Dword dwUploadIdx = 0;

		// READ BUFFER Command Block Setup
		printf("Read Start\n");
		for (;;) {
			execCCBCommand(&ucb, bufDownload);
			dwUploadIdx += MAX_DATA_BUFFER_LENGTH;
			if (dwUploadIdx < dwUploadSize) {
				Dword dwRemainSize = dwUploadSize - dwUploadIdx;
				if (dwRemainSize < MAX_DATA_BUFFER_LENGTH) {
					ucb.cdb[6] = (Byte) ((dwRemainSize >> 16) & 0x0000ff);
					ucb.cdb[7] = (Byte) ((dwRemainSize >> 8) & 0x0000ff);
					ucb.cdb[8] = (Byte) ((dwRemainSize) & 0x0000ff);
					ucb.read = dwRemainSize;
				}
			} else {
				break;
			}
		}
	}

	gettimeofday(&end, NULL);

	Dword elapsedTime = Common::Instance.GetElapsedTimeInMillSec(start, end);
	printf("Read Elapsed time (msec): %ld\n", elapsedTime);
	return rc;
}

Byte Uart3InterfaceTest(Byte bCellNo, int v5Supply, int v12Supply,
		Dword uartBaudRate, Dword uart3BaudRate, Word pullUpVoltage,
		Dword interCharacterDelay, Dword ackTime, int dmaTimeout) {

    Byte 	rc = 0;
	Word    uartVer;
	GET_DRIVE_STATE_DATA driveStateData;
	GETSETU3PARAM parameterData;
	Word size = 0;
	Dword dwTimeout = 30000;

	Byte writeToReqSenseScsiCmdData[10] = {0x3B, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
	Byte reqSenseScsimdData[6] = {0x03, 0x00, 0x00, 0x00, 0x20, 0x00};
	Byte readScsiCmdData[10] = {0x3C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
	Byte writeData[16384];

	rc = siSetUartBaudrate(bCellNo, uartBaudRate);
	rc = setUartPullupVoltage(bCellNo, pullUpVoltage);
	rc = siSetAckTimeout(bCellNo, ackTime);
	rc = siSetInterCharacterDelay(bCellNo, interCharacterDelay);
	rc = setTargetVoltage(bCellNo, v5Supply, v12Supply);

	sleep(10);

	rc = siGetDriveUartVersion(bCellNo, &uartVer);
	if (rc == success) {
		printf("siGetDriveUartVersion %d\n", uartVer);
	} else {
		printf("ERROR: siGetDriveUartVersion failed");
	}

	rc = siChangeToUart3(bCellNo, 0x0000, dwTimeout);
	if(rc != success) {
		printf("ERROR: siChangeToUart3 failed");
	}

	rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siQueryRequest: Passed\n");
	} else {
		printf("siQueryRequest failed with error code 0x%x\n", rc);
		goto FINISH;
	}

	size = sizeof(parameterData);
	rc = siGetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
	if (rc == uart3AckDataAvailable_DriveRetainsControl) {
		printf("UART Parameters Data: \n");
		printf("UART Version: 0x%x\n", (parameterData.bUartProtocolVersionH << 8) + parameterData.bUartProtocolVersionL);
		printf("UART Line Speed: 0x%x\n", (parameterData.bUartLineSpeedH << 8) + parameterData.bUartLineSpeedL);
		printf("UART Ack Delay: %d usec\n", (parameterData.bAckDelayTimeInMicroSecondsH << 8) + parameterData.bAckDelayTimeInMicroSecondsL);
		printf("Max Write Block Size: %d bytes\n", (parameterData.bMaxBlockSizeForWriteHH << 24) + (parameterData.bMaxBlockSizeForWriteHL << 16) + (parameterData.bMaxBlockSizeForWriteLH << 8) + parameterData.bMaxBlockSizeForWriteLL);
		printf("Max Read Block Size: %d bytes\n", (parameterData.bMaxBlockSizeForReadHH << 24) + (parameterData.bMaxBlockSizeForReadHL << 16) + (parameterData.bMaxBlockSizeForReadLH << 8) + parameterData.bMaxBlockSizeForReadLL);
		printf("DMA Timeout: %d msec\n", (parameterData.bDMAtimeoutInMilliSecondsH << 8) + parameterData.bDMAtimeoutInMilliSecondsL);
		printf("FIFO Timeout: %d msec\n", (parameterData.bFIFOtimeoutInMilliSecondsH << 8) + parameterData.bFIFOtimeoutInMilliSecondsL);
		printf("Transmit Segment Size: %d bytes\n", (parameterData.bTransmitSegmentSizeHH << 24) + (parameterData.bTransmitSegmentSizeHL << 16) + (parameterData.bTransmitSegmentSizeLH << 8) + parameterData.bTransmitSegmentSizeLL);
		printf("Inter-Segment Delay: %d usec\n", (parameterData.bInterSegmentDelayInMicroSecondsH << 8) + parameterData.bInterSegmentDelayInMicroSecondsL);
	} else {
		printf("siGetUARTParametersRequest Failed with error code 0x%x\n", rc);
	}

	rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siQueryRequest: Passed\n");
	} else {
		printf("siQueryRequest failed with error code 0x%x\n", rc);
		goto FINISH;
	}

	parameterData.bUartLineSpeedH = (lineSpeed16666000 >> 8) & 0xFF;
	parameterData.bUartLineSpeedL = lineSpeed16666000 & 0xFF;

	rc = siSetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siSetUARTParametersRequest Passed\n");
		rc = siSetUartBaudrate(bCellNo, 16666000);
		if (rc != success) {
			printf("siSetUartBaudrate failed with error code 0x%x\n", rc);
			goto FINISH;
		}
		rc = siGetUARTParametersRequest(bCellNo, &parameterData, dwTimeout);
		if (rc == uart3AckDataAvailable_DriveRetainsControl) {
			printf("UART Parameters Data: \n");
			printf("UART Version: 0x%x\n", (parameterData.bUartProtocolVersionH << 8) + parameterData.bUartProtocolVersionL);
			printf("UART Line Speed: 0x%x\n", (parameterData.bUartLineSpeedH << 8) + parameterData.bUartLineSpeedL);
			printf("UART Ack Delay: %d usec\n", (parameterData.bAckDelayTimeInMicroSecondsH << 8) + parameterData.bAckDelayTimeInMicroSecondsL);
			printf("Max Write Block Size: %d bytes\n", (parameterData.bMaxBlockSizeForWriteHH << 24) + (parameterData.bMaxBlockSizeForWriteHL << 16) + (parameterData.bMaxBlockSizeForWriteLH << 8) + parameterData.bMaxBlockSizeForWriteLL);
			printf("Max Read Block Size: %d bytes\n", (parameterData.bMaxBlockSizeForReadHH << 24) + (parameterData.bMaxBlockSizeForReadHL << 16) + (parameterData.bMaxBlockSizeForReadLH << 8) + parameterData.bMaxBlockSizeForReadLL);
			printf("DMA Timeout: %d msec\n", (parameterData.bDMAtimeoutInMilliSecondsH) + parameterData.bDMAtimeoutInMilliSecondsL);
			printf("FIFO Timeout: %d msec\n", (parameterData.bFIFOtimeoutInMilliSecondsH << 8) + parameterData.bFIFOtimeoutInMilliSecondsL);
			printf("Transmit Segment Size: %d bytes\n", (parameterData.bTransmitSegmentSizeHH << 24) + (parameterData.bTransmitSegmentSizeHL << 16) + (parameterData.bTransmitSegmentSizeLH << 8) + parameterData.bTransmitSegmentSizeLL);
			printf("Inter-Segment Delay: %d usec\n", (parameterData.bInterSegmentDelayInMicroSecondsH << 8) + parameterData.bInterSegmentDelayInMicroSecondsL);
		} else {
			printf("siGetUARTParametersRequest Failed with error code 0x%x\n", rc);
		}
	} else {
		printf("siSetUARTParametersRequest Failed with error code 0x%x\n", rc);
	}

	rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siQueryRequest: Passed\n");
	} else {
		printf("siQueryRequest failed with error code 0x%x\n", rc);
		goto FINISH;
	}

	Byte data[512];
	rc = siEchoRequest(bCellNo, 512, data, dwTimeout);
	if (rc == uart3AckDataAvailable_DriveRetainsControl) {
		printf("siEchoRequest Passed\n");
    	for (int i = 0; i < 512; i++) {
			if(isprint(data[i]))
				printf("%c", data[i]);
			else
				printf(".");
    		if( (i % 60) == 0 && i) printf("\n");
    	}
    	printf("\n");
	} else {
		printf("siEchoRequest failed with error code 0x%x\n", rc);
	}

	rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siQueryRequest: Passed\n");
	} else {
		printf("siQueryRequest failed with error code 0x%x\n", rc);
		goto FINISH;
	}

	// Write Buffer command to Request Sense command
	rc = siSCSICmdRequest(bCellNo, 10, writeToReqSenseScsiCmdData, dwTimeout);
	if ((rc != uart3DriveBusyWithDataReceived) && (rc != uart3AckReady)) {
		printf("siSCSICmdRequest failed with error code 0x%x\n", rc);
	} else {
		rc = siWriteDataRequest(bCellNo, 16384, 1, writeData, dwTimeout);
		if (rc != uart3AckSenseAvailable) {
			printf("siWriteDataRequest failed with error code 0x%x. Sense is not available\n", rc);
		} else {

			// Change SCI command to Request Sense command
			rc = siSCSICmdRequest(bCellNo, 6, reqSenseScsimdData, dwTimeout);
			if ((rc != uart3DriveBusyWithDataReceived) && (rc != uart3AckReady)) {
				printf("siSCSICmdRequest failed with error code 0x%x\n", rc);
			} else {
				Byte readBuf[32];
				Word readLength = 32;
				rc = siReadDataRequest(bCellNo, &readLength, 1, readBuf, dwTimeout);
			}
			while (true){
				rc = siQueryRequest(bCellNo, 0x01, dwTimeout);
				if (rc == uart3AckBusy){
					break;
				}
			}

			while (true){
				rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
				if (rc == uart3AckSCSICommandComplete){
					printf("SCSI command \"WRITE BUFFER to REQUEST SENSE command\" completed.\n");
					break;
				}
			}
		}
	}

	// Finish Request Sense command

	// Write Buffer command
	rc = siSCSICmdRequest(bCellNo, 10, writeToReqSenseScsiCmdData, dwTimeout);
	if ((rc != uart3DriveBusyWithDataReceived) && (rc != uart3AckReady)) {
		printf("siSCSICmdRequest failed with error code 0x%x\n", rc);
	} else {
		// Write data
		for (int i = 0; i < 16384; i++) {
			writeData[i] = i % 256;
		}

		for (int i = 1; i <= 4; i++) {
			rc = siWriteDataRequest(bCellNo, 16384, i, writeData, dwTimeout);
			if ((rc != uart3AckDataRequested) && (rc != uart3DriveBusyWithDataReceived)) {
				printf("siWriteDataRequest failed with error code 0x%x\n", rc);
				// Since the write buffer failed lets abort the SCSI command
				rc = siAbortSCSICmdRequest(bCellNo, dwTimeout);
				if (rc != uart3AckReady) {
					printf("siAbortSCSICmdRequest failed with error code 0x%x\n", rc);
				}
				break;
			}
		}

		while (true){
			rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
			if (rc == uart3AckSCSICommandComplete){
				printf("SCSI command \"WRITE BUFFER\" completed.\n");
				break;
			}
		}
		// Read Buffer command
		rc = siSCSICmdRequest(bCellNo, 10, readScsiCmdData, dwTimeout);
		if ((rc != uart3DriveBusyWithDataReceived) && (rc != uart3AckReady)) {
			printf("siSCSICmdRequest failed with error code 0x%x\n", rc);
		} else {
			Byte readBuf2[16384];
			Word len = 16384;
			for (int i = 1; i <= 4; i++) {
				rc = siReadDataRequest(bCellNo, &len, i, readBuf2, dwTimeout);
				if (rc != uart3AckDataAvailable_DriveRetainsControl) {
					printf("siReadDataRequest failed with error code 0x%x\n", rc);
					rc = siAbortSCSICmdRequest(bCellNo, dwTimeout);
					if (rc != uart3AckReady) {
						printf("siAbortSCSICmdRequest failed with error code 0x%x\n", rc);
					}
					break;
				}
			}
			while (true){
				rc = siQueryRequest(bCellNo, 0x01, dwTimeout);
				if (rc == uart3AckBusy){
					break;
				}
			}

			while (true){
				rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
				if (rc == uart3AckSCSICommandComplete){
					printf("SCSI command \"READ BUFFER\" completed.\n");
					break;
				}
			}
		}
	}

	rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siQueryRequest: Passed\n");
	} else {
		printf("siQueryRequest failed with error code 0x%x\n", rc);
		goto FINISH;
	}

	size = sizeof(driveStateData);
	rc = siGetDriveStateRequest(bCellNo, &driveStateData, dwTimeout);
	if (rc == uart3AckDataAvailable_DriveRetainsControl) {
		printf("Drive State Data:\n");
		printf("Drive State Version: 0x%x\n", driveStateData.wDriveStateVersion);
		printf("Drive Operating State: 0x%x\n", driveStateData.wOperatingState);
		printf("Drive Functional Mode: 0x%x\n", driveStateData.wFunctionalMode);
		printf("Drive Degraded Reason: 0x%x\n", driveStateData.wDegradedReason);
		printf("Drive Broken Reason: 0x%x\n", driveStateData.wBrokenReason);
		printf("Drive Reset Cause: 0x%x\n", driveStateData.wResetCause);
		printf("Drive Showstop State: 0x%x\n", driveStateData.wShowstopState);
		printf("Drive Showstop Reason: 0x%x\n", driveStateData.wShowstopReason);
		printf("Drive Showstop Value: 0x%lx\n", driveStateData.dwShowstopValue);
	} else {
		printf("siGetDriveStateRequest Failed with error code 0x%x\n", rc);
	}

	rc = siQueryRequest(bCellNo, 0x00, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siQueryRequest: Passed\n");
	} else {
		printf("siQueryRequest failed with error code 0x%x\n", rc);
		goto FINISH;
	}

	rc = siSetUart2ModeRequest(bCellNo, dwTimeout);
	if (rc == uart3AckReady) {
		printf("siSetUart2ModeRequest Passed\n");
	} else {
		printf("siSetUart2ModeRequest Failed with error 0x%x\n", rc);
	}

	rc = siSetUartBaudrate(bCellNo, 115200);
	if (rc != success) {
		printf("siSetUartBaudrate failed with error code 0x%x\n", rc);
		goto FINISH;
	}

	rc = siGetDriveUartVersion(bCellNo, &uartVer);
	if (rc == success) {
		printf("siGetDriveUartVersion %d\n", uartVer);
	} else {
		printf("siGetDriveUartVersion Failed with error code 0x%x\n", rc);
	}
//
//	rc = siDriveResetRequest(bCellNo, dwTimeout);
//	if (rc == uart3AckReady) {
//		printf("siDriveResetRequest Passed\n");
//	} else {
//		printf("siDriveResetRequest Failed with error code 0x%x\n", rc);
//	}
//
//	rc = siGetDriveUartVersion(bCellNo, &uartVer);
//	if (rc == success) {
//		printf("siGetDriveUartVersion %d\n", uartVer);
//	} else {
//		printf("siGetDriveUartVersion Failed with error code 0x%x\n", rc);
//	}

FINISH:
	rc = setTargetVoltage(bCellNo, 0, 0);

	return rc;
}

int main(int argc, char* argv[]) {

	int slotIndex = 0;
	int testIndex = 0;
	Dword uartBaudRate = 115200;
	Dword uart3BaudRate = 16666000;
	int interCharacterDelay = 0;
	int ackTimeout = 0;
	int v5Supply = 5000;
	int v12Supply = 12000;
	int pullUpVoltage = 1800;
	int loglevel = 1;
	int continueIfInitializeFail = 0;
	int dmaTimeout = 100;
	bool writeVerify = false;

	if (argc == 1) {
		// No arguments have been passed
		PrintHelp();
	}

	for (int count = 1; count < argc; count++) {
		if (argv[count][0] == '-') {
			// It is an argument
			if (strcmp(argv[count], "-si") == 0){
				++count;
				slotIndex = atoi(argv[count]);
			} else if (strcmp(argv[count], "-ti") == 0){
				++count;
				testIndex = atoi(argv[count]);
			} else if (strcmp(argv[count], "-br") == 0) {
				++count;
				uartBaudRate = atol(argv[count]);
			} else if (strcmp(argv[count], "-icd") == 0) {
				++count;
				interCharacterDelay = atoi(argv[count]);
			} else if (strcmp(argv[count], "-ack") == 0) {
				++count;
				ackTimeout = atoi(argv[count]);
			} else if (strcmp(argv[count], "-5v") == 0) {
				++count;
				v5Supply = atoi(argv[count]);
			} else if(strcmp(argv[count], "-12v") == 0) {
				++count;
				v12Supply = atoi(argv[count]);
			} else if(strcmp(argv[count], "-puv") == 0) {
				++count;
				pullUpVoltage = atoi(argv[count]);
			} else if(strcmp(argv[count], "-ll") == 0) {
				++count;
				loglevel = atoi(argv[count]);
			} else if(strcmp(argv[count], "-cif") == 0) {
				++count;
				continueIfInitializeFail = atoi(argv[count]);
			} else if (strcmp(argv[count], "--help") == 0) {
				PrintHelp();
				return 0;
			} else if (strcmp(argv[count], "-u3br") == 0) {
				++count;
				uart3BaudRate = atol(argv[count]);
			} else if (strcmp(argv[count], "-wv") == 0) {
				writeVerify = true;
				++count;
			} else if (strcmp(argv[count], "-dma") == 0) {
				++count;
				dmaTimeout = atol(argv[count]);
			}
		}
	}

	if ((slotIndex == 0) || (testIndex == 0)) {
		PrintHelp();
		return -1;
	}

	printf("Parameters:\n");
	printf("SlotIndex: %d\n", slotIndex);
	printf("TestIndex: %d\n", testIndex);
	printf("5VSupply : %d mVolts \n", v5Supply);
	printf("12VSupply: %d mVolts\n", v12Supply);
	printf("UartPullUpVoltage: %d mVolts\n", pullUpVoltage);
	printf("UART Baudrate: %ld\n", uartBaudRate);
	printf("UART3 Baudrate: %ld\n", uart3BaudRate);
	printf("Inter-character delay: %d \n", interCharacterDelay);
	printf("Ack Timeout: %d \n", ackTimeout=3);
	printf("DMA Timeout: %d \n", dmaTimeout);
	printf("Log Level: %d\n", loglevel);
	printf("Chunk size: %d\n", UART3_MAX_CHUNK_SIZE);
	printf("Verify writes?: %s\n", writeVerify?"Yes":"No");
	printf("Continue if initialize fail: %s\n", continueIfInitializeFail?"True":"Fail");
	
	siSetDebugLogLevel(slotIndex, loglevel);

	Byte status = siInitialize(slotIndex);
	char errorMessage[MAX_ERROR_STRING_LENGTH];
	Byte lastErrorCode;
	if (status != success) {
		printf("siInitialize failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		if (!continueIfInitializeFail) {
			return -1;
		} else {
			printf("Continuing with the test ..\n");
		}
	}

	status = siSetAckTimeout(slotIndex, ackTimeout);
	if (status != success) {
		printf("siSetAckTimeout failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		return -1;
	}
	status = siSetInterCharacterDelay(slotIndex, interCharacterDelay);
	if (status != success) {
		printf("siSetInterCharacterDelay failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		return -1;
	}

	status = siSetAckTimeout(slotIndex, ackTimeout);
	if (status != success) {
		printf("siSetAckTimeout failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		return -1;
	}

	status = setUartPullupVoltage(slotIndex, pullUpVoltage);
	if (status != success) {
		printf("setUartPullupVoltage failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		return -1;
	}

	status = siSetUartBaudrate(slotIndex, uartBaudRate);
	if (status != success) {
		printf("siSetUartBaudrate failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		return -1;
	}

	status = setTargetVoltage(slotIndex, v5Supply, v12Supply);
	if (status != success) {
		printf("setTargetVoltage failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		return -1;
	}

	sleep(5);

	Word uartVer = 0;
	status = siGetDriveUartVersion(slotIndex, &uartVer);
	if (status != success) {
		printf("getDriveUartVersion failed with error code 0x%x\n", status);
		siGetLastErrorCause(slotIndex, errorMessage, &lastErrorCode);
		printf("Last Error Cause: 0x%x (%s)\n", lastErrorCode, errorMessage);
		return -1;
	}
	printf("Drive UART version: %d\n", uartVer);

	switch(testIndex) {
	case 1:
		Uart3BatchTest(slotIndex, v5Supply, v12Supply, uartBaudRate,
				uart3BaudRate, pullUpVoltage);
		break;
	case 2:
		Uart3InterfaceTest(slotIndex, v5Supply, v12Supply, uartBaudRate,
				uart3BaudRate, pullUpVoltage, interCharacterDelay, ackTimeout, dmaTimeout);
		break;
	case 3:
		Uart3PerformanceTest(slotIndex, uartBaudRate, uart3BaudRate, dmaTimeout, writeVerify);
		break;
	default:
		PrintHelp();
		return -1;
	}

	status = siFinalize(slotIndex);
	if (status != success) {
		printf("siFinalize failed with error code 0x%x\n", status);
		return -1;
	}
}

void PrintHelp() {
	printf("\n");
	printf("Usage: uart3test -si <slotindex> -ti <testindex> "
			"[-ubr <baudrate> -icd <intercharacterdelay> -act <acktimeout> -5v <5vsupply> -12v <12vsupply> -puv <pullupvoltage> -ll <loglevel>] -cif <1/0> -u3br <baudrate>"
			"--help\n");
	printf("\nRequired arguments:\n");
	printf("si <slotindex>: Slot index ( 1 -140)\n");
	printf("ti <testindex>: Test index. 1 - Uart3 batch test, 2 - Uart3 interface test, 3 - Performance test\n");
	printf("\nOptional arguments:\n");
	printf("ubr <baudrate>: UART Baud rate. This is the baudrate used during the initial power up. (Default: 115200)\n");
	printf("icd <intercharacterdelay>: Inter-character delay in bit times (Default: 0)\n");
	printf("act <acktimeout>: Ack timeout. It is used for UART 2 commands. Set it to 0 for UART 3 commands (Default: 0)\n");
	printf("5v <5vsupply>: 5V supply voltage in milli volts. Range: 4500 - 5500 (Default: 5000)\n");
	printf("12v <12vsupply>: 12V supply voltage in milli volts. Range: 10800 - 13200 (Default: 12000)\n");
	printf("puv <pullupvoltage>: Pull up voltage in milli volts (Default: 1800)\n");
	printf("ll <loglevel>: Wrapper logging level (0 -4) (Default: 1)\n");
	printf("cif <1/0>: Continue test if siInitialize fails. 1 - Yes, 0 - No (Default: 0)\n");
	printf("-u3br <baudrate>: UART3 baud rate. This baudrate will be used once drive is switched to UART3 mode (Default: 16666000\n");
	printf("help: Print the help\n");
	printf("\n");
}

#ifdef __cplusplus
}
#endif

