/*!
 * @file
 * 
 * 
 * 
 * 
 * 
 */


/*!
 * System Include Files
 */
#include <alloca.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "neptune_sio2.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
};
#endif


#define MAX_SLOTS (140+1)

typedef struct {
	int id;
	int force;
} parm;

void *update_dio_fw(void *arg)
{
	int rc;
    class Neptune_sio2 *ns2;
    TER_Status status;
	TER_SlotInfo pSlotInfo;
    DP  dp;
	
	
	ns2 = new Neptune_sio2();
  	ns2->TER_Connect("10.101.1.2");
	
	parm *p=(parm *)arg;
	
	status = ns2->TER_IsDioPresent ( p->id-1, &dp );
    if ( status != TER_Status_none ) {
		printf("Slot %d, DIO is not responding. DIO may not be present \n", p->id);
	}
    else if ( DP_FLAGS_main_app_xsum_not_valid & dp.dp_flags || DP_FLAGS_upgrade_recommended & dp.dp_flags || p->force ) {
		printf("Slot %d, Started DIO firmware update... \n", p->id);
    	status = ns2->TER_UpgradeMicrocode (p->id-1);
		if( status != 0 ) {
			printf("Slot %d, ERROR: TER_UpgradeMicrocode failed [rc=0x%08x] \n", p->id, rc);
		}
		else {
	    	status = ns2->TER_GetSlotInfo(p->id-1,  &pSlotInfo);
			if( status == 0 ) {
				printf("Slot %d, DIO firmware update completed successfully to <%s> \n", p->id, pSlotInfo.DioFirmwareVersion);
			}
		}

    }
	else {
	   status = ns2->TER_GetSlotInfo(p->id-1,  &pSlotInfo);
	   if( status == 0 ) {
		   printf("Slot %d, DIO firmware update not needed. Current firmware version <%s> \n", p->id, pSlotInfo.DioFirmwareVersion);
	   }
	}
 
    ns2->TER_Disconnect();
  	return (NULL);
}

int main(int argc, char* argv[]) 
{
	int n, from_slot, to_slot, i;
	pthread_t *threads;
	parm *p;
	int force = 0;

	if (argc < 3)
	{
		printf ("Usage: updatediofw <from_slot> <to_slot> [--force] \n",argv[0]);
		exit(1);
	}

    if (argc == 4) {
		if( 0 == strcmp("--force", argv[3])) {
			force = 1;
		}
		else {
			printf ("Usage: updatediofw <from_slot> <to_slot> [--force] \n",argv[0]);
			exit(1);		
		}
		
	}

	from_slot = atoi(argv[1]);
	to_slot = atoi(argv[2]);
	
	n=to_slot-from_slot+1; /* Number of lots to update */

	if ((n < 1) || (n > MAX_SLOTS))
	{
		printf ("Slot number should be between 1 and %d.\n",MAX_SLOTS);
		exit(1);
	}

	threads=(pthread_t *)malloc(MAX_SLOTS*sizeof(*threads));
	p=(parm *)malloc(MAX_SLOTS*sizeof(parm));

	/* Start up a thread for each slot */
	for (i = from_slot; i <= to_slot; i++)
	{
		p[i].id=i;
		p[i].force=force;
		pthread_create(&threads[i], NULL, update_dio_fw, (void *)(p+i));
	}

	/* Synchronize the completion of each thread. */
	for (i = from_slot; i <= to_slot; i++)
	{
		pthread_join(threads[i],NULL);
	}
	free(p);
	free(threads);
}


