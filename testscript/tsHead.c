#include "ts.h"

#define INT2BIT(i) ((1<<i) & 0xffffffff)

/**
 * <pre>
 * Description:
 *   None.
 *
 * Example:
 *   current bad head   0x0020 ---+          <= provide by Umemura-san
 *                                |
 *                                V
 *   current bad head   0 1 2 3 4 5 6 7 8 9
 *                                |
 *                                V
 *   current ae head    9 8 7 6 4 3 2 1 0 5  <= provide by Umemura-san
 *                                |             (may be changed in daptive BPI model)
 *                                +-+
 *                                  |
 *                                  V
 *   default ae head    9 8 7 6 5 4 3 2 1 0  <= fixed data. given by config file?
 *                                  |           (depend on drive type/model)
 *                                  V
 *   new bad head       0 1 2 3 4 5 6 7 8 9
 *                                  |
 *   new bad head       0x0040 <----+
 * </pre>
 *****************************************************************************/
Dword convertBadHead(Dword current_bad_head, Dword max_head_num, Byte *current_head_table, Byte *default_head_table)
{
  Dword i = 0, j = 0;
  Dword new_bad_head = 0;
  Dword current_ae_head = 0;
  Dword default_ae_head = 0;

  ts_printf(__FILE__, __LINE__, "convertBadHead current_bad_head=%xh max_head_num=%d", current_bad_head, max_head_num);

  putBinaryDataDump(&current_head_table[0], 0, max_head_num + 1);
  putBinaryDataDump(&default_head_table[0], 0, max_head_num + 1);

  for (i = 0 ; i <= max_head_num ; i++) {

    if (current_bad_head & INT2BIT(i)) {

      current_ae_head = current_head_table[i];
      default_ae_head = default_head_table[i];

      ts_printf(__FILE__, __LINE__, "find bad head: current_ae_head=%d, default_ae_head=%d", current_ae_head, default_ae_head);

      for (j = 0 ; j <= max_head_num ; j++) {
        if (default_head_table[j] == current_ae_head) {
          new_bad_head |= INT2BIT(j);
          break;
        }
      }
    }
  }
  ts_printf(__FILE__, __LINE__, "current_bad_head=%xh -> new_bad_head=%xh", current_bad_head, new_bad_head);
  return new_bad_head;
}

/**
 * <pre>
 * Description:
 *   None.
 * </pre>
 *****************************************************************************/
Byte createDriveBadHeadTable(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  Byte rc = 1;
  Word badhead = 0;
  Byte srstHeadNumber = 0;
  Byte origHEADTABLE[10] = {0, };
  Dword origbadhead = 0;

  ts_printf(__FILE__, __LINE__, "createDriveBadHeadTable");

  /* report required? */
  if (!tscb->isBadHeadReport) {
    ts_printf(__FILE__, __LINE__, "no bad head report");
    return 0;
  }

  /* bad head table */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_BAD_HEAD, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove(&badhead, &tscb->bSectorBuffer[0], sizeof(badhead));


  /* head number */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_HEAD_NUMBER, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  srstHeadNumber = tscb->bSectorBuffer[0];

  /* origHEADTABLE */
  rc = driveIoCtrlByKeywordWithRetry(tscb, KEY_HEAD_CONVERTION_TABLE, 0, READ_FROM_DRIVE);
  if (rc) {
    return 1;
  }
  memmove(&origHEADTABLE[0], &tscb->bSectorBuffer[0], sizeof(origHEADTABLE));

  /* memory overflow range check */
  if (srstHeadNumber > sizeof(origHEADTABLE)) {
    return 1;
  } else if (srstHeadNumber > sizeof(tscb->bTestDefaultBadHeadTable)) {
    return 1;
  }

  /* bad head conversion */
  ts_printf(__FILE__, __LINE__, "headnum : %d  old badHead:%x", srstHeadNumber, badhead);
  if (badhead) {
    origbadhead = convertBadHead(badhead, srstHeadNumber, &origHEADTABLE[0], &tscb->bTestDefaultBadHeadTable[0]);
    reportHostBadHead(tscb, origbadhead);
  }
  ts_printf(__FILE__, __LINE__, "headnum : %d  new badHead:%x", srstHeadNumber, origbadhead);

  return 0;
}
