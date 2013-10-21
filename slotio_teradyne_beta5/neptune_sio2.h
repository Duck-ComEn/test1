#ifndef __neptune_sio2_h__
#define __neptune_sio2_h__

// define to become thread safe and thread aware
//#define USE_THREAD_SAFETY

#include <assert.h>

//#include "libfcts.h"
#if !defined(NEPTUNE_CLIENT_BUILD)
#include "neptune_sio2_rpc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if defined(NEPTUNE_CLIENT_BUILD)
#define FALSE 0
#define CLIENT void *
  extern void xclnt_destroy( CLIENT *c );
#else // NEPTUNE_CLIENT_BUILD
#include "rpc/rpc.h"
#endif // NEPTUNE_CLIENT_BUILD
#ifdef __cplusplus
}
#endif

#if defined(USE_THREAD_SAFETY)
#include "pthread.h"
#endif

//
// TER_IsDioPresent support
//
#define DP_FLAGS_none                    0
#define DP_FLAGS_timeout                 1
#define DP_FLAGS_in_boot_loader          2
#define DP_FLAGS_main_app_xsum_not_valid 4
#define DP_FLAGS_upgrade_recommended     8
#define DP_FLAGS_1030_mismatch           16

// Note: must be a multiple of 4 bytes
typedef struct dio_present_struct_t {
  int dp_flags;

  //
  // values from DIO interrogation
  //

  int dp_dio_board_type; // cmd9

  int dp_dio_rev_major;  // cmd3
  int dp_dio_rev_minor;

  int dp_m9240_major;    // 1006
  int dp_m9240_minor;

  int dp_k9240_major;    // K9240
  int dp_k9240_minor;

  //
  // values from SIO2_Boot.txt, cnk, getter, etc
  //

  // DIO Application
  //
  // Extracted from DIO_Application_<major>_<minor>.bin
  // file name at init time
  // For us to be up-to-date
  //    dp_dio_rev_major == u_major &&
  //    dp_dio_rev_minor == u_minor
  //  must be true
  int u_major;
  int u_minor;

  // 9240 Application
  //
  // Use 254 (hardcoded 9240 application number) to
  // lookup the largest corresponding minor in the
  // .cnk file and return that value here.
  // For us to be up-to-date,
  //     dp_k9240_minor == k9240_cnk_minor
  //  must be true
  int k9240_cnk_minor;

  // 9240 Init
  //
  // Use dp_dio_board_type to lookup the largest
  // corresponding minor in the .cnk file and
  // return that value here.
  // For us to be up-to-date,
  //     dp_m9240_major == dp_dio_board_type &&
  //     dp_m9240_minor == minor_in_cnk_file
  //   must be true
  int minor_in_cnk_file;

  // upgrader_check_major_minor
  //   -1 - no microcode file
  //    0 - wrong rev
  //    1 - rev ok
  //    2 - invalid DIO_Application file length
  int upgrader_check_major_minor;

  // keep track of sis info for ict
  int sis_dio_application_major;
  int sis_dio_application_minor;
  int sis_9240_init_major;
  int sis_9240_init_minor;
  int sis_9240_application_major;
  int sis_9240_application_minor;
  int sis_dio_type;
  int sis_slotid;
  unsigned int sis_u1;
  unsigned int sis_u2;

} __attribute__((__packed__)) DP;

// Note: Also defined in sockTunnel-1.0/errlog.h !!!
#define ERRLOG_LOG_SIZE 132

#define HUGE_BUFFER_MAX 65536

// TER_SetSignal TER_Signals
enum TER_Signals {
  SigFIDReset     = 1,
  SigFIDBootstrap = 2,
  SigPM2          = 4,
};
typedef enum TER_Signals TER_Signals;

// TER_Status
typedef unsigned int TER_Status;

//////////////////////////////////////////////////////////////////////////
// DEFINED LENGTHS OF STRINGS
//////////////////////////////////////////////////////////////////////////

			    // cal> where used ???
#define TER_MAX_ERROR_STRING_LENGTH      1024
			    // cal> where used ???
#define TER_EXPLANATION_STRING_LENGTH    256
			    // cal> where used ???
#define TER_PROGRESS_CODE_LENGTH         64
			    // cal> where used ???
#define TER_IP_ADDRESS_STRING_LENGTH     16		

#define TER_ACTUAL_VOLTAGE_NOT_SUPPORTED	-100000
#define TER_ACTUAL_CURRENT_NOT_SUPPORTED	-100000

//////////////////////////////////////////////////////////////////////////
// DEFINED LED STATES
//////////////////////////////////////////////////////////////////////////

typedef enum TER_LedState						// Selects the state of an LED
{
    LedOff		= 0,	// LED is off
    Yellow_Steady	= 0x5,	// LED is yellow glowing steadily
    Yellow_FlashSlow	= 0x9,	// LED is yellow flashing slowly
    Yellow_FlashFast	= 0xD,	// LED is yellow flashing rapidly
    Green_Steady	= 0x6,	// LED is green glowing steadily
    Green_FlashSlow	= 0xA,	// LED is green flashing slowly
    Green_FlashFast	= 0xE,	// LED is green flashing rapidly
    Orange_Steady	= 0x7,	// LED is orange glowing steadily
    Orange_FlashSlow	= 0xB,	// LED is orange flashing slowly
    Orange_FlashFast	= 0xF,	// LED is orange flashing rapidly
    Yellow_Comm		= 0x10,	// LED is used for diagnostics/communication (Neptune 2 DIO only)
    Yellow_CommHeartbeat = 0x11	// LED is used for diagnostics/communication (Neptune 2 DIO only)
} TER_LedState;

enum TER_ResetType
{
  TER_ResetType_Reset_None            = 0x0,  // Take no action
  TER_ResetType_Reset_SendBuffer      = 0x1,  // Clear send buffer
  TER_ResetType_Reset_ReceiveBuffer   = 0x2,  // Clear receive buffer
  TER_ResetType_Reset_DioSoftReset    = 0x4,  // Send Soft Reset to DIO
  TER_ResetType_Reset_Slot            = 0x8,  // Temperature Regulation off
  TER_ResetType_Reset_FPGA            = 0x10, // Reset the FPGA
  TER_ResetType_Reset_DioReboot       = 0x20  // Reboot the DIO
};

enum TER_DebugType
{
    TER_DebugType_None			= 0x0, // Take no action
    TER_DebugType_ClearDioStatus	= 0x1, // Clear DIO Extended Error Status
    TER_DebugType_PollOffAll		= 0x2, // disable polling on all channels
    TER_DebugType_PollOnAll		= 0x3, // enable polling on all channels
    TER_DebugType_PollOff		= 0x4, // disable polling on current channel
    TER_DebugType_PollOn		= 0x5, // enable polling on current channel
};

typedef enum TER_Notifier
{
  TER_Notifier_Cleared			   = 0x00000,
  TER_Notifier_FanFault                    = 0x00002,
  TER_Notifier_LatchedSlotPcbOverTemp      = 0x00004,
  TER_Notifier_SlotCircuitOverTemp         = 0x00008,

  TER_Notifier_DriveRemoved                = 0x00010,
  TER_Notifier_DriveOverCurrent            = 0x00040,
  TER_Notifier_SledRemoved            	   = 0x00080,

  TER_Notifier_SlotCircuitFault            = 0x00100,
  TER_Notifier_SledTempSensorFault         = 0x00200,
  TER_Notifier_LatchedSledOverTemp         = 0x00400,
  TER_Notifier_LatchedSledUnderTemp        = 0x00800,

  TER_Notifier_SledOverTemp                = 0x01000,
  TER_Notifier_SledTemperatureFault        = 0x02000,
  TER_Notifier_DiodeFault                  = 0x04000,

  TER_Notifier_SledHeaterFault             = 0x10000,
  TER_Notifier_SledHeaterFaultShort        = 0x20000,
  TER_Notifier_SledHeaterFaultOpen         = 0x40000,

  TER_Notifier_TempEnvelopeFault           = 0x01000000,
  TER_Notifier_TempRampNotCompleted        = 0x02000000,


} TER_Notifier;

typedef enum TER_StatusBits
{
    // Provides an indication of whether the slot, sled and drive are present
    TER_IsSlotPresent		=0x00000001,	// true if present
    TER_IsSledPresent		=0x00000002,	// true if present
    TER_IsDrivePresent		=0x00000004,	// true if present

    IsTempControlEnabled	=0x00000010,	// true if temperature regulation is enabled
    IsTempTargetReached		=0x00000020,	// true when the target temperature has been reached

    IsRampUp			=0x00000040,	// true if heater is turned on
    IsRampDown			=0x00000080,	// true if fan is turned on

    IsFIDReset                  =0x00001000,    // true if FIDReset
    IsFIDBootStrap              =0x00002000,    // true if FIDBootStrap
    IsPM2                       =0x00004000,    // true if PM2

} TER_StatusBits;

struct TER_SlotStatus {
  int slot_errors;
  int slot_status_bits;
  int temp_drive_x10;
  int temp_slot_x10;
  int cool_demand_percent;
  int heat_demand_percent;
  int blower_rpm;

  int voltage_actual_3v_mv;
  int voltage_actual_5v_mv;
  int voltage_actual_12v_mv;
  int voltage_actual_18v_mv;
  int voltage_actual_heat_mv;

  int current_actual_3a_mv;
  int current_actual_5a_mv;
  int current_actual_12a_mv;
  int current_actual_heat_mv;
};
typedef struct TER_SlotStatus TER_SlotStatus;

// make sure bool is defined as unsigned int
#ifdef bool
#undef bool
#endif
#define bool unsigned int

struct targetTemperature {
  int temp_target_x10;
  int cool_ramp_rate_x10;
  int heat_ramp_rate_x10;
  bool cooler_enable;
  bool heater_enable;
};
typedef struct targetTemperature targetTemperature;

struct dioInfoStatus {
  int cnt;
  unsigned char data[8*9];
};
typedef struct dioInfoStatus dioInfoStatus;

struct pollArray {
  unsigned char poll_array [ 8 ];
};
typedef struct pollArray pollArray;

#define BB_BUFFER_MAX 2048
struct terBigBuffer {
  unsigned int ter_data_length;
  unsigned char ter_bb_data [ BB_BUFFER_MAX ];
};
typedef struct terBigBuffer terBigBuffer;

#define GET_SLOT_INFO_LEN_8   9
#define GET_SLOT_INFO_LEN_16 17
typedef struct TER_SlotInfo
{
  char SioPcbSerialNumber [ GET_SLOT_INFO_LEN_8  ];
  char SioAppVersion      [ GET_SLOT_INFO_LEN_16 ];
  char SioFpgaVersion     [ GET_SLOT_INFO_LEN_16 ];
  char DioPcbSerialNumber [ GET_SLOT_INFO_LEN_8  ];
  char DioFirmwareVersion [ GET_SLOT_INFO_LEN_16 ];
  char SioPcbPartNum      [ GET_SLOT_INFO_LEN_16 ];
  char SlotPcbPartNum     [ GET_SLOT_INFO_LEN_16 ];

} TER_SlotInfo;

struct TER_SlotSettings
{
  int power_supply_on_off;
  int baud_rate;
  int serial_logic_levels_mv;
  int temperature_target_x10;
  int temp_ramp_rate_cool_x10;
  int temp_ramp_rate_heat_x10;
  int slot_status_bits;        // TER_StatusBits
  int voltage_margin_3v_mv;
  int voltage_margin_5v_mv;
  int voltage_margin_12v_mv;
};
typedef struct TER_SlotSettings TER_SlotSettings;

typedef struct 
{
	bool targetTemperatureReached;
	int	positiveRampRate;			// in tenths of a Celsius/minute
	int currentTemperature;
	int targetTemperature;
	int targetTemperatureInitial;   // Initial target. unadjusted temperature target in deg C * 100
	int targetTemperatureSetTime;
	int htemp;						// htemp value sent by the test application in deg C * 100
	int htempAdjustmentSetTime;		// last time htemp adj was done. The time that we have set a new target temperature based on htemp
	int htempWaitTime;				// The time that we must wait before sending a new target temperature
	int timeDelta;
	int inletDelta;
	int tempDelta;

	int hTempAdjustmentEnabled;         // config [0] 
  	int hTempAdjustmentWaitTimeOffset;  // config [1] 
  	int hTempDeltaMin;                  // config [2] 
  	int hTempDeltaMax;                  // config [3] 
  	int hTempAdjustmentIncrementSize;   // config [4] 
  	int hTempAdjustmentMaxSetPoint;     // config [5] 

} TER_HtempInfo; 


#define NEPTUNE_DIO_CMD_SIZE 8
#define NEPTUNE_SLOTID_MAX 140
#define NEPTUNE_RECEIVE_PACKET_LEN_MAX 256

//
// On-Wire Commands
//

// implemented in rocketNet

#define TER_TCP_COMMAND_RESET_DIO           1
#define TER_TCP_COMMAND_PING                2
//#define TER_TCP_COMMAND_DOWNLOAD_FIRMWARE   3
#define TER_TCP_COMMAND_DIO_COMMAND         4
#define TER_TCP_COMMAND_set_registers       5
#define TER_TCP_COMMAND_get_registers       7
#define TER_TCP_COMMAND_modify_registers    23
#define TER_TCP_COMMAND_get_sio_info_status 58
#define TER_TCP_COMMAND_Sio_Receive_Huge_Buffer 8
#define TER_TCP_COMMAND_SEND_BUFFER         9
#define TER_TCP_COMMAND_SEND_BUFFER_ACK    10

#define TER_TCP_COMMAND_SetInterCharacterDelay 11
#define TER_TCP_COMMAND_GetInterCharacterDelay 12

#define TER_TCP_COMMAND_SetACKTimeout          13
#define TER_TCP_COMMAND_GetACKTimeout          14

#define TER_TCP_COMMAND_HugeBufferWriteRead    15

#define TER_TCP_COMMAND_ForceSingleByteTransmit 16

#define TER_TCP_COMMAND_TwoByteOneByte         17

#define TER_TCP_COMMAND_GetLastErrors          18

#define TER_TCP_COMMAND_ClearTxHalt            19

#define TER_TCP_COMMAND_ClearCurrentJournal    20
#define TER_TCP_COMMAND_SetJournalMask         21
#define TER_TCP_COMMAND_SnapshotCurrentJournal 22
#define TER_TCP_COMMAND_StamperToString        24

#define TER_TCP_COMMAND_SetTemperatureEnvelope 25
#define TER_TCP_COMMAND_GetTemperatureEnvelope 26

#define TER_TCP_COMMAND_IsDioPresent           27

#define TER_TCP_COMMAND_UpgradeMicrocode       28

#define TER_TCP_COMMAND_Read9240Register       29
#define TER_TCP_COMMAND_Write9240Register      30
#define TER_TCP_COMMAND_LoadFieldScreen        31

#define TER_TCP_COMMAND_EnableCmd9Polling      32
#define TER_TCP_COMMAND_GetCmd9PollArray       33
#define TER_TCP_COMMAND_SetFanDutyCycle        34
#define TER_TCP_COMMAND_SetInterPacketDelay    35
#define TER_TCP_COMMAND_Set5V12VSequenceTime   36

// combined TER_IsDioPresent + TER_UpgradeMicrocode
#define TER_TCP_COMMAND_siInitialize           37

// implemented in RPC

#define TER_TCP_COMMAND_Dio_Command           1
#define TER_TCP_COMMAND_Read_Fpga_Register    2
#define TER_TCP_COMMAND_Write_Fpga_Register   3
#define TER_TCP_COMMAND_Sio_Send_Buffer       4
#define TER_TCP_COMMAND_Sio_Receive_Buffer    5
#define TER_TCP_COMMAND_Set_Serial_Parameters 6
#define TER_TCP_COMMAND_Set_Serial_Enable     7
#define TER_TCP_COMMAND_Set_Power_Enabled     8
#define TER_TCP_COMMAND_PingXDR               9
#define TER_TCP_COMMAND_ResetSlot            10
#define TER_TCP_COMMAND_SetDioPollOptions    11
#define TER_TCP_COMMAND_SioReboot            12
#define TER_TCP_COMMAND_SetSerialSelect      13
#define TER_TCP_COMMAND_GetDIOInfoStatus     14
#define TER_TCP_COMMAND_IsSlotThere          15
#define TER_TCP_COMMAND_SetSerialLevels      16
#define TER_TCP_COMMAND_GetSlotSettings      17
#define TER_TCP_COMMAND_SetTargetTemperature 18
#define TER_TCP_COMMAND_SetTempControlEnable 19
#define TER_TCP_COMMAND_GetSlotStatus        20
#define TER_TCP_COMMAND_SetPowerVoltage      21
#define TER_TCP_COMMAND_GetSlotInfo          22
#define TER_TCP_COMMAND_GetUserString        23
#define TER_TCP_COMMAND_SetUserString        24
#define TER_TCP_COMMAND_SetSignal            25
#define TER_TCP_COMMAND_IsDrivePresent       26
#define TER_TCP_COMMAND_GetErrorLog          27
#define TER_TCP_COMMAND_Read_N_Fpga_Register 28
#define TER_TCP_COMMAND_StartSlotPoll        29
#define TER_TCP_COMMAND_GetSlotPoll          30
#define TER_TCP_COMMAND_DioMultiCmd          31
#define TER_TCP_COMMAND_GetTargetTemperature 32

#define XTER_TCP_COMMAND_Dio_Command           0x78000001
#define XTER_TCP_COMMAND_Read_Fpga_Register    0x78000002
#define XTER_TCP_COMMAND_Write_Fpga_Register   0x78000003
#define XTER_TCP_COMMAND_Sio_Send_Buffer       0x78000004
#define XTER_TCP_COMMAND_Sio_Receive_Buffer    0x78000005
#define XTER_TCP_COMMAND_Set_Serial_Parameters 0x78000006
#define XTER_TCP_COMMAND_Set_Serial_Enable     0x78000007
#define XTER_TCP_COMMAND_Set_Power_Enabled     0x78000008
#define XTER_TCP_COMMAND_PingXDR               0x78000009
#define XTER_TCP_COMMAND_ResetSlot             0x7800000a
#define XTER_TCP_COMMAND_SetDioPollOptions     0x7800000b
#define XTER_TCP_COMMAND_SioReboot             0x7800000c
#define XTER_TCP_COMMAND_SetSerialSelect       0x7800000d
#define XTER_TCP_COMMAND_GetDIOInfoStatus      0x7800000e
#define XTER_TCP_COMMAND_IsSlotThere           0x7800000f
#define XTER_TCP_COMMAND_SetSerialLevels       0x78000010
#define XTER_TCP_COMMAND_GetSlotSettings       0x78000011
#define XTER_TCP_COMMAND_SetTargetTemperature  0x78000012
#define XTER_TCP_COMMAND_SetTempControlEnable  0x78000013
#define XTER_TCP_COMMAND_GetSlotStatus         0x78000014
#define XTER_TCP_COMMAND_SetPowerVoltage       0x78000015
#define XTER_TCP_COMMAND_GetSlotInfo           0x78000016
#define XTER_TCP_COMMAND_GetUserString         0x78000017
#define XTER_TCP_COMMAND_SetUserString         0x78000018
#define XTER_TCP_COMMAND_SetSignal             0x78000019
#define XTER_TCP_COMMAND_IsDrivePresent        0x7800001a
#define XTER_TCP_COMMAND_GetErrorLog           0x7800001b
#define XTER_TCP_COMMAND_Read_N_Fpga_Register  0x7800001c
#define XTER_TCP_COMMAND_StartSlotPoll         0x7800001d
#define XTER_TCP_COMMAND_GetSlotPoll           0x7800001e
#define XTER_TCP_COMMAND_DioMultiCmd           0x7800001f
#define XTER_TCP_COMMAND_GetTargetTemperature  0x78000020

// implement in Marshal.py on-the-wire commands

#define MARTER_TCP_COMMAND_ResetSlot             1001
#define MARTER_TCP_COMMAND_Set_Power_Enabled     1002
#define MARTER_TCP_COMMAND_Set_Serial_Parameters 1003
#define MARTER_TCP_COMMAND_Set_Serial_Enable     1004
#define MARTER_TCP_COMMAND_Sio_Send_Buffer       1005
#define MARTER_TCP_COMMAND_Sio_Receive_Buffer    1006
#define MARTER_TCP_COMMAND_IsSlotThere           1007
#define MARTER_TCP_COMMAND_GetSlotInfo           1008
#define MARTER_TCP_COMMAND_GetSlotSettings       1009
#define MARTER_TCP_COMMAND_GetSlotStatus         1010
#define MARTER_TCP_COMMAND_GetUserString         1011
#define MARTER_TCP_COMMAND_SetUserString         1012
#define MARTER_TCP_COMMAND_SetTempControlEnable  1013
#define MARTER_TCP_COMMAND_SetTargetTemperature  1014
#define MARTER_TCP_COMMAND_SetPowerVoltage       1015
#define MARTER_TCP_COMMAND_SetSerialLevels       1016
#define MARTER_TCP_COMMAND_SetSerialSelect       1017
#define MARTER_TCP_COMMAND_SetSignal             1018
#define MARTER_TCP_COMMAND_Debug	         1024


#define MARTER_TCP_COMMAND_GetTargetTemperature  1020

// Private Teradyne commands:
#define MARTER_TCP_COMMAND_DioCmd          0x01000000

// not in spec and not implemented:
#define MARTER_TCP_COMMAND_IsDrivePresent        1019
//#define MARTER_TCP_COMMAND_Dio_Command           1020
//#define MARTER_TCP_COMMAND_Read_Fpga_Register    1021
//#define MARTER_TCP_COMMAND_Write_Fpga_Register   1022
//#define MARTER_TCP_COMMAND_PingXDR               1023

// not in spec and not implemented:
#define MARTER_TCP_COMMAND_Dio_Command           1020
#define MARTER_TCP_COMMAND_Read_Fpga_Register    1021
#define MARTER_TCP_COMMAND_Write_Fpga_Register   1022
#define MARTER_TCP_COMMAND_PingXDR               1023

//
// Error Codes for TER_Status
//

//typedef unsigned int TER_Status;

#define TER_Status_none                   0
#define TER_Status_dio_timeout            1
#define TER_Status_socket_not_open        2
#define TER_Status_slotid_error           3
#define TER_Status_Invalid_Mode           4    // DUT power off or serial port not enabled.
#define TER_Status_Error_Argument_Invalid 5    // bufferLength < 0 or > buffer size (2048)
#define TER_Status_receive_packet_len_err 6
#define TER_Status_invalid_reset_type     7
#define TER_Status_data_length_error      8
#define TER_Status_linux_error            9
#define TER_Status_send_buffer_already_in_progress 10
#define TER_Status_not_safe_to_transmit   11
#define TER_Status_no_room_in_output_silo 12
#define TER_Status_file_not_found         13
#define TER_Status_cant_open_file         14
#define TER_Status_cant_read_file         15
#define TER_Status_ucode_update_errors    16
#define TER_Status_unknown_status         17
#define TER_Status_Buffer_Overflow        18
#define TER_Status_missing_tuple          19
#define TER_Status_drive_power_off        20
#define TER_Status_dio_sequence_error     21
#define TER_Status_rpc_fail               22
#define TER_Status_Timeout                23
#define TER_Status_NotImplemented         24
#define TER_Status_Read_Only_Value        25
#define TER_Status_Unknown_Pseudo_Addr    26
#define TER_Status_Invalid_Baud_Rate      27
#define TER_Status_Invalid_Stop_Bits      28
#define TER_Status_Not_Supported          29
#define TER_Status_No_Target_Temp         30
#define TER_Status_ucode_upgrade_busy     31
#define TER_Status_dio_invalid_request    32
#define TER_Status_ucode_file_too_big     33
#define TER_Status_no_client_connection   34
#define TER_Status_high_ucode_ugrade_cnt  35

// Note: These must correspond to dio_control.h DIO_STATUS_xxx
#define TER_Status_DIO_STATUS_timed_out               TER_Status_dio_timeout
#define TER_Status_DIO_STATUS_slot_busy               37
#define TER_Status_DIO_STATUS_no_dio                  38
#define TER_Status_DIO_STATUS_slotid_out_of_range     39
#define TER_Status_DIO_STATUS_cant_recover_from_error 40
#define TER_Status_DIO_STATUS_retrys_exhausted        41
#define TER_Status_DIO_STATUS_bad_slot_number         42
#define TER_Status_DIO_STATUS_command_drained         43
#define TER_Status_DIO_STATUS_response_out_of_seq     44
#define TER_Status_DIO_STATUS_dio_reported_error      45
#define TER_Status_DIO_STATUS_command_mismatch        46
#define TER_Status_DIO_STATUS_dio_rx_cksum_error      47
#define TER_Status_DIO_STATUS_force_25msec_error      48

// Used for TER_SioHugeBufferWriteRead()
#define TER_Status_64k_count_exceeded     49
#define TER_Status_transmitter_is_halted  50

// bad case in super_state
#define TER_Status_invalid_super_state_case 51

// extra receive chars from dio
#define TER_Status_extra_receive_chars_from_dio 52

// see TER_Status neptune_sio2::TER_RegisterForEvent ( SLOT_MASK *slot_mask, enum registerable_event_enum event_mask )
// for details
enum registerable_event_enum {
  registerable_event_drive_present = 1
};

#ifdef __cplusplus

typedef struct slot_mask_struct {
  unsigned int slot_mask [ 5 ];
} SLOT_MASK;

struct modifyRegistersStruct {
  unsigned int register_address;
  unsigned int value_mask;
  unsigned int register_value;
};
typedef struct modifyRegistersStruct modifyRegistersStruct;

struct setRegistersStruct {
  unsigned int register_address;
  unsigned int register_value;
};
typedef struct setRegistersStruct setRegistersStruct;

struct getRegistersCmdStruct {
  unsigned int register_address;
};
typedef struct getRegistersCmdStruct getRegistersCmdStruct;

struct getRegistersRespStruct {
  unsigned int register_value;
};
typedef struct getRegistersRespStruct getRegistersRespStruct;

class Neptune_sio2
{
 private:

  /* Default timeout can be changed using clnt_control() */
  struct timeval TIMEOUT; // = { 25, 0 };

  bool uart_off;

  char connect_ipaddress [ 32 ];
  int fd;
  bool neptune_sio2_trace_flag;
  int last_errno;
  /* RPC */
  CLIENT *client;
  /* Marshal.py */
  bool enable_marshal;

#if defined(USE_THREAD_SAFETY)
  pthread_mutex_t pthread_mutex;
#endif

  // range check slotid
  // Note: SIO2_Application will adjust slotid as necessary
  bool ss_check_slotid ( int slotid ) {
    bool res = true;
    if ( enable_marshal ) {
      if ( slotid < 1 || slotid > NEPTUNE_SLOTID_MAX ) {
	res = false;
      }
    } else {
      if ( slotid < 0 || slotid >= NEPTUNE_SLOTID_MAX ) {
	res = false;
      }
    }
    return res;
  }

  // get touple count
  int get_touple_count ( int fct ) {
    int res = 0;

    if ( enable_marshal ) {
      switch ( fct & 0xffffff )
	{
	case TER_TCP_COMMAND_Dio_Command:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_Read_Fpga_Register:
	  res = 2;
	  break;
	case TER_TCP_COMMAND_Write_Fpga_Register:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_Sio_Send_Buffer:
	  res = 4;
	  break;
	case TER_TCP_COMMAND_Sio_Receive_Buffer:
	  res = 5;
	  break;
	case TER_TCP_COMMAND_Set_Serial_Parameters:
	  res = 5;
	  break;
	case TER_TCP_COMMAND_Set_Serial_Enable:
	  res = 4;
	  break;
	case TER_TCP_COMMAND_Set_Power_Enabled:
	  res = 5;
	  break;
	case TER_TCP_COMMAND_PingXDR:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_ResetSlot:
	  res = 4;
	  break;
	case TER_TCP_COMMAND_SetSerialSelect:
	  res = 4;
	  break;
	case TER_TCP_COMMAND_IsSlotThere:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_SetSerialLevels:
	  res = 4;
	  break;
	case TER_TCP_COMMAND_SetTargetTemperature:
	  res = 8;
	  break;
	case TER_TCP_COMMAND_GetTargetTemperature:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_GetSlotSettings:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_SetTempControlEnable:
	  res = 4;
	  break;
	case TER_TCP_COMMAND_GetSlotStatus:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_SetPowerVoltage:
	  res = 5;
	  break;
	case TER_TCP_COMMAND_GetSlotInfo:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_GetUserString:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_SetUserString:
	  res = 4;
	  break;
	case TER_TCP_COMMAND_IsDrivePresent:
	  res = 3;
	  break;
	case TER_TCP_COMMAND_SetSignal:
	  res = 5;
	  break;

	default:
	  printf("%s: Warning: unknown touple count\n",__FUNCTION__);

	  return 0;
      }
    }

    return res;
  }

  // begin encoding a rocketNet packet
  unsigned int encodeRocketNetLen (int cnt)
  {
    unsigned int x;

    x = 'r';
    x <<= 24;
    x |= (cnt & 0xffffff);
    x = htonl(x);
    return x;
  }

  // begin decoding a rocketNet packet
  int decodeRocketNetLen (unsigned int x)
  {
    int res;

    x = ntohl(x);
    res = x & 0xffffff;
    return res;
  }

  // receive a packet
  unsigned int do_recv ( void *buf,
			 int cnt,
			 int flag )
  {
    int sts = 1;
    char *wbuf = (char *)buf;

  retry:;
    //if ( sts > 0 ) printf("do_recv: waiting of %d bytes\n", cnt);

    sts = recv (fd,wbuf,cnt,flag);

    //if ( sts > 0 ) printf("do_recv: received %d bytes\n",sts);

    if ( 0 == sts ) {
      goto retry;
    }
    if ( sts == -1 ) {
      last_errno = errno;
      if ( neptune_sio2_trace_flag ) {
	printf("%s: recv error = %d, <%s>\n",
	       __FUNCTION__,
	       errno,
	       strerror(errno));
      }
      return TER_Status_linux_error;
    }
    if ( cnt ) {
      wbuf += sts;
      cnt -= sts;
      if ( 0 != cnt )
	goto retry;
    }

    return TER_Status_none;
  }

  // send a packet
  unsigned int do_send ( void *buf, int cnt, int flag )
  {
    int sts;

    /* send buffer */
    sts = send ( fd, buf, cnt, 0 );
    if ( sts < 0 || sts != cnt ) {
      last_errno = errno;
      if ( neptune_sio2_trace_flag ) {
	printf("%s: error sending = %d <%s>\n",
	       __FUNCTION__,
	       errno,
	       strerror(errno));
      }
      sts = TER_Status_linux_error;
    }
    return TER_Status_none;
  }

  // get file size
  unsigned int get_file_size ( char *file_name ) {
    struct stat sb;

    if ( stat( file_name , &sb ) < 0 ) {
      return 0;
    }
    return (unsigned int)sb.st_size;
  }

 public:

  unsigned char SIO;

  /*!
   * \b Function: Neptune_sio2 constructor
   */
  Neptune_sio2 ( void )
    {
      connect_ipaddress[0] = 0;

      TIMEOUT.tv_sec = 60;
      TIMEOUT.tv_usec = 0;

      uart_off = true;

      enable_marshal = FALSE;
      fd = -1;
      client = NULL;
      neptune_sio2_trace_flag = false;

#if defined(USE_THREAD_SAFETY)
      pthread_mutex_init(&pthread_mutex,NULL);
#endif
    };

  /*!
   * \b Function: Neptune_sio2 constructor
   *
   * @param [ in ] c_trace_flag Set to TRUE to enable tracing
   */
  Neptune_sio2 ( bool c_trace_flag )
    {
      connect_ipaddress[0] = 0;

      TIMEOUT.tv_sec = 60;
      TIMEOUT.tv_usec = 0;

      uart_off = true;

      enable_marshal = FALSE;
      fd = -1;
      client = NULL;
      neptune_sio2_trace_flag = c_trace_flag;

#if defined(USE_THREAD_SAFETY)
      pthread_mutex_init(&pthread_mutex,NULL);
#endif
    };

  /*!
   * \b Function: Neptune_sio2 destructor
   */
  virtual ~Neptune_sio2 ( void )
    {
      // don't leak client
      if ( client ) {
	xclnt_destroy(client);
	client = NULL;
      }

      close(fd);
      fd = -1;
#if defined(USE_THREAD_SAFETY)
      pthread_mutex_destroy(&pthread_mutex);
#endif
    }

  //
  // Begin GNR
  //
//Warning: Automatically generated function. See gnr for details.
//
// TER_Get12VRiseAndFallTimes
//
TER_Status TER_Get12VRiseAndFallTimes ( int slotid, int *rise_time_ms, int *fall_time_ms );
//Warning: Automatically generated function. See gnr for details.
//
// TER_Get5VRiseAndFallTimes
//
TER_Status TER_Get5VRiseAndFallTimes ( int slotid, int *rise_time_ms, int *fall_time_ms );
//Warning: Automatically generated function. See gnr for details.
//
// TER_GetTemperatureEnvelope
//
TER_Status TER_GetTemperatureEnvelope ( int slotid, double *degrees );
//Warning: Automatically generated function. See gnr for details.
//
// TER_GetTemperatureReachedDelay
//
TER_Status TER_GetTemperatureReachedDelay ( int slotid, int *temp_reached_seconds );
//Warning: Automatically generated function. See gnr for details.
//
// TER_Set12VRiseAndFallTimes
//
TER_Status TER_Set12VRiseAndFallTimes ( int slotid, int rise_time_ms, int fall_time_ms );
//Warning: Automatically generated function. See gnr for details.
//
// TER_Set5VRiseAndFallTimes
//
TER_Status TER_Set5VRiseAndFallTimes ( int slotid, int rise_time_ms, int fall_time_ms );
//Warning: Automatically generated function. See gnr for details.
//
// TER_SetFanRPMClamp
//
TER_Status TER_SetFanRPMClamp ( int slotid, int fan_rpm_clamp );
//Warning: Automatically generated function. See gnr for details.
//
// TER_SetTemperatureEnvelope
//
TER_Status TER_SetTemperatureEnvelope ( int slotid, double degrees );
//Warning: Automatically generated function. See gnr for details.
//
// TER_SetTemperatureReachedDelay
//
TER_Status TER_SetTemperatureReachedDelay ( int slotid, int temp_reached_seconds );
  //
  // End GNR
  //

//
// TER_siInitialize ( int slotid )
//
 TER_Status TER_siInitialize ( int slotid );

//
// TER_Set5V12VSequenceTime ( int slotid, int time )
//
 TER_Status TER_Set5V12VSequenceTime ( int slotid, int time );

//
// TER_GetNeptuneSio2BuildVersion ( unsigned int *build_version )
//
// Where:
//        int *build_version is returned
//
 TER_Status TER_GetNeptuneSio2BuildVersion ( unsigned int *build_version );

//
// TER_SetFanDutyCycle ( int slotid, int duty_cycle )
//
// Where:
//    slotid     is [0..139]
//    duty_cycle is [20..65]
//
 TER_Status TER_SetFanDutyCycle ( int slotid, int duty_cycle );

//
// TER_SetInterPacketDelay ( int slotid, int micro_seconds )
//
// Where:
//    slotid        is [0..139]
//    micro_seconds is [0..2550]
//
 TER_Status TER_SetInterPacketDelay ( int slotid, int micro_seconds );

  //
  // cmd9_poll engine
  //
 typedef struct cmd9_status_struct_t {
   unsigned int css_status;    // completion status - see TER_Status
   unsigned char css_reply[8]; // and the data as presented by the dio
 } CMD9_STATUS;

 //
 // TER_EnableCmd9Polling ( int seconds )
 //
 // On Call:
 //
 //    seconds = 0, turn off cmd9 pollilng engine
 //    seconds 1..10, start the cmd9 polling engine with the indicated interval
 //
 TER_Status TER_EnableCmd9Polling ( int seconds );

 //
 // TER_GetCmd9PollArray ( CMD9_STATUS *resp )
 //
 // On Return:
 //
 //    'resp' must be an array of 140 CMD9_STATUS blocks that will be filled
 //           in from cached data from the SIO
 //
 TER_Status TER_GetCmd9PollArray ( CMD9_STATUS *resp );

  //
  // TER_LoadFieldScreen
  //
  TER_Status TER_LoadFieldScreen ( int slotid,
				   char *file_name );

  //
  // TER_Read9240Register
  //
  // On Call:
  //
  //  slotid   - the slotid, origin 0
  //  i2c_addr - address of 9240 on i2c bus
  //  cnt      - number of bytes to read: 1,2, or 4
  //  addr     - address to read
  //
  TER_Status TER_Read9240Register ( int slotid,
				    int i2c_addr, int cnt, unsigned int addr,
				    unsigned int *result );
  //
  // TER_Write9240Register
  //
  TER_Status TER_Write9240Register ( int slotid,
				     int i2c_addr, int cnt, unsigned int addr,
				     unsigned int data );

  //
  // TER_UpgradeMicrocode ( int slotid )
  //
  TER_Status TER_UpgradeMicrocode ( int slotid );

  //
  // TER_IsDioPresent
  //
  TER_Status TER_IsDioPresent ( int slotid, DP *dp );

  //
  // TER_SetTemperatureEnvelope
  //
  TER_Status TER_SetTemperatureEnvelope ( int slotid, unsigned int envelope );
  //
  // TER_GetTemperatureEnvelope
  //
  TER_Status TER_GetTemperatureEnvelope ( int slotid, unsigned int *envelope );

  //
  // TER_dio_recv (for testing only)
  //
  TER_Status TER_dio_recv ( char *buf, int cnt );

  //
  // TER_ClearTxHalt
  //
  TER_Status TER_ClearTxHalt ( int slotid );

  //
  // TER_GetLastErrors
  //
  //
  // Note: 'buf' must be ERRLOG_LOG_SIZE in size
  //
  TER_Status TER_GetLastErrors ( int slotid, char *buf );

  //
  // TER_TwoByteOneByte
  //
  // On Call
  //
  //   slotid 0-> 139
  //
  //   val - 0 - Disable
  //         1 - Enable  (default)
  //
  TER_Status TER_TwoByteOneByte ( int slotid, int val );

  //
  // TER_ForceSingleByteTransmit
  //
  TER_Status TER_ForceSingleByteTransmit ( int slotid, int single_byte );

  //
  // TER_SetACKTimeout
  //
  // Note:
  //
  //  slotid
  //
  //    0 -> 139
  //
  //  timeout_index
  //
  //    0 - 100 msec (default)
  //    1 - 500 msec
  //    2 - 1000 msec
  //    3 - 2000 msec
  //
  TER_Status TER_SetACKTimeout ( int slotid, int timeout_index );

  //
  // TER_GetACKTimeout
  //
  // Note:
  //
  //  slotid
  //
  //    0 -> 139
  //
  //  timeout_index
  //
  //    0 - 100 msec (default)
  //    1 - 500 msec
  //    2 - 1000 msec
  //    3 - 2000 msec
  //
  TER_Status TER_GetACKTimeout ( int slotid, int *timeout_index );

  //
  // TER_SetInterCharacterDelay
  //
  TER_Status TER_SetInterCharacterDelay ( int slotid, int val );
  //
  // TER_GetInterCharacterDelay
  //
  TER_Status TER_GetInterCharacterDelay ( int slotid, int *val );

  //
  // TER_SetInternalFPGALoopback
  //
  // val - if 1, set loopback
  //       if 0, clear loopback
  //
  TER_Status TER_SetInternalFPGALoopback ( int slotid, int val );

  //
  // TER_DioMultiCmd
  //
  TER_Status TER_DioMultiCmd ( SLOT_MASK *slot_mask, unsigned char *dio_command );

  //
  // TER_StartSlotPoll
  //
  TER_Status TER_StartSlotPoll ( int slotid, int cnt, unsigned char *dio_command );

  //
  // TER_GetSlotPoll
  //
  TER_Status TER_GetSlotPoll ( int slotid, int *cnt, unsigned char *dio_response );

  //
  // TER_SetSATAPort
  //
  TER_Status TER_SetSATAPort ( int slotid, int sata_byte3 );

  //
  // TER_GetSATAPort
  //
  TER_Status TER_GetSATAPort ( int slotid, int *OUTPUT );

  //
  // TER_SetSignal
  //
  TER_Status TER_SetSignal ( int slotid,
			     TER_Signals signalMask,
			     TER_Signals signalState );

  //
  // TER_GetErrorLog
  //
  TER_Status TER_GetErrorLog ( int slotid, char *error_string );

  //
  // TER_IsDrivePresent
  //
  TER_Status TER_IsDrivePresent ( int slotid, int *slot_information );

  //
  // TER_GetUserString
  //
  TER_Status TER_GetUserString ( int slotid, char *str );

  //
  // TER_SetUserString
  //
  TER_Status TER_SetUserString ( int slotid, char *str );

  //
  // TER_SetPowerVoltage
  //
  TER_Status TER_SetPowerVoltage ( int slotid, int select_mask, int supply_value_mv );

  //
  // TER_SetTargetTemperature
  //
  TER_Status TER_SetTargetTemperature ( int slotid, struct targetTemperature *tt );

  //
  // TER_GetTargetTemperature
  //
  TER_Status TER_GetTargetTemperature ( int slotid, struct targetTemperature *tt );

  //
  // TER_SetSerialLevels
  //
  // Note: On certain platforms, if logic_levels_mv is equal to zero,
  //       the call will ground the TX line to the drive, then return.
  //       See USE_TX_GROUND_ON_PUP for details in config.h of the
  //       SIO2_Application.
  //
  //       Othersize, logic_levels_mv must be between 1200 and 3300 or
  //       the call will fail.
  //
  TER_Status TER_SetSerialLevels ( int slotid, int logic_levels_mv );

  //
  // TER_IsSlotThere
  //
  TER_Status TER_IsSlotThere ( int slotid );

  //
  // TER_GetDIOInfoStatus
  //
  TER_Status TER_GetDIOInfoStatus ( int slotid, dioInfoStatus *data );

  //
  // TER_SendFile
  //
  // Send a binary file to the hdd or initiator
  //
  // Useful for downloading microcode into the initiator.
  //
  TER_Status TER_SendFile ( int slotid, char *filename );

  //
  // TER_SetSlotPower
  //
  // Power Up/Down a slot based on dio command 0x04
  //
  TER_Status TER_SetSlotPower ( int slotid, int onoff_flag );

  //
  // TER_SetSerialSelect
  //
  // val
  //     0 - send data to hdd
  //     1 - send data to initiator
  //
  TER_Status TER_SetSerialSelect ( int slotid,
				      int val );

  //
  // TER_GetRegisters
  //
  TER_Status TER_GetRegisters ( int cnt,
				struct getRegistersCmdStruct *grcs,
				struct getRegistersRespStruct *grrs );

  //
  // TER_SetRegisters
  //
  TER_Status TER_SetRegisters ( int cnt,
				struct setRegistersStruct *srs );

  //
  // TER_ModifyRegisters
  //
  TER_Status TER_ModifyRegisters ( int cnt,
				   struct modifyRegistersStruct *mrs );

  //
  // TER_SetDioLeds
  //
  TER_Status TER_SetDioLeds ( int slotid, int flag );

  //
  // TER_SetSocketFileno
  //
  // Useful for opening a fd with python, then using the swig binding
  //
  // Example:
  //
  //    TER_SetSocketFileno ( socket.fileno() );
  //
  TER_Status TER_setSocketFileno ( int fd );

  //
  // TER_ShowPollArray
  //
  TER_Status TER_ShowPollArray ( struct pollArray *pa );

  //
  // TER_SioReboot
  //
  TER_Status TER_SioReboot ( void );

  //
  // TER_ConnectionType
  //
  TER_Status TER_ConnectionType ( int type );

  //
  // TER_SetDioPollOptions
  //
  TER_Status TER_SetDioPollOptions ( int refresh_period, struct pollArray *poll_array );

  //
  // TER_makeAddr_build
  //
  unsigned int TER_makeAddr_build ( unsigned int fpga_select,
				    unsigned int register_group,
				    unsigned int slot_select,
				    unsigned int specific_register_select );

  //
  // ReadFpgaRegister
  //
  // Note: WARNING: You can crash the SIO2_Application if you use an incorrect
  //                address.
  //
  // Some Handy Pseudo Addresses
  //
  //  0xda000000 - code_revision;
  //  0xda000004 - i2c_example_readmfg (   );
  //  0xda000008 - timestamp
  //  0xda00000c - i2c_example_read    ( 0 ); - temperature ppc    S8.8
  //  0xda000010 - cpld_revision
  //  0xda000014 - uboot_revision;
  //  0xda000018 - i2c_example_read    ( 1 ); - temperature fpga   S8.8
  //  0xda00001c - i2c_example_read    ( 2 ); - temperature tmp422 S8.8
  //  0xda000020 - port_pins_get       (   );
  //
  TER_Status TER_ReadFpgaRegister ( unsigned int addr, unsigned int *response );
  //
  // ReadNFpgaRegister - read multiple fpga registers where
  //   n [ 1 .. 140 ]
  //
  TER_Status TER_ReadNFpgaRegister ( int n, unsigned int *addr, unsigned int *response );

  //
  // TER_WriteFpgaRegister
  //
  // Note: WARNING: You can crash the SIO2_Application if you use an incorrect
  //                address.
  //
  TER_Status TER_WriteFpgaRegister ( unsigned int addr, unsigned int data );

  //
  // Connect to SIO2
  //
  TER_Status TER_Connect ( char *ipaddress );
  // overload
  TER_Status TER_Connect ( void ) {
    return TER_Connect ( (char *)"10.101.1.2" );
  }

  //
  // Disconnect from SIO2
  //
  TER_Status TER_Disconnect ( void );

  //
  // Enable Slot Power
  //
  TER_Status TER_SetPowerEnabled ( int slotid, int selectMask, int powerOnOff );

  //
  // Send data to drive - use XDR
  //
  TER_Status TER_SioSendBuffer(int slotid, unsigned int bufferLength, unsigned char *buffer);

  //
  // Send data to drive - use rocketNet
  //
  TER_Status TER_SioSendBufferRn(int slotid, unsigned int bufferLength, unsigned char *buffer);

  //
  // Get data from drive - use XDR
  //
  TER_Status TER_SioReceiveBuffer(int slotid,
				  unsigned int *dataLength,
				  unsigned int timeout,
				  unsigned char *buffer,
				  unsigned int *received_status_word_1);

  //
  // Get data from drive - use XDR/Big Buffer Mapping for SWIG
  //
  TER_Status TER_SioReceiveBufferBB(int slotid, unsigned int timeout, struct terBigBuffer *big_buffer );

  //
  // Huge Buffer Write Read
  //
  //   This call will first write 'write_cnt' bytes of data pointed to by 'write_ptr'
  //   then immediately, it will begin a read of '*read_cnt' bytes.
  //
  //   'write_cnt' can be between 0 and 2048 bytes
  //   '*read_cnt' can be between 0 and 65536 bytes
  //
  //   maximum_64k_count_allowed is the maximum number of allowable 64k operations, either in
  //   progress or pending that is allowed before this command will execute.
  //   Because each FPGA (every 35 slots) contains two 64k buffers, usable values
  //   may be:
  //
  //     -1 - turns off feature
  //      0 - no one else may be using a 64k buffer.
  //      1 - there is one other person using a 64k buffer
  //      2 - there are two persons using 64k buffers but no one is pended
  //      2+N - there are two persons using 64k buffers and N persons are pended
  //
  //   If maximum_64k_count_allowed exceeds the number on the SIO2 for that FPGA,
  //   an error will be returned (TER_Status_64k_count_exceeded) with NO DATA send or
  //   received from the FPGA. In this case, the call must be re-issued sometime later.
  //
  //   Note: values 0,1 should be used for testing only.
  //
  //   For example, if you set maximum_64k_count_allowed to five, the call will succeed
  //   if there are two current 64k buffer uses, and three users are pended waiting.
  //
  //   On Return: '*read_cnt' will be the number of bytes read into 'read_ptr'
  //
  //   If 'timeout_seconds' is 0 on call, the write will proceed, then the call
  //   will return immediately with however many bytes can be read without waiting.
  //
  TER_Status TER_SioHugeBufferWriteRead ( int slotid,
					  int write_cnt, unsigned char *write_ptr,
					  int *read_cnt, unsigned char *read_ptr,
					  int timeout_seconds,
					  unsigned int *received_status_word_1,
					  int maximum_64k_count_allowed );

  //
  // Send huge buffer to drive
  //
  TER_Status TER_SioSendHugeBuffer(int slotid, unsigned int bufferLength, unsigned char *buffer,
				   unsigned int addrs, unsigned int addrs_cnt );
  
  //
  // Get huge buffer from drive
  //
  // Note: Not supported on all platforms.
  //
  TER_Status TER_SioReceiveHugeBuffer ( int slotid, unsigned int *dataLength, int timeout, unsigned char *buffer,
					unsigned int addrs, unsigned int addrs_cnt );

  //
  // Execute a DIO Command
  //
  TER_Status TER_DioCommand ( int slotid, unsigned char *cmd, unsigned char *resp );
  //
  // Execute a DIO Command for swig
  //
  TER_Status TER_DioCommandS ( int slotid, char *cmd, char *dioCmdResp );

  //
  // Ping
  //
  TER_Status TER_Ping ( int slotid );

  //
  // PingXDR
  //
  TER_Status TER_PingXDR ( int slotid );

#if 0
  //
  // ResetSlotRn
  //
  TER_Status TER_ResetSlotRn ( int slotid, enum TER_ResetType reset_type );
#endif

  //
  // ResetSlot
  //
  TER_Status TER_ResetSlot ( int slotid, enum TER_ResetType reset_type );

  //
  // Set Update DIO Micrcode Mask
  //
  TER_Status TER_SetUpdateDIOMicrocodeMask ( int slotid,
					     SLOT_MASK *slot_mask);

  //
  // Get Update DIO Microcode Mask
  //
  // Returns: TRUE if slotid bit is set in slot_mask
  //
  bool TER_GetUpdateDIOMicrocodeMask ( int slotid,
				       SLOT_MASK *slot_mask );

#if 0
  //
  // Update DIO Micrcode
  //
  TER_Status TER_UpdateDIOMicrocode ( SLOT_MASK *slot_mask,
				      int firmware_type,
				      char *file_name );
#endif

  //
  // TER_Status TER_ConvertStatusToString(int slotID, TER_Status status, char statusStr[])
  //
  // Converts the return value from any method in this interface to a human
  // read-able string.
  //
  // Parameters:
  // "status" - The return value
  // "statusStr[]" - A buffer to hold the returned string.
  //
  // Errors: Unknown status.
  //
  TER_Status TER_ConvertStatusToString(int slotid, TER_Status status, char *statusStr);

  // TER_Status TER_SetSerialEnable(int slotID, bool enableSerial)
  //
  // Enables the serial port
  //
  // Parameters:
  //  "enableSerial" - If True, serial pins are powered and active
  //                   If false, no power is applied to serial pins
  //
  // Notes:
  //
  //   - Turning DUT power off will also disable the serial pins in the proper sequence
  //   - Turning DUT power on will NOT enable the serial pins.
  //   - TER_SetSerialEnable(true) will have no effect if the DUT power is off.
  //   - TER_SetSerialEnable(false) will remove power from the serial pins regardless
  //     of the state of DUT power.
  //
  //   - ??? TER TODO: Reset will disable the serial port.
  //
  // Notes:
  //
  //    This command notifies the DIO to enable/disable the serial pins but
  //    does not modify the UART enable bit in the FPGA, which is always left on.
  //
  TER_Status TER_SetSerialEnable(int slotid, bool enableSerial);

  //
  // TER_Status TER_GetSlotInfo(int slotid, TER_SlotInfo* pSlotStatus);
  //
  // Get all fixed info about a slot.(Serial number, versions, capabilities, etc..)
  //
  // Parameters:
  //
  // "pSlotInfo" - pointer to struct of type TER_SlotInfo
  //
  TER_Status TER_GetSlotInfo(int slotid, TER_SlotInfo* pSlotInfo);

  //
  // TER_Status TER_GetSlotSettings
  //
  TER_Status TER_GetSlotSettings ( int slotid, TER_SlotSettings* settings );

  //
  // TER_Status TER_GetSlotStatus(int slotID, TER_SlotStatus* pSlotStatus);
  //
  // Get all variable slot environmental and setup status data.
  //
  // Parameters:
  // "pSlotStatus" - pointer to struct of type TER_SlotStatus
  //
  TER_Status TER_GetSlotStatus(int slotid, TER_SlotStatus* pSlotStatus);

  // Set the temperate target and the temperature ramp rate
  //
  //
  // Parameters:
  // "tempeTargetX10"  - final stabilized value of the temperature (measured after the drive) in degrees celsius X10
  //                     Legal Temperature Range:  See Spec
  // "tempRampRateX10" - Temperature ramp rate in degrees celsiusX10 per minut minute
  //                     Legal Ramp Rates:  See Spec
  //
  // Example:
  //
  //	result=TER_SetTempTarget(355, 10) // Go to 35.5 degrees C at a rate of
  //                                    // 1.0 degrees C per minute
  //
  //
  // Errors:	TER_Status_Invalid_Argument		// Illegal target or rate.
  //
  TER_Status TER_SetTempTarget(int slotid, int tempTargetX10, int temperatureRampRateX10);

  //
  // TER_SetTempControlEnable
  //
  TER_Status TER_SetTempControlEnable ( int slotid, bool enable );

  // TER_Status TER_GetThermalDemand(int slotID, int pCoolDemandPct, int pHeatDemandPct);
  //
  // Gets values indicating how hard the slot is working to cool and heat the
  // drive.
  //
  // Parameters:
  //
  // "pCoolDemand" - Returns the percent of cooling capability being used.
  // "pHeatDemand" - Returns the percent of heating capability being used.
  //
  // TODO: Should these values just be part of the VarStatus struct?
  //
  TER_Status TER_GetThermalDemand(int slotid, int pCoolDemandPct, int pHeatDemandPct);

  //
  // TER_Status TER_GoToSafeTemp(int slotid, int safeTemperature_x10);
  //
  // Take drive to a safe temperature (typically for removal purposes)
  // *** Only cooling will be used ***
  //
  // Notes: Temperature Regulation must be enabled explicitly.
  //		  It is recommended to set ramp rate to maximum to ensure quick completion.
  //
  // Parameters:
  //
  // enable_disable_flag - 1 = enable, 0 = disable
  //
  TER_Status TER_GoToSafeTemp(int slotid, int enable_disable_flag);

  //
  // TER_Status TER_SetSerialBaudRate(int slotID, int baudRate_bps);
  //
  // Bit rate of the serial port, in units of bits/seconds
  //
  // Parameters:
  // "baudRate_bps" - actual symbol rate in bits per second
  //
  // Errors:	TER_Status_Error_Argument_Invalid:  If baudRate_bps is not supported
  //
  TER_Status TER_SetSerialBaudRate(int slotid, int baudRate_bps);

  //
  // TER_Status TER_SetSerialParameters ( )
  //
  // Sets the parameters appropriate for the serial port
  //
  // Parameters:
  //
  //    baudRate
  //
  //      0 - 19.2
  //      1 - 38.4
  //      2 - 57.6
  //      3 - 115.2
  //
  //    numStopBits
  //
  //      1 .. 8
  //
  // Errors:
  //
  //
  TER_Status TER_SetSerialParameters(int slotId, int baudRate, int numStopBits );

  //
  // TER_RegisterForEvent ( SLOT_MASK *slot_mask, enum event_enum event_mask )
  //
  // register for event
  //
  // On Call:
  //
  //    slot_mask - one bit set for each of the 140 slotids to monitor
  //
  //    event_mask - a bitmask of the events you wish to register for
  //
  // On Return:
  //
  //    TER_Status
  //
  // Notes:
  //
  //    ??? TODO - When an event is detected, what action should the SIO_Super_Application take?
  //
  // Notes:
  //
  //    Here's a simple example of registering slotid 5 for registerable_event_drive_present
  //
  //      memset(slot_mask,0,sizeof(SLOT_MASK));
  //      TER_SetUpdateDIOMicrocodeMask ( 5, slot_mask );
  //      TER_RegisterForEvent ( slot_mask, registerable_event_drive_present );
  //
  TER_Status TER_RegisterForEvent ( SLOT_MASK *slot_mask,
				    enum registerable_event_enum event_mask );

  //
  // TER_UnRegisterForEvent ( SLOT_MASK *slot_mask, enum event_enum event_mask )
  //
  // unregister for event
  //
  // On Call:
  //
  //    slot_mask - one bit set for each of the 140 slotids to monitor
  //
  //    event_mask - a bitmask of the events you wish to register for
  //
  // On Return:
  //
  //    TER_Status
  //
  // Notes:
  //
  TER_Status TER_UnRegisterForEvent ( SLOT_MASK *slot_mask,
				      enum registerable_event_enum event_mask );

  /*!
   * \b Function: Conditions the neptune_sio2 library to use MRPC proticol
   *
   * @param [ in ] enable Enable or disable the marshal code generation
   * @returns TER_Status
   * @deprecated Warning: Do not call.
   */
  TER_Status TER_EnableMarshal ( bool enable ) {
    enable_marshal = enable;
    return TER_Status_none;
  }

  //
  // Journaling
  //
  typedef enum ns2_journal_mask_e {
    NS2_Journal_none = 0,
    NS2_Journal_states = 0x100 // (TMSK_stamper)
  } NS2_JOURNAL_MASK;

#define SMALL_STAMPER

  // Note: duplicated in ../sockTunnel-1.0/stamper.h
  // Note: must be multiple of 4
  typedef struct stamper_struct {
    struct timeval stamper_tv;
    unsigned char stamper_type;
    char stamper_slotid;
    unsigned short stamper_ss_state;

#if defined(SMALL_STAMPER)
    unsigned short stamper_ss_completion_status;
#else
    int stamper_ss_completion_status;

    unsigned short stamper_cnt; // if we go to stamp, and we already have this
                                // particular stamp event, we just increment
                                // this count
#endif
    unsigned short stamper_sub_type;

    // used by STAMPER_TYPE_dio
    unsigned char stamper_cmd  [ NEPTUNE_DIO_CMD_SIZE ];
    unsigned char stamper_resp [ NEPTUNE_DIO_CMD_SIZE ];

  } STAMPER;

  typedef struct ns2_journal_struct_t {
    int cnt; // number of stamper elements
    STAMPER stamper[];
  } NS2_JOURNAL_BUFFER;

  //
  // GetStamperSlotNumber
  //
  int TER_GetStamperSlotNumber ( STAMPER *s ) {
    return s->stamper_slotid;
  }

  //
  // StamperIsSetJournalMask
  //
  int TER_StamperIsSetJournalMask ( STAMPER *s ) {
    if ( 1 == s->stamper_type && (199<<8) == s->stamper_ss_state ) {
      return 1;
    }
    return 0;
  }

  //
  // Enable/ Disable journaling on a slot
  //
  TER_Status TER_SetJournalMask ( int slotid, NS2_JOURNAL_MASK mask );

  //
  // Snapshot Current Journal
  //
  // Note: The system will allocate an NS2_JOURNAL_BUFFER and
  //       it is the callers responsibility to free it.
  //
  TER_Status TER_SnapshotCurrentJournal ( int slotid, NS2_JOURNAL_BUFFER **b );

  //
  // Stamper to String
  //
  TER_Status TER_StamperToString ( STAMPER *s, int *str_len, char *str );

  //
  // Free an NS2_JOURNAL_BUFFER
  //
  TER_Status TER_FreeJournalBuffer ( NS2_JOURNAL_BUFFER *b );

  //
  // Clear a journal on the SIO
  //
  TER_Status TER_ClearCurrentJournal ( int slotid );

  //
  // Set hitachi temp - htemp algorithm
  //
  TER_Status TER_SetHitachiTemp(int slotid, TER_HtempInfo *slot);

};

#endif // __cplusplus
#endif // __neptune_sio2_h__
