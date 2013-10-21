##-------------------
include ../testcase/makefile.testcase

##-------------------
APP_NAME = libhi.a
APP_OBJS = hiMain.o hiPipe.o
CFLAGS = $(CFLAGS_DEFAULT)
GCCFLAGS = 
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
