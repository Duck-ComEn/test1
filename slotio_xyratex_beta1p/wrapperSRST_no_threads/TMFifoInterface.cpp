// altered to build under Windows as well as Linux
// - making use of os-independent OS_FIFO functions as used by TestManager itself

// ALSO I removed the lookup into TestmManger.ini (hardcoded path)
//      to get the 'PartialFIFOFileName'
//
//  The default PartialFIFOFileName is now HARDCODED (both here and in TestManger)
//  - we could consider using an environment variable if we really want it configurable
//--------------------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "xytypes.h" // defines OS_IS_* plus uint8 etc.

// from OSSpecific.h
#include <fcntl.h>     // O_NONBLOCK, O_RDONLY etc
#include <unistd.h>

#define FIFO_HANDLE     int
#define INVALID_HANDLE_VALUE   -1  // already a standard define in Windows
int OS_FIFO_Write( FIFO_HANDLE FifoHandle, char * Buffer, int BufferSize);
void OS_FIFO_Close( FIFO_HANDLE FifoHandle  );
FIFO_HANDLE OS_FIFO_Create( char * pszFifoName, int OS_FIFO_Flags ); // server uses this
FIFO_HANDLE OS_FIFO_Open( char * pszFifoName, int OS_FIFO_Flags ); // clients use this
	//
	#define OS_FIFO_FLAGS_DIRECTION_OUTBOUND  0x01
	#define OS_FIFO_FLAGS_DIRECTION_INBOUND   0x02

  // the pipe is created in BLOCKED mode, unless the following OPTION is also specified
	#define OS_FIFO_FLAGS_WANT_NON_BLOCKED_MODE  0x04  // believe this is only relevant to INBOUND (reads)

  // the pipe is created in MESSAGE mode, unless the following OPTION is also specified
	#define OS_FIFO_FLAGS_WANT_BYTE_MODE  0x08



#define IM_EXPORTING_CLASS_TM_FIFO_INTERFACE // tells following header to EXPORT (rather than import) the class
#include "TMFifoInterface.h"

//#include "xylogger.h"

// m_fifo_handle is declared in the header as a void *
// (to avoid class users needing to know any os-specific implementation and associated headers)
// The following macro avoids us having to cast the void * m_fifo_handle to a FIFO_HANDLE everywhere on use in this implementation
#define M_FIFO_HANDLE ((FIFO_HANDLE) m_fifo_handle)

//----------------------------------------------------------------------------
CTMFifoInterface::CTMFifoInterface(int CellIndex, int TrayIndex) :
//----------------------------------------------------------------------------
	m_InstantiationOK(true),
	m_CellIndex(CellIndex),
	m_TrayIndex(TrayIndex),
	m_fifo_handle( (void *)INVALID_HANDLE_VALUE)
{
	// default partial fifoname now hardcoded
	// (used to look this up in TestManger.ini - but hard to locate that cross-OS)
	// TODO: we could look for an ENVIRONMENT VARIABLE set by the TestManager to indicate some non-statndard name
	strcpy( m_PartialFIFOFileName, "XYTMFIFO");

	if ( m_InstantiationOK == true )
		if ( BuildFullFIFOFileName() != 0 )
			m_InstantiationOK = false;

	if ( m_InstantiationOK == true )
		if ( OpenFifo() != 0 )
			m_InstantiationOK = false;
}

//----------------------------------------------------------------------------
CTMFifoInterface::~CTMFifoInterface()
//----------------------------------------------------------------------------
{
	CloseFifo();
}

//----------------------------------------------------------------------------
bool CTMFifoInterface::CheckInstantiationOK(void)
//----------------------------------------------------------------------------
{
	return m_InstantiationOK;
}

//----------------------------------------------------------------------------
bool CTMFifoInterface::WriteToFifo(char *szAttribute, char *szValue)
//----------------------------------------------------------------------------
{
	bool ReturnCode = false;
	int ByteWritten = 0;

	if ( m_InstantiationOK == true )
	{
		ByteWritten = snprintf(m_SendBuffer, en_MaxFIFOData, "%s:%s", szAttribute, szValue);
		if ( ByteWritten != -1)
		{
			ByteWritten = OS_FIFO_Write( (FIFO_HANDLE)m_fifo_handle, m_SendBuffer, (int)(strlen(m_SendBuffer) + 1));
			if ( ByteWritten > 0)
			{
				ReturnCode = false;
			}
			else
			{
				//LogError(SEV_ERROR, "CTMFifoInterface::WriteToFifo() - Failed to write data - %s", strerror(errno));
				ReturnCode = false;
			}
		}
		else
		{
			//LogError(SEV_ERROR, "CTMFifoInterface::WriteToFifo() - Trying to send too much data!");
			ReturnCode = false;
		}
	}
	else
	{
		//LogError(SEV_ERROR, "CTMFifoInterface::WriteToFifo() - prior Instantiation failure");
		ReturnCode = false;
	}

	return ReturnCode;
}




//----------------------------------------------------------------------------
char * CTMFifoInterface::GetCharArray( void)  // debug
//----------------------------------------------------------------------------
{
	return CharArray;
}

//----------------------------------------------------------------------------
void CTMFifoInterface::SetCharArray( char  * szData)  // debug
//----------------------------------------------------------------------------
{
	strcpy( CharArray, szData);
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------- PRIVATE METHODS ------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
int CTMFifoInterface::BuildFullFIFOFileName()
//----------------------------------------------------------------------------
{
	int ReturnCode = 0;
	int ByteWritten = 0;

	// Build full fifo file name.
	ByteWritten = snprintf(m_FullFIFOFileName, en_MaxFullFIFOFileNameLength,
							"%s%03d%1d", m_PartialFIFOFileName, m_CellIndex, m_TrayIndex);
	if (ByteWritten == -1)
	{
		//LogError(SEV_ERROR, "CTMFifoInterface::BuildFullFIFOFileName() - Full FIFO file name too long");
		ReturnCode = 1;
	}

	return ReturnCode;
}

//----------------------------------------------------------------------------
int CTMFifoInterface::OpenFifo()
//----------------------------------------------------------------------------
{
	int ReturnCode = 0;

	// safer/more-flexible/essential to specify non-blocking mode here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	//m_fifo_handle = (void *) OS_FIFO_Open( m_FullFIFOFileName, OS_FIFO_FLAGS_DIRECTION_OUTBOUND);
	
	// FIX 24Aug06 under Linux, TestManager creates all possible fifos when it starts up
	// (but only listens on one after starting a testcase on it)
	// if a testcase is run manually (Not through testmanager) then
	// the normal client fifo-open here will block indefinatley waiting for the server side (which never happens) !!!!
	// and the testcase process effectively totally hangs!!!!!!
	
	// so safer/more-flexible/essential to specify non-blocking mode here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	 
	m_fifo_handle = (void *) OS_FIFO_Open( m_FullFIFOFileName, OS_FIFO_FLAGS_DIRECTION_OUTBOUND + OS_FIFO_FLAGS_WANT_NON_BLOCKED_MODE); // 24Aug06
	
	
	if ( m_fifo_handle == (void *) INVALID_HANDLE_VALUE )
	{
		//LogError(SEV_ERROR, "CTMFifoInterface::OpenFifo() - OS_FIFO_Open Failed %s", strerror(errno));
		ReturnCode = 1;
	}

	return ReturnCode;
}

//----------------------------------------------------------------------------
void CTMFifoInterface::CloseFifo()
//----------------------------------------------------------------------------
{
	if ( (FIFO_HANDLE)m_fifo_handle != INVALID_HANDLE_VALUE )
	{
		 OS_FIFO_Close( (FIFO_HANDLE) m_fifo_handle);
	} 
}



#if defined OS_IS_WINDOWS

	#error "Dont build this file under windows - just link with TMFifoInterface_lib_msvc7.lib instead !!!!"
	

#elif defined OS_IS_LINUX   //==================================================================================================


//------------------------------------------------------------------------------
FIFO_HANDLE OS_FIFO_Open( char * pszFifoName, int OS_FIFO_Flags ) // see OS_FIFO_FLAGS_* defines in OSSpecific.h
// as used by a 'client'
// Under Linux, this just opens (Server MUST have Created Fifo FIRST)
//------------------------------------------------------------------------------
{
	FIFO_HANDLE fh = INVALID_HANDLE_VALUE;
	bool CreateError = false;

	char szFullPipeName[1024];
	sprintf(szFullPipeName, "/tmp/%s", pszFifoName);
	
	// If the FIFO file isn't there, panic!
	if (access(szFullPipeName, F_OK) == -1)
	{
		/*
		LogError(OS_TraceLevel, "OS_FIFO_Open() - need to make FIFO \"%s\" ", szFullPipeName);
		if ( mkfifo(pszFifoName, 0777) != 0 )
		{
			LogError(SEV_FATAL, "OS_FIFO_Open() - Failed to create FIFO \"%s\"", szFullPipeName);
			CreateError = true;
		}
		*/
		//LogError(SEV_ERROR, "OS_FIFO_Open() - Fifo \"%s\" has not yet been created", szFullPipeName);
		CreateError = true;
	}

	if ( CreateError == false)
	{
		// Try to open the fifo
		// NOTE: this same open is used by:
		//     the TMFIFoWrite(testcase) -  for writing TO the TestManager
		//     the TestManager -  for reading FROM the Testcase
		int mode = 0;
		bool error = false;
		if ( ( OS_FIFO_Flags & OS_FIFO_FLAGS_DIRECTION_OUTBOUND ) && ( OS_FIFO_Flags & OS_FIFO_FLAGS_DIRECTION_INBOUND ) )
		{
			mode = O_RDWR;
		} 
		else if ( OS_FIFO_Flags & OS_FIFO_FLAGS_DIRECTION_OUTBOUND ) 
		{
			mode = O_WRONLY;
		} 
		else if ( OS_FIFO_Flags & OS_FIFO_FLAGS_DIRECTION_INBOUND ) 
		{
			mode = O_RDONLY;
		} 
		else
		{
			//LogError(SEV_ERROR, "OS_FIFO_Open() - no direction specified");
			mode = 0; // error
			error = true;
		}   
		
		if ( OS_FIFO_Flags & OS_FIFO_FLAGS_WANT_NON_BLOCKED_MODE )
		{
			mode |= O_NONBLOCK;
		}
		
		if ( OS_FIFO_Flags & OS_FIFO_FLAGS_WANT_BYTE_MODE )
		{
			// ???? mode |= ??;
			//LogError(SEV_ERROR, "OS_FIFO_Open() - OS_FIFO_FLAGS_WANT_BYTE_MODE not yet supported (defaults to message mode)");
		}
		
		if (error==false)
		{
			fh = open(szFullPipeName, mode);
			
			
			if (fh == INVALID_HANDLE_VALUE)
			{
				//LogError(SEV_ERROR, "OS_FIFO_Open() - Failed to open FIFO \"%s\"", szFullPipeName);
				//LogError(SEV_ERROR, "          errno=%d(%s) - OS_FIFO_Flags=0x%x raw Linux Mode flags=0x%x ",
				//                       errno, strerror(errno), OS_FIFO_Flags, mode);
			}
		} 
		else
		{
			fh = INVALID_HANDLE_VALUE;
		} 
	}
	else
	{
		fh = INVALID_HANDLE_VALUE;
	}

	return fh;
}

//------------------------------------------------------------------------------
int OS_FIFO_Write( FIFO_HANDLE FifoHandle, char * Buffer, int BufferSize)
//------------------------------------------------------------------------------
{
	unsigned long nBytesWritten = 0;

	nBytesWritten = write(
											 FifoHandle,
											 Buffer,
											 BufferSize
										 );

	 return (int) nBytesWritten;
}


//------------------------------------------------------------------------------
void OS_FIFO_Close( FIFO_HANDLE FifoHandle  )
//------------------------------------------------------------------------------
{
	if (FifoHandle != INVALID_HANDLE_VALUE)
	{
		close( FifoHandle);
	}
	else
	{
		//LogError(SEV_ERROR, "OS_FIFO_Close - supplied FIFO_HANDLE was INVLAID");
	}
}



#else    //===================================================================================================
	#error "OS_IS_ undefined"
#endif
