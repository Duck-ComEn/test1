##-------------------
include ../testcase/makefile.testcase

##-------------------
APP_NAME = libtc.a
APP_OBJS = tcMain.o tcFilter.o tcDebug.o
CFLAGS = $(CFLAGS_DEFAULT)
LINK_OBJS = $(APP_OBJS)

##-------------------
$(APP_NAME): $(LINK_OBJS)
	@echo "Linking $@"
	ar -r $@ $(LINK_OBJS)
	@rm -f $(APP_OBJS)
	@ls -l $@

##-------------------
EXE_NAME = testcase
EXE_OBJS = $(LIB_TC) $(LIB_TS) $(LIB_SH) $(LIB_CS) $(LIB_SM) $(LIB_SI) $(LIB_HI) $(LIB_VC)
GCCFLAGS = -lpthread -lstdc++ -g -rdynamic $(LIBS) -L$(LIBPATH)
TAG_FILES = ../hostio/*.[ch] ../slotio/*.[ch]* ../testscript/*.[ch] ../signalhandler/*.[ch] ../chamberscript/*.[ch] ../syncmanager/*.[ch] ../versioncontrol/*.[ch] *.[ch] $(INCLUDE1)/*.h*

##-------------------
$(EXE_NAME): $(EXE_OBJS)
	@echo "Linking $@"
	gcc -o $@ $(EXE_OBJS) $(GCCFLAGS)
	@etags $(TAG_FILES)
	@ls -l $@

##-------------------
clean:
	@rm -f $(EXE_NAME) $(APP_NAME) $(APP_OBJS) 
	@rm -f *~
	@rm -f \#*\
