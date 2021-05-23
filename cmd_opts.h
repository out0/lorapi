#ifndef _CMD_OPTS_H
#define _CMD_OPTS_H

#include <getopt.h>
#include <iostream>

using namespace std;

#define EXEC_MODE_BRIDGE 0
#define EXEC_MODE_RAWTEST 1
#define EXEC_MODE_PINGPONG 2

typedef struct tArgs
{
    int rxFreq;
    int txFreq;
    int sf;
    int bw;
    int cr;
    uint8_t mode;
} tArgs;

tArgs *getCommandOptions(int argc, char **argv);

#endif