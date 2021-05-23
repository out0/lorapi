#include <string.h>
#include "cmd_opts.h"


const char *optString = "h?";
/*
     { "rx frequency (in hz)", optional_argument, NULL, 'rx' },
    { "tx frequency (in hz)", optional_argument, NULL, 'tx' },
    { "spread factor", optional_argument, NULL, 'sf' },
    { "bandwidth", optional_argument, NULL, 'bw' },
    { "code rate", optional_argument, NULL, 'cr' },
    { "help", no_argument, NULL, 'h' },
    { NULL, no_argument, NULL, 0 }
    */
const struct option longOpts[] = {
    {"rx", required_argument, NULL, 0},
    {"tx", required_argument, NULL, 0},
    {"sf", required_argument, NULL, 0},
    {"bw", required_argument, NULL, 0},
    {"cr", required_argument, NULL, 0},
    {"rawtest", no_argument, NULL, 0},
    {"pingpong", no_argument, NULL, 0},
    {"help", no_argument, NULL, 'h'},
    {NULL, no_argument, NULL, 0}};

void displayUsage(char *p)
{
    cout << p << " [--rx|tx|sf|bw|cr] [--rawtest]" << endl;
    cout << "--rx\t\tSets Rx Frequency for Node's uplink window" << endl;
    cout << "--tx\t\tSets Tx Frequency for Node's downlink windows 1 and 2" << endl;
    cout << "--sf\t\tSets the Spread Factor" << endl;
    cout << "--bw\t\tSets the Bandwidth" << endl;
    cout << "--cr\t\tSets the Code rate" << endl;
    cout << "--rawtest\t\tRaw test mode: sends back via LoRa whatever it receives (ignoring LoraWAN processing)" << endl;
    cout << "--pingpong\t\tPing-pong test mode: sends back via LoRaWAN whatever payload it receives" << endl;
    exit(1);
}

tArgs *getCommandOptions(int argc, char **argv)
{
    int longIndex = 0, opt = 0;
    tArgs *res = new tArgs();
    res->rxFreq = -1;
    res->txFreq = -1;
    res->sf = -1;
    res->bw = -1;
    res->cr = -1;
    res->mode = EXEC_MODE_BRIDGE;

    while ((opt = getopt_long(argc, argv, optString, longOpts, &longIndex)) != -1)
    {
        switch (opt)
        {
        case 'h':
        case '?':
            displayUsage(argv[0]);
            break;
        case 0:
            if(strcmp( "rx", longOpts[longIndex].name ) == 0 ) 
                    res->rxFreq = atoi(optarg);
            else if(strcmp( "tx", longOpts[longIndex].name ) == 0 ) 
                    res->txFreq = atoi(optarg);
            else if(strcmp( "sf", longOpts[longIndex].name ) == 0 ) 
                    res->sf = atoi(optarg);
            else if(strcmp( "bw", longOpts[longIndex].name ) == 0 ) 
                    res->bw = atoi(optarg);
            else if(strcmp( "cr", longOpts[longIndex].name ) == 0 ) 
                    res->cr = atoi(optarg);
            else if(strcmp( "rawtest", longOpts[longIndex].name ) == 0 ) 
                    res->mode = EXEC_MODE_RAWTEST;
            else if(strcmp( "pingpong", longOpts[longIndex].name ) == 0 ) 
                    res->mode = EXEC_MODE_PINGPONG;
            break;
        default:
            break;
        }
    }
    return res;
}
