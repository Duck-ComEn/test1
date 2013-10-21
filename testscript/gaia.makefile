##-------------------
#include ../testcase/makefile_include_testcase
CFLAGS_DEFAULT = -Werror -Wall -Wshadow -Wredundant-decls -g -D_REENTRANT -DUNIT_TEST
INCLUDE1  = 
LIBS    = 
LIBPATH = .

##-------------------
APP_NAME = gaia
APP_OBJS = tsAbort.o tsCom.o tsEnv.o tsError.o tsHead.o tsHost.o tsIdfy.o tsLib.o tsMain.o tsParse.o tsPoll.o tsRaw.o tsStat.o gaia.o
CFLAGS = $(CFLAGS_DEFAULT)
GCCFLAGS = -lpthread -lstdc++ -g $(LIBS) -L$(LIBPATH)
LINK_OBJS = $(APP_OBJS)

##-------------------
$(APP_NAME): $(LINK_OBJS)
	@echo "Linking $@"
	gcc -o $@ $(LINK_OBJS) $(GCCFLAGS)
	@rm -f $(APP_OBJS)
	@ls -l $@

##-------------------
clean:
	@rm -f $(APP_NAME) $(APP_OBJS)
	@rm -f *~
	@rm -f \#*\
