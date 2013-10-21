/*
 * terSioCommon.h
 *
 *  Created on: Jul 6, 2011
 *      Author: Kunal Goradia
 */

#ifndef TER_SIO_COMMON_H_
#define TER_SIO_COMMON_H_

enum TER_ResetType {
	TER_ResetType_Reset_None = 0x0, // Take no action
	TER_ResetType_Reset_SendBuffer = 0x1, // Clear send buffer
	TER_ResetType_Reset_ReceiveBuffer = 0x2, // Clear receive buffer
	TER_ResetType_Reset_Slot = 0x4, // Send Soft Reset to DIO
	TER_ResetType_Reset_TxHalt = 0x5, // Do a TxHalt Reset
// Reboot the DIO
};

enum TER_Status {
	TER_Status_none = 0,
	TER_Status_dio_timeout = 1,
	TER_Status_no_client_connection = 2,
	TER_Status_slotid_error = 3,
	TER_Status_InvalidMode = 4,
	TER_Status_Error_Argument_Invalid = 5,
	TER_Status_receive_packet_len_err = 6, // TODO: Is this obsolete?
	TER_Status_HW_Timeout_Error = 7,
	TER_Status_OvertempError = 8,
	TER_Status_Send_Receive_Error = 9,
	TER_Status_file_not_found = 13,
	TER_Status_Buffer_Overflow = 18,
	TER_Status_rpc_fail = 22,
	TER_Status_Timeout = 23,
	TER_Status_NotImplemented = 24,
	TER_Status_Read_Only_Value = 25,
	TER_Status_Unknown_Pseudo_Addr = 26,
	TER_Status_Invalid_Baud_Rate = 27,
	TER_Status_Invalid_Stop_Bits = 28,
	TER_Status_ucode_upgrade_busy = 31,
	TER_Status_ucode_file_too_big = 33,
	TER_Status_UpgradeInfoMissing = 34,
	TER_Status_high_ucode_ugrade_cnt = 35,
	TER_Status_DIO_STATUS_timed_out = TER_Status_dio_timeout,
	TER_Status_DIO_STATUS_no_dio  =38,
	TER_Status_DIO_STATUS_slotid_out_of_range = TER_Status_slotid_error,
	TER_Status_DIO_STATUS_retrys_exhausted = 41,
	TER_Status_DIO_STATUS_command_drained = 43,
	TER_Status_DIO_STATUS_response_out_of_seq = 44,
	TER_Status_DIO_STATUS_dio_reported_error = 45,
	TER_Status_DIO_STATUS_command_mismatch = 46,
	TER_Status_DIO_STATUS_dio_rx_cksum_error = 47,
	TER_Status_DIO_STATUS_force_25msec_error = 48,
	TER_Status_64k_count_exceeded = 49,
	TER_Status_transmitter_is_halted = 50,
	TER_Status_HTemp_Sled_Temperature_Fault  = 51,
	TER_Status_HTemp_Max_Adjustment_Reached = 52,
	TER_Status_HTemp_Min_Adjustment_Reached = 53,
        TER_Status_DctNotReadyToStart			= 54,
        TER_Status_DctAlreadyStarted			= 55,
        TER_Status_DctNotStarted			= 56,
	
	TER_Status_Send_Receive_Timeout = 57,
	TER_Status_Invalid_Response = 58,
	TER_STATUS_MAX,
};

typedef enum TER_Notifier {
	TER_Notifier_Cleared 			= 0x00000000,
	TER_Notifier_SledHeaterFaultShort 	= 0x00000001,
	TER_Notifier_SledHeaterFaultOpen 	= 0x00000002,
	TER_Notifier_BlowerFault 		= 0x00000004,
	TER_Notifier_SlotCircuitOverTemp 	= 0x00000008,
	TER_Notifier_DriveRemoved 		= 0x00000010,
	TER_Notifier_3_3OverCurrent 		= 0x00000020,
	TER_Notifier_DriveOverCurrent 		= 0x00000040,
	TER_Notifier_SlotCircuitFault 		= 0x00000100,
	TER_Notifier_LatchedSledOverTemp 	= 0x00000400,
	TER_Notifier_LatchedSledUnderTemp 	= 0x00000800,
	TER_Notifier_SledOverTemp 		= 0x00001000,
	TER_Notifier_SledTemperatureFault 	= 0x00002000,
	TER_Notifier_SledDiodeFault 		= 0x00010000,
	TER_Notifier_TempEnvelopeFault 		= 0x01000000,
	TER_Notifier_TempRampNotCompleted 	= 0x02000000,
	TER_Notifier_Msk 			= 0x0000ffff,
} TER_Notifier;

typedef enum TER_StatusBits {
	// Provides an indication of whether the slot, sled and drive are present
	IsSlotPresent 			= 0x00000001, // true if present
	IsSledPresent 			= 0x00000002, // true if present
	IsDrivePresent 			= 0x00000004, // true if present
        IsSsdPresent		=0x00000008, 	// true if SSD component is present

	IsTempControlEnabled 	= 0x00000010, // true if temperature regulation is enabled
	IsTempTargetReached 	= 0x00000020, // true when the target temperature has been reached
	IsRampUp 				= 0x00000040, // true if DIO is ramping up
	IsRampDown 				= 0x00000080, // true if DIO is ramping down
	IsHeaterEnabled 		= 0x00000100, // true if heater enabled
	IsCoolerEnabled 		= 0x00000200, // true if cooler enabled
	IsFIDReset 				= 0x00001000, // true if FIDReset
	IsFIDBootStrap 			= 0x00002000, // true if FIDBootStrap
	IsPM2 					= 0x00004000, // true if PM2

} TER_StatusBits;

enum TER_UpgradeSlotFlags {
	DP_FLAGS_none = 0,
	DP_FLAGS_timeout = 1,
	DP_FLAGS_in_boot_loader = 2,
	DP_FLAGS_main_app_xsum_not_valid = 4,
	DP_FLAGS_upgrade_recommended = 8,
	DP_FLAGS_1030_mismatch = 16,
	DP_FLAGS_invalid_dio_tpn = 32
};

#endif /* TER_SIO_COMMON_H_ */
