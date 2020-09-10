CC = gcc
CFLAGS = -I.
DEPS = headers.h
OBJ = main.o probe_stuffing.o utils.o nl_callbacks.o
LIBNL = $(shell pkg-config --cflags --libs libnl-genl-3.0 )

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBNL) -pthread
#$@ full name of the current target
#$< name of the first dependency 
all: $(OBJ)
	$(CC) -g -o probe_stuffing $^ $(LIBNL) -pthread

clean:
	rm -f probe_stuffing
	rm -f *.o
