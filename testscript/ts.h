#ifndef _TS_H_
#define _TS_H_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <setjmp.h>

/* ---------------------------------------------------------------------- */
#if defined(LINUX)
/* ---------------------------------------------------------------------- */
#include <stdlib.h>
#include <time.h>
#include <unistd.h>  // for sleep

#include "../testcase/tcTypes.h"
#include "../slotio/libsi.h"
#include "../hostio/libhi.h"
/************* 2010.11.4 y.katayama for syncmanager ***********************/
#include "../chamberscript/libcs.h"
#include "../syncmanager/libsm.h"
/************* 2010.11.4 y.katayama for syncmanager ***********************/
#include "libts.h"
#include "../versioncontrol/libvc.h"   // 2011.06.14 Satoshi Takahashi

#define BLOCKSIZE (1)
#define ZData     (0x04)
#define ZCmd      (0x0A)
#define ZUpload (0x14)
#define TEST_TIME (0x5CE)
#define FINAL_MODE (2)

void put_que(QUE *que);
Byte is_que(void);
QUE *get_que(void);
void DI_(void);
void EI_(void);
Word getCellComModuleTaskNo(Byte bCellAddress);

/* ---------------------------------------------------------------------- */
#else   // else LINUX
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
#if defined(UNIT_TEST)
/* ---------------------------------------------------------------------- */
#include <stdlib.h>
#include <time.h>
#include <unistd.h>  // for sleep

#define BLOCKSIZE (1)
#define ZData     (0x04)
#define ZCmd      (0x0A)
#define ZUpload (0x14)


void put_que(QUE *que);
Byte is_que(void);
QUE *get_que(void);
void DI_(void);
void EI_(void);
Word getCellComModuleTaskNo(Byte bCellAddress);

/* ---------------------------------------------------------------------- */
#else  // else of UNIT_TEST
/* ---------------------------------------------------------------------- */
#include "..\equ.h"
#include "..\mos_sh3.h"
#include "..\common.h"
#include "..\task_no.h"
#include "..\CellIo\envApi.h"
#include "..\gaia.h"
Byte *sprintfdcom(Byte *buf, Byte *format, va_list ap);

/* ---------------------------------------------------------------------- */
#endif  // end of UNIT_TEST
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
#endif  // end of LINUX
/* ---------------------------------------------------------------------- */


enum {
  NUM_OF_CELL_CONTROLLER_TASK = 240,

  /*
   * <pre>
   * [Normal Task ID]
   *   HostComModule       0x64
   *   HostIoModule        0x65
   *   CellManager         0x01
   *   CellController      0x101 -to- 0x1e0
   *   CellComModule       0x10 -to- 0x2e
   *   CellIoModule        0x30
   *   EnvironmentControl  0x51 -to- 0x55
   * [OS Task ID]
   *   Master              0xff    Idle Task (Invalid QUE Handling)
   *   Mainte              0xf1    Writing CONT Card Firmware Image on Flash ROM
   *
   * </pre>
   *****************************************************************************/
  TOP_OF_CELL_CONTROLLER_TASK_ID = 0x101,

  /** It is useful for strcmp amd memcmp to be human readable. */
  MATCH = 0,

  /** Drive Communication Mode */
  WRITE_TO_DRIVE = 0,
  READ_FROM_DRIVE = 1,

  /** Drive Communication Protocol */
  READ_DRIVE_MEMORY_PROTOCOL = 0x41,
  WRITE_DRIVE_MEMORY_PROTOCOL = 0x42,
  IDENTIFY_DRIVE_PROTOCOL = 0x43,

  /** bAccessFlag in DRIVE_MEMORY_CONTROL_BLOCK */
  IS_DIRECT_ACCESS = 0,
  IS_INDIRECT_ACCESS,
  IS_IDENTIFY_ACCESS,
  IS_DIRECT_ACCESS_WITH_OFFSET,
  IS_INDIRECT_ACCESS_WITH_OFFSET,

  /** bEndianFlag in DRIVE_MEMORY_CONTROL_BLOCK */
  IS_ENDIAN_8BIT = 0,
  IS_ENDIAN_16BIT_LITTLE_TO_8BIT,
  IS_ENDIAN_32BIT_LITTLE_TO_8BIT,
  IS_ENDIAN_16BIT_LITTLE_TO_16BIT,
  IS_ENDIAN_32BIT_LITTLE_TO_32BIT,
  IS_ENDIAN_16BIT_BIG_TO_16BIT,

  /** bReportFlag in DRIVE_MEMORY_CONTROL_BLOCK */
  IS_REPORT_NOT_REQUIRED = 0,
  IS_REPORT_BEFORE_RAWDATA_DUMP = 1,
  IS_REPORT_AFTER_RAWDATA_DUMP = 2,
  IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP = 3,
  IS_REPORT_BEFORE_RAWDATA_DUMP_ONLY_SRST = 4,   /* same report with 1 */
  IS_REPORT_BEFORE_RAWDATA_DUMP_ONLY_FINAL = 5,   /* same report with 1 */
  IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_SRST = 6,    /* same report with 3 */
  IS_GET_BEFORE_REPORT_AFTER_RAWDATA_DUMP_ONLY_FINAL = 7,   /* same report with 3 */
  /**
   * Bit Assign of STATUS_AND_CONTROL
   *   wDriveEchoVersion =  0 : 8bit (PTB,PTC)
   *                     => 2 :16bit  (JPT or later)
   * <pre>
   * Bit              Off(0)                        On(1)
   * --- ---------------------------------  ---------------------------------
   *  0  No reporting                       Under reporting rawdata
   *  1  Comp. rawdata transfer   (Tester)  Ready rawdata             (SRST)
   *  2  Read comp. w/ successful (Tester)  Retry request             (Tester)
   *  3  Memory reporting comp.   (Tester)  Under memory reporting    (SRST)
   *  4  No FFT data               (SRST)   Available FFT data        (SRST)
   *  5  No MCS data               (SRST)   Available MCS data        (SRST)
   *  6  Comp. setup data          (SRST)   Under data setup process  (SRST)
   *  7  No error @ setup data     (SRST)   Error @ setup data        (SRST)
   *
   *   8   Reserved
   *   9                                        Grading flag from SRST (final mode)
   *  10   Reserved
   *  11   Reserved
   *  12   Reserved
   *  13   Reserved
   *  14   Reserved
   *  15                                        Request clear reporting limit timer to SRST
  * </pre>
   ***************************************************************************************
   2010/05/13 Support JPT or later 16bit Rawdata flag Y.Enomoto
   ***************************************************************************************/
  IS_REPORTING_COMPLETE = 0x00,
  IS_UNDER_REPORTING_MODE = 0x01,
  IS_RAWDATA_READY = 0x02,
  IS_RAWDATA_TRANSFER_COMP = ~0x02,
  IS_RETRY_REQUEST = 0x04,
  IS_CLEAR_RETRY_REQUEST = ~0x04,
  /*  IS_UNDER_MEMORY_REPORTING = 0x08, */
  IS_STOP_REQ_FROM_SRST = 0x08,
  IS_MEMORY_REPORTING_COMP = ~0x08,
  IS_PES_DATA_READY          = 0x10,
  IS_FFT_DATA_AVAILABLE = 0x10,   /* no meaurement item for JET/EB7 PES data and etc  parametric data head version*/
  IS_PARAMETRIC_DATA_READY   = 0x20, /* parametric data unit version*/
  IS_MCS_DATA_AVAILABLE = 0x20,   /* no meaurement item for JET/EB7 */
  IS_UNDER_DATA_SETUP_PROCESS = 0x40,
  IS_ERROR_AT_SETUP = 0x80,
  IS_REPORTING_ERROR_FLAG    = 0x80,
  IS_GRADING_MODEL           = 0x200,
  IS_GRADING_MODEL_CLEAR     = ~0x200,
  IS_CLEAR_REPORTING_TIMER   = ~0x8000,
  CLEAR_TIMER_BYTE = 0x80,

  /**
   * Bit Assign of Polling Action State
   * <pre>
   *  Bit              Off(0)                        On(1)
   *  --- ---------------------------------  ---------------------------------
   *   0   Get temperature with report status to PC
   *   1   Get SRST step
   *   2   Get SRST Status & Control flag
   *   3   Reserved
   *   4   Reserved
   *   5   Reserved
   *   6   Reserved
   *   7   Reserved
   * </pre>
   ***************************************************************************************/
  IS_GET_TEMPERATURE = 0x01,
  IS_GET_SRST_STEP = 0x02,
  IS_GET_STATUS_AND_CONTROL = 0x04,
  IS_RAWDATA_READ_STATUS = 0x08,

  /**
    * Bit Assign of IMMEDIATE_FLAG
    * <pre>
    *  Bit              Off(0)                        On(1)
    *  --- ---------------------------------  ---------------------------------
    *   0   Under testing                      Test time out
    *   1   Under testing                      Emergency power shutdown
    *   2   Reserved
    *   3   Reserved
    *   4   Reserved
    *   5   Reserved
    *   6   Reserved
    *   7   Reserved
    * </pre>
    ***************************************************************************************/
  IS_TEST_TIME_OUT = 0x01,
  IS_EMERGENCY_POWER_SHUTDOWN = 0x02,

  /**
   * Bit Assign of UART Report Status flag
   * <pre>
   * Bit 7654 3210
   * --- ---------------------------------  ---------------------------------
   *     0000 0000                           UnderSRST
   *     0000 0001                           SRSTComp. AND UnderUARTReport
   *     0000 0010                           SRSTComp. AND (UARTReportComp. OR TimeOut)
   *     others                              Reserved
   * </pre>
   ***************************************************************************************/
  IS_UNDER_SRST = 0x00,
  IS_SRST_COMP_AND_UNDER_UART_REPORT = 0x01,
  IS_SRST_COMP_AND_UNDER_UART_REPORT_OR_TIME_OUT = 0x02,


  /**
   * Bit Assign of Re-SRST prohibition flag
   * <pre>
   *  Bit              Off(0)                        On(1)
   *  --- ---------------------------------  ---------------------------------
   *   0   Reserved
   *   1   Reserved
   *   2   Reserved
   *   3   Re-SRST OK                         Re-SRST NG
   *   4   Reserved
   *   5   Reserved
   *   6   Reserved
   *   7   Reserved
   * </pre>
   ***************************************************************************************/
  IS_RE_SRST_NG = 0x08,

  /**
   * Bit Assign of Uart flag
   * <pre>
   *  Bit              Off(0)                        On(1)
   *  --- ---------------------------------    ---------------------------------
   *   0   Result reporting disable             Result reporting enable
   *   1   Request prepair next block to SRST   Data ready
  * </pre>
   ***************************************************************************************/
  IS_UARTFLAG_TIMEOUT = 0x01,
  IS_UARTFLAG_BURSTMODE = 0x02,

  /** getDriveRawdata FSM state list. */
  GET_DRIVE_RAWDATA_FSM_before_initialize = 0,
  GET_DRIVE_RAWDATA_FSM_initialize,
  GET_DRIVE_RAWDATA_FSM_get_drive_error_code,
  GET_DRIVE_RAWDATA_FSM_get_drive_data_before_rawdata_dump,
  GET_DRIVE_RAWDATA_FSM_get_parameters_for_srst_result,
  GET_DRIVE_RAWDATA_FSM_wait_till_srst_result_ready_on_drive_memory,
  GET_DRIVE_RAWDATA_FSM_dump_srst_result,
  GET_DRIVE_RAWDATA_FSM_check_drive_error_for_srst_result,
  GET_DRIVE_RAWDATA_FSM_request_next_page_for_srst_result,
  GET_DRIVE_RAWDATA_FSM_get_parameters_for_pes,
  GET_DRIVE_RAWDATA_FSM_wait_till_pes_ready_on_drive_memory,
  GET_DRIVE_RAWDATA_FSM_dump_pes,
  GET_DRIVE_RAWDATA_FSM_check_drive_error_for_pes,
  GET_DRIVE_RAWDATA_FSM_request_next_page_for_pes,
  GET_DRIVE_RAWDATA_FSM_get_parameters_for_mcs,
  GET_DRIVE_RAWDATA_FSM_wait_till_mcs_ready_on_drive_memory,
  GET_DRIVE_RAWDATA_FSM_dump_mcs,
  GET_DRIVE_RAWDATA_FSM_check_drive_error_for_mcs,
  GET_DRIVE_RAWDATA_FSM_request_next_page_for_mcs,
  GET_DRIVE_RAWDATA_FSM_get_drive_data_after_rawdata_dump,
  GET_DRIVE_RAWDATA_FSM_finalize,

  /** identifyDrive FSM state list. */
  IDENTIFY_DRIVE_FSM_before_initialize = 0,
  IDENTIFY_DRIVE_FSM_data_from_Echo_before_sequence,
  IDENTIFY_DRIVE_FSM_srst_sequence,
  IDENTIFY_DRIVE_FSM_data_from_Echo_after_sequence,
  IDENTIFY_DRIVE_FSM_serial_number,
  IDENTIFY_DRIVE_FSM_mfg_id,
  IDENTIFY_DRIVE_FSM_mfg_id_on_label,
  IDENTIFY_DRIVE_FSM_status,
  IDENTIFY_DRIVE_FSM_signature,
  IDENTIFY_DRIVE_FSM_environment,
  IDENTIFY_DRIVE_FSM_report_to_host,

  /** identifyDrive(Echo command) Max Data size. */
  DRIVE_ECHO_DATA_SIZE = 512,

  /** error code */
  ERROR_CODE_EXT_cannot_get_sn = 0x00050000,                  /**< GOD5 */
  ERROR_CODE_EXT_cannot_communicate_with_drive = 0x00010001,  /**< GOD2 */
  ERROR_CODE_EXT_unit_reason = 0x00030000,                    /**< GOD3 */
  ERROR_CODE_EXT_reporting_timeout = 0x00070000,              /**< GOD7 */
  ERROR_CODE_EXT_RawdataPagecorrupt = 0x0006000A,             /**< JPT:6116 PTB:GOD6 */
  ERROR_CODE_EXT_detect_abnormalreporting = 0x00060000,      /**< GOD6 */
  ERROR_CODE_EXT_cannot_finalizereporting = 0x00020004,      /**< 6116 */
  ERROR_CODE_EXT_no_error_code = 0x0002FF00,                 /**< 6xxx [COM_22] */
  ERROR_CODE_EXT_re_srst_ok = 0x00040000,                     /**< 6110 */
  ERROR_CODE_EXT_re_srst_ng = 0x00040002,                     /**< 6100 */
  ERROR_CODE_srst_incomplete_re_srst_ng = 0x30,               /**< 6100 */
  ERROR_CODE_srst_incomplete_re_srst_ok = 0x31,               /**< 6110 */
  ERROR_CODE_EXT_enough_time_final = 0x80002,                 /**< 5910 */

  ERROR_CODE_grading_nopass2ID = 0x00068180,    /* NO pass2ID in SW tbale [2STEP_02] */
  ERROR_CODE_grading_noswtb = 0x00068181,     /* Nothing SW tbale with Grading flag [2STEP_02] */
  ERROR_CODE_grading_wrpass2ID = 0x00068182,    /* Write pass2 ID fail [2STEP_02] */
  ERROR_CODE_grading_pending = 0x00068183,    /* Detect "PEND" Signeture fail [2STEP_02] */
  ERROR_CODE_grading_swtabel = 0x0006818F,

  /** error code for abort */
  ERROR_CODE_EXT_cell_insertion_check_error = 0x000f9930,     /**< 9930 */
  ERROR_CODE_EXT_cell_voltage_error = 0x000f9931,             /**< 9931 */
  ERROR_CODE_EXT_cell_temperature_error = 0x000f9932,         /**< 9932 */
  ERROR_CODE_EXT_cell_abort_by_test_script = 0x000f9933,      /**< 9933 */
  ERROR_CODE_EXT_cell_abort_by_user = 0x000f9934,             /**< 9934 */


  /** grading error sub code */
  NO_PASS2ID = 0x01,
  NO_SW_TBL = 0x02,
  SET_PASS2ID_ERROR = 0x04,
  PENDING_ERROR = 0x08,

  /** tester log report mode */
  IF_DRIVE_PASS = 0x01,
  IF_DRIVE_FAIL = 0x02,
  IF_DRIVE_GODx = 0x04,

  /** com test mode */
  NORMAL_TEST_SCRIPT = 0,
  HOST_COM_LOOPBACK_TEST,
  HOST_COM_DOWNLOAD_TEST,
  HOST_COM_MESSAGE_TEST,
  HOST_COM_UPLOAD_TEST,
  DRIVE_COM_READ_TEST,
  DRIVE_COM_READ_THEN_HOST_COM_UPLOAD_TEST,
  DRIVE_COM_ECHO_TEST,
  POWER_ON_ONLY_TEST,
  DRIVE_POLL_ONLY_TEST,
  DRIVE_TEMPERATURE_TEST,
  DRIVE_VOLTAGE_TEST,
  DRIVE_COM_ECHO_TEST_WITH_POWER_CYCLE,

  /** com test mode range */
  HOST_COM_UPLOAD_TEST_MIN_DATA_SIZE = 5,
  HOST_COM_UPLOAD_TEST_MAX_DATA_SIZE = (63 * 1024),
  DRIVE_COM_READ_TEST_MIN_DATA_SIZE = 1,
  DRIVE_COM_READ_TEST_MAX_DATA_SIZE = (63 * 1024),

  /** abort */
  ABORT_REQUEST_BY_USER = 1,
  ABORT_REQUEST_BY_TEST_SCRIPT,

  /** voltage limit */
  VOLTAGE_LIMIT_UART_LOW_MV = 1000,
  VOLTAGE_LIMIT_UART_HIGH_MV = 3300,
  VOLTAGE_LIMIT_5V_LOW_MV = 4750,
  VOLTAGE_LIMIT_5V_HIGH_MV = 5250,
  VOLTAGE_LIMIT_12V_LOW_MV = 10800,
  VOLTAGE_LIMIT_12V_HIGH_MV = 13200,

  /** uart protocol */
  UART_PROTOCOL_IS_A00 = 0,
  UART_PROTOCOL_IS_ARM,

  /** misc. */
  MAX_CONFIG_FILE_SIZE_IN_BYTE = (63 * 1024),
  MAX_SIZE_OF_DMCB = 128,    /** Max size of DRIVE_MEMORY_CONTROL_BLOCK */
  MAX_NUMBER_OF_FORCE_STOP_PF_CODE = 16,    /** Max number of DRIVE_MEMORY_CONTROL_BLOCK */
  TSCB_SIGNATURE = 0x12345678,
  DRIVE_PLUG_RETRY_SEC = 10, /**< up to RS422 polling time */
  MAX_REPORT_DATA_SIZE_BYTE = (63 * 1024), /**< this + header size shall be < 63KB */
  DEFAULT_LOG_SIZE_IN_KBYTE = 16,
  MAX_LOG_SIZE_IN_KBYTE = 2048, /**< [DEBUG] shall be smaller in production level code */
  DEFAULT_LOG_MIRROR_TO_STDOUT = 0,
  MAX_SIZE_OF_DUMP_BINARY = (8 * 1024),
  SLEEP_UNIT_SEC = 29,
  SLEEP_MODE_PARTIAL = 0,
  SLEEP_MODE_SLUMBER,
  SLEEP_TIME_AFTER_DRIVE_UNLOAD_SEC = 10,  /**< wait time from unload request to complete */
  SLEEP_TIME_BEFORE_RAWDATA_DUMP_FINALIZE = 5,  /**< wait time from last rawdata dump to finalize */
  MEMORY_WATER_MARK_THRESHOLD_IN_BYTE = (12 * 1024 * 1024), /**< 10% of total memory (128MB) */
  WAIT_TIME_SEC_FOR_MEMORY_ALLOC = 23,
  RAWDATA_RETRY_CONT = 16,
  FUNALIZE_RETRY_CONT = 12,
  /* ---------------------------------------------------------------------- */
#if defined(LINUX)
  /* ---------------------------------------------------------------------- */
  WAIT_TIME_SEC_FOR_DRIVE_RESPONSE = 30,
  UART_PULLUP_VOLTAGE_DEFAULT = 2700,  // add for 9000 2013.06.12 y.katayama
  /* ---------------------------------------------------------------------- */
#else   // else of LINUX
  /* ---------------------------------------------------------------------- */
  WAIT_TIME_SEC_FOR_DRIVE_RESPONSE = 31, /**< [DEBUG] shall be larger than [DriveComTimeout] x [NumOfCellInOnePort] */
  /* ---------------------------------------------------------------------- */
#endif  // end of LINUX
  /* ---------------------------------------------------------------------- */
  WAIT_TIME_SEC_FOR_DRIVE_RESPONSE_POLL = 2,
  MAX_UART_DATA_SIZE_BYTE = (63 * 1024), /**< this + header size shall be < 63KB */
  DRIVE_ACCESS_RETRY_INTERVAL_SEC = 3,
  DRIVE_SECTOR_SIZE_BYTE = 512,
  WAIT_TIME_SEC_FOR_DRIVE_IDENTIFY_POLL = 10,
  WAIT_TIME_SEC_FOR_DRIVE_RAWDATA_FLAG_POLL = 0, /**< Note: 1 is too long. sleep(0), or usleep(100*1000) is better */
  WAIT_TIME_SEC_FOR_HOST_RESPONSE = 37,
  WAIT_TIME_SEC_FOR_HOST_RESPONSE_POLL = 2,
  WAIT_TIME_SEC_FOR_RELEASE_CPU = 0,    /**< [DEBUG] need optimization */
  RELEASE_CPU_TRIGGER = 10,
  DRIVE_PES_HEADER_SIZE = 16,
  CELL_TEST_LIBRARY_VERSION_SIZE = 24,
  DOWNLOAD_FILE_CHUNK_SIZE = 15000,
  DOWNLOAD_FILE_NAME_COMPARE_SIZE = 16,
  WAIT_TIME_SEC_FOR_CHAMBER_SCRIPT_RESPONSE_POLL = 1,
};

#define TESTCODE_VERSION                     "Script ver019KPK 2Step"

#define MY_TASK_ID get_task()
#define MY_CELL_NO (MY_TASK_ID - TOP_OF_CELL_CONTROLLER_TASK_ID) /**< 0, 1, 2, ... [TOP_OF_CELL_CONTROLLER_TASK_ID - 1]*/

#define LOGIC_CARD_SERVER_STR                "LTSServer"
#define MESSAGE_ID_STR                       "setSerial:"
#define MESSAGE_GRADING_ID_STR               "setGradeSerial:"
#define MESSAGE_GRADING_ID_STR_WITHFLAG      "setGradeSerialWithFlag:"
#define MESSAGE_STATUS_STR                   "uartstatus:"
#define MESSAGE_BAD_HEAD_STR                 "badHead:"
#define MESSAGE_PRECOMP_COMMAND_STR          "srstPreComp:rc:resultFile:pfTable:"
#define MESSAGE_COMP_COMMAND_STR             "srstComp:rc:resultFile:pfTable:"
#define MESSAGE_TEST_TIMEOUT                 "setTestTimeout:"

#define MESSAGE_FUNCTION_NAME_STR            "SELFTEST"
#define MESSAGE_PRECOMP_FUNCTION_NAME_STR    "SRSTFAIL"
#define MESSAGE_COMMAND_STR                  "msg:"
#define MESSAGE_HOST_RESPONSE_FOR_FILE       "recvFile:"
#define MESSAGE_HOST_RESPONSE_FOR_COMP       "U????"

#define MESSAGE_DOWNLOAD_CMD_STR             "downloadCMDCompare:"
#define MESSAGE_DOWNLOAD_SYNC_STR            "downloadSyncCompare:"

#define MESSAGE_SEPARATOR                    'S'
#define INTEGER_SEPARATOR                    'L'
#define SIGNATURE_SRST_RUNNING               "SELF"
#define SIGNATURE_SRST_ABORT                 "ABOR"
#define SIGNATURE_SRST_COMPLETE              "COMP"
#define SIGNATURE_SRST_PENDING               "PEND"

#define SIGNATURE_SELFSRST     "SELFSRST"
#define SIGNATURE_COMPSRST     "COMPSRST"
#define SIGNATURE_SELFFIN0     "SELFFIN0"
#define SIGNATURE_SELFFIN1     "SELFFIN1"
#define SIGNATURE_COMPFIN1     "COMPFIN1"
#define SIGNATURE_ABORFIN1     "ABORFIN1"
#define SIGNATURE_ABORFIN0     "ABORFIN0"
#define SIGNATURE_ABORSRST     "ABORSRST"
#define SIGNATURE_PENDSRST     "PENDSRST"
#define SIGNATURE_PENDFIN0     "PENDFIN0"
#define SIGNATURE_PENDFIN1     "PENDFIN1"

#define OPTIMUS 1

#define KEY_SRST_TOP_ADDRESS                "SRST_TOP_ADDRESS    "
#define KEY_SERIAL_NUMBER                   "SERIALNUMBER        "
#define KEY_MFG_ID                          "MFGID               "
#define KEY_SRST_PARAMETER                  "SRSTPARAMETER       "
#define KEY_SRST_SEQUENCE                   "SRSTALLSTATUS       "
#define KEY_SRST_SEQUENCE2                  "SRSTALLSTATUS       "
#define KEY_IDENTIFY                        "IDENTIFY            "
#define KEY_MICRO_REVISION_LEVEL            "MicroRevisionLevel  "
#define KEY_SERVO_SECTOR_NUMBER             "ServoSectorNumber   "
#define KEY_HEAD_NUMBER                     "HeadNumber          "
#define KEY_HEAD_TABLE                      "HEADTABLE           "
#define KEY_HEAD_CONVERTION_TABLE           "HEADTABLE           "
#define KEY_TPI_BPI                         "TPI_BPI_Table       "
#define KEY_SPINDLE_RPM                     "SpindleRpm          "
#define KEY_MFGID_ON_LABEL                  "MFGID_ON_LABEL      "
#define KEY_TEMPERATURE                     "TEMPERATURE         "
#define KEY_STEP_COUNT                      "STEP_COUNT          "
#define KEY_STATUS_AND_CONTROL              "STATUS_AND_CONTROL  "
#define KEY_STATUS_AND_CONTROL_B            "STATUS_AND_CONTROL_B"
#define KEY_IMMEDIATE_FLAG                  "IMMEDIATE_FLAG      "
#define KEY_UART_FLAG                       "UART_FLAG           "
#define KEY_LAST_PAGE                       "LASTPAGE            "
#define KEY_LAST_POINTER                    "LASTPOINTER         "
#define KEY_NUMBER_OF_SETUP_PAGE            "NUMOFSETUPPAGE      "
#define KEY_SRST_RESULT                     "SRSTRESULT          "
#define KEY_PES_TOTAL_NUMBER_OF_PAGE        "PESTOTALPAGE        "
#define KEY_PES_CYLINDER                    "PESCYLINDER         "
#define KEY_PES_HEAD                        "PESHEAD             "
#define KEY_PES_TEMPERATURE                 "PESTEMPERATURE      "
#define KEY_PES_DATA_COUNT                  "PESDATACOUNT        "
#define KEY_PES_SPINDLE_SPEED               "PESSPINDLESPEED     "
#define KEY_PES_NUMBER_OF_SERVO_SECTOR      "PESSERVOSECTOR      "
#define KEY_NUMBER_OF_SERVO_SECTOR          "SERVOSECTOR         "
#define KEY_PES_DATA                        "PESDATAUARTHD       "
#define KEY_DRIVE_PARAMETER_BYTE            "DriveParameterByte  "
#define KEY_DRIVE_PARAMETER                 "DriveParameter      "
#define KEY_ERROR_CODE                      "ERRORCODE           "
#define KEY_BAD_HEAD                        "badHead:            "
#define KEY_SRST_SIGNATURE                  "SRSTSIGNATURE       "
#define KEY_ALTTRK_TBL                      "ALTTRKTBL           "
#define KEY_ZONE_PARAMETER                  "ZONEPARAMETER       "
#define KEY_ZONE_IMAGE                      "ZONE_IMAGE          "
#define KEY_SRVM_IMAGE                      "SRVM_IMAGE          "
#define KEY_RE_SRST_PROHIBIT                "RESRSTPROHIBIT      "
#define KEY_GEOPARAMETER                    "GEOPARAMETER        "
#define KEY_TPI_INFORMATION                 "TPI_INFORMATION     "
#define KEY_PASS2ID                         "PASS2ID             "
#define KEY_GRADING_FLAG                    "GRADING_FLAG        "
#define KEY_GRADING_FLAG_WF                 "GRADING_FLAG_WF     "
#define KEY_OFFSET_HEAD                     "OFFSET_HEAD         "
#define KEY_GRADING_REPORT                  "GRADING             "
#define KEY_LIBRARY_VERSION                 "LIBRARY_VERSION     "   /* [019KP3] */
#define KEY_SWITCH_TABLE                    "SWITCH_TABLE        "
#define KEY_TIME_LOG                        "TIME_LOG            "
#define KEY_TESTER_TEMPERATURE_LOG          "EFTSORDATA          "
#define KEY_TESTER_UART_LOG                 "UARTCOMMUNICATIONLOG"
#define KEY_TESTER_CONFIG_LOG               "EFTSCONFIGDATA      "
#define KEY_TESTER_ERROR_LOG                "ErrorMessage       !"
#define KEY_TESTER_LOG                      "TESTERLOG           "
#define KEY_TESTER_RECORD                   "TESTERRECORD        "
#define KEY_TESTER_DUMMY_DATA               "TESTERDUMMYDATA     "
#define KEY_TESTER_VOLTAGE_DATA             "CONDITION           "


#define SWITCH_TABLE_NAME                   "gdinfo."

#define EACH_WORD_WRITE      6

#define SRST_PART       0x10
#define FINAL_PART       0x20

#define REPORT_PRECOMP      1
#define REPORT_COMP       2
#define RECEIVE_BUFFER_ID 1  //add for GOD5 problem  2013.06.12  y.katayama


Word wReadBlocksize;
Byte bTesterflag;
Word wParamPageNumber;


typedef struct test_script_recorder_block {
  Dword dwDriveTestTimeSec;
  Word wTempInHundredth;
  Word wV5InMilliVolts;
  Word wV12InMilliVolts;
  Byte bDriveTemperature;
  Byte bDriveStepCount;
} TEST_SCRIPT_RECORDER_BLOCK;

typedef struct test_script_abort_block {
  Byte isEnable;
  Byte isRequested;
  jmp_buf env;
} TEST_SCRIPT_ABORT_BLOCK;

typedef struct test_script_log_block {
  Byte isTesterLogMirrorToStdout;
  ZQUE *zque; /**< keep it for free memory */
  Byte *bLogTop;
  Byte *bLogCur;
  Byte *bLogEnd;
  Dword dwCurLogSizeInByte;
  Dword dwMaxLogSizeInByte;
} TEST_SCRIPT_LOG_BLOCK;

typedef struct uart_performance_log_block {
  Dword dwNumOfCommand;
  Dword dwNumOfCommandSuccess;
  Dword dwTotalDataSize;
} UART_PERFORMANCE_LOG_BLOCK;

typedef struct test_script_error_block {
  Byte isFatalError;
  Byte isAbortByUser;
  Byte isDriveUnplugged;
  Byte isDrivePowerFail;
  Byte isForceStopBySrst;
  Byte isCellTemperatureAbnormal;
  Byte isDriveTestTimeout;
  Byte isDriveIdentifyTimeout;
  Byte isDriveWriteImmidiateflagError;
  Byte isDriveWriteRetryRequestError;
  Byte isDriveRawdataDumpTimeoutError;
  Byte isDriveRawdataDumpReportingFlagError;
  Byte isDriveRawdataDumpDriveError;
  Byte isDriveRawdataDumpTotalPageError;
  Byte isDriveRawdataDumpPageIndexError;
  Byte isDriveRawdataDumpPesPageError;
  Byte isDriveRawdataFinalizeError;
  Byte isDriveRawdataDumpOtherError;
  Byte isDriveSinetureError;
  Byte isGradingError;    /* 0x01:no PASS2ID  0x02:SW table  0x04:Pass2ID write error  0x08:pening */
} TEST_SCRIPT_ERROR_BLOCK;

typedef struct drive_memory_control_block {
  Dword dwAddress;
  Dword dwAddressOffset;
  Word wSize;
  Byte bAccessFlag;
  Byte bEndianFlag;
  Byte bReportFlag;
  Byte bName[20+1];
} DRIVE_MEMORY_CONTROL_BLOCK;

typedef struct test_result_save_block {
  Byte *bHeaderNameaddress;
  Byte *bSaveaddressTop;
  Dword dwSavelengthInByte;
} TEST_RESULT_SAVE_BLOCK;

typedef struct test_script_control_block {
  /** SRST top address */
  Dword dwDriveSrstTopAddress;

  /** signature */
  Dword dwSignature;

  /** cell test library version */
  Byte bCellTestLibraryVersion[CELL_TEST_LIBRARY_VERSION_SIZE];

  /** host infomation */
  Word wHostID;
  Byte bTestResultFileName[128+1];
  Byte bTestPfTableFileName[128+1];

  /** environment information */
  Word wCurrentTempInHundredth;
  Word wCurrentV5InMilliVolts;
  Word wCurrentV12InMilliVolts;

  /** test params given by configuration file */
  Byte bTestMfgId[6+1];
  Byte bTestLabelMfgIdOffset;
  Byte bTestSerialNumber[8+1];
  Byte bTestDefaultBadHeadTable[10];
  Byte bTestUartProtocol;
  Dword dwTestUartBaudrate;
  Word wDriveForceStopPFcode[MAX_NUMBER_OF_FORCE_STOP_PF_CODE];

  /** Switching Table for Graging */
  Byte bSwitchTable[256];

  /** Drive Status  */
  Byte isDriveIdentifyComplete;
  Byte isDriveRawdataComplete;
  Word wDriveHeadNumber;
  Byte bDriveSerialNumber[8];
  Byte bDriveMfgId[6];
  Byte bDriveMfgIdOnLabel[6];
  Byte bDrivePass2Id[6];
  Byte bDriveMicroLevel[8];
  Byte bDriveHeadTable[10];
  Byte bDriveBPITPITable[20];
  Byte bDriveTemperature;
  Byte bDriveTemperatureMax;
  Byte bDriveTemperatureMin;
  Byte bDriveStepCount;
  Byte bDriveSkip_SRST_result;  /* [COM_22] */
  Byte bDriveSrstuartflag;
  Byte bDrivePORrequestflag;
  Word wDriveStatusAndControlFlag;
  Word wDriveReSrstProhibit;
  Dword dwDriveGradingFlag; /* MRN,MRK,MRS -> Word size EB7,JET,JGR -> DWord size */
  Dword dwDriveGradingWaterFallFlag;
  Byte bOffsetHead;
  Byte bPass2flag;
  Byte bStatusControl;

  Byte bCheckflag;

  Dword dwDriveResultTotalSizeByte;
  Dword dwDriveResultDumpSizeByte;
  Dword dwDrivePesTotalSizeByte;
  Dword dwDrivePesDumpSizeByte;
  Dword dwDriveParametricSizeByte;
  Dword dwDriveParametricDumpSizeByte;

  Dword dwDrivePesCylinder;
  Byte bDrivePesHeadNumber;
  Byte bDrivePesTemperature;
  Word wDrivePesPage[10];
  Word wDrivePesDataCount; /* MRN,MRK,MRS -> Reporting PES page / head */
  Word wDrivePesSpindleSpeed;
  Word wDrivePesNumberOfServoSector;
  Byte bDriveSignature[128];

  Word wDriveNativeErrorCode;
  Dword dwDriveExtendedErrorCode;

  /** Test Control given by test script file  */
  Dword dwDriveTestElapsedTimeSec;
  Dword dwDriveFinalTestElapsedTimeSec;
  Dword dwDriveTestIdentifyTimeoutSec;
  Dword dwDriveTestStatusPollingIntervalTimeSec;
  /* [019KPC] */
  Dword dwTimeoutSecFromStartToSRSTEndCutoff;   /* dwDriveTestCutoffTimeoutSec */
  Dword dwTimeoutSecFromStartToSRSTEnd;         /* dwDriveTestTimeoutSec */
  Dword dwTimeoutSecFromStartToFinalEndCutoff;  /* dwDriveFinalTestCutoffTimeoutSec */
  Dword dwTimeoutSecFromStartToFinalEnd;        /* dwDriveFinalTestTimeoutSec */


  Dword dwDriveTestRawdataDumpTimeoutSec;



  Dword dwDriveAmbientTemperatureTarget;
  Dword dwDriveAmbientTemperaturePositiveRampRate;
  Dword dwDriveAmbientTemperatureNegativeRampRate;

  Dword dwDrivePowerSupply5V;
  Dword dwDrivePowerSupply12V;
  Dword dwDrivePowerRiseTime5V;
  Dword dwDrivePowerRiseTime12V;
  Dword dwDrivePowerIntervalFrom5VTo12V;
  Dword dwDrivePowerOnWaitTime;
  Dword dwDrivePowerOffWaitTime;
  Dword dwDriveUartPullup;
  Word wDriveEchoVersion;
  Word wDriveECClength;
  Word wDriveMaxbufferpagesize;


  /** Debug Contorol given by test script file  */
  Byte isNoPlugCheck;
  Byte isNoPowerControl;
  Byte isNoPowerOffAfterTest;
  Byte isNoDriveFinalized;
  Byte isBadHeadReport;
  Byte isMultiGrading;
  Byte isDriveVoltageDataReport;
  Byte isReSrstProhibitControl;
  Byte isTpiSearch;
  Word wTpiSearchOffset;
  Word wTpiSearchSize;

  Byte isForceDriveTestCompFlagOn;
  Byte isForceDriveUnload;
  Byte isTesterLogReportEnable;
  Byte isTesterLogMirrorToStdout;
  Byte isFakeErrorCode;
  Dword dwFakeErrorCode;
  Dword dwTesterLogSizeKByte;
  Byte isTestMode;
  Dword dwTestModeParam1;
  Dword dwTestModeParam2;
  Dword dwTestModeParam3;

  /** time stamp */
  Dword dwTimeStampAtTestStart;
  Dword dwTimeStampAtFinalTestStart;
  Dword dwTimeStampAtTurnDrivePowerSupplyOn;
  Dword dwTimeStampAtIdentifyDrive;
  Dword dwTimeStampAtWaitDriveTestCompleted;
  Dword dwTimeStampAtGetDriveRawdata;
  Dword dwTimeStampAtFinalize;
  Dword dwTimeStampAtTestEnd;
  Dword dwTimeStampAtSRSTTestEnd;

  /** uart perfmance */
  UART_PERFORMANCE_LOG_BLOCK uartPerformanceLogBlock[IDENTIFY_DRIVE_PROTOCOL - READ_DRIVE_MEMORY_PROTOCOL + 1];

  /** Error Status  */
  TEST_SCRIPT_ERROR_BLOCK testScriptErrorBlock;

  /** Drive Information given by test script file  */
  DRIVE_MEMORY_CONTROL_BLOCK driveMemoryControlBlock[MAX_SIZE_OF_DMCB];

  /** cell card inventory data */
  CELL_CARD_INVENTORY_BLOCK cellCardInventoryblock;

  TEST_RESULT_SAVE_BLOCK testResultSaveBlock[64];
  Word wTestResultSaveBlockCounter;
  Byte bLibraryVersion[256];   /* [019KP3] */
  Dword dwVersionLength;       /* [019KP3] */
  Byte bPartFlag;
  Byte bSwtablePickupf;

  /** Sector Buffer of Drive Memory Write/Read (align bSectorBuffer by Dword pad) */
  Dword dwPad;
  Byte bSectorBuffer[0x21000];  /** size 0x10000 -> 0x10800 (512+16)*128sectors  2010.11.26 Memory corruption on MRN in GSP */

  TEST_SCRIPT_RECORDER_BLOCK testScriptRecorderBlock[4096];
  Dword dwTestScriptRecorderBlockCounter;
  /* DO NOT Add member here. You should add meber above dwPad if need */
  /* Because reportHostTester Log() cuts after dwPad members to reduce log size */

} TEST_SCRIPT_CONTROL_BLOCK;

/** tsMain */
extern void runTestScriptMain(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptDestructor(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptSrst(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptHostComLoopbackTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptHostComDownloadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptHostComMessageTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptHostComUploadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptDriveComReadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptDriveComReadThenHostComUploadTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptDriveComEchoTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptPowerOnOnly(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptDrivePollOnly(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptTemperatureTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void optTime(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte JumpProcess(Byte *string);
extern Byte WaitNT(void);
extern void runTestScriptVoltageTest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void runTestScriptDriveComEchoTestWithPowerCycle(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsLib */
extern void runTestScript(ZQUE *que);
/* extern void abortTestScript(Byte bCellNo); ==> called by user/host only */
extern Byte isTestScriptAbortRequested(void);
extern void ts_abort(void);
extern Byte *getCellTestLibraryVersion(void);
extern Byte textInQuotationMark2String(Byte *textInQuotationMark, Byte *bString, Dword dwStringSize);
extern Byte hhhhmmssTimeFormat2Sec(Byte *hhhhmmss, Dword *dwTimeSec);
extern void sec2hhhhmmssTimeFormat(Dword dwTimeSec, Byte *hhhhmmss);
extern Dword ts_memcmp_exp(Byte *s1, Byte *s2, Dword n);
extern Dword ts_strtoul(Byte *S, Byte **PTR, Dword BASE);
extern Byte ts_isprint_mem(char *s, long n);
extern Byte ts_isalnum_mem(Byte *s, Dword n);
extern Dword *memcpyWith32bitLittleEndianTo32bitConversion(Dword *dwOUT, Byte *bIN, Dword N);
extern Dword *memcpyWith32bitBigEndianTo32bitConversion(Dword *dwOUT, Byte *bIN, Dword N);
extern Word *memcpyWith16bitLittleEndianTo16bitConversion(Word *wOUT, Byte *bIN, Dword N);
extern Word *memcpyWith16bitBigEndianTo16bitConversion(Word *wOUT, Byte *bIN, Dword N);
extern Byte *memcpyWith32bitLittleEndianTo8bitConversion(Byte *bOUT, Byte *bIN, Dword N);
extern Byte *memcpyWith16bitLittleEndianTo8bitConversion(Byte *bOUT, Byte *bIN, Dword N);
extern void ts_qfree(void *ptr);
extern void ts_qfree_then_abort(void *ptr);
extern void *ts_qalloc(Dword NBYTES);
extern Byte ts_qis(void);
extern void *ts_qget(void);
extern void ts_qput(void *ptr);
extern void resetTestScriptTimeInSec(void);
extern Dword getTestScriptTimeInSec(void);
extern void ts_sleep_partial(Dword dwTimeInSec);
extern void ts_sleep_slumber(Dword dwTimeInSec);
extern void resetTestScriptLog(Dword dwMaxLogSizeInByte, Byte isTesterLogMirrorToStdout);
extern void getTestScriptLog(Byte **bLogData, Dword *dwCurLogSizeInByte, Dword *dwMaxLogSizeInByte);
extern void ts_printf(Byte *bFileName, Dword dwNumOfLine, Byte *description, ...);
extern void putBinaryDataDump(Byte *dwPhysicalAddress, Dword dwLogicalAddress, Dword size);
extern Dword convA2H( Byte data );
extern Dword calcAtoUL( Byte *data, Dword size );
extern Byte checkAlpha(char *s, long n);

/** tsParse */
extern Byte parseTestScriptFile(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *scriptFile, Word wFileSize); /*[019KPE]*/
extern Byte queryDriveMemoryControlBlock(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, DRIVE_MEMORY_CONTROL_BLOCK **dmcb);
extern Byte *queryDriveDataAddress(TEST_SCRIPT_CONTROL_BLOCK *tscb, DRIVE_MEMORY_CONTROL_BLOCK *dmcb);
extern void freeMemoryForSaveData(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsCom */
extern Byte driveIoCtrl(Dword dwAddress, Word wSize, Byte *bData, Byte bProtocol, UART_PERFORMANCE_LOG_BLOCK *uplb);
extern Byte driveIoCtrlByKeywordWithRetry(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bKeyword, Dword dwTimeoutSec, Byte isRead);
extern Byte GetdrivePespage(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte Getdriveerrorcode(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsEnv */
extern Byte dumpCellInventoryData(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte getEnvironmentStatus(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte isEnvironmentError(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte turnDrivePowerSupplyOn(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte turnDrivePowerSupplyOff(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsIdfy */
extern Byte identifyDrive(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte getEchoData(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsHead */
extern Byte createDriveBadHeadTable(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsPoll */
extern Byte waitDriveTestCompleted(TEST_SCRIPT_CONTROL_BLOCK *tscb);
/*extern Byte waitDriveFinalTestCompleted(TEST_SCRIPT_CONTROL_BLOCK *tscb);*/
extern void recordDriveAndEnvironmentStatus(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsRaw */
extern Byte getDriveRawdata(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsError */
extern Byte createErrorCode(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsHost */
extern void reportHostId(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void reportHostGradingId(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void reportHostTestStatus(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void reportHostTestData(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword dwSize);
extern void reportHostBadHead(TEST_SCRIPT_CONTROL_BLOCK *tscb, Word wBadHeadData);
extern void reportHostTestComplete(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte bComp);
extern void reportHostTesterLog(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void reportHostTestMessage(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *description, ...);
extern void reportHostErrorMessage(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *description, ...);
extern void downloadFileChild(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword *dwSize, Dword dwOffset, Byte isBroadcast, Byte doCompare);
extern void downloadFile(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword dwSize);
extern void downloadFileInBroadcast(TEST_SCRIPT_CONTROL_BLOCK *tscb, Byte *bName, Byte *bData, Dword dwSize);
extern void reportHostTestTimeOut(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsAbort */
extern Byte requestDriveTestAbort(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte requestDriveUnload(TEST_SCRIPT_CONTROL_BLOCK *tscb);

/** tsStat */
extern Byte getDriveStatus(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte isDriveRawdataReady(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte isDrivePesReady(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte isDriveMcsReady(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte isDriveRawdataSetupError(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte isDriveRawdataSetupReady(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte isDriveReportingComplete(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte setDriveRawdataTransferComplete(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte getDriveRawdataStatusAndControl(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte clearReportingTimerOnSsrst(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte getDriveTemperature(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte getSRSTstep(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte setDriveRawdataRetryRequest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte clearDriveRawdataRetryRequest(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte grading(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte getHead(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte setDrivePass2ID(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Byte getGradingflag(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern void getSWtable(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern Dword next_position(Byte *, Byte);
extern Dword cutout_sw_bit(Byte *, Dword *);
extern Byte clearGradingFlag(TEST_SCRIPT_CONTROL_BLOCK *tscb);
extern unsigned char *prepare_buf(char *msgptr, unsigned char *dataptr, Byte len);
/*extern unsigned long next_position(unsigned char *start_point,char delimiter);*/
#endif
