#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>

#include "tcTypes.h"
#include "common.h"
#include "commonEnums.h"
#include "libu3.h"

/**
 * <pre>
 * Description:
 *   Execute SCSI CCB command on UART-3 protocol
 * Arguments:
 *   CCB  *cb   : INPUT - CCB structure as below.
 *    - unsigned char cdblength;         // CDB length
 *    - unsigned char senselength;       // sense data length
 *    - unsigned char cdb[CDBSIZE];      // CDB block
 *    - unsigned char sense[SENSESIZE];  // sense data block
 *    - unsigned long read;              // read data length
 *    - unsigned long write;             // write data length
 *    - long timeout;                    // timeout [msec]
 *
 *   void *data : INPUT - Transfer data buffer pointer.
 *
 * Return:
 *  T.B.D.
 *
 * </pre>
 *****************************************************************************/
long execCCBCommand(CCB_DEBUG *cb, void *data) {
	Byte rAck = success;
	Byte ack = success;
	Byte isSenseEnable = 0;
	Byte isSCSBEnable = 0;
	CCB cb_local;
	Byte bCellNo = 0;

	cb->bytesTransferred = 0;
	Common::Instance.WriteToLog(LogTypeTrace, "Entry: %s, cb=0x%x, data=0x%x\n",
			__FUNCTION__, cb, data);

	// Validate arguments
	if (cb == NULL) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Null argument has been passed. cb=0x%x\n",
				__FUNCTION__, cb);
		rAck = nullArgumentError;
		goto FUNCTIONEND;
	}

	Common::Instance.WriteToLog(LogTypeTrace,
			"%s- cdblength=%d, senselength=%d, scsblength=%d, read=%d, write=%d, T/O=%d\n",
			__FUNCTION__, cb->cdblength, cb->senselength, cb->scsblength, cb->read, cb->write,
			cb->timeout);

	bCellNo = cb->bCellNo;

	// Validate arguments
	if ((rAck = Common::Instance.SanityCheck(bCellNo)) != success) {
		Common::Instance.WriteToLog(LogTypeError,
				"ERROR: %s- Sanity check failed with error code 0x%x\n",
				__FUNCTION__, rAck);
		goto FUNCTIONEND;
	}

	if ((cb->cdblength < MINCDBLENGTH) || (MAXCDBLENGTH < cb->cdblength)) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. CDB length %d is out of range. Range is %d - %d\n",
				__FUNCTION__, cb->cdblength, MINCDBLENGTH, MAXCDBLENGTH);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	if (cb->timeout < 1) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. Timeout %d is less than 1 milli second\n",
				__FUNCTION__, cb->timeout);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	if (MAXSENSELENGTH < cb->senselength) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. Sense length %d is more than %d\n",
				__FUNCTION__, cb->senselength, MAXSENSELENGTH);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	if (MAXSCSBLENGTH < cb->scsblength) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. SCSB length %d is more than %d\n",
				__FUNCTION__, cb->scsblength, MAXSCSBLENGTH);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	if ((0 < cb->read) && (0 < cb->write)) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. Both read %d and write %d lengths are more than 0\n",
				__FUNCTION__, cb->read, cb->write);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	if (((0 == cb->read) && (0 == cb->write)) && (data != NULL)) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Data pointer 0x%x is not NULL but read and write lengths are 0\n",
				__FUNCTION__, data);
		rAck = invalidArgumentError;
		goto FUNCTIONEND;
	}

	if (((0 < cb->read) || (0 < cb->write)) && (data == NULL)) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Data pointer 0x%x is NULL but read or write length is more than 0\n",
				__FUNCTION__, data);
		rAck = nullArgumentError;
		goto FUNCTIONEND;
	}

	if ((MAX_UART3_SCSI_DATA_LENGTH < cb->read)
			|| (MAX_UART3_SCSI_DATA_LENGTH < cb->write)) {
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Invalid argument. Read length %d or Write length %d is more than the maximum data length %d\n",
				__FUNCTION__, cb->read, cb->write, MAX_UART3_SCSI_DATA_LENGTH);
		rAck = outOfRange;
		goto FUNCTIONEND;
	}

	// sense switch
	if (0 == cb->senselength) {
		isSenseEnable = 0; // disable
	} else {
		isSenseEnable = 1;
	}

	// SCSB switch
	if (0 == cb->scsblength) {
		isSCSBEnable = 0; // disable
	} else {
		if (cb->forcescsbreport) {
			isSCSBEnable = 1;
		} else {
			isSCSBEnable = 0;
		}
	}

	// copy CCB_DEBUG to CCB
	memset((Byte*) &cb_local, 0x00, sizeof(cb_local));
	cb_local.cdblength = cb->cdblength;
	cb_local.read = cb->read;
	cb_local.write = cb->write;
	cb_local.timeout = cb->timeout;
	memcpy((Byte*) &cb_local.cdb[0], (Byte*) &cb->cdb[0], cb_local.cdblength);

	// SCSI Command
	rAck = siExecuteSCSICommand(&cb_local, data, bCellNo);

	switch (rAck) {
	case uart3AckSCSICommandComplete:
		Common::Instance.WriteToLog(LogTypeTrace,
				"%s: SCSI command completed successfully\n", __FUNCTION__);
		cb->bytesTransferred = cb->read > cb->write ? cb->read:cb->write;
		ack = success;
		break;
	case uart3AckSenseAvailable:
		Common::Instance.WriteToLog(LogTypeTrace, "%s: Sense available\n",
				__FUNCTION__);
//		bec = rc_CommandFailure;
		isSenseEnable = 1;
		if (cb->forcescsbreport || (0 < cb->scsblength)) {
			isSCSBEnable = 1; // SCSB
		}
		break;
	default:
		Common::Instance.WriteToLog(
				LogTypeError,
				"ERROR: %s- Unexpected ack received 0x%x from siExecuteSCSICommand\n",
				__FUNCTION__, rAck);
		cb->bytesTransferred = 0;
		goto FUNCTIONEND;
		break;
	}

	// REQUEST SENSE Command
	if (isSenseEnable) {

		CCB scb;
		Byte *sense = &cb->sense[0];

		memset((Byte*) &scb, 0x00, sizeof(scb));
		scb.cdb[0] = 0x03; // REQUEST SENSE Command Opcode
		scb.cdb[4] = MAXSENSELENGTH;
		scb.read = MAXSENSELENGTH;
		scb.cdblength = 6; // REQUEST SENSE Command length
		scb.timeout = 10000; // 10sec

		rAck = siExecuteSCSICommand(&scb, sense, bCellNo);
		switch (rAck) {
		case uart3AckSCSICommandComplete:
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: SCSI command completed during REQUEST SENSE command\n",
					__FUNCTION__);
			if (!cb->bytesTransferred)
				cb->bytesTransferred = MAXSENSELENGTH;
//			bec = rc_CommandFailure;
			break;
		case uart3AckSenseAvailable:
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Sense available during REQUEST SENSE command\n",
					__FUNCTION__);
			/* TBD */
//			bec = rc_CommandFailure;
			break;
		default:
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- Unexpected ack received 0x%x from siExecuteSCSICommand during REQUEST SENSE command\n",
					__FUNCTION__, rAck);
			cb->bytesTransferred = 0;
			goto FUNCTIONEND;
		}
	}

	// RETRIEVE SCSB Command
	if (isSCSBEnable) {

		CCB scb;
		Byte *status = &cb->scsb[0];

		memset((Byte*) &scb, 0x00, sizeof(scb));
		scb.read = cb->scsblength;
		scb.cdb[0] = 0xC3; // RETRIEVE SCSB Opcode ??
		scb.cdb[2] = 0x04;
		scb.cdb[3] = (Byte) ((scb.read >> 8) & 0x00ff);
		scb.cdb[4] = (Byte) (scb.read & 0x00ff);
		scb.cdblength = 10;
		scb.timeout = 10000; // 10sec

		rAck = siExecuteSCSICommand(&scb, status, bCellNo);
		switch (rAck) {
		case uart3AckSCSICommandComplete:
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: SCSI command completed during RETRIEVE SCSB command\n",
					__FUNCTION__);
			rAck = success;
			cb->scsblength = scb.read;
			if (!cb->bytesTransferred)
				cb->bytesTransferred = scb.read;
			break;
		case uart3AckSenseAvailable:
			Common::Instance.WriteToLog(LogTypeTrace,
					"%s: Sense available during RETRIEVE SCSB command\n",
					__FUNCTION__);
			/* TBD */
//			bec = rc_CommandFailure;
			break;
		default:
			Common::Instance.WriteToLog(
					LogTypeError,
					"ERROR: %s- Unexpected ack received 0x%x from siExecuteSCSICommand during RETRIEVE SCSB command\n",
					__FUNCTION__, rAck);
			cb->scsblength = 0;
			cb->bytesTransferred = 0;
			goto FUNCTIONEND;
		}
	}

FUNCTIONEND:
//	rc = rc_Complete - bec;
	Common::Instance.WriteToLog(LogTypeTrace, "Exit : %s, exit code=0x%x\n",
			__FUNCTION__, rAck);
	RET(rAck);
}

