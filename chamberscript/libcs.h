#ifndef _LIBCS_H_
#define _LIBCS_H_

#ifdef __cplusplus
extern "C"
{
#endif

  void *runChamberScriptThread(void *arg);

  /**
   * <pre>
   * Description:
   *   Request synchronization with chamber script
   * Arguments:
   *   *dwForwardingTimeInSec - OUTPUT - See note for details
   * Return:
   *   0      : Success to go to the label
   *   1      : The label is not found (no label in the sequence or label had already passed)
   *   2      : Requested to go to the label, but waiting other test script ready to go
   *   other  : Reserved
   * Note:
   *   This function is temporary solution to support Mariner early phase evaluation,
   *   therefore this is very limited function for synchronization between chamber script and test script.
   *     - label() argument is not supported (label(#A) and label(#B) is SAME for this function)
   *     - label() should not be greater than one. this function cannot handle two or more label()
   *
   *     - test script should call this function at end of the test (e.g. call in runTestScriptDestructor())
   *
   *   This is non-blocking function.
   *
   *   *dwForwardingTimeInSec includes not actual time passing, but fowarding time
   *   from current sequence pointer to the label location by this goto function.
   *   *dwForwardingTimeInSec is valid only if return code is 0.
   *
   *   If test script find retun code 0, then it should manipulate its timeout value
   *   by substructing *dwForwardingTimeInSec.
   * Example:
   *   In case of 2 test scripts running, first test script calls this function and
   *   gets return code 2, because second test script has not called yet.
   *   First test script continues to call this function with appropriate interval (e.g. 1 min).
   *
   *   Second test script calls this function to get return code 0, then reduce
   *   its timeout (e.g. tscb->dwDriveTestElapsedTimeSec += *dwForwardingTimeInSec)
   *
   *   First test script calls this function then find return code 0. First test script
   *   reduce its timeout as well.
   * </pre>
   *****************************************************************************/
  Byte csGotoRequest(Dword *dwForwardingTimeInSec);
  Byte csSendSyncMessage(int type, int param[8], char *stringa);
  Byte csDeleteResumeIdx(void);
  Byte csWriteResumeIdx( Dword minute, Word csindex, Byte tempResumeMode);
  Byte csReadResumeIdx( Dword *minute, Word *csIndex, Byte *tempResumeMode);
  Byte csDeleteElapsedLine( const char *sequenceFile, long sequenceLength,
                            char *shortSequence, long *shortLength );

#ifdef __cplusplus
}
#endif

#endif
