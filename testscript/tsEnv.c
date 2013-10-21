#include "ts.h"
#define  RECEIVE_BUFFER_ID 1

/**
 * <pre>
 * Description:
 *   Dump cell inventoy data.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte dumpCellInventoryData(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  CELL_CARD_INVENTORY_BLOCK *ccib = &tscb->cellCardInventoryblock;
  CELL_CARD_INVENTORY_BLOCK *p = NULL;
  Byte rc = 0;

  ts_printf(__FILE__, __LINE__, "dumpCellInventoryData");

  rc = getCellInventory(MY_CELL_NO + 1, &p);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "error - getCellInventory %d", rc);
    tseb->isFatalError = 1;
    return 1;
  }
  memcpy(ccib, p, sizeof(*ccib));
  ts_printf(__FILE__, __LINE__, "sizeof(*ccib) = %d", sizeof(*ccib));

  ts_printf(__FILE__, __LINE__, "bPartsNumber");
  putBinaryDataDump(ccib->bPartsNumber, 0, sizeof(ccib->bPartsNumber));

  ts_printf(__FILE__, __LINE__, "bCardRev");
  putBinaryDataDump(ccib->bCardRev, 0, sizeof(ccib->bCardRev));

  ts_printf(__FILE__, __LINE__, "bCardType");
  putBinaryDataDump(ccib->bCardType, 0, sizeof(ccib->bCardType));

  ts_printf(__FILE__, __LINE__, "bHddType");
  putBinaryDataDump(ccib->bHddType, 0, sizeof(ccib->bHddType));

  ts_printf(__FILE__, __LINE__, "bSerialNumber");
  putBinaryDataDump(ccib->bSerialNumber, 0, sizeof(ccib->bSerialNumber));

  ts_printf(__FILE__, __LINE__, "bConnectorLifeCount");
  putBinaryDataDump(ccib->bConnectorLifeCount, 0, sizeof(ccib->bConnectorLifeCount));

  return 0;
}

/**
 * <pre>
 * Description:
 *   Dump cell inventoy data.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte getEnvironmentStatus(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 0;

  ts_printf(__FILE__, __LINE__, "getEnvironmentStatus");

  rc = getCurrentTemperature(MY_CELL_NO + 1, &tscb->wCurrentTempInHundredth);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "error %d", rc);
    tseb->isFatalError = 1;
  }

  rc = getCurrentVoltage(MY_CELL_NO + 1, &tscb->wCurrentV5InMilliVolts, &tscb->wCurrentV12InMilliVolts);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "error %d", rc);
    tseb->isFatalError = 1;
  }

  ts_printf(__FILE__, __LINE__, "cell temp = %d, x_volt = %d, y_volt = %d", tscb->wCurrentTempInHundredth, tscb->wCurrentV5InMilliVolts, tscb->wCurrentV12InMilliVolts);

  return 0;
}

/**
 * <pre>
 * Description:
 *   Wait drive test completed.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte isEnvironmentError(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 1;
  Word wErrorStatus = 0xffff;

  rc = isDrivePlugged(MY_CELL_NO + 1);
  if (!rc) {
    if (tscb->isNoPlugCheck) {
      ts_printf(__FILE__, __LINE__, "Warning - Drive Plug Off");
    } else {
      ts_printf(__FILE__, __LINE__, "Error - Drive Plug Off");
      tseb->isDriveUnplugged = 1;
      return 1;
    }
  }

  rc = getVoltageErrorStatus(MY_CELL_NO + 1, &wErrorStatus);
  if (rc) {
    if (tscb->isNoPowerControl) {
      ts_printf(__FILE__, __LINE__, "Warning - Disable poower on");
    } else {
      ts_printf(__FILE__, __LINE__, "Error - Voltatge 0x%04x", wErrorStatus);
      tseb->isDrivePowerFail = 1;
      return rc;
    }
  }

  rc = getTemperatureErrorStatus(MY_CELL_NO + 1, &wErrorStatus);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "Error - Temp Environment 0x%04x", wErrorStatus);
    tseb->isCellTemperatureAbnormal = 1;
    return rc;
  }

  return 0;
}

/**
 * <pre>
 * Description:
 *   Wait drive test completed.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte turnDrivePowerSupplyOn(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 0;

  ts_printf(__FILE__, __LINE__, "turnDrivePowerSupplyOn");

  rc = isEnvironmentError(tscb);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "error %d", rc);
    return rc;
  }

  if (tscb->isNoPowerControl) {
    ts_printf(__FILE__, __LINE__, "warning - disable power on");

  } else {
    /* range check */
    if (0 == tscb->dwDriveUartPullup) {
      /* in case of power off */
    } else if ((tscb->dwDriveUartPullup < VOLTAGE_LIMIT_UART_LOW_MV) ||
               (VOLTAGE_LIMIT_UART_HIGH_MV < tscb->dwDriveUartPullup)) {
      ts_printf(__FILE__, __LINE__, "dwDriveUartPullup out of range %d", tscb->dwDriveUartPullup);
      return 1;
    }
    if (0 == tscb->dwDrivePowerSupply5V) {
      /* in case of power off */
    } else if ((tscb->dwDrivePowerSupply5V < VOLTAGE_LIMIT_5V_LOW_MV) ||
               (VOLTAGE_LIMIT_5V_HIGH_MV < tscb->dwDrivePowerSupply5V)) {
      ts_printf(__FILE__, __LINE__, "dwDrivePowerSupply5V out of range %d", tscb->dwDrivePowerSupply5V);
      return 1;
    }
    if (0 == tscb->dwDrivePowerSupply12V) {
      /* in case of power off */
    } else if ((tscb->dwDrivePowerSupply12V < VOLTAGE_LIMIT_12V_LOW_MV) ||
               (VOLTAGE_LIMIT_12V_HIGH_MV < tscb->dwDrivePowerSupply12V)) {
      ts_printf(__FILE__, __LINE__, "dwDrivePowerSupply12V out of range %d", tscb->dwDrivePowerSupply12V);
      return 1;
    }

    if (tscb->dwTestUartBaudrate > 0) {
      rc = siSetUartBaudrate(MY_CELL_NO + 1, tscb->dwTestUartBaudrate);
      if (rc) {
        ts_printf(__FILE__, __LINE__, "error %d", rc);
        tseb->isFatalError = 1;
        return rc;
      }
    }

    rc = siSetUartProtocol(MY_CELL_NO + 1, tscb->bTestUartProtocol);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
    }

    rc = setVoltageRiseTime(MY_CELL_NO + 1, tscb->dwDrivePowerRiseTime5V, tscb->dwDrivePowerRiseTime12V);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
    }

    rc = setVoltageInterval(MY_CELL_NO + 1, tscb->dwDrivePowerIntervalFrom5VTo12V);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
    }

    rc = setTargetVoltage(MY_CELL_NO + 1, tscb->dwDrivePowerSupply5V, tscb->dwDrivePowerSupply12V);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
    }
    ts_sleep_slumber(1);

    rc = setUartPullupVoltage(MY_CELL_NO + 1, tscb->dwDriveUartPullup);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
    }
  }

  ts_sleep_slumber(tscb->dwDrivePowerOnWaitTime);

  rc = clearFPGABuffers( MY_CELL_NO + 1 , RECEIVE_BUFFER_ID );
  if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
  }
  ts_sleep_slumber(10);

  rc = isEnvironmentError(tscb);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "error %d", rc);
    return rc;
  }

  return rc;
}

/**
 * <pre>
 * Description:
 *   Wait drive test completed.
 * Arguments:
 *   *tscb - Test script control block
 * </pre>
 *****************************************************************************/
Byte turnDrivePowerSupplyOff(TEST_SCRIPT_CONTROL_BLOCK *tscb)
{
  TEST_SCRIPT_ERROR_BLOCK *tseb = &tscb->testScriptErrorBlock;
  Byte rc = 1;

  ts_printf(__FILE__, __LINE__, "turnDrivePowerSupplyOff");

  /* turn off voltage supply */
  if (tscb->isNoPowerControl || tscb->isNoPowerOffAfterTest) {
    ts_printf(__FILE__, __LINE__, "Warning - Disable poower off");
  } else {
    rc = setUartPullupVoltage(MY_CELL_NO + 1, 0);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
    }
    ts_sleep_slumber(1);
    rc = setTargetVoltage(MY_CELL_NO + 1, 0, 0);
    if (rc) {
      ts_printf(__FILE__, __LINE__, "error %d", rc);
      tseb->isFatalError = 1;
      return rc;
    }
  }
  ts_sleep_slumber(tscb->dwDrivePowerOffWaitTime);

  /* check enviroment error */
  rc = isEnvironmentError(tscb);
  if (rc) {
    ts_printf(__FILE__, __LINE__, "error %d", rc);
    return rc;
  }

  return rc;
}
