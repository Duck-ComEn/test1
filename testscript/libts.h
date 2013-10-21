#ifndef _LIBTS_H_
#define _LIBTS_H_

void *runTestScriptThread(void *arg);
Byte set_task(Word w);
Word get_task_base_offset(void);
Word get_task(void);
Byte get_MY_CELL_NO_by_index(Word wIndex, Word *wMyCellNo);
void abortTestScript(Byte bCellNo);

#endif
