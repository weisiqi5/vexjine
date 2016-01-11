include ../../Makefile.inc

all: Time.o TimeLogger.o VirtualTimeline.o

CFLAGS+=-c -fPIC $(CPPFLAGS)

Time.o: Time.cpp  Time.h
	$(CC) $(CFLAGS) $(CPPFLAGS_TIME) $(INCLUDE_DIRS) $< -o $@

TimeLogger.o: TimeLogger.cpp TimeLogger.h
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@

VirtualTimeline.o: VirtualTimeline.cpp VirtualTimeline.h
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $< -o $@

clean:
	rm -rf *.o
