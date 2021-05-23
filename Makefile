# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC=g++
CFLAGS=-c -Wall -fpermissive -I./
LIBS = -Lliblorawan.la -L/lib -lwiringPi -lmosquittopp -lpthread

PROG ?= lora_pi
OBJS = aes.o base64.o sx1276.o main.o log.o lw.o lw-log.o cmac.o str2hex.o cmd_opts.o mqtt_bridge.o
#

all: $(PROG)

.cpp.o:
	g++ -Wall -pedantic -ggdb -O2 -c -o $@ $<

$(PROG): $(OBJS)
	g++ -Wall -pedantic -ggdb -O2 -o $@ $(OBJS) $(LIBS)

clean:
	rm *.o lora_pi

