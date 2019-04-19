TARGET = pitabd
CC = gcc
CCFLAGS = -c -I$$HOME/include -std=c99 -O3 -Wall -Wno-parentheses -Wno-char-subscripts
LD = gcc
LDFLAGS =
LIBS = -lm -lbcm2835 -lX11 -lXss

$(TARGET): battery.o display.o idle.o io.o logging.o main.o
	$(LD) $(LDFLAGS) -o $(TARGET) *.o $(LIBS)
	strip $(TARGET)

battery.o: battery.c battery.h
	$(CC) $(CCFLAGS) battery.c

display.o: display.c display.h
	$(CC) $(CCFLAGS) display.c

idle.o: idle.c
	$(CC) $(CCFLAGS) idle.c

io.o: io.c io.h
	$(CC) $(CCFLAGS) io.c

logging.o: logging.c logging.h
	$(CC) $(CCFLAGS) logging.c

main.o: main.c battery.h display.h idle.h io.h logging.h
	$(CC) $(CCFLAGS) main.c

clean:
	rm -f battery.o
	rm -f display.o
	rm -f idle.o
	rm -f io.o
	rm -f logging.o
	rm -f main.o

install: $(TARGET)
	cp $(TARGET) /usr/local/sbin
	mkdir -p /usr/local/share/pitabd
