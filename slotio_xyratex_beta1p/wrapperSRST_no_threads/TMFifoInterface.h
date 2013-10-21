//----------------------------------------------------------------------------
// Defines an interface to be used by TestCases
// for communication to the TestMangaer over fifos
//
// Currently all that is supported is for the testcase to send/set
// the value for a named attribute
//
// NOTE: this is a customer-facing interface 
// - keep it clean and simple!
//----------------------------------------------------------------------------

class CTMFifoInterface
{
	public:
		enum {
				en_MaxPartialFIFOFileNameLength = 128,
				en_MaxFullFIFOFileNameLength = en_MaxPartialFIFOFileNameLength + 3 + 1,  // 3 digits CellIndex,  1 digit tray index
				en_MaxFIFOData = 4096};  // (keep <= 4096, the PIPE_BUF from linux/limits.h)

		CTMFifoInterface( int CellIndex, int TrayIndex);
		~CTMFifoInterface();

		bool CheckInstantiationOK( void);

		bool WriteToFifo( char *szAttribute, char *szValue);


		char *GetCharArray( void);  // debug
		void SetCharArray( char  * szData);  // debug

	private:
		bool m_InstantiationOK;
		int  m_CellIndex;
		int  m_TrayIndex;
		char m_PartialFIFOFileName[ en_MaxPartialFIFOFileNameLength];
		char m_FullFIFOFileName[ en_MaxFullFIFOFileNameLength];
		void *m_fifo_handle;  // keep generic here - implementation needs to cast to an OS_FIFO_HANDLE
		char m_SendBuffer[ en_MaxFIFOData];

		char CharArray[255];   // debug

		int BuildFullFIFOFileName();
		int  OpenFifo();
		void CloseFifo();
};

//----------------------------------------------------------------------------
