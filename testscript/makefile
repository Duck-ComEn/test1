##-------------------
include ../testcase/makefile.testcase

##-------------------
APP_NAME = libts.a
APP_OBJS = tsAbort.o tsCom.o tsEnv.o tsError.o tsHead.o tsHost.o tsIdfy.o tsLib.o tsMain.o tsParse.o tsPoll.o tsRaw.o tsStat.o
CFLAGS = $(CFLAGS_DEFAULT)
LINK_OBJS = $(APP_OBJS) $(LIB_CD)

##-------------------
$(APP_NAME): $(LINK_OBJS)
	@echo "Linking $@"
	ar -r $@ $(LINK_OBJS)
	@rm -f $(APP_OBJS)
	@ls -l $@

##-------------------
clean:
	@rm -f $(APP_NAME) $(APP_OBJS)
	@rm -f *~
	@rm -f \#*\
