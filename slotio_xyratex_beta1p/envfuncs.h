// prototype local functions  (Needs xytypes.h)
//-----------------------------------------------------------------------------------
	
void ObtainValidateAndDisplayEnvClassVersionInfo( Byte bCellNo );

bool CheckForAndReportAnyCellStatusErrors( Byte bCellNo, bool InitialCall);  //(returns true if there IS a problem) 

void ObtainAndLogCellFirmwareLimitsAndVersions( Byte bCellNo ); 


bool Configure_SATA_P11_as_Driven_Low_For_Autospinup( Byte bCellNo );    
bool Configure_SATA_P11_as_Float_To_PREVENT_Autospinup_using_envRIMIO(  Byte bCellNo ); 

bool TurnOnRiser( Byte bCellNo); 

Byte SetFansToRPMControl( Byte bCellNo, uint16 u16DFanRPM, uint16 u16EfanRPM); // for if/when needed ??



#include <unistd.h>

#include <sys/time.h> // for gettimeofday()

#define GET_CURRENT_MILLI_SECONDS_COUNTER()  Homebrew_timeGetTime()

unsigned long Homebrew_timeGetTime ( void ); // prototype our implementation of the standard Windows gettimeofday()
















