#ifndef _LIBVC_H_
#define _LIBVC_H_

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * <pre>
   * Description:
   *   get version string
   * Arguments:
   *   *string - OUTPUT - this function sets pointer to version string
   *   *length - OUTPUT - this function sets length of version string
   * Return:
   *   zero if no error. otherwise, error
   * Version String Format:
   *   comma separated format to include following information
   *    1. date
   *    2. testcase Version
   *    3. base version
   *    4. hardware library version
   *    5. editor
   *    6. change item
   *    7. remark
   * Usage:
   *    sample code :
   *        Byte *string = NULL;
   *        Dword length = 0;
   *        Byte rc = 0;
   *      
   *        rc = vcGetVersion(&string, &length);
   *        if (rc) {
   *          printf("error on vcGetVersion rc=%d\n", rc);
   *        } else {
   *          printf("vcGetVersion,%s,%ld\n", string, length);
   *        }
   *    in stdout :
   *        vcGetVersion,2011/06/14,V01.00,V00.99,Neptune-II_V3.2,Satoshi Takahashi,Support Neptune-II V3.2 for Htemp,,93
   * </pre>
   *****************************************************************************/
  Byte vcGetVersion(Byte **string, Dword *length);

#ifdef __cplusplus
}
#endif

#endif
