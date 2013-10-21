//libsi_xyexterns.h
// lib's externs of Tracing flags. pstats etc 

// (pulled out of libsi.h until 'bool' issue is understood/resolved)

//-------------------------------------------------------------------
// NOTE: the intention is to have the TESTCASE include this file, 
//       - eg to MODIFY the tracing flags dynamically/real-time
//       (and also to access serial statitstics etc)
//-------------------------------------------------------------------

extern bool WantLogAPIInfo; // default=true 
extern bool WantLogAPITrace; // default=false

extern bool WantLogAPIEntryExit; // default=false

// (Note there is now a special API to control Protocol Type (old/new/nerer etc)) 

extern struct _protocol_stats pstats[255]; // READ-access from TestCase (Index via bCellNo)    (struct defined in tcTypes.h)

