#include "ts.h"

#define tcPrintf ts_printf

/* ---------------------------------------------------------------------- */
int main(int argc, char *argv[])
/* ---------------------------------------------------------------------- */
{
  FILE *fp = NULL;
  ZQUE *zque = NULL;
  Dword size = 0;
  Dword i = 0;
  int numOfLoop = 1;

  pthread_mutex_init(&mutex_slotio, NULL);

  for (i = 0 ; i < numOfLoop ; i++) {
    /* open config file */
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
      exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* copy data */
    zque = calloc(sizeof(ZQUE) + size + 16, 1);
    if (zque == NULL) {
      exit(EXIT_FAILURE);
    }
    fread(&zque->bData[0], 1, size, fp);
    fclose(fp);
    zque->wLgth = size + 1;
    zque->bData[zque->wLgth - 1] = '\0';

    /* run */
    set_task(get_task_base_offset());
    pthread_mutex_lock(&mutex_slotio);
    runTestScript((ZQUE *)zque);
  }

  exit(EXIT_FAILURE);
}

// #include "si.h"

pthread_mutex_t mutex_slotio;

/* ---------------------------------------------------------------------- */
Byte siInitialize(Byte bCellNo)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("siInitialize,%d\n", bCellNo);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte siFinalize(Byte bCellNo)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("siFinalize,%d\n", bCellNo);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte isDrivePlugged(Byte b)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("isDrivePlugged,%d\n", b);
  return 1;
}

/* ---------------------------------------------------------------------- */
Byte setTargetTemperature(Byte bCellNo, Word wTempInHundredth)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("setTargetTemperature,%d,%d\n", bCellNo, wTempInHundredth);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte getCurrentTemperature(Byte bCellNo, Word *wTempInHundredth)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("getCurrentTemperature,%d,%d\n", bCellNo, *wTempInHundredth = 2500);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte setSafeHandlingTemperature(Byte bCellNo, Word wTempInHundredth)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("setSafeHandlingTemperature,%d,%d\n", bCellNo, wTempInHundredth);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte setTargetVoltage(Byte bCellNo, Word wV5InMilliVolts, Word wV12InMilliVolts)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("setTargetVoltage,%d,%d,%d\n", bCellNo, wV5InMilliVolts, wV12InMilliVolts);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte getCurrentVoltage(Byte bCellNo, Word *wV5InMilliVolts, Word *wV12InMilliVolts)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("getCurrentVoltage,%d,%d,%d\n", bCellNo, *wV5InMilliVolts = 5000, *wV12InMilliVolts = 12000);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte getVoltageErrorStatus(Byte b, Word *w)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("getVoltageErrorStatus,%d,%d\n", b, *w = 0);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte getTemperatureErrorStatus(Byte b, Word *w)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("getTemperatureErrorStatus,%d,%d\n", b, *w = 0);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte setVoltageRiseTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("setVoltageRiseTime,%d,%d,%d\n", bCellNo, wV5TimeInMsec, wV12TimeInMsec);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte setVoltageInterval(Byte bCellNo, Word wTimeFromV5ToV12InMsec)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("setVoltageInterval,%d,%d\n", bCellNo, wTimeFromV5ToV12InMsec);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte setUartPullupVoltage(Byte bCellNo, Word wUartPullupVoltageInMilliVolts)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("setUartPullupVoltage,%d,%d\n", bCellNo, wUartPullupVoltageInMilliVolts);
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte getCellInventory(Byte bCellNo, CELL_CARD_INVENTORY_BLOCK **stCellInventory)
/* ---------------------------------------------------------------------- */
{
  static CELL_CARD_INVENTORY_BLOCK st;

  TCPRINTF("getCellInventory,%d\n", bCellNo);

  memset((Byte *)&st, 0x00, sizeof(st));
  *stCellInventory = &st;
  return 0;
}

/* ---------------------------------------------------------------------- */
Byte siReadDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData, Word wTimeoutInMillSec)
/* ---------------------------------------------------------------------- */
{
  TCPRINTF("siReadDriveMemory,%d,%ld,%d,%d\n", bCellNo, dwAddress, wSize, wTimeoutInMillSec);
  return 0;
}
