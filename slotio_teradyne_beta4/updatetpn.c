#include <stropts.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define SLEEP_TIME 1

int main(int argc, char *argv[])
{
	char achTelnetString[255+1];
	FILE *fhTelnet = NULL;

	if (argc != 2) {
		printf("Usage: updatetpn <cmd> \n");
		return 1;
	}

	if ( strcmp ( argv[1], "help" ) == 0 ) {
		return 1;
	}

	sprintf(achTelnetString, "telnet 10.101.1.2 100");
	fhTelnet = popen(achTelnetString, "w");
	if(fhTelnet != NULL) {

		sleep(SLEEP_TIME);
		fprintf(fhTelnet, "rocket\n");
		fflush(fhTelnet);

		sleep(SLEEP_TIME);
		fprintf(fhTelnet, "%s\n", argv[1]);
		fflush(fhTelnet);
		
		
		sleep(SLEEP_TIME);
		fprintf(fhTelnet, "q\n");
		fflush(fhTelnet);
		
		sleep(SLEEP_TIME);
		pclose(fhTelnet);
		fhTelnet = NULL;
	}

	return(0);
}
