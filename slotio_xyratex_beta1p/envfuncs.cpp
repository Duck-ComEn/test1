//==============================================================================
// envfuncs.cpp
//  - lower-level and support functions for ENVironment Control
//     (temperature, voltage, plug-detect etc.)
//==============================================================================

#include "libsi.h"
#include "libsi_xyexterns.h" // // for lib's externs of Tracing flags, pstats etc

#include "siMain.h"

#include "envfuncs.h" // own interface header (function prototypes etc) 


#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>


//---------------------------------------------------------------------------
void ObtainValidateAndDisplayEnvClassVersionInfo( Byte bCellNo)
//---------------------------------------------------------------------------
{
	ASSERT_INITIALIZED_VOID 
	xyrc xyrc = xyrcOK;

	uint16 u16InterfaceVersion = 0;
	xyrc = CCELLENV::GetInterfaceVersion( u16InterfaceVersion);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: CCELLENV::GetInterfaceVersion() FAILED - xyrc=%d=%s\n",  __func__, bCellNo, xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
	}
	
	
	uint16 u16ImplementationVersion = 0;
	xyrc = CCELLENV::GetImplementationVersion( u16ImplementationVersion);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: CCELLENV::GetImplementationVersion() FAILED - xyrc=%d=%s\n",  __func__, bCellNo, xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
	}

	LOG_INFO "CellEnv: InterfaceVer=%u, ImplementationVer=%u\n", u16InterfaceVersion, u16ImplementationVersion);

	uint16 u16ExpectedInterfaceVersion = CELLENV_INTERFACE_VERSION;  // (#define in CellEnv*.h)
	if ( u16InterfaceVersion != u16ExpectedInterfaceVersion)
	{
		LOG_ERROR "%s,%d: CellEnv Interface version conflict: app built using v%u, lib/so built using v%u \n",  __func__, bCellNo, u16ExpectedInterfaceVersion, u16InterfaceVersion);
	}
	
	
 
	int16 i16CurrentTemperatureTenths = 0; 
	xyrc = pCellEnv->GetCurrentTemp( i16CurrentTemperatureTenths);
	if ( xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: GetCurrentTemp() failed - xyrc=%d=%s\n",  __func__, bCellNo, xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
	}
	LOG_INFO "CellEnv: Current temperature = %u tenths Celsius\n", i16CurrentTemperatureTenths);

}


//---------------------------------------------------------------------------
bool CheckForAndReportAnyCellStatusErrors( Byte bCellNo, bool InitialCall)   //(returns true if there IS a problem)
//---------------------------------------------------------------------------
{
	bool mybrc = false; // init to no-error

	ASSERT_INITIALIZED_BOOL_TRUE 
	xyrc xyrc = xyrcOK;

	char szInfoText[2048] = "";
	int iInfoText = 0;

	CellStatusFlags_t CellStatusFlags;
	xyrc = pCellEnv->GetStatusFlags( CellStatusFlags);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: GetStatusFlags() failed - xyrc=%d=%s\n", __func__, bCellNo,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
	}
	
	if (CellStatusFlags & csfFirmwareError)
	{  
		mybrc = true; // return error to caller
		uint8 u8NumFirmwareErrors;
		xyrc = pCellEnv->GetNumberOfFirmwareErrors( u8NumFirmwareErrors );
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: GetNumberOfFirmwareErrors() failed - xyrc=%d=%s\n", __func__, bCellNo,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
		}  
		else
		{ 
			LOG_INFO "Status Flags indicate that Cell has %d firmware error(s) - attemptimg to get reason string(s)...\n",u8NumFirmwareErrors);
			iInfoText +=sprintf(&szInfoText[iInfoText], "CellStatusFlags show %d firmware errors", u8NumFirmwareErrors);
			char szBuf[64];  // firmware error text is max 64 bytes 
			for (int i = 0; i < u8NumFirmwareErrors; i++)
			{
				xyrc = pCellEnv->GetFirmwareErrorMessageByIndex( i,  szBuf, sizeof(szBuf) );
				if (xyrc != xyrcOK)
				{
					LOG_ERROR "%s,%d: GetFirmwareErrorMessageByIndex() failed - xyrc=%d=%s\n", __func__, bCellNo,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
				}  
				else
				{ 
					if (szBuf[0] != '\0')
					{
						iInfoText +=sprintf(&szInfoText[iInfoText], " msg%d=%s ", i, szBuf );
						LOG_INFO "  %s\n", szBuf );
					}
					else
					{
						LOG_INFO "  <no text>\n");
					}  
				}
			}
		}   
	} // csfFirmwareError


	if (CellStatusFlags & csfHeatersTripped)
	{
		mybrc = true; // return error to caller
		LOG_INFO "Cell Status Flags indicate HeatersTripped\n");
		iInfoText +=sprintf(&szInfoText[iInfoText], " csfHeatersTripped," );
	}

	if (CellStatusFlags & csfStepperNotReferenced)  
	{
		mybrc = true; // return error to caller
		LOG_INFO "Cell Status Flags indicate StepperNotReferenced\n");
		iInfoText +=sprintf(&szInfoText[iInfoText], " StepperNotReferenced," );
	}

	if (CellStatusFlags & csfBadEEPROMData)  
	{
		mybrc = true; // return error to caller
		LOG_INFO "Cell Status Flags indicate BadEEPROMData\n");
		iInfoText +=sprintf(&szInfoText[iInfoText], " BadEEPROMData," );
	}

	if (CellStatusFlags & csfOverTempLatch)  
	{
		mybrc = true; // return error to caller
		LOG_INFO "Cell Status Flags indicate OverTempLatch\n");
		iInfoText +=sprintf(&szInfoText[iInfoText], " OverTempLatch," );
	}

	if (CellStatusFlags & csfVCLimitTrip)  
	{
		mybrc = true; // return error to caller
		LOG_INFO "Cell Status Flags indicate VCLimitTrip\n");
		iInfoText +=sprintf(&szInfoText[iInfoText], " VCLimitTrip," );
	}

	if (CellStatusFlags & csfSupplyInterlockTrip)  
	{
		LOG_WARNING "Cell Status Flags indicate SupplyInterlockTrip\n");
		iInfoText +=sprintf(&szInfoText[iInfoText], " SupplyInterlockTrip," );
		
		// easiest way to get bit decode LOGGED...
		Word wStatus = 0;
		getVoltageErrorStatus( bCellNo, &wStatus); // this user-facing function does the bit-decode and LOGGING for us !
		
		// (optionally) try to clear it
		if (InitialCall)
		{
			LOG_WARNING "  InitialCall - So trying to clear it - by explicitly turning off supplies\n");
			
			uint16 sid = 0;
			#if defined XCALIBRE
				switch (psda[bCellNo].TrayIndex)
				{
					case 0: // lower
						sid =  SID_VARIABLE_LOWCURRENT_X + SID_VARIABLE_HIGHCURRENT_X; 
						break;
					case 1: // upper
						sid =  SID_VARIABLE_LOWCURRENT_Y + SID_VARIABLE_HIGHCURRENT_Y; 
						break;
					default:
						LOG_ERROR "%s,%d - cannot handle TrayIndex %u\n", __func__, bCellNo, psda[bCellNo].TrayIndex);
						mybrc = true; // return error to caller
						break;
				}
			#elif defined OPTIMUS
				sid = SID_OPTIMUS_SINGLE_DRIVE_SUPPLY;
			#endif	
			
			pCellEnv->SetSupplyOff( sid );
			
			sleep(1); // sec
			xyrc = pCellEnv->GetStatusFlags( CellStatusFlags);
			if (CellStatusFlags & csfSupplyInterlockTrip) 
			{
				LOG_INFO "   failed to clear it\n");
				mybrc = true; // return error to caller
			}
			else
			{
				iInfoText +=sprintf(&szInfoText[iInfoText], "(cleared ok)" );
				LOG_INFO "   appeared to clear it\n");
			}
		}
		else
		{
			LOG_INFO " (Not InitialCall so NOT attempting to clear it)\n");
			mybrc = true; // return error to caller
		}
		
		// log details of any SupplyInterlockTrip
		if (CellStatusFlags & csfSupplyInterlockTrip)
		{
			// easiest way to get bit decode LOGGED...
			Word wStatus = 0;
			getVoltageErrorStatus( bCellNo, &wStatus); // this user-facing function does the bit-decode and LOGGING for us !
		}	 

	}

	if (CellStatusFlags & csfDriveFanSpeedTrip)  
	{
		//mybrc = true; // these ARE NOT ERRORS - just information !!!!
		LOG_INFO "Cell Status Flags indicate DriveFanSpeedTrip (info only NOT an error)\n");
	}

	if (CellStatusFlags & csfElectronicsFanSpeedTrip)  
	{
		//mybrc = true; // these ARE NOT ERRORS - just information !!!!

		LOG_INFO "Cell Status Flags indicate ElectronicsFanSpeedTrip (info only NOT an error)\n");
	}


	// if any single line problem message constructed above 
	if (iInfoText > 0)
	{
		LOG_INFO "%s", szInfoText );
	}

	return mybrc;   
}
   
   
   
//---------------------------------------------------------------------------
void ObtainAndLogCellFirmwareLimitsAndVersions( Byte bCellNo ) 
//---------------------------------------------------------------------------
{  
	ASSERT_INITIALIZED_VOID 
	xyrc xyrc = xyrcOK;
	
	CellLimitsStructure CellLimits;
	xyrc = pCellEnv->GetCellLimits( CellLimits);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: GetCellLimits() failed - xyrc=%d=%s\n", __func__, bCellNo,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
	} 
	
	LOG_INFO "CellLimits says: (NB all temps in TENTHS DegC, all rates in TENTHS-per-Minute)\n");
	LOG_INFO "  RequestableTemperatureRange: Min=%d  Max=%d\n",CellLimits.i16MinTemp, CellLimits.i16MaxTemp);
	LOG_INFO "  PositiveRampRate: MaxRequestable=%d  Default=%d\n",CellLimits.i16MaxPositiveRampRate, CellLimits.i16DefaultPositiveRampRate);
	LOG_INFO "  NegativeRampRate: MaxRequestable=%d  Default=%d\n",CellLimits.i16MaxNegativeRampRate, CellLimits.i16DefaultNegativeRampRate);
	LOG_INFO "  SafeHandlingTemp: default=%d\n",CellLimits.i16DefaultMaxHandlingTemp);
	LOG_INFO " ---\n");

	LOG_INFO "  CellFirmware=%.2x.%.2x   FPGA=%.2x.%.2x (SpecialFuncs=0x%.2X)", 
							CellLimits.u8FirmwareVersionMajor,CellLimits.u8FirmwareVersionMinor, CellLimits.u8RBFVersionMajor,CellLimits.u8RBFVersionMinor, CellLimits.u8RBFSpecialFunctions);
	LOG_INFO "  CellSerialNumber %.6lX", CellLimits.u32CellSerialNumber);

	char x[255]="";
	int i=0;
	if (CellLimits.u8ModeFlags & CL_MODE_NOFANS) i+=sprintf(&x[i], "NO_FANS! + ");
	if (CellLimits.u8ModeFlags & CL_MODE_USB_HIGHSPEED) i+=sprintf(&x[i], "USB_HIGHSPEED + ");
	if (CellLimits.u8ModeFlags & CL_MODE_DOUBLE_DENSITY) i+=sprintf(&x[i], "DOUBLE_DENSITY + ");
	if (CellLimits.u8ModeFlags & CL_MODE_MULTI_ENV) i+=sprintf(&x[i], "MULTI_ENV + ");
	if (CellLimits.u8ModeFlags & CL_MODE_3DP) i+=sprintf(&x[i], "3DP + ");
	LOG_INFO "  u8ModeFlags=0x%.2X (=%s)", CellLimits.u8ModeFlags, x);
	
	char * p;
	switch (CellLimits.u8CCCType)
	{
		case CCCTYPE_GXD4: p = "GXD4"; break;
		case CCCTYPE_GXD4SIO: p = "GXD4-SIO"; break;
		default: p="UNSUPPORTED"; break;
	}
	LOG_INFO "  u8CCCType=%u (=%s)", CellLimits.u8CCCType, p);
	
	#if defined OPTIMUS
	
		//LOG_INFO "  u8ProjectCode=%u", CellLimits.u8ProjectCode);

		// Optimus Extensions (on-going - just some ideas...
		LOG_INFO "----------- Optimus Extensions: -----------------");
		LOG_INFO "ENV fpga v%.2X.%.2X h", CellLimits.Optimus_Extension.u8EnvCardFPGAVersionMajor, CellLimits.Optimus_Extension.u8EnvCardFPGAVersionMinor);
		LOG_INFO "i16FirmwareThermalSafetyTripTemp=%u", CellLimits.Optimus_Extension.i16FirmwareThermalSafetyTripTemp);
		LOG_INFO "FPGA_DfanTripLimit_RPM: min=%u max=%u", CellLimits.Optimus_Extension.u16FPGA_DfanTripLimit_MinRPM, CellLimits.Optimus_Extension.u16FPGA_DfanTripLimit_MaxRPM);
		LOG_INFO "FPGA_EfanTripLimit_RPM: min=%u max=%u", CellLimits.Optimus_Extension.u16FPGA_EfanTripLimit_MinRPM, CellLimits.Optimus_Extension.u16FPGA_EfanTripLimit_MaxRPM);
		LOG_INFO "UserRequestableDfan_RPM: min=%u max=%u", CellLimits.Optimus_Extension.u16UserRequestable_Dfan_MinRPM, CellLimits.Optimus_Extension.u16UserRequestable_Dfan_MaxRPM);
		LOG_INFO "UserRequestableEfan_RPM: min=%u max=%u", CellLimits.Optimus_Extension.u16UserRequestable_Efan_MinRPM, CellLimits.Optimus_Extension.u16UserRequestable_Efan_MaxRPM);
		LOG_INFO "POM v%u.%u (Status=0x%.2X, Reset_src=0x%.2X)", CellLimits.Optimus_Extension.u8POMVersionMajor, CellLimits.Optimus_Extension.u8POMVersionMinor, CellLimits.Optimus_Extension.u8POMStatus,  CellLimits.Optimus_Extension.u8POMResetSource);
		LOG_INFO "u8NumberOfSupportedSlots=%u", CellLimits.Optimus_Extension.u8NumberOfSupportedSlots);
		LOG_INFO "u8PSUSuppliesAreLiteNotFull=%u", CellLimits.Optimus_Extension.u8PSUSuppliesAreLiteNotFull);
		LOG_INFO "u8HBAPresent=%u", CellLimits.Optimus_Extension.u8HBAPresent);
		LOG_INFO "u16ConfigCode_or_ConfigBits=0x%.4X", CellLimits.Optimus_Extension.u16ConfigCode_or_ConfigBits);
		LOG_INFO "u8NumberOfDUTSuppliesPerSlot=%u", CellLimits.Optimus_Extension.u8NumberOfDUTSuppliesPerSlot);  
		LOG_INFO "u8NumberOfSupportedEnvs=%u", CellLimits.Optimus_Extension.u8NumberOfSupportedEnvs);  

	#endif	
}
  
  
//---------------------------------------------------------------------------
bool Configure_SATA_P11_as_Float_To_PREVENT_Autospinup_using_envRIMIO(  Byte bCellNo )  
//---------------------------------------------------------------------------
{
	bool brc = true;
	xyrc xyrc; 

	#if defined XCALIBRE //--------------------------------------------------------------------------

		uint16 u16RIMIO_mask_data = 0;
		uint16 u16RIMIO_mask_cpld_direction_control = 0;
		
		// bit definitions to suit 'production' first production Interposers / CPLD v3.00  
		switch (psda[bCellNo].TrayIndex)
		{
			case 0: // lower
				u16RIMIO_mask_data                   |= 0x0100; // RIMIO_8 = bit8 is the 'data' bit - that we want to set gpio input
				u16RIMIO_mask_cpld_direction_control |= 0x0080; // RIMIO_7 = bit7 is the 'cpld control' bit - that we want to set gpio input
				break;
			case 1: // upper
				u16RIMIO_mask_data                   |= 0x0040; // RIMIO_6 = bit6 is the 'data' bit - that we want to set gpio input
				u16RIMIO_mask_cpld_direction_control |= 0x0020; // RIMIO_5 = bit5 is the 'cpld control' bit - that we want to set gpio input
				break;
			default:
				LOG_ERROR "%s,%d: invalid TrayIndex of %u from supplied bCellNo - so returning false\n", __func__, bCellNo, psda[bCellNo].TrayIndex);
				brc = false;
				break;
		}
		
		if (brc)
		{
			xyrc = pCellEnv->RIMIO_ConfigGpio(  u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control); 
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: RIMIO_ConfigGpio(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
			}
			
			xyrc = pCellEnv->RIMIO_ConfigDriven(  u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control); 
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: RIMIO_ConfigDriven(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
			}
			
			xyrc = pCellEnv->RIMIO_DriveHighOrFloat(  u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control);  // BOTH bits drivenhigh/float to set as inputs
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: RIMIO_DriveHighOrFloat(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
			}
		
		}
		LOG_INFO "%s,%d: Tray %u - done\n", __func__, bCellNo, psda[bCellNo].TrayIndex);

	
		
	#elif defined OPTIMUS  //--------------------------------------------------------------------------
	
		// (no need for TrayIndex here)
		uint16 u16RIMIO_mask = 0x0001;   // DUT_IO 0 = rimio 0 is wired to SATA_Pin11 (note this is also used by UART - but when put into ALT_FN mode by BulkSerialIOLineControl)
		
		xyrc = pCellEnv->RIMIO_ConfigGpio(  u16RIMIO_mask ); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: RIMIO_ConfigGpio(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
		}
		
		xyrc = pCellEnv->RIMIO_ConfigDriven(  u16RIMIO_mask); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: RIMIO_ConfigDriven(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
		}
		
		xyrc = pCellEnv->RIMIO_DriveHighOrFloat(  u16RIMIO_mask); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: RIMIO_DriveHighOrFloat(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask, xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
		}

		LOG_INFO "%s,%d: Tray %u - done\n", __func__, bCellNo, psda[bCellNo].TrayIndex);

		
	#endif //--------------------------------------------------------------------------

	
	return brc;
} 


//---------------------------------------------------------------------------
bool Configure_SATA_P11_as_Driven_Low_For_Autospinup( Byte bCellNo )    
//---------------------------------------------------------------------------
{
	bool brc = true;
	xyrc xyrc; 

	#if defined XCALIBRE //--------------------------------------------------------------------------

		uint16 u16RIMIO_mask_data = 0;
		uint16 u16RIMIO_mask_cpld_direction_control = 0;
		
		// bit definitions to suit 'production' first production Interposers / CPLD v3.00  
		switch (psda[bCellNo].TrayIndex)
		{
			case 0: // lower
				u16RIMIO_mask_data                   |= 0x0100; // RIMIO_8 = bit8 is the 'data' bit - that we want to set gpio output and drive low
				u16RIMIO_mask_cpld_direction_control |= 0x0080; // RIMIO_7 = bit7 is the 'cpld control' bit - - that we want to set gpio output and drive high (to set CPLD direction to drive)
				break;
			case 1: // upper
				u16RIMIO_mask_data                   |= 0x0040; // RIMIO_6 = bit6 is the 'data' bit - that we want to set gpio output and drive low
				u16RIMIO_mask_cpld_direction_control |= 0x0020; // RIMIO_5 = bit5 is the 'cpld control' bit - - that we want to set gpio output and drive high (to set CPLD direction to drive)
				break;
			default:
				LOG_ERROR "%s,%d: invalid TrayIndex of %u from supplied bCellNo - so returning false\n", __func__, bCellNo, psda[bCellNo].TrayIndex);
				brc = false;
				break;
		}
		
		if (brc)
		{
			xyrc = pCellEnv->RIMIO_ConfigGpio(  u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control); 
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: RIMIO_ConfigGpio(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
			}
			
			xyrc = pCellEnv->RIMIO_ConfigDriven(  u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control); 
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: RIMIO_ConfigDriven(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask_data + u16RIMIO_mask_cpld_direction_control,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
			}
			
			xyrc = pCellEnv->RIMIO_DriveHighOrFloat( u16RIMIO_mask_cpld_direction_control);  
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: RIMIO_DriveHighOrFloat(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask_cpld_direction_control,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
			}

			xyrc = pCellEnv->RIMIO_DriveLow( u16RIMIO_mask_data); // this is the actual drive low of the sata pin 11
			if (xyrc != xyrcOK)
			{
				LOG_ERROR "%s,%d: RIMIO_DriveLow(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask_data,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
			}
		
		}
		LOG_INFO "%s,%d: Tray %u - done\n", __func__, bCellNo, psda[bCellNo].TrayIndex);

	
		
	#elif defined OPTIMUS  //--------------------------------------------------------------------------
	
		// (no need for TrayIndex here)
		uint16 u16RIMIO_mask = 0x0001;   // DUT_IO 0 = rimio 0 is wired to SATA_Pin11 (note this is also used by UART - but when put into ALT_FN mode by BulkSerialIOLineControl)
		
		xyrc = pCellEnv->RIMIO_ConfigGpio(  u16RIMIO_mask ); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: RIMIO_ConfigGpio(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
		}
		
		xyrc = pCellEnv->RIMIO_ConfigDriven(  u16RIMIO_mask); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: RIMIO_ConfigDriven(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask,  xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
		}
		
		xyrc = pCellEnv->RIMIO_DriveLow(  u16RIMIO_mask); 
		if (xyrc != xyrcOK)
		{
			LOG_ERROR "%s,%d: RIMIO_DriveLow(0x%.4X) FAILED - xyrc=%d=%s\n", __func__, bCellNo, u16RIMIO_mask, xyrc, CCELLENV::ConvertXyrcToPsz(xyrc));
		}

		LOG_INFO "%s,%d: Tray %u - done\n", __func__, bCellNo, psda[bCellNo].TrayIndex);

		
	#endif //--------------------------------------------------------------------------

	
	return brc;
} 





//---------------------------------------------------------------------------
bool TurnOnRiser( Byte bCellNo)
//---------------------------------------------------------------------------
{
	bool brc = true;

	xyrc xyrc = pCellEnv->SetSupplyOn( SID_FIXED_RISER_3V3 + SID_FIXED_RISER_5V );
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "TurnOnRiser: SetSupplyOn( SID_FIXED_RISER_3V3 + SID_FIXED_RISER_5V ) FAILED - xyrc=%d=%s\n", xyrc, CCELLENV::ConvertXyrcToPsz(xyrc) );
	}
	sleep(1); // allow it to process and settle (fine-tune later)??
  
	// NOTE: 29Apr2010
	// Powering the riser means that the CPLD on it will just have received power.
	//
	// The production CPLD (and FPGA) require that the CPLD config registers are written
	//  to appropriate values before use for 1-wire serial (or even for sata p11 state at drive power-up)
	// It is important that the power to the CPLD has 'settled' before attempting to write config bytes, hence the sleep above
	//
	// The cell firmware will do this for you automatically for you when you call BulkSerialIOLineControl()
	// See SerialInitialise() in serialfuncs.cpp
  
	// NOTE: 01May10 the shipped v3.00 CPLD then requires its config bytes to be written to 'normal' values after it is powered.
	//  This is performed automatically by the cell-firmware  ~500mS after it processes a riser power on (and before it returns).
	//  This gives the Testcase the option of using either  Env->RIMIO* functions   or  RIM->BulkSerialIOLineControl   to control satap11 at drive power on
	//----------------------------------------------------------------------------------------------------------------------------------
 	 
	return brc;
}

//---------------------------------------------------------------------------
unsigned long Homebrew_timeGetTime ( void )
//  our implementation of the standard Windows gettimeofday()
//  - return number of milliseconds since os-booted (WATCH OUT FOR THAT 43-DAY WRAPAROUND !!!!!!!!!!!)
//---------------------------------------------------------------------------
{
	struct timeval tvNow;
	gettimeofday( &tvNow, NULL);

	unsigned long MilliSecsCounter = tvNow.tv_sec * 1000 + (tvNow.tv_usec / 1000);

	return MilliSecsCounter;
}
 
 
//---------------------------------------------------------------------------
Byte SetFansToRPMControl( Byte bCellNo, uint16 u16DFanRPM, uint16 u16EfanRPM)  
//---------------------------------------------------------------------------
{
	Byte brc = 0;
	xyrc xyrc = xyrcOK;
	
	ASSERT_INITIALIZED 

	LOG_ERROR "%s,%d: SetFansToRPMControl: Setting fans to RPM mode at RPMs of (drive=%u, elec=%u) \n", __func__, bCellNo, u16DFanRPM, u16EfanRPM);
  
	xyrc = pCellEnv->SetDriveFanRPM( u16DFanRPM);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: SetDriveFanRPM(%u) failed xyrc=%d=%s\n", __func__, bCellNo, u16DFanRPM, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		brc = (Byte) xyrc;
	}
	xyrc = pCellEnv->SetElectronicsFanRPM( u16EfanRPM);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: SetElectronicsFanRPM(%u) failed xyrc=%d=%s\n", __func__, bCellNo, u16EfanRPM, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		brc = (Byte) xyrc;
	}
	xyrc = pCellEnv->SetFanSpeedControl(true);
	if (xyrc != xyrcOK)
	{
		LOG_ERROR "%s,%d: SetFanSpeedControl(true) failed xyrc=%d=%s\n", __func__, bCellNo, xyrc, CELLENV_XYRC_TO_STRING(xyrc));
		brc = (Byte) xyrc;
	}
  
	return brc;
}


