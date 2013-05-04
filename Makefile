CFLAGS += -g

serial:	serial.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

serial.o:	serial.c
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY:	clean

clean:
	-rm -rf *.o serial
