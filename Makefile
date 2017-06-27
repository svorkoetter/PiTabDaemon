# Makefile for "pitabd".
# Generated automatically by makemf with the command line:
# makemf -O pitabd
# Do not edit this file!

TARGET = pitabd
CC = gcc
CCFLAGS = -c -I$$HOME/include -O
LD = gcc
LDFLAGS =  -L$$HOME/lib
LIBS = -lm

## Copyright (c) 2017 by Stefan Vorkoetter

## This file is part of PiTabDaemon.

## PiTabDaemon is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## PiTabDaemon is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with PiTabDaemon. If not, see <http://www.gnu.org/licenses/>.

LIBS += -lbcm2835 -lX11 -lXss
CCFLAGS += -std=c99 -O3 -Wall -Wno-parentheses -Wno-char-subscripts

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
	cp ../Sounds/chimes.wav /usr/local/share/pitabd/shutdown.wav