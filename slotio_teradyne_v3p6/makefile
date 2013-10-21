.SUFFIXES:
.SUFFIXES: .cpp .c .o

.PHONY: all clean

LIBNAME = libsi.a
LIB_CPP_SRCS = common.cpp uart2Interface.cpp 
#LIB_CPP_SRCS = common.cpp uart2Interface.cpp uart3Protocol.cpp uart3Interface.cpp scsiCmd.cpp ccbCmd.cpp
LIB_OBJS = ${LIB_CPP_SRCS:.cpp=.o} ${LIB_C_SRCS:.c=.o}
LIBS = -lTeradyneSio
LIBSDIR = -L/usr/teradyne/neptune/lib
INCLUDES = -I/usr/teradyne/neptune/include
CFLAGS = -O -Wall -Werror -gstabs+ -DNEPTUNE_CLIENT_BUILD -DLINUX
CPP=g++

.cpp.o:
		$(CPP) $(CFLAGS) $(INCLUDES) -c $<
		
all: $(LIBNAME) test

$(LIBNAME): $(LIB_OBJS)
		ar -r $@ $(LIB_OBJS)
		
test: $(LIBNAME) wapitest.o uart3test.o
	$(CPP) wapitest.o $(LIBS) $(LIBSDIR) $(LIBNAME) -o wapitest
	$(CPP) uart3test.o $(LIBS) $(LIBSDIR) $(LIBNAME) -o uart3test  

clean:
	rm -Rf $(LIB_OBJS) $(EXE_OBJS) $(LIBNAME)
	rm -f *.o
	rm -f uart3test
#	rm -f wapitest uart3test
