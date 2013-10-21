// OS-independent system headers
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define OS_IS_LINUX


#include "libsi.h"  // the hgst generic slot-interface to different suppliers (defined and supplied by HGST)
                    // NOTE: we also get the HGST-preffered typedefs Byte, Word, Dword etc from here (via its inclusion of tcTypes.h)

#include "wrapperSRST_common.h"  // own header (testcase #defines etc)

#include "miscfuncs.h" // interface to our lower-level functions (printlog etc)


#include "CellTypes.h" // OPTIONAL (for xyrc enum...

// prototype helpers (at end of this file)
bool isDriveAlreadyPowered( Byte bCellNo);




//--------------------------------------------------------------------------------------------
void * DriveThreadFunc( void * arg)  // arg is a bCellNo
//--------------------------------------------------------------------------------------------
{
	// arg is a bCellNo
	int x = (int) arg;
	Byte bCellNo = x;
	
	int cellindex = 0;
	int trayindex = 0;
	/*
	  hgst cell id is one start (1, 2, ...). 
	  It is created here from supplied supplier cell/slot id 
	  (and then converted back to supplier values in the library)
	
	  xc3dp: bCellNo = (cellIndex - 1) x 2  + (trayIndex)     + 1
	  op:    bCellNo = (cellIndex - 1) x 12 + (trayIndex - 1) + 1
	
	  note - only xc3dp trayindex is zero-base value
	*/
	
	#if defined XCALIBRE  //------------------------------------
		cellindex = ((bCellNo-1) / 2) + 1;  // CellIndex is 1-based,  typically 24 cells (One column) per TestPC
		trayindex = ((bCellNo-1) % 2);    //  XCAL TrayIndex is 0-based (and there are 2 trays in 3DP - index starts at bottom/lower slot)
	#elif defined OPTIMUS  //------------------------------------
		cellindex = ((bCellNo-1) / 12) + 1; // CellIndex is 1-based,  typically 12 cells per TestPC (2 rows of 6)
		trayindex = ((bCellNo-1) % 12) + 1; //  Optimus TrayIndex is 1-based (and there are 12 trays in initial Optimus cells, index starts at lower-left and 'snakes' across and up the cell - see sticker on cell-front-panel)
	#else
		#error "bollocks"	
	#endif	
	
	printlog("DriveThreadFunc(bCellNo=%u, (ie trayindex %d): started \n", bCellNo, trayindex);
	
	Byte sirc = 0;
	
	
	
	
	
	
	// check for drive-plugged
	//-------------------------------------------------------------------------------
	if (WantPlugCheck)
	{
		sirc = isDrivePlugged( bCellNo );
		if (sirc == 0)
		{
			UpdateTMTestInfoLine("No Drive plugged in tray %d - aborting",trayindex); 
			EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_NO_DRIVE_PLUGGED );  // NO RETURN !!!!!
		}
		else if (sirc == 1)
		{
			printlog("Confirmed Drive plugged in tray %d\n", trayindex);
		}
		else
		{
			printlog("isDrivePlugged FAILED sirc=%d\n", sirc);
		}
	}
	else
	{
		printlog("SKIPPING test for drive-plugged in tray %d\n",trayindex);
	}
	







	// Set Voltage Levels and power on drive
	//-------------------------------------------------------------------------------
	if (WantPowerDrive)
	{
		if (isDriveAlreadyPowered( bCellNo) == true)  
		{
			printlog("Drive was already powered (so skipping our power-up and delay) \n");
		}
		else
		{
			#if defined XCALIBRE
				printlog("Setting nominal voltage levels on BOTH drive supplies (both 5V & both 12V\n");
				 
				// NOTE: riser is now turned on automatically in siInitialize 
				//       since no suitable API in wrapper !!!!
	
				
				// NOTE: Configure_SATA_P11_as_Float_To_PREVENT_Autospinup now done automatically in siInitialize
				//       since no suitable API in wrapper !!!!
	

				printlog("Turning on Drive supplies for Tray %d\n", trayindex);
				sirc = setTargetVoltage( bCellNo, 5000, 12000);
				
			#elif defined OPTIMUS
			
				printlog("Setting nominal voltage level on single drive supply\n");
				 
				// NOTE: Configure_SATA_P11_as_Float_To_PREVENT_Autospinup now done automatically in siInitialize
				//       since no suitable API in wrapper !!!!

				printlog("Turning on Drive supply for Tray %d\n", trayindex);
				sirc = setTargetVoltage( bCellNo, 5000, 0);
				
			#endif
	
			// Allow drive power to come-up (typical 100 - 200 mS before supplies switch on (PWM pre-charge + soft-start)
			sleep (1);  // seconds

			if (WantSerialComms)
			{
	
				#define INITIAL_DELAY_SECS  (30 + 60)  // power-on-ready + srst startup time  
				//#define INITIAL_DELAY_SECS  (20)  // early testing on CHAV !!!  
				printlog("SRST drives need some time to power up and settle");
				printlog(" before they can respond to serial commands ");
				printlog("sleeping %d seconds", INITIAL_DELAY_SECS);
				for (int i = INITIAL_DELAY_SECS; i>0; i--)
				{
					sleep( 1 );
					fprintf(stdout, "\r%d secs to go", i);
					fflush(stdout);
					
					if (i % InfoUpdateFrequencySeconds == 0)
					{
						//UpdateTMTestInfoLine("%d secs delay for Drive spin-up/SerialReady", i);
					}
					
					if (clean_exit_requested)
					{
						EndTestcaseSafely( EXITCODE_TESTCASE_FAILED_RECEIVED_SIG_TERM );  // NO RETURN !!!!!
					}
				}
			}
			
		} // drive already powered
	}
	else
	{
		printlog("SKIPPING Powering-on of the drive (due to -np cmdline option)\n");
	}




	if (WantSerialComms)
	{
		#define DESIRED_SRST_BAUDRATE  1843200    // 921600
		
		printlog("Initialising 1-wire serial comms at %u Baud", DESIRED_SRST_BAUDRATE);

		sirc = siSetUartBaudrate(  bCellNo, DESIRED_SRST_BAUDRATE ); // activates UART drivers (requires drive-plugged and powered-up), sets one-wire-mode, TxInterCharacterDayay, BaudRate etc.
		
		// 25May - we now have an API to set protocol type
		//  bProtocol 0: A00 protocol (PTB), 1: ARM protocol (JPK)
		Byte bProtocol = (WantNEWPROTOCOL)?1:0;
		sirc = siSetUartProtocol( bCellNo, bProtocol); 
	}
	



	for (int loop = 1; loop <= LoopCount; loop++)
	{

		if (WantSerialComms)
		{
			// have a serial transaction with the drive 
			//----------------------------------------------------------------------
			
			#if defined XCALIBRE
				Dword dwAddress = 0x009e6c00; // 3.5 JPK srst sequence
			#elif defined OPTIMUS
				//Dword dwAddress = 0x00060200; // for drive model = MPB  =garbage on 2.5 EB4 !
				Dword dwAddress = 0x00070200; //  =garbage on 2.5 EB4 !
			#endif
			
			Word wSize = u16nBytesOfDriveDataToBeRead; // (changeable via -p cmdline option)
			Byte bData[0xffff+1]; // handle up-to 64K ish
			
			//Word wTimeoutInMillSec = 2500; // NOTE: wrapper can now handle any value 
			Word wTimeoutInMillSec = 3000; // (we see occasional timeouts at 2500) NOTE: wrapper can now handle any value 
			
			sirc = siReadDriveMemory( bCellNo, dwAddress, wSize, bData, wTimeoutInMillSec);
		}
		else
		{
			// enable only for stress-testing of two thread polling env temp at same time)
			Word wTempInHundredth = 0;
			sirc = getCurrentTemperature( bCellNo, &wTempInHundredth);
			if (sirc != 0)
			{
				printlog("WaitForOnTempIndication: error %d from getCurrentTemperature - aborting drivethread", sirc);
				break;
			}
			printlog("drivethread: no serial so polling temp rapidly as a stress test - temp now %d", wTempInHundredth);
			
		}
	
	}



	
	//printlog("sleeping a while, before exiting");
	//sleep(30);
	
	
	
	printlog("DriveThreadFunc(bCellNo=%u, (ie trayindex %d): ended \n", bCellNo, trayindex);
	return NULL;
}



// helpers 

bool isDriveAlreadyPowered( Byte bCellNo)
//-------------------------------------------------------------------------------------------------------------
{
	bool brc = false;
	Word w5, w12;
	Byte sirc = getTargetVoltage( bCellNo, &w5, &w12);
	if (sirc == 0)
	{
		brc = ((w5>0) || (w12 > 0)) ? true : false;
	}
	else
	{
		printlog("WaitForOnTempIndication: error %d from getCurrentTemperature - aborting wait", sirc);
	}
	return brc;
}

