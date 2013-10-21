#include "tc.h"

#define MATCH                          0     // makes use of strncmp() more readable

struct symlist {
  char *name;
  size_t start;
  size_t length;
  struct symlist *next;
} *symbols = NULL;

/* ---------------------------------------------------------------------- */
void tcInitCaller()
/* ---------------------------------------------------------------------- */
{
  char line[1024];
  char command[1024];
  snprintf(command, sizeof(command), "nm -S /proc/%d/exe", getpid());
  FILE *fp = popen(command, "r");
  if (fp == NULL) {
    perror(command);
  }
  while (fgets(line, sizeof(line), fp) != NULL) {
    size_t start, length;
    char name[1024];
    if (sscanf(line, "%x %x T %s", &start, &length, name) == 3) {
      struct symlist *s = malloc(sizeof(struct symlist));
      if (s == NULL) {
        perror("malloc");
      }
      s->name = strdup(name);
      s->start = start;
      s->length = length;
      s->next = symbols;
      symbols = s;
    }
  }
}


/* ---------------------------------------------------------------------- */
const char *tcGetCaller(unsigned int level)
/* ---------------------------------------------------------------------- */
{
  const char *caller = "";
  size_t from = (size_t)0;
  struct symlist *s;

  switch (level) {
  case 0:
    from = (size_t)__builtin_return_address(0);
    break;
  case 1:
    from = (size_t)__builtin_return_address(1);
    break;
  case 2:
    from = (size_t)__builtin_return_address(2);
    break;
  case 3:
    from = (size_t)__builtin_return_address(3);
    break;
  case 4:
    from = (size_t)__builtin_return_address(4);
    break;
  case 5:
    from = (size_t)__builtin_return_address(5);
    break;
  case 6:
    from = (size_t)__builtin_return_address(6);
    break;
  case 7:
    from = (size_t)__builtin_return_address(7);
    break;
  case 8:
    from = (size_t)__builtin_return_address(8);
    break;
  case 9:
    from = (size_t)__builtin_return_address(9);
    break;
  case 10:
    from = (size_t)__builtin_return_address(10);
    break;
  case 11:
    from = (size_t)__builtin_return_address(11);
    break;
  case 12:
    from = (size_t)__builtin_return_address(12);
    break;
  case 13:
    from = (size_t)__builtin_return_address(13);
    break;
  case 14:
    from = (size_t)__builtin_return_address(14);
    break;
  case 15:
    from = (size_t)__builtin_return_address(15);
    break;
  case 16:
    from = (size_t)__builtin_return_address(16);
    break;
  case 17:
    from = (size_t)__builtin_return_address(17);
    break;
  case 18:
    from = (size_t)__builtin_return_address(18);
    break;
  case 19:
    from = (size_t)__builtin_return_address(19);
    break;
  default:
    return caller;
    break;
  }

  for (s = symbols; s != NULL; s = s->next) {
    if (s->start <= from && from < s->start + s->length) {
      caller = s->name;
    }
  }
  return caller;
}


/* ---------------------------------------------------------------------- */
void tcDumpCaller(void)
/* ---------------------------------------------------------------------- */
{
  const char *caller = NULL;
  unsigned int level = 0;

  for (level = 1; ;level++) {
    caller = tcGetCaller(level);
    if (MATCH == strcmp(caller, "")) {
      break;
    }
    TCPRINTF("caller(%d) = %s\n", level, caller);
  }
}


TEST_SCRIPT_CONTROL_BLOCK *tcUnitTestTscb = NULL;
/* ---------------------------------------------------------------------- */
TEST_SCRIPT_CONTROL_BLOCK *tcGetTscb(int size)
/* ---------------------------------------------------------------------- */
{
  tcUnitTestTscb = malloc(size);
  return tcUnitTestTscb;
}


unsigned char *tcUnitTestBuffer = NULL;
/* ---------------------------------------------------------------------- */
unsigned char *tcGetBuffer(int size)
/* ---------------------------------------------------------------------- */
{
  tcUnitTestBuffer = malloc(size);
  return tcUnitTestBuffer;
}

extern Dword convertBadHead(Dword current_bad_head, Dword max_head_num, Byte *current_head_table, Byte *default_head_table);
/* ---------------------------------------------------------------------- */
int tcBadHeadUnitTest(void)
/* ---------------------------------------------------------------------- */
{
  Byte currentHeadTable[][10] = {{9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},

    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},

    {9, 8, 7, 6, 4, 3, 2, 1, 0, 5},
    {9, 8, 7, 6, 4, 3, 2, 1, 0, 5},
    {9, 8, 7, 6, 4, 3, 2, 1, 0, 5},
    {9, 8, 7, 6, 4, 3, 2, 1, 0, 5},
    {9, 8, 7, 6, 4, 3, 2, 1, 0, 5},
    {9, 8, 7, 6, 4, 3, 2, 1, 0, 5},

    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},

    {0, 1, 2, 3, 4, 5, 7, 7, 7, 7},
    {0, 1, 2, 3, 4, 5, 7, 7, 7, 7},
    {0, 1, 2, 3, 4, 5, 7, 7, 7, 7},
    {0, 1, 2, 3, 4, 5, 7, 7, 7, 7},
    {0, 1, 2, 3, 4, 5, 7, 7, 7, 7},
    {0, 1, 2, 3, 4, 5, 7, 7, 7, 7},

    {5, 4, 2, 1, 0, 3, 7, 7, 7, 7},
    {5, 4, 2, 1, 0, 3, 7, 7, 7, 7},
    {5, 4, 2, 1, 0, 3, 7, 7, 7, 7},
    {5, 4, 2, 1, 0, 3, 7, 7, 7, 7},
    {5, 4, 2, 1, 0, 3, 7, 7, 7, 7},
    {5, 4, 2, 1, 0, 3, 7, 7, 7, 7}
  };

  Byte defaultHeadTable[][10] = {{9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},

    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},

    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {9, 8, 7, 6, 5, 4, 3, 2, 1, 0},

    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},

    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},

    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7},
    {5, 4, 3, 2, 1, 0, 7, 7, 7, 7}
  };

  Dword max_head_num[] = {9,
                          9,
                          9,
                          9,
                          9,
                          9,

                          9,
                          9,
                          9,
                          9,
                          9,
                          9,

                          9,
                          9,
                          9,
                          9,
                          9,
                          9,

                          5,
                          5,
                          5,
                          5,
                          5,
                          5,

                          5,
                          5,
                          5,
                          5,
                          5,
                          5,

                          5,
                          5,
                          5,
                          5,
                          5,
                          5
                         };

  Dword current_bad_head[] = {0x0000,
                              0x0001,
                              0x0080,
                              0x0200,
                              0x0281,
                              0x03ff,

                              0x0000,
                              0x0001,
                              0x0080,
                              0x0200,
                              0x0281,
                              0x03ff,

                              0x0000,
                              0x0001,
                              0x0080,
                              0x0200,
                              0x0281,
                              0x03ff,

                              0x0000,
                              0x0001,
                              0x0008,
                              0x0020,
                              0x0029,
                              0x003f,

                              0x0000,
                              0x0001,
                              0x0008,
                              0x0020,
                              0x0029,
                              0x003f,

                              0x0000,
                              0x0001,
                              0x0008,
                              0x0020,
                              0x0029,
                              0x003f
                             };

  Dword expected_new_bad_head[] = {0x0000,
                                   0x0001,
                                   0x0080,
                                   0x0200,
                                   0x0281,
                                   0x03ff,

                                   0x0000,
                                   0x0200,
                                   0x0004,
                                   0x0001,
                                   0x0205,
                                   0x03ff,

                                   0x0000,
                                   0x0001,
                                   0x0100,
                                   0x0010,
                                   0x0111,
                                   0x03ff,

                                   0x0000,
                                   0x0001,
                                   0x0008,
                                   0x0020,
                                   0x0029,
                                   0x003f,

                                   0x0000,
                                   0x0020,
                                   0x0004,
                                   0x0001,
                                   0x0025,
                                   0x003f,

                                   0x0000,
                                   0x0001,
                                   0x0010,
                                   0x0004,
                                   0x0015,
                                   0x003f
                                  };

  int i = 0;
  Dword output_new_bad_head = 0;

  for (i = 0 ; i < sizeof(max_head_num) / sizeof(max_head_num[0]) ; i++) {
    output_new_bad_head = convertBadHead(current_bad_head[i], max_head_num[i], currentHeadTable[i], defaultHeadTable[i]);
    ts_printf(__FILE__, __LINE__, "current head table = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", currentHeadTable[i][0], currentHeadTable[i][1], currentHeadTable[i][2], currentHeadTable[i][3], currentHeadTable[i][4], currentHeadTable[i][5], currentHeadTable[i][6], currentHeadTable[i][7], currentHeadTable[i][8], currentHeadTable[i][9]);
    ts_printf(__FILE__, __LINE__, "default head table = %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", defaultHeadTable[i][0], defaultHeadTable[i][1], defaultHeadTable[i][2], defaultHeadTable[i][3], defaultHeadTable[i][4], defaultHeadTable[i][5], defaultHeadTable[i][6], defaultHeadTable[i][7], defaultHeadTable[i][8], defaultHeadTable[i][9]);
    ts_printf(__FILE__, __LINE__, "max head number = %d", max_head_num[i]);
    ts_printf(__FILE__, __LINE__, "current bad head bit field = %04xh", current_bad_head[i]);
    ts_printf(__FILE__, __LINE__, "    new bad head bit field = %04xh", output_new_bad_head);

    if (expected_new_bad_head[i] != output_new_bad_head) {
      return 1;
    }
  }

  return 0;
}
