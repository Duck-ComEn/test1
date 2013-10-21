################################################################################
# Makefile for the Hitachi Project
# Notes:
# 1. The original Hitachi files (that we're re-using) generate many warning, 
#    mainly for passing unsigned char* to functions expecting char*. 
#    To eliminate the display of these warnings, the Hitachi files are 
#    compiled with -w (supress warnings).
# 2. The Teradyne supplied files (C++) are warning free.  They are compiled   
#    with the -Werror switch to treat warnings as errors.
################################################################################

-include ./makefile.init

RM := rm -rf

# Every subdirectory with source files must be described here
SUBDIRS := \
. \

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
./neptune_hitachi_interface.cpp \
./hitachi_sio2.cpp \

C_SRCS = \


OBJS = \
neptune_hitachi_interface.o \
hitachi_sio2.o \
neptune_hitachi_comm.o \

C_DEPS = \
neptune_hitachi_comm.d \

CPP_DEPS = \
neptune_hitachi_interface.d \
hitachi_sio2.d \

# Each subdirectory must supply rules for building sources it contributes
%.o: ./%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DNEPTUNE_CLIENT_BUILD -O0 -g -g3 -Wall -Werror -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ./%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DNEPTUNE_CLIENT_BUILD -O0 -g -g3 -w -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

USER_OBJS := neptune_sio2.a librpclib.a

LIBS :=




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: librpclib.a neptune_sio2.a wapitest libsi.a


# Tool invocations
wapitest: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -o "wapitest" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Prepare a release
preprel:
	@echo "#define BUILD_DATE \"`date`\"" > build_date.inc
	@echo "#define NEPTUNE_SIO2_LIB_VER " >> build_date.inc
	@echo "#define SIO2_SUPER_APP_VER " >> build_date.inc
	@echo "*****  Manually edit ver.h to update build number  *****"
	
	
	
# Other Targets
clean:
	-$(RM) $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) wapitest libsi.a
	-@echo ' '

doxygen:
	doxygen hitach_interface_doxyfile

.PHONY: all clean dependents
.SECONDARY:

-include ./makefile.targets


##-------------------
APP_NAME = libsi.a
APP_OBJS = neptune_hitachi_interface.o neptune_hitachi_comm.o
CFLAGS = $(CFLAGS_DEFAULT)
LINK_OBJS = $(APP_OBJS) $(LIB_CD)

##-------------------
$(APP_NAME): $(LINK_OBJS) $(USER_OBJS) makefile
	@echo "Linking $@"
	ar -r $@ $(LINK_OBJS) $(USER_OBJS)
	@rm -f $(LINK_OBJS)
	@ls -l $@
