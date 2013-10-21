#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>  // for getpid, sleep, usleep
#include <errno.h>
#include "../testcase/tcTypes.h"
#include "../slotio/libsi.h"
#include "../testscript/libts.h"
#include "../syncmanager/libsm.h"
#include "libsh.h"

enum {
  WAIT_TIME_IN_SEC_AFTER_REQUESTING_ABORT = 300,          /**< it shall be larger than SLEEP_UNIT_SEC + SLEEP_TIME_AFTER_DRIVE_UNLOAD_SEC + dwDrivePowerOffWaitTime */
  WAIT_TIME_FOREVER = 60 * 60 * 24 * 365
};

/**
 * <b>
 * Version: V1.00 <br>
 * Date:    01 Jun 2006</b><p>
 *
 * This function is SIGNAL thread. <p>
 *
 * Reference) http://www.ne.jp/asahi/hishidama/home/tech/unix/signal.html
 *            http://codezine.jp/article/detail/1973
 *            http://codezine.jp/article/detail/1591?p=1
 *
 * We need to catch signals so we can exit gracefully <br>
 * ie leaving the cell in a safe/sensible state <br>
 * It is advisable to avoid, where possible, having our process killed off whilst <br>
 * within a Xyratex Cell Method (this can leave the cell firmware in an <br>
 * indeterminate state, which must be recovered next time you try to use the cell. <p>
 *
 * myhandler will set a flag and continue. <br>
 *  (main loop will test flag at suitable times and exit gracefully) <p>
 *
 ***************************************************************************************/
void *runSignalHandlerThread(void *arg)
{
  sigset_t set;
  int ret = 0;
  Dword i = 0;
  SM_MESSAGE smMessage;
  Byte rc = 0;

  TCPRINTF("Entry: pthread_t,%08lxh", (unsigned long)pthread_self());

  // Mutex Lock
  ret = pthread_mutex_lock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }

  // initialize 'set'
  ret = sigfillset(&set);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }

  // take away the signals from mask list
  ret = sigdelset(&set, SIGKILL);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }
  ret = sigdelset(&set, SIGSTOP);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }

  // mask the signals
  ret = pthread_sigmask(SIG_SETMASK, &set, NULL);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }

  // Mutex Unlock
  ret = pthread_mutex_unlock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }

  while (1) {
    siginfo_t info;
    int goExit = 1;

    // Wait for Signal
    errno = 0;
    ret = sigwaitinfo(&set, &info);
    if (ret < 0) {
      //
      // note:
      //   in case of failure, testcase shall exit? or kill signal hander?
      //   even signal handler killed, testcase probably works.
      //   but if testcase hangs up, then testcase never finish.
      //
      // reference:
      //   see /usr/include/asm-generic/errno-base.h for errno macros
      //
      int errsv = errno;
      TCPRINTF("warning - %d %d, %d", ret, errno, info.si_signo);
      if (errsv == EINTR) {
        TCPRINTF("Caught signal SIGINT1" );
        //
        // note:
        //   gcore (called at exit()) raises bunch of 'kill' signals (SIGHUP, SIGINT, SIGTERM, SIGSTOP)
        //   this should exit immediately when this received catchable/ignorerable signals
        //
        // reference:
        //   see http://src.gnu-darwin.org/src/usr.bin/gcore/gcore.c.html for signals raised by gcore
        //
        goto END_RUN_SIGNAL_HANDLER;
      } else {
        TCPRINTF("Caught signal SIGINT2" );
        goto ERROR_RUN_SIGNAL_HANDLER;
      }
    }

    //
    // show signal type
    //
    // reference:
    //   see /usr/include/bits/signum.h for signal macros
    //
    goExit = 1;
    switch (info.si_signo) {
    case SIGINT:
      TCPRINTF("Caught signal SIGINT" );
      break;
    case SIGQUIT:
      TCPRINTF("Caught signal SIGQUIT" );
      break;
    case SIGILL:
      TCPRINTF("Caught signal SIGILL" );
      break;
    case SIGABRT:
      TCPRINTF("Caught signal SIGABRT" );
      break;
    case SIGFPE:
      TCPRINTF("Caught signal SIGFPE" );
      break;
    case SIGKILL:
      TCPRINTF("Caught signal SIGKILL" );
      break;
    case SIGSEGV:
      TCPRINTF("Caught signal SIGSEGV" );
      break;
    case SIGPIPE:
      TCPRINTF("Caught signal SIGPIPE" );
      break;
    case SIGALRM:
      TCPRINTF("Caught signal SIGALRM" );
      break;
    case SIGTERM:
      TCPRINTF("Caught signal SIGTERM" );
      break;
    case SIGUSR1:
      TCPRINTF("Caught signal SIGUSR1" );
      break;
    case SIGUSR2:
      TCPRINTF("Caught signal SIGUSR2" );
      break;
    case SIGCHLD:  // it occurs when testcase exits.
      TCPRINTF("Caught signal SIGCHLD" );
      goExit = 0;
      break;
    case SIGCONT:  // fg
      TCPRINTF("Caught signal SIGCONT" );
      goExit = 0;
      break;
    case SIGSTOP:
      TCPRINTF("Caught signal SIGSTOP" );
      break;
    case SIGTSTP:  // CTRL + Z
      TCPRINTF("Caught signal SIGTSTP" );
      goExit = 0;
      break;
    case SIGTTIN:
      TCPRINTF("Caught signal SIGTTIN" );
      break;
    case SIGTTOU:
      TCPRINTF("Caught signal SIGTTOU" );
      break;
    case SIGWINCH:  // it occurs when its controlling terminal changes size
      TCPRINTF("Caught signal SIGWINCH" );
      goExit = 0;
      break;
    default:
      TCPRINTF("Caught unknown signal %d", info.si_signo);
      break;
    }
    if (goExit) {
      TCPRINTF(" - will exit at earliest safe opportunity!!!!!!!!!!!!!!!" );
      break;
    } else {
      TCPRINTF(" - ignore this signal" );
    }
  }

  // abort testscript(s)
  ret = pthread_mutex_lock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }
  if (tccb.optionSyncManagerDebug) {
    smMessage.type   = SM_SH_SD_ABORT;
    char string[] = "SignalHandler:ABORT";
    strcpy(smMessage.string, string);
    rc = smSendSyncMessage(&smMessage, SM_SH_SD_TIMEOUT);
    if (rc) {
      TCPRINTF("error %d", ret);
      goto ERROR_RUN_SIGNAL_HANDLER;
    }
    // Mutex Unlock
    ret = pthread_mutex_unlock(&mutex_slotio);
    if (ret) {
      TCPRINTF("error %d", ret);
      goto ERROR_RUN_SIGNAL_HANDLER;
    }
  } else {
    for (i = 0 ; ; i++) {
      Word wMyCellNo = 0;
      rc = get_MY_CELL_NO_by_index(i, &wMyCellNo);
      if (rc) {
        break;
      }
      TCPRINTF("error %d", wMyCellNo + 1);
      abortTestScript(wMyCellNo + 1);
    }
  }
// Mutex Unlock
  ret = pthread_mutex_unlock(&mutex_slotio);
  if (ret) {
    TCPRINTF("error %d", ret);
    goto ERROR_RUN_SIGNAL_HANDLER;
  }
  sleep(WAIT_TIME_IN_SEC_AFTER_REQUESTING_ABORT);   // [DEBUG] need to define appropriate number to give testscript to abort

ERROR_RUN_SIGNAL_HANDLER:
  tcExit(EXIT_FAILURE);
  return NULL;

END_RUN_SIGNAL_HANDLER:
//  pthread_exit(NULL);
  sleep(WAIT_TIME_FOREVER);
  return NULL;
}
