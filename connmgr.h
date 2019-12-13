#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>  
#include "tcpsock.h"
#include "lib/dplist.h"


typedef struct sensor_node_data sensor_node_data_t;


/*
This method holds the core functionality of your connmgr.
It starts listening on the given port and when when a 
sensor node connects it writes the data to a sensor_data_recv file. 
This file must have the same format as the sensor_data file in assignment 6 and 7
*/
void connmgr_listen(int port_number);
/*
This method should be called to clean up the connmgr, 
and to free all used memory. 
After this no new connections will be accepted
*/
void connmgr_free();
