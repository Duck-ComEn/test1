#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../testcase/tcTypes.h"

#define VC_DATE                                  "2011/09/09"
#define VC_TESTCASE_VERSION                      "V01.04"
#define VC_BASE_VERSION                          "V01.03"
#define VC_HARDWARE_LIBRARY_VERSION              "Neptune_v3.2"
#define VC_EDITOR                                "YUKARI KATAYAMA"
#define VC_CHANGE_ITEM                           "neptune NT initialize support"
#define VC_REMARK                                ""

#define VC_STRING                                \
  VC_DATE ","                                    \
  VC_TESTCASE_VERSION ","                        \
  VC_BASE_VERSION ","                            \
  VC_HARDWARE_LIBRARY_VERSION ","                \
  VC_EDITOR ","                                  \
  VC_CHANGE_ITEM ","                             \
  VC_REMARK                                      \
  
#define VC_MAX_LENGTH                            128


// -------------------------------------------------------------------
// For Unit Test
// -------------------------------------------------------------------
#if 0
#include "libvc.h"

#undef TCPRINTF
#define TCPRINTF printf

int main(void) 
{
   Byte *bString = NULL;
   Dword dwLength = 0;
   Byte rc = 0;

   rc = vcGetVersion(&bString, &dwLength);
   printf("\nvcGetVersion,%d,%s,%ld\n", rc, bString, dwLength);

   rc = vcGetVersion(NULL, &dwLength);
   printf("\nvcGetVersion,%d,%s,%ld\n", rc, bString, dwLength);

   rc = vcGetVersion(&bString, NULL);
   printf("\nvcGetVersion,%d,%s,%ld\n", rc, bString, dwLength);

   rc = vcGetVersion(NULL, NULL);
   printf("\nvcGetVersion,%d,%s,%ld\n", rc, bString, dwLength);

   return 0;
}
#endif


// ----------------------------------------------------------------------
Byte vcGetVersion(Byte **string, Dword *length)
// ----------------------------------------------------------------------
{
  static Byte version[] = VC_STRING;
  Byte rc = 0;

  TCPRINTF("Entry:%s",&version[0]);

  if (strlen((char *)&version[0]) > VC_MAX_LENGTH) {
    version[VC_MAX_LENGTH] = '\0';
  }

  if ((string == NULL) || (length == NULL)) {
    TCPRINTF("do nothing (just showing version at Entry: point, 0x%x, 0x%x", (unsigned int)string, (unsigned int)length);
    rc = 1;
    goto VC_GET_VERSION_END;
  }

  *string = &version[0];
  *length = (Dword)strlen((char *)&version[0]);

 VC_GET_VERSION_END:
  TCPRINTF("Exit:%d", rc);
  return rc;
}
