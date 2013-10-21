// This file was processed by space2tab v0.2 on Fri Jun 04 16:49:20 2010 (with NominalSpacesPerIndent of 2)

#define WRAPPER_IMPLEMENTATION_VERSION "20100604a+pullup-ChrisB"
	// siSetDriveFanRPM
	// create/use libsi_xyexterns.h
	// clean-up getCurrentVoltage
	// added simple INTERFACE/IMPLEMENTATION version label scheme


#include "libsi.h"
#include "libsi_xyexterns.h" // // for lib's externs of Tracing flags, pstats etc


#include "siMain.h"

#include "envfuncs.h" // only basic helpers
#include "serialProtocolInPC.h" // 

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define ENV_ERROR_INDICATION_ON_RETURN_CODE    // [DEBUG] 

pthread_mutex_t mutex_slotio;

/*
	Testcase is supplied with a CellIndex and TrayIndex (by Supplier's TestManager)
	
	But the functions in this wrapper all take a HGST bCellNo (255 max)
  
	Convention is that tescase converts CellIndex and TrayIndex to bCellNo
	by the following maths:
		xcalibre/3DP: bCellNo = (CellIndex - 1) x 2  + (TrayIndex)     + 1
				optimus: bCellNo = (CellIndex - 1) x 12 + (TrayIndex - 1) + 1

		note: TrayIndex is 1-based for Optimus, 0-based for Xcalibre/3DP
*/



// these are extern'd in libsi.h for TC use
bool WantLogAPIEntryExit = true; 
bool WantLogAPIInfo = true; 
bool WantLogAPITrace = true; 


bool Initialize_success = false;

bool First_Initialize = true; // no thread has ever yet called into this static lib


struct psd psda[255];  // per_slot_data_indexed_by_bCellNo


// note only ever one instance of Cell Classes per-testcase (regardless of number of threads or slots) !!
#if defined XCALIBRE
	CCellEnv              *pCellEnv;
	CCellRIMHGSTSerial3DP *pCellRIM;
#elif defined OPTIMUS
	CCellEnvHGSTOptimus *pCellEnv;
	CCellRIMHGSTSerialOptimus *pCellRIM;
#endif	


#ifdef __cplusplus
extern "C"
{
#endif



//----------------------------------------------------------------------
//Byte siInitialize( Byte bCellNo, int CellIndex, int TrayIndex, int EnvironmentId  )
Byte siInitialize( Byte bCellNo  )
//----------------------------------------------------------------------
{
	// siInitialze is important (and rare) enough to log its entry and exit at INFO priority !
	LOG_INFO "%s,%d Entered WRAPPER_IMPLEMENTATION_VERSION=%s (library supporting WRAPPER_INTERFACE_VERSION=%s) \n", __func__, bCellNo, WRAPPER_IMPLEMENTATION_VERSION, WRAPPER_INTERFACE_VERSION );

	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	// de-compose bCellNo back into CellIndex and TrayIndex (one time only) - and store in instance variable struct (keyed/indexed off bCellNo))
	//-------------------------------------------------------------------------------------------------------------------------------------------

	#if defined XCALIBRE  //------------------------------------
	
		psda[bCellNo].CellIndex = ((bCellNo-1) / 2) + 1; // CellIndex is 1-based,  typically 24 cells (One column) per TestPC
		psda[bCellNo].TrayIndex =  (bCellNo-1) % 2;      //  XCAL TrayIndex is 0-based (and there are 2 trays in 3DP - index starts at bottom/lower slot)
		
		psda[bCellNo].EnvironmentId = 0; // XCAL environments are 0-based (and there is only one environment in 3DP cell)
		
		psda[bCellNo].UARTChannelId = psda[bCellNo].TrayIndex;
		
		psda[bCellNo].PlugStatusBit = (psda[bCellNo].TrayIndex==0) ? PSB_DD_LOWER_DRIVE_A_PLUGGED  : PSB_DD_UPPER_DRIVE_B_PLUGGED; 

		psda[bCellNo].PSU_sid_5V  = (psda[bCellNo].TrayIndex==0) ? SID_VARIABLE_LOWCURRENT_X  : SID_VARIABLE_LOWCURRENT_Y; 
		psda[bCellNo].PSU_sid_12V = (psda[bCellNo].TrayIndex==0) ? SID_VARIABLE_HIGHCURRENT_X : SID_VARIABLE_HIGHCURRENT_Y; 
		psda[bCellNo].PSU_sid_mask = psda[bCellNo].PSU_sid_5V + psda[bCellNo].PSU_sid_12V;
  
		LOG_INFO "%s,%d: XCALIBRE: CellIndex=%d TrayIndex=%d EnvironmentId=%d  UARTChannelId=%d, PSU_sid_mask=0x%.2X\n",
							__func__, bCellNo, psda[bCellNo].CellIndex, psda[bCellNo].TrayIndex, psda[bCellNo].EnvironmentId, psda[bCellNo].UARTChannelId, psda[bCellNo].PSU_sid_mask );

		if (psda[bCellNo].TrayIndex > 1)
		{
			LOG_ERROR "%s,%d: TrayIndex %d (from bCellNo %d) is TOO LARGE (only support 0 or 1)\n", __func__, bCellNo, psda[bCellNo].TrayIndex, bCellNo);
			return (Byte) xyrcParameterOutOfRange; // UNSTRUCTURED ERROR RETURN !!!
		}

	#elif defined OPTIMUS  //------------------------------------
  	
		psda[bCellNo].CellIndex = ((bCellNo-1) / 12) + 1; // CellIndex is 1-based,  typically 12 cells per TestPC (2 rows of 6)
		psda[bCellNo].TrayIndex = ((bCellNo-1) % 12) + 1; //  Optimus TrayIndex is 1-based (and there are 12 trays in initial Optimus cells, index starts at lower-left and 'winds' across and up the cell - see sticker on cell-front-panel)
		
		psda[bCellNo].EnvironmentId = psda[bCellNo].TrayIndex; // Optimus environments are 1-based, and there is a seperate environment per-slot/tray 
		
		psda[bCellNo].UARTChannelId = 0; // fixed/ignored/unused on current Optimus cells
		
		psda[bCellNo].PlugStatusBit = PSB_OPTIMUS_DRIVE_PLUGGED; 

		psda[bCellNo].PSU_sid_5V  = SID_OPTIMUS_DRIVE_SUPPLY_X;
		psda[bCellNo].PSU_sid_12V = 0;  // NO 12V supply on current Optimus cells !!
		psda[bCellNo].PSU_sid_mask = psda[bCellNo].PSU_sid_5V + psda[bCellNo].PSU_sid_12V;
		
  
		LOG_INFO "%s,%d: OPTIMUS: CellIndex=%d TrayIndex=%d EnvironmentId=%d  UARTChannelId=%d, PSU_sid_mask=0x%.2X\n",
							__func__, bCellNo, psda[bCellNo].CellIndex, psda[bCellNo].TrayIndex, psda[bCellNo].EnvironmentId, psda[bCellNo].UARTChannelId, psda[bCellNo].PSU_sid_mask );

		if ((psda[bCellNo].TrayIndex > 12) || (psda[bCellNo].TrayIndex == 0))
		{
			LOG_ERROR "%s,%d: TrayIndex %d (from bCellNo %d) is TOO LARGE (only support 1 to 12)\n", __func__, bCellNo, psda[bCellNo].TrayIndex, bCellNo);
			return (Byte) xyrcParameterOutOfRange; // UNSTRUCTURED ERROR RETURN !!!
		}
		
		
	#else  //------------------------------------
		#error "require XCALIBRE or OPTIMUS to be defined"
	#endif  //------------------------------------




	if ( pCellEnv == NULL)
	{
		pCellEnv = CCELLENV::CreateInstance( psda[bCellNo].CellIndex, psda[bCellNo].EnvironmentId);
		if ( pCellEnv != NULL)
		{
			xyrc = pCellEnv->IsCellOnlineOnUSB();
			if (xyrc != xyrcOK)
			{
				if (xyrc == xyrcCellNotPlugged)
				{
					LOG_ERROR "%s,%d: Cell with Index %d is currently NOT ONLINE (or is NOT OPERATIONAL)\n", __func__, bCellNo, psda[bCellNo].CellIndex);
				}
				else
				{
					LOG_ERROR "%s,%d: CellIndex=%d (EnvironmentId=%d) IsCellOnline failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].CellIndex, psda[bCellNo].EnvironmentId, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
				}	
			}
			else
			{	
				Initialize_success = true; // need this to overcome checks in following helpers (re-asserted at end)
				ObtainValidateAndDisplayEnvClassVersionInfo( bCellNo);
		
				ObtainAndLogCellFirmwareLimitsAndVersions( bCellNo);
			
				#if defined XCALIBRE
				
					LOG_INFO "%s,%d: automatically turning on riser (to power CPLD on flex)\n", __func__, bCellNo );
					TurnOnRiser( bCellNo );  // VITAL for CPLD - and no other hook/API to do it ..... 
			
				#elif defined OPTIMUS
					
					// (no equivalent)
					
				#endif
				
				siSetLed(bCellNo, lsOn);
				
				// now create the RIM class -------------------------------------
				
				pCellRIM = CCELLRIM::CreateInstance(  psda[bCellNo].CellIndex, psda[bCellNo].EnvironmentId);
				if (pCellRIM != NULL)
				{
					LOG_INFO "%s,%d: automatically Configure_SATA_P11_as_Float_To_PREVENT_Autospinup\n", __func__, bCellNo );
					Configure_SATA_P11_as_Float_To_PREVENT_Autospinup_using_envRIMIO( bCellNo ); // need to find a better place !!!!!!!!!!!!!!! (especially for multi-drives per TC)
		
				}
				else
				{
					LOG_ERROR "%s,%d: CCellRIM...::CreateInstance returned NULL - MAJOR SYSTEM ERROR / resource shortage!!!!", __func__, bCellNo); 
				}
				
				
			}
		}
		else
		{
			LOG_ERROR "%s,%d: CCellEnv...::CreateInstance returned NULL - MAJOR SYSTEM ERROR!!!/ resource shortage!!!!", __func__, bCellNo); 
		}
	
	}
	else
	{
		LOG_INFO "%s,%d: skipped env class construction\n", __func__, bCellNo);
	}
 
   
	brc = (Byte) xyrc;
  
	Initialize_success = (brc == 0) ? true : false;
	First_Initialize = !Initialize_success; 
  
	//LOG_EXIT "%s,%d (rc=%u)\n", __func__, bCellNo, brc);
	LOG_INFO "%s,%d (rc=%u)\n", __func__, bCellNo, brc); // siInitialze is important (and rare) enough to log its entry and exit an INFO priority !
	return brc;
}


//----------------------------------------------------------------------
Byte siFinalize( Byte bCellNo)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);

	if (pCellEnv != NULL) 
	{
	  //[DEBUG]		setTargetVoltage(bCellNo, 0, 0);
	  //[DEBUG]		setSafeHandlingTemperature(bCellNo, 3500);

		siSetLed(bCellNo, lsOff);

		delete pCellEnv;
		pCellEnv = NULL;
	}
	else
	{
		LOG_INFO "%s,%d: no env class to delete\n", __func__, bCellNo);
	}
  
	if (pCellRIM != NULL) 
	{
		delete pCellRIM;
		pCellRIM = NULL;
	}
	else
	{
		LOG_INFO "%s,%d: no RIM class to delete\n", __func__, bCellNo);
	}

	LOG_EXIT "%s,%d\n", __func__, bCellNo);
	return 0;
}


//----------------------------------------------------------------------
Byte isCellEnvironmentError( Byte bCellNo)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	ASSERT_INITIALIZED 

	CellStatusFlags_t EnvStatusFlags = 0;
	xyrc = pCellEnv->GetStatusFlags( EnvStatusFlags);
	if (xyrc == xyrcOK)
	{
		// FILTER out on-temp
		EnvStatusFlags &= ~csfOnTemp; // clear bit
	}
	else
	{
		LOG_ERROR "%s,%d: GetStatusFlags failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		// CANNOT return xyrc in this case (return is the FLAGS)
	}

#if 0
	LOG_EXIT "[DEBUG] Error Force");
	EnvStatusFlags = 0xffff;
#endif
	
	brc = (Byte) EnvStatusFlags;  // WARNING LOSS TO SOME FLAGS !!(RPM wander etc)!!!!!!!!!!!!!!!!!!!!!!!!!!!
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte getVoltageErrorStatus(Byte bCellNo, Word *wErrorStatus)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	*wErrorStatus=0;
	ASSERT_INITIALIZED 

	CellStatusFlags_t EnvStatusFlags = 0;
	xyrc = pCellEnv->GetStatusFlags( EnvStatusFlags);
	if (xyrc == xyrcOK)
	{
		EnvStatusFlags &= ~csfOnTemp; // FILTER out on-temp
		*wErrorStatus = EnvStatusFlags;
		
		if (EnvStatusFlags & csfSupplyInterlockTrip)
		{
			uint16 u16SupplyInterlockTripReasonFlags = 0;
			xyrc = pCellEnv->GetSupplyInterlockTripReason( u16SupplyInterlockTripReasonFlags);
			if (xyrc == xyrcOK)
			{
				*wErrorStatus = u16SupplyInterlockTripReasonFlags;
				LOG_WARNING "%s,%d: csfSupplyInterlockTrip - so setting wErrorStatus to u16SupplyInterlockTripReasonFlags of 0x%.2X", __func__, bCellNo, u16SupplyInterlockTripReasonFlags );
				
				// bit decode  (sit* defines in CellTypes.h) 
				char szInfoText[1024];
				int iInfoText = 0;
				
				#if defined XCALIBRE  //------------------------------------
		
					// NOTE: these bit definitions are from Maximus
					//  TODO: need to confirm/define what 3DP supports (eg it has only 1 efan) !!!!!!!!!!!!!!!!!
					if (u16SupplyInterlockTripReasonFlags & sitWatchdogFault)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitWatchdogFault\n");
					if (u16SupplyInterlockTripReasonFlags & sitElectronicsFan0Fault)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitElectronicsFan0Fault\n");
					if (u16SupplyInterlockTripReasonFlags & sitElectronicsFan1Fault)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitElectronicsFan1Fault\n");
					if (u16SupplyInterlockTripReasonFlags & sitElectronicsFan2Fault)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitElectronicsFan2Fault\n");
					if (u16SupplyInterlockTripReasonFlags & sitDriveFanFault)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitDriveFanFault\n");
					if (u16SupplyInterlockTripReasonFlags & sitPlugDetectFault)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitPlugDetectFault\n");
					if (u16SupplyInterlockTripReasonFlags & sitPSU_X_OVP)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitPSU_X_OVP\n");
					if (u16SupplyInterlockTripReasonFlags & sitPSU_Y_OVP)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitPSU_Y_OVP\n");
					
				#elif defined OPTIMUS  //------------------------------------
				
					if (u16SupplyInterlockTripReasonFlags & sitDUT_Not_Plugged)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitDUT_Not_Plugged\n");
					if (u16SupplyInterlockTripReasonFlags & sitDrive_Fan_RPM_Out_Of_FPGA_Limits)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitDrive_Fan_RPM_Out_Of_FPGA_Limits\n");
					if (u16SupplyInterlockTripReasonFlags & sitDUT_PSU_OVP)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitDUT_PSU_OVP\n");
					if (u16SupplyInterlockTripReasonFlags & sitCellInterlockAffectingSupplyGroup)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitCellInterlockAffectingSupplyGroup\n");
					if (u16SupplyInterlockTripReasonFlags & sitCellElectronics_Fan1_RPM_Out_Of_FPGA_Limits)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitCellElectronics_Fan1_RPM_Out_Of_FPGA_Limits\n");
					if (u16SupplyInterlockTripReasonFlags & sitCellElectronics_Fan2_RPM_Out_Of_FPGA_Limits)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitCellElectronics_Fan2_RPM_Out_Of_FPGA_Limits\n");
					if (u16SupplyInterlockTripReasonFlags & sitCellWatchdogNotEnabled)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitCellWatchdogNotEnabled\n");
					if (u16SupplyInterlockTripReasonFlags & sitCell24V_OVP)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitCell24V_OVP\n");
					if (u16SupplyInterlockTripReasonFlags & sitCell24V_FAIL)	  iInfoText+= sprintf( &szInfoText[iInfoText], "sitCell24V_FAIL\n");
				#endif  //------------------------------------
				LOG_WARNING "%s,%d: decode of above flags=%s", __func__, bCellNo, szInfoText );
			}
			else
			{
				LOG_ERROR "%s,%d: GetSupplyInterlockTripReason failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
			}
		}

	}
	else
	{
		LOG_ERROR "%s,%d: GetStatusFlags failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
	}
	
	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
#if defined(ENV_ERROR_INDICATION_ON_RETURN_CODE)
	LOG_INFO "ENV_ERROR_INDICATION_ON_RETURN_CODE");
	return isCellEnvironmentError(bCellNo);
#else
	return brc;
#endif
}


//----------------------------------------------------------------------
Byte getTemperatureErrorStatus(Byte bCellNo, Word *wErrorStatus)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	*wErrorStatus=0;
	ASSERT_INITIALIZED 

	CellStatusFlags_t EnvStatusFlags = 0;
	xyrc = pCellEnv->GetStatusFlags( EnvStatusFlags);
	if (xyrc == xyrcOK)
	{
		EnvStatusFlags &= ~csfOnTemp; // FILTER out on-temp
		*wErrorStatus = EnvStatusFlags;
		
		#if defined OPTIMUS //-------------------------------------
			if (EnvStatusFlags & csfHeaterInterlockTrip)
			{
				uint16 u16HeaterInterlockTripReasonFlags = 0;
				xyrc = pCellEnv->GetHeaterInterlockTripReason( u16HeaterInterlockTripReasonFlags);
				if (xyrc == xyrcOK)
				{
					*wErrorStatus = u16HeaterInterlockTripReasonFlags;
					LOG_WARNING "%s,%d: csfHeaterInterlockTrip - so setting wErrorStatus to u16HeaterInterlockTripReasonFlags of 0x%.2X", __func__, bCellNo, u16HeaterInterlockTripReasonFlags );
					// bit decode  (hit* defines in CellTypes.h) 
					char szInfoText[1024];
					int iInfoText = 0;
					if (u16HeaterInterlockTripReasonFlags & hitHeater_Did_Not_Come_On_When_Requested)	  iInfoText+= sprintf( &szInfoText[iInfoText], "hitHeater_Did_Not_Come_On_When_Requested\n");
					if (u16HeaterInterlockTripReasonFlags & hitDrive_Fan_RPM_Out_Of_FPGA_Limits)	  iInfoText+= sprintf( &szInfoText[iInfoText], "hitDrive_Fan_RPM_Out_Of_FPGA_Limits\n");
					if (u16HeaterInterlockTripReasonFlags & hitCellInterlockAffectingHeaterGroup)	  iInfoText+= sprintf( &szInfoText[iInfoText], "hitCellInterlockAffectingHeaterGroup\n");
					if (u16HeaterInterlockTripReasonFlags & hitCellElectronics_Fan1_RPM_Out_Of_FPGA_Limits)	  iInfoText+= sprintf( &szInfoText[iInfoText], "hitCellElectronics_Fan1_RPM_Out_Of_FPGA_Limits\n");
					if (u16HeaterInterlockTripReasonFlags & hitCellElectronics_Fan2_RPM_Out_Of_FPGA_Limits)	  iInfoText+= sprintf( &szInfoText[iInfoText], "hitCellElectronics_Fan2_RPM_Out_Of_FPGA_Limits\n");
					if (u16HeaterInterlockTripReasonFlags & hitCellWatchdogNotEnabled)	  iInfoText+= sprintf( &szInfoText[iInfoText], "hitCellWatchdogNotEnabled\n");
					if (u16HeaterInterlockTripReasonFlags & hitCell24V_OVP)	    iInfoText+= sprintf( &szInfoText[iInfoText], "hitCell24V_OVP\n");
					if (u16HeaterInterlockTripReasonFlags & hitCell24V_FAIL)	  iInfoText+= sprintf( &szInfoText[iInfoText], "hitCell24V_FAIL\n");
					LOG_WARNING "%s,%d: decode of above flags=%s", __func__, bCellNo, szInfoText );
				}
				else
				{
					LOG_ERROR "%s,%d: GetHeaterInterlockTripReason failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
				}
			}
		#elif defined XCALIBRE //-------------------------------------
			// Xcalibre does not provide GetHeaterInterlockTripReason
			//  - so just output the Cell/Env Status flags
			//
			// TODO: maybe we can do a magic ReadFX2Reg to provide something similar ?
			
		#endif  //-------------------------------------
	}
	else
	{
		LOG_ERROR "%s,%d: GetStatusFlags failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
	}
	
	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
#if defined(ENV_ERROR_INDICATION_ON_RETURN_CODE)
	LOG_INFO "ENV_ERROR_INDICATION_ON_RETURN_CODE");
	return isCellEnvironmentError(bCellNo);
#else
	return brc;
#endif
}



//----------------------------------------------------------------------
Byte clearCellEnvironmentError( Byte bCellNo)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	ASSERT_INITIALIZED 

	CellStatusFlags_t EnvStatusFlags = 0;
	xyrc = pCellEnv->GetStatusFlags( EnvStatusFlags);
	if (xyrc == xyrcOK)
	{
		if ( EnvStatusFlags & csfElectronicsFanSpeedTrip)
		{
			xyrc = pCellEnv->ClearElectronicsFanSpeedTrip( );
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: ClearElectronicsFanSpeedTrip failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
			}  
		}	
		
		if ( EnvStatusFlags & csfDriveFanSpeedTrip)
		{
			xyrc = pCellEnv->ClearDriveFanSpeedTrip( );
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: ClearDriveFanSpeedTrip failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
			}  
		}  
		
	}
	else
	{
		LOG_ERROR "%s,%d: GetStatusFlags failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		brc = (Byte) xyrc;
	}
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte getCurrentTemperature(Byte bCellNo, Word *wTempInHundredth)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d \n", __func__, bCellNo );
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	int16 i16CurrentTempInTenths = 0;
	xyrc = pCellEnv->GetCurrentTemp( i16CurrentTempInTenths);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: GetCurrentTemp failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
	}

	*wTempInHundredth = i16CurrentTempInTenths * 10;
	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d  wTempInHundredth=%u\n", __func__, bCellNo, brc, *wTempInHundredth);
	return brc;
}


Word wTargetTempInHundredth = 0;  // [DEBUG] add by S.Takahashi

//----------------------------------------------------------------------
Byte setTargetTemperature(Byte bCellNo, Word wTempInHundredth)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d  wTempInHundredth=%u\n", __func__, bCellNo, wTempInHundredth);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	int16 i16RampTempInTenths = wTempInHundredth / 10;
	xyrc = pCellEnv->GoToTempWithRamp( i16RampTempInTenths);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: GoToTempWithRamp  i16RampTempInTenths=%d failed xyrc=%d=%s\n", __func__, bCellNo, i16RampTempInTenths, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
	} else {  // [DEBUG] add by S.Takahashi
		wTargetTempInHundredth = wTempInHundredth;
	}
	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}



//----------------------------------------------------------------------
Byte getTargetTemperature(Byte bCellNo, Word *wTempInHundredth)  // [DEBUG] add by S.Takahashi
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);
	ASSERT_INITIALIZED
	Byte brc = 0;

	*wTempInHundredth = wTargetTempInHundredth;
	LOG_EXIT "%s,%d rc=%d  wTempInHundredth=%u\n", __func__, bCellNo, brc, *wTempInHundredth);
	return brc;
}



//----------------------------------------------------------------------
Byte setSafeHandlingTemperature(Byte bCellNo, Word wTempInHundredth)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d  wTempInHundredth=%u\n", __func__, bCellNo, wTempInHundredth);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	int16 i16SafeHandlingTempInTenths = wTempInHundredth / 10;
	xyrc = pCellEnv->SetMaxHandlingTemp( i16SafeHandlingTempInTenths);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: SetMaxHandlingTemp (i16SafeHandlingTempInTenths=%d) failed xyrc=%d=%s\n", __func__, bCellNo, i16SafeHandlingTempInTenths, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
	}
	else
	{
		xyrc = pCellEnv->GoToSafeHandlingTemp();
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: GoToSafeHandlingTemp () failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
		} else {  // [DEBUG] add by S.Takahashi
			wTargetTempInHundredth = wTempInHundredth;
		}
	}

	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte setPositiveTemperatureRampRate(Byte bCellNo, Word wRateInHundredthsPerMinute)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d  wRateInHundredthsPerMinute=%u\n", __func__, bCellNo, wRateInHundredthsPerMinute);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	int16 i16TenthsPerMinute = wRateInHundredthsPerMinute / 10;
	xyrc = pCellEnv->SetPositiveRampRate( i16TenthsPerMinute);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: SetPositiveRampRate (i16TenthsPerMinute=%d) failed xyrc=%d=%s\n", __func__, bCellNo, i16TenthsPerMinute, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
	}

	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte setNegativeTemperatureRampRate(Byte bCellNo, Word wRateInHundredthsPerMinute)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d  wRateInHundredthsPerMinute=%u\n", __func__, bCellNo, wRateInHundredthsPerMinute);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	int16 i16TenthsPerMinute = wRateInHundredthsPerMinute / 10;
	xyrc = pCellEnv->SetNegativeRampRate( i16TenthsPerMinute);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: SetNegativeRampRate (i16TenthsPerMinute=%d) failed xyrc=%d=%s\n", __func__, bCellNo, i16TenthsPerMinute, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
	}

	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}





//----------------------------------------------------------------------
Byte setTargetVoltage(Byte bCellNo, Word wV5InMilliVolts, Word wV12InMilliVolts)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d  wV5InMilliVolts=%u  wV12InMilliVolts=%u\n", __func__, bCellNo, wV5InMilliVolts, wV12InMilliVolts);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	uint16 sidON = 0;
	uint16 sidOFF = 0;
	
	#if defined OPTIMUS  //------------------------------
	
		// Current Optimus cells only support 1-PSU per slot 
		//     TODO: test for 2-PSU per slot support dynamically (from a field in CellLimits struct)

		if (wV12InMilliVolts > 0)
		{
			LOG_ERROR "%s,%d - Optimus does not support non-zero 12V (you requested %u mV) - ignoring that part of request\n", __func__, bCellNo,  wV12InMilliVolts);
			wV12InMilliVolts = 0;
		} 
	
	#endif  //------------------------------
	
	
	if (brc == 0)
	{
		sidOFF += (wV5InMilliVolts == 0) ? psda[bCellNo].PSU_sid_5V : 0;
		sidOFF += (wV12InMilliVolts == 0) ? psda[bCellNo].PSU_sid_12V : 0;
		if (sidOFF != 0)
		{
			xyrc = pCellEnv->SetSupplyOff( sidOFF );
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: SetSupplyOff( sid=0x%.2X )  failed xyrc=%d=%s\n", __func__, bCellNo, sidOFF, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
			}
		}
		

		if (wV5InMilliVolts > 0)
		{
			xyrc = pCellEnv->SetSupplyVoltageLevel( psda[bCellNo].PSU_sid_5V, wV5InMilliVolts);
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: SetSupplyVoltageLevel (sid=0x%.2X, level=%u) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].PSU_sid_5V, wV5InMilliVolts, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
			}
		}


		if (wV12InMilliVolts > 0)
		{
			xyrc = pCellEnv->SetSupplyVoltageLevel( psda[bCellNo].PSU_sid_12V, wV12InMilliVolts);
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: SetSupplyVoltageLevel (sid=0x%.2X, level=%u) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].PSU_sid_12V, wV12InMilliVolts, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
			}
		}
		
		sidON += (wV5InMilliVolts > 0) ? psda[bCellNo].PSU_sid_5V : 0;
		sidON += (wV12InMilliVolts > 0) ? psda[bCellNo].PSU_sid_12V : 0;
		if (sidON != 0)
		{
			xyrc = pCellEnv->SetSupplyOn( sidON );
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: SetSupplyOn( sid=0x%.2X )  failed xyrc=%d=%s\n", __func__, bCellNo, sidON, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
			}
		}
	}
	
	brc = (Byte) xyrc;
	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}
 

//----------------------------------------------------------------------
Byte getTargetVoltage(Byte bCellNo, Word *wV5InMilliVolts, Word *wV12InMilliVolts)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d\n", __func__, bCellNo);
		Byte brc = 0;
	xyrc xyrc = xyrcOK;
	*wV5InMilliVolts = 0;
	*wV12InMilliVolts = 0;
  
	ASSERT_INITIALIZED
	
	uint16 v = 0;
	
	#if defined XCALIBRE //------------------------------------------------------------------
	
		xyrc = pCellEnv->GetRequestedSupplyVoltageLevel( psda[bCellNo].PSU_sid_5V, v );
		if (xyrc == xyrcOK)
		{
			*wV5InMilliVolts = v;  // ( but may be changed to 0 below if actually off)
		}
		else
		{
			if (xyrc == xyrcMethodNotImplementedYet) // non-marginable cells/firmware return this !!!
			{
				*wV5InMilliVolts = 5000;  // ( but may be changed to 0 below if actually off)
			}
			else
			{
				LOG_ERROR "%s,%d: GetRequestedSupplyVoltageLevel( 0x%X) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].PSU_sid_5V, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
			}	
		}
		
		xyrc = pCellEnv->GetRequestedSupplyVoltageLevel( psda[bCellNo].PSU_sid_12V, v );
		if (xyrc == xyrcOK)
		{
			*wV12InMilliVolts = v;  // ( but may be changed to 0 below if actually off)
		}
		else
		{
			if (xyrc == xyrcMethodNotImplementedYet) // non-marginable cells/firmware return this !!!
			{
				*wV12InMilliVolts = 12000;  // ( but may be changed to 0 below if actually off)
			}
			else
			{
				LOG_ERROR "%s,%d: GetRequestedSupplyVoltageLevel( 0x%X) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].PSU_sid_12V, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
			}	
		}
		
		// now need to consider whether switched off - in which case we must report zero !
		uint16 sidmask = 0;
		xyrc = pCellEnv->GetAllSupplyOnOffStates( sidmask );
		if (xyrc == xyrcOK)
		{
			if ((sidmask & psda[bCellNo].PSU_sid_5V) == 0)
			{
				*wV5InMilliVolts = 0;
			}	
			if ((sidmask & psda[bCellNo].PSU_sid_12V) == 0)
			{
				*wV12InMilliVolts = 0;
			}	
		}
		else
		{
			LOG_ERROR "%s,%d: GetAllSupplyOnOffStates( ) failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
		}
		
		
		
	#elif defined OPTIMUS   //------------------------------------------------------------------
	
		xyrc = pCellEnv->GetRequestedSupplyVoltageLevel( psda[bCellNo].PSU_sid_5V, v );
		if (xyrc == xyrcOK)
		{
			*wV5InMilliVolts = v; // ( but may be changed to 0 below if actually off)
			*wV12InMilliVolts = 0;
		}
		else
		{
			if (xyrc == xyrcMethodNotImplementedYet) // non-marginable cells/firmware return this !!!
			{
				*wV5InMilliVolts = 5000;  // ( but may be changed to 0 below if actually off)
			}
			else
			{
				LOG_ERROR "%s,%d: GetRequestedSupplyVoltageLevel( 0x%X) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].PSU_sid_5V, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
			}	
		}
		
		
		// now need to consider whether switched off - in which case we must report zero !
		uint16 sidmask = 0;
		xyrc = pCellEnv->GetAllSupplyOnOffStates( sidmask );
		if (xyrc == xyrcOK)
		{
			if ((sidmask & psda[bCellNo].PSU_sid_5V) == 0)
			{
				*wV5InMilliVolts = 0;
			}	
		}
		else
		{
			LOG_ERROR "%s,%d: GetAllSupplyOnOffStates( ) failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
		}
		
	#endif  //------------------------------------------------------------------
	

	LOG_EXIT "%s,%d rc=%d V5InMilliVolts=%u wV12InMilliVolts=%u\n", __func__, bCellNo, brc, *wV5InMilliVolts, *wV12InMilliVolts);
	return brc;
}



//----------------------------------------------------------------------
Byte getCurrentVoltage(Byte bCellNo, Word *wV5InMilliVolts, Word *wV12InMilliVolts)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d\n", __func__, bCellNo);
		Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	*wV5InMilliVolts = 0;
	*wV12InMilliVolts = 0;
	
	ASSERT_INITIALIZED
	
	uint16 hcx,hcy,lcx,lcy = 0;
	xyrc = pCellEnv->GetMeasuredVariableVoltageLevels( hcx,hcy,lcx,lcy );
	if (xyrc == xyrcOK)
	{
		#if defined XCALIBRE
			if (psda[bCellNo].TrayIndex == 0)
			{
				*wV5InMilliVolts = lcx;
				*wV12InMilliVolts = hcx;
			}
			else
			{
				*wV5InMilliVolts = lcy;
				*wV12InMilliVolts = hcy;
			}
		#elif defined OPTIMUS
			// optimus returns value for its single supply in first parameter 
			// (future second supply will be in second parameter, which is returned as 0 by current single-supply cells)
			*wV5InMilliVolts = hcx;  // first param
			//
			// [DEBUG]
			// The 9999 is a convention we use for 'not-available/not-implemented'.
			// It signals that this cell does not support measurements on the non-existant 12V supply.
			//
			//*wV12InMilliVolts = hcy; // second param (future use, 0 for now)
			if (hcy == 9999) {
				LOG_WARNING "%s,%d - guess 12V supply not implemented -- set 0 instead of 9999", __func__, bCellNo);
				*wV12InMilliVolts = (hcy == 9999) ? 0 : hcy;
			} else {
				*wV12InMilliVolts = hcy; // second param (future use, 0 for now)
			}
		#endif		
		 
	}
	else
	{
		LOG_ERROR "%s,%d: GetMeasuredVariableVoltageLevels() failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc) );
	}
	
	LOG_EXIT "%s,%d rc=%d V5InMilliVolts=%u wV12InMilliVolts=%u\n", __func__, bCellNo, brc, *wV5InMilliVolts, *wV12InMilliVolts);
	return brc;
}



//----------------------------------------------------------------------
Byte setVoltageRiseTime(Byte bCellNo, Word wV5TimeInMsec, Word wV12TimeInMsec)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d,%d,%d\n", __func__, bCellNo, wV5TimeInMsec, wV12TimeInMsec);
	Byte brc = 0;
 	
	LOG_WARNING "%s,%d,%d,%d - QUIETLY IGNORED BY XYRATEX\n", __func__, bCellNo, wV5TimeInMsec, wV12TimeInMsec);
 	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}

//----------------------------------------------------------------------
Byte setVoltageInterval(Byte bCellNo, Word wTimeFromV5ToV12InMsec)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d,%d\n", __func__, bCellNo, wTimeFromV5ToV12InMsec);
	Byte brc = 0;
 	
	LOG_WARNING "%s,%d,%d,- QUIETLY IGNORED BY XYRATEX\n", __func__, bCellNo, wTimeFromV5ToV12InMsec);
 	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte getCellInventory(Byte bCellNo, CELL_CARD_INVENTORY_BLOCK **stCellInventory)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d\n", __func__, bCellNo);
	static CELL_CARD_INVENTORY_BLOCK st;
	Byte brc = 0;
	

	memset((Byte *)&st, 0x00, sizeof(st));
	*stCellInventory = &st;
  
	LOG_WARNING "%s,%d,- IGNORED BY XYRATEX (returning empty structure)\n", __func__, bCellNo);
  
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte isDrivePlugged(Byte bCellNo)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	ASSERT_INITIALIZED 

	uint32 u32StatusBits = 0;
	xyrc = pCellEnv->GetPlugStatusBits( u32StatusBits);
	if (xyrc == xyrcOK)
	{
		brc =  (u32StatusBits & psda[bCellNo].PlugStatusBit) ? 1 : 0; 
	}
	else
	{
		LOG_ERROR "%s,%d: GetPlugStatusBits failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		brc = (Byte) xyrc;
	}
	LOG_EXIT "%s,%d rc=%d (full StatusBits=0x%.4lx\n", __func__, bCellNo, brc, u32StatusBits);
	return brc;
}



//======================================================================================================================================
//======================================================================================================================================
// RIM/Serial functions
//======================================================================================================================================
//======================================================================================================================================


//----------------------------------------------------------------------
Byte siSetUartBaudrate(Byte bCellNo, Dword dwBaudrate)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d, %lu\n", __func__, bCellNo, dwBaudrate);
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
 	
	//------------------------------------------------------------------------------
	// most convenient place to perform overall serial configuration/initialisation
	//------------------------------------------------------------------------------
	
	memset ( &(pstats[bCellNo]), 0, sizeof(struct _protocol_stats)); // clear the pstat structure - used for all protocol/throughput statistics and errors
	
	if (xyrc == xyrcOK)
	{
		// configure the IO lines used for serial (from default of gpio/'safe' -  to uart/'driven')
		uint8 RouteCode = BSIOLC_ROUTE_CODE_VIA_RISER_SATA_P11; // 
		uint8 TxBitMode = BSIOLC_BITMODE_ALTFN; // see CellTypes.h - just means 'controlled/driven' by UART
		uint8 RxBitMode = 0; // ignored in Optimus 1-wire
		uint8 SRQBitMode = 0; // ignored in Optimus for HGST
		
		LOG_INFO "%s,%d: activating UART IO, RouteCode=BSIOLC_ROUTE_CODE_VIA_RISER_SATA_P11 (%d) (now safe to 'drive' line into the drive)", __func__, bCellNo, RouteCode);
		
		xyrc = pCellRIM->BulkSerialIOLineControl( psda[bCellNo].UARTChannelId, RouteCode, TxBitMode, RxBitMode, SRQBitMode);
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: BulkSerialSetBaudRate( %d, %lu) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].UARTChannelId, dwBaudrate, xyrc, CCELLRIM::ConvertXyrcToPsz(xyrc) );
		}
	}     
	

	if (xyrc == xyrcOK)
	{
		// Set the UART Parameters approprite for "1-wire":
		//   DISABLE_RX_DURING_TX 
		// (note also, cell always disables/floats cell's Tx line when cell is not actively transmitting)
		LOG_INFO "%s,%d: setting BSPARAM_DISABLE_RX_DURING_TX", __func__, bCellNo);
		uint8 u8ParameterId = BSPARAM_DISABLE_RX_DURING_TX; // see CellRIMSerialHGST.h (and/or CellRIMSerialHGST.txt)
		uint8 u8Value = 1; // bool 1=true=yes
		xyrc = pCellRIM->BulkSerialSetParameter( psda[bCellNo].UARTChannelId, u8ParameterId, u8Value);
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: BulkSerialSetParameter( id=%d, val=%d ) failed xyrc=%d=%s\n", __func__, bCellNo, u8ParameterId, u8Value, xyrc, CCELLRIM::ConvertXyrcToPsz(xyrc) );
		}
	}     
	
	
	if (xyrc == xyrcOK)
	{
		// set the inter-character delay on the Transmitter
		uint8 u8ParameterId = BSPARAM_TX_INTER_CHARACTER_DELAY; // see CellRIMSerialHGST.h (and/or CellRIMSerialHGST.txt)
		uint8 u8ICDInUnitsOf10uS = 28;  // 28 * 10uS = 280uS  
		LOG_INFO "%s,%d: setting TX_INTER_CHARACTER_DELAY to %u*10uS", __func__, bCellNo, u8ICDInUnitsOf10uS);
		xyrc = pCellRIM->BulkSerialSetParameter( psda[bCellNo].UARTChannelId, u8ParameterId, u8ICDInUnitsOf10uS);
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: BulkSerialSetParameter( id=%d, val=%d ) failed xyrc=%d=%s\n", __func__, bCellNo, u8ParameterId, u8ICDInUnitsOf10uS, xyrc, CCELLRIM::ConvertXyrcToPsz(xyrc) );
		}
	}     


	if (xyrc == xyrcOK)
	{
		// set the BaudRate on the UART
		LOG_INFO "%s,%d: setting BaudRate to %lu (channel %d)", __func__, bCellNo,  dwBaudrate, psda[bCellNo].UARTChannelId );
		xyrc = pCellRIM->BulkSerialSetBaudRate( psda[bCellNo].UARTChannelId, dwBaudrate );
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: BulkSerialSetBaudRate( %d, %lu) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].UARTChannelId, dwBaudrate, xyrc, CCELLRIM::ConvertXyrcToPsz(xyrc) );
		}
	} 
		
 	
	brc = (Byte)xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}



//----------------------------------------------------------------------
Byte setUartPullupVoltage(Byte bCellNo, Word wUartPullupVoltageInMilliVolts)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d,%d\n", __func__, bCellNo, wUartPullupVoltageInMilliVolts);
 	
	// 11Jun10 ChrisB
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	uint16 u16UartPullupVoltageInMilliVolts = wUartPullupVoltageInMilliVolts;

#if 1
	// [DEBUG]	
	LOG_INFO "[DEBUG] require Cell F/W update? to support UART voltage change");
	brc = 0;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
#endif

	#if defined XCALIBRE
	
		//LOG_WARNING "%s,%d,%d,- QUIETLY IGNORED BY XYRATEX (for Xcalibre)\n", __func__, bCellNo, wUartPullupVoltageInMilliVolts);
		
		// 15Jun10: NEEDS cellfirmware 22.04 or later !!!!!
	
		#ifndef ENVOPTIMUS_PARAM_ID_DUT_IO_VCC_SELECT // // NORMALLY comes from ??.h
			#define BSPARAM_UART_PULL_UP_VOLTAGE      3  // Value is a 2-BYTE number (2000 and 3300 supported, (power-on default is 3300)
		#endif


		xyrc = pCellRIM->BulkSerialSetParameter( psda[bCellNo].UARTChannelId, BSPARAM_UART_PULL_UP_VOLTAGE, (uint16)wUartPullupVoltageInMilliVolts ); // 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: CellRIM->BulkSerialSetParameter(chan %d, BSPARAM_UART_PULL_UP_VOLTAGE to %d) failed xyrc=%d=%s\n", __func__, bCellNo, psda[bCellNo].UARTChannelId, u16UartPullupVoltageInMilliVolts, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		}

	#elif defined OPTIMUS

		// 11Jun10: NEEDS cellfirmware(CellShell) a1.40 or later !!!!!
		
		#ifndef ENVOPTIMUS_PARAM_ID_DUT_IO_VCC_SELECT // // NORMALLY comes from CellEnvHGSTOptimus_params.h
			#define ENVOPTIMUS_PARAM_ID_DUT_IO_VCC_SELECT              (ENVOPTIMUS_PARAM_BASE_XYINTERNAL+36) // 11Jun10 fw a1.40->  uint16 (in mV eg 3300) (supported= 1200, 1500, 1800, 2500, 3300)  (power-on default is 1800) (set only) 
		#endif


		xyrc = pCellEnv->SetParameter( ENVOPTIMUS_PARAM_ID_DUT_IO_VCC_SELECT, (void *)&u16UartPullupVoltageInMilliVolts, sizeof(u16UartPullupVoltageInMilliVolts) ); // (see CellEnvHGSTOptimus_params.h)
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: CellEnv->SetParameter(ENVOPTIMUS_PARAM_ID_DUT_IO_VCC_SELECT to %d) failed xyrc=%d=%s\n", __func__, bCellNo, u16UartPullupVoltageInMilliVolts, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		}

	#endif

	brc = (Byte) xyrc;
	
 	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte setUartPullupVoltageDefault(Byte bCellNo)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d\n", __func__, bCellNo);

	// 11Jun10 ChrisB
	ASSERT_INITIALIZED
	Byte brc = 0;
	uint16 u16UartPullupVoltageInMilliVolts = 0;
	
	#if defined XCALIBRE
		u16UartPullupVoltageInMilliVolts = 2000;
	#elif defined OPTIMUS
		u16UartPullupVoltageInMilliVolts = 1800;
	#endif

	brc = setUartPullupVoltage(bCellNo, u16UartPullupVoltageInMilliVolts);

	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}

// [DEBUG] add this function  2010.05.17
//----------------------------------------------------------------------
Byte siWriteDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData, Word wTimeoutInMillSec)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d, dwAddress=0x%.8lX, wSize=%d, wTimeoutInMillSec=%d\n", __func__, bCellNo, dwAddress, wSize, wTimeoutInMillSec);
	Byte brc = 0;
 	
	brc = SerialSimpleWriteMemoryDEBUG (bCellNo, psda[bCellNo].UARTChannelId,  dwAddress,  wSize,  bData,  wTimeoutInMillSec);
 	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte siReadDriveMemory(Byte bCellNo, Dword dwAddress, Word wSize, Byte *bData, Word wTimeoutInMillSec)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d, dwAddress=0x%.8lX, wSize=%d, wTimeoutInMillSec=%d\n", __func__, bCellNo, dwAddress, wSize, wTimeoutInMillSec);
	Byte brc = 0;
 	
	// [DEBUG] replace SerialSimpleReadMemory() with SerialSimpleReadMemoryDEBUG to 
	// [DEBUG] copy only data part to bData instead of all reveive packet that contains sync
	// [DEBUG] checksum etc.
	brc = SerialSimpleReadMemoryDEBUG (bCellNo, psda[bCellNo].UARTChannelId,  dwAddress,  wSize,  bData,  wTimeoutInMillSec);
 	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


// [DEBUG] add this function  2010.05.17
//----------------------------------------------------------------------
Byte siEchoDrive(Byte bCellNo, Word wSize, Byte *bData, Word wTimeoutInMillSec)
//----------------------------------------------------------------------
{
	LOG_ENTRY  "%s,%d, wSize=%d, wTimeoutInMillSec=%d\n", __func__, bCellNo, wSize, wTimeoutInMillSec);
		Byte brc = 0;
 	
		brc = SerialSimpleEchoDEBUG (bCellNo, psda[bCellNo].UARTChannelId,  wSize,  bData,  wTimeoutInMillSec);
 	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


// [DEBUG] add this function  2010.05.17
extern Byte ProtocolType; // (defined in serialProtocolInPC.cpp (private to library))
//----------------------------------------------------------------------
Byte siSetUartProtocol(Byte bCellNo, Byte bProtocol)
//---------------------------------------------------------------------- 
{
	LOG_ENTRY  "%s,%d, bProtocol=%u, \n", __func__, bCellNo, bProtocol);
	Byte brc = 0;

	ProtocolType = bProtocol;  // 0= A00 protocol (PTB),  1= ARM protocol (JPK)
	
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}







//======================================================================================================================================
//======================================================================================================================================
// Additions offered by Xyratex/ChrisBarnett
//======================================================================================================================================
//======================================================================================================================================

Byte siGetReasonStringForReturnCode( Byte brc, char* pCallersBuffer, Word wBufferSize)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d  brc=%u\n", __func__, 0, brc);
	xyrc xyrc = (enum _xyrc) brc;
	
	const char *p = CELLENV_XYRC_TO_STRING(xyrc);
	strncpy( pCallersBuffer, p, wBufferSize);
	*(pCallersBuffer+(wBufferSize-1))= '\0';
	
	LOG_EXIT "%s,%d \n", __func__, 0);
	return 0;
}

Byte isOnTemp( Byte bCellNo)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d\n", __func__, bCellNo);
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	ASSERT_INITIALIZED 

	CellStatusFlags_t EnvStatusFlags = 0;
	xyrc = pCellEnv->GetStatusFlags( EnvStatusFlags);
	if (xyrc == xyrcOK)
	{
		brc = (EnvStatusFlags & csfOnTemp) ? 1 : 0; 
	}
	else
	{
		LOG_ERROR "%s,%d: GetStatusFlags failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		brc = (Byte) xyrc;
	}
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}



Byte SetFPGATestMode( Byte bCellNo)
//----------------------------------------------------------------------
{
	LOG_ENTRY "%s,%d \n", __func__, bCellNo);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;

	LOG_WARNING "  SETTING FPGA INTO TEST MODE !!!!!!!!!!!!!!!POTENTIALLY DANGEROUS!!!!!!!!!!!!!!!!!!!!!!!!!");
	
	#if defined XCALIBRE
		
		// NOTE: magic address for 3DP is 0x800a  
		// set bit 0 bit of u8MmioRBFSpecialFunctions 
		// beware must not affect bit1=3DP mode bit, SO NEED TO DO A READ,MODIFY,WRITE
		uint8 u8val = 0;
		xyrc = pCellEnv->ReadFX2Reg(  0x800a, u8val); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: ReadFX2Reg failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		} 
		
		u8val |= 0X01; // SET BIT 0
		
		xyrc = pCellEnv->WriteFX2Reg( 0x800a, u8val); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: WriteFX2Reg failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		} 
	
	#elif defined OPTIMUS

		uint8 u8Val = 1;
		xyrc = pCellEnv->SetParameter( ENVOPTIMUS_PARAM_ID_FPGA_TEST_MODE, (void *)&u8Val, sizeof(u8Val) ); // (see CellEnvHGSTOptimus_params.h)
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: CellEnv->SetParameter(ENVOPTIMUS_PARAM_ID_FPGA_TEST_MODE to %d) failed xyrc=%d=%s\n", __func__, bCellNo, u8Val, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		}

	#endif


	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}




Byte siSetDriveFanRPM( Byte bCellNo, int DriveFanRPM)  // ChrisB: 04Jun10
//----------------------------------------------------------------------
// allowable range varies by product, and firmware version 
// (returns xyrcParameterOutOfRange if request is outside allowed range)
{
	LOG_ENTRY "%s,%d DriveFanRPM=%d\n", __func__, bCellNo, DriveFanRPM);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	xyrc = pCellEnv->SetFanSpeedControl( true); // set RPM-control active (in case currently in voltage-control - which is default on Xcalibre)
	if (xyrc == xyrcOK)
	{
		xyrc = pCellEnv->SetDriveFanRPM( DriveFanRPM); 
		if (xyrc == xyrcOK)
		{
			// all done
		}
		else
		{
			LOG_ERROR "%s,%d: SetDriveFanRPM( %d) failed xyrc=%d=%s\n", __func__, bCellNo, DriveFanRPM, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		} 
	}
	else
	{
		LOG_ERROR "%s,%d: SetFanSpeedControl(active) failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
	} 

	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


Byte siSetElectronicsFanRPM( Byte bCellNo, int ElectronicsFanRPM)  // SatoshiT: 09Jun10
//----------------------------------------------------------------------
// allowable range varies by product, and firmware version 
// (returns xyrcParameterOutOfRange if request is outside allowed range)
{
	LOG_ENTRY "%s,%d ElectronicsFanRPM=%d\n", __func__, bCellNo, ElectronicsFanRPM);
	ASSERT_INITIALIZED
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	xyrc = pCellEnv->SetFanSpeedControl( true); // set RPM-control active (in case currently in voltage-control - which is default on Xcalibre)
	if (xyrc == xyrcOK)
	{
		xyrc = pCellEnv->SetElectronicsFanRPM( ElectronicsFanRPM); 
		if (xyrc == xyrcOK)
		{
			// all done
		}
		else
		{
			LOG_ERROR "%s,%d: SetElectronicsFanRPM( %d) failed xyrc=%d=%s\n", __func__, bCellNo, ElectronicsFanRPM, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		} 
	}
	else
	{
		LOG_ERROR "%s,%d: SetFanSpeedControl(active) failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
	} 

	brc = (Byte) xyrc;
	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


Byte siSetFanRPMDefault( Byte bCellNo)  // SatoshiT: 07Jun10
//----------------------------------------------------------------------
// * Optimus *
//
//   Default FanControl mode:  RPM (active)         
//     Dfan Default RPM: 6300
//     Dfan Allowable Range: 2800 - 6500 
//     Efan Default RPM: 5200
//     Efan Allowable Range: 4000 - 6200
//     (above are queryable by testcase, in 'extended' area of CellLimits structure)
//   
//     Default: Xyratex PES test had been done with Dfan=4200rpm, Efan=5200
//
//     Notes: Efans shared by whole cell (12-slots) 
//            one Efan deliberately runs 100 RPM different to the other to avoid 'beats'
//
// * Xcalibre-3DP *
//
//   Default FanControl mode:  Voltage (ie inactive)         
//     Dfan Default RPM: 2500 (once FanControl mode activated)
//     Dfan Allowable Range: 1500 - 3300 
//     Efan Default RPM: 3100 (once FanControl mode activated)
//     Efan Allowable Range: 1800 - 3700
//     (above are not queryable by testcase)
//
//   Default : Xyratex PES test had been done with Dfan=2750RPM, Efan=3600RPM
// 
//   Notes: In 'voltage-control mode' (as opposed to RPM-control mode), we just apply 
//           a 'fixed' voltage to the fans and their RPM is uncontrolled (ie will/may 
//           vary with valve-position, back-pressures in heat exchanger, altitude, 
//           bearing wear etc.). The voltage used today on 3DP is 'max' (i.e. 255  
//           counts on the 8-bit control DAC) for both Drive and Electronics  fans.
//
{
	LOG_ENTRY "%s,%d FanRPMDefault\n", __func__, bCellNo);
	ASSERT_INITIALIZED
	Byte brc = 0;
	int DriveFanRPM = 0;
	int ElectronicsFanRPM = 0;
	
	#if defined XCALIBRE
		DriveFanRPM = 2750;
		ElectronicsFanRPM = 3600;
	#elif defined OPTIMUS
		DriveFanRPM = 4200;
		ElectronicsFanRPM = 5200;
	#endif
	if (brc == 0) {
		brc = siSetDriveFanRPM(bCellNo, DriveFanRPM);
	}
	if (brc == 0) {
		brc = siSetElectronicsFanRPM(bCellNo, ElectronicsFanRPM);
	}

	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}


//----------------------------------------------------------------------
Byte siSetLed(Byte bCellNo, Byte bMode)  // SatoshiT: 10Jun10
//----------------------------------------------------------------------
{
	Byte brc = 0;

	LOG_ENTRY "%s,%d,%d\n", __func__, bCellNo, bMode);

//	LOG_INFO "[DEBUG] force LED turned off, because GSP XCalibre machine sometimes fails infrared communication due to RED LED");
//	bMode = lsOff;

	if (pCellEnv != NULL) 
	{
		pCellEnv->SetLEDA( (enumLEDState)bMode );
 	 	#if defined XCALIBRE 
 			LOG_INFO "[DEBUG] force LED turned off, because GSP XCalibre machine sometimes fails infrared communication due to RED LED");
			pCellEnv->SetLEDB( lsOff );
		#elif defined OPTIMUS
			pCellEnv->SetLEDB( (enumLEDState)bMode );
 		#endif

	} else {
		brc = 1;
	}

	LOG_EXIT "%s,%d rc=%d\n", __func__, bCellNo, brc);
	return brc;
}

// Yukari Katayama 14/June/2011 add for commonality with teradyne
//----------------------------------------------------------------------
Byte setTemperatureEnvelope( Byte bCellNo, Word wEnvelopeTempInTenth)
//----------------------------------------------------------------------
{
	Byte brc = 0;

	LOG_ENTRY "%s,%d,%d\n", __func__, bCellNo, wEnvelopeTempInTenth);

	LOG_INFO "[DEBUG] dummy function");
	return brc;
}

// Yukari Katayama 14/June/2011 add for commonality with teradyne
//----------------------------------------------------------------------
Byte adjustTemperatureControlByDriveTemperature(Byte bCellNo, Word wTempInHundredth)
//----------------------------------------------------------------------
{
	Byte brc = 0;

	LOG_ENTRY "%s,%d,%d\n", __func__, bCellNo, wTempInHundredth);

	LOG_INFO "[DEBUG] dummy function");
	return brc;
}

#ifdef __cplusplus
}
#endif





