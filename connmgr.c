/****************************************************************************************************
** FILENAME: connmgr.c                                                                             **
** Author: Konjit Sileshi                                                                          **
** Implementation of connection manager that listens for incoming sensor data from sensor nodes and**
** writes all processed data to a file called ‘sensor data_recv’ using a classical Multiplexing I/O** 
** solution, poll().                                                                               **
**                                                                                                 **
** Reference: http://cboard.cprogramming.com/c-programming/158125-sockets-using-poll.html          **  
** Help:Compile- gcc -Wall -std=c11 -Werror -D TIMEOUT=5 main.c connmgr.c -o server -lsock -llist  **
**     :Compile- gcc -Wall -std=c11 -Werror sensor_node.c -o clinet -lsock                         **
** Run : First run the server ./server 1234 the ./clinet 101 60 127.0.0.1 1234                     **
** Where -lsock -llist are shared libraries needed to compile the files.                           **
****************************************************************************************************/


#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h> 
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "tcpsock.h"
#include "dplist.h"
#include "connmgr.h"
#include "config.h"

#define POLL_SIZE 8
#define SENSOR_DATA "sensor_data_recv"

#if defined (TIMEOUT )
  #define SET_TIMEOUT TIMEOUT 
#else
  #error "undefined acceptable sensor timeout"
#endif


typedef struct sensor_node_packet{
  int sensor_sockfd;
  sensor_ts_t last_time_stamp;
  tcpsock_t * sensor_tcp_socket;
}sensor_node_packet_t;

void * element_copy( void * element );
void element_free( void ** element );
int element_compare(void * first_element, void * second_element);
void remove_inactive_peer();

int dplist_errno;
static dplist_t * conn_list = NULL;
struct pollfd poll_list[POLL_SIZE];

void connmgr_listen( int port_number )
{
  tcpsock_t * server, * client;
  int  server_sockfd, check_flag = 0;
  int numfds = 0; 
  struct pollfd poll_list[POLL_SIZE]; 

  conn_list = dpl_create( &element_copy, &element_free, &element_compare );
  assert(dplist_errno == DPLIST_NO_ERROR);
 
  // opening file to write into it
  FILE * fp_sensor_data;
  fp_sensor_data = fopen( SENSOR_DATA, "w" );
  
  // open server socket using bind() and listen() syscall
  assert(tcp_passive_open( &server, port_number ) == TCP_NO_ERROR);
  assert(tcp_get_sd( server, &server_sockfd ) == TCP_NO_ERROR);
  
  // Adding the server fd to poll list
  poll_list[0].fd = server_sockfd;
  poll_list[0].events = POLLIN;
  numfds++;
  printf("Waiting for clients...\n");
  // keep polling and reading until all read file discriptors are closed
  do
  {
    // terminating connections which do not show any activities in time TIMEOUT.
    remove_inactive_peer();
    
    // poll will block until some read fds is ready for reading   
    int retval = poll(poll_list, numfds, 5000);
	
	//time out and no other fds were ready
    if( retval == 0 )
    {
      printf("poll() timed out!\n"); 
      check_flag = 1;
    }
    if( retval > 0 )
    {
      for( int fd_index = 0; fd_index < numfds; fd_index++ )
      {
        if(poll_list[0].revents & POLLIN)
	    {  
          if(poll_list[fd_index].fd == server_sockfd) 
	      {
            int client_fd;
            sensor_ts_t now;
            assert(tcp_wait_for_connection( server, &client )== TCP_NO_ERROR);
	        assert(tcp_get_sd(client , &client_fd)== TCP_NO_ERROR);
	     
	        sensor_node_packet_t * poll_data = ( sensor_node_packet_t* )malloc(sizeof(sensor_node_packet_t));
	        assert( poll_data != NULL );
	     
	        poll_data->sensor_tcp_socket = client;
	        poll_data->sensor_sockfd = client_fd;
	        poll_data->last_time_stamp = time(&now);;
		 
   	        dpl_insert_at_index(conn_list, (void*)poll_data, dpl_size(conn_list), true);
			assert(dplist_errno == DPLIST_NO_ERROR);
	        poll_list[numfds].fd = client_fd;
	        poll_list[numfds].events = POLLIN;
			printf("Adding client on file discriptor: %d\n", client_fd);
	        FREE(poll_data);
	      }
		}
      }
	  sensor_data_t data;
      int bytes;
	  for( int l = 0;  l < dpl_size(conn_list); l++ )
      { 
        sensor_node_packet_t * poll_node = (sensor_node_packet_t*)dpl_get_element_at_index(conn_list, l);
		tcpsock_t * client_request = poll_node->sensor_tcp_socket;
        if(poll_list[l].events & POLLIN) 
		{
		  bytes = sizeof(data.id);
		  int result_id = tcp_receive( client_request,(void *)&data.id, &bytes);
		  bytes = sizeof(data.value);
		  int result_value = tcp_receive( client_request,(void *)&data.value, &bytes);
	      bytes = sizeof(data.ts);
		  int result_ts = tcp_receive( client_request, (void *)&data.ts, &bytes);
		  if ((result_id == TCP_NO_ERROR) &&(result_value == TCP_NO_ERROR)&&(result_ts == TCP_NO_ERROR) && bytes ) 
		  {
			//Writing to binary file
			fwrite(&(data.id), sizeof(data.id),1, fp_sensor_data); 
			fwrite(&(data.value),sizeof(data.value),1, fp_sensor_data); 
			fwrite(&(data.ts),sizeof((long int)data.ts),1, fp_sensor_data); 
			sensor_ts_t new_timestamp = time(&new_timestamp);
			poll_node->last_time_stamp = new_timestamp;
		  }
		  else 
		  {
    	    if ((result_id == TCP_CONNECTION_CLOSED) &&(result_value == TCP_CONNECTION_CLOSED)&&(result_ts == TCP_CONNECTION_CLOSED)) 
			{ 
		      printf("Peer has closed connection\n");
			}	
			else 
			{
			  printf("Error occured on connection to peer\n");
			}
			assert(tcp_close(&client_request)==TCP_NO_ERROR);
			close(poll_list[l].fd);
            numfds--;
            poll_list[l].events = 0;
            printf("Removing client on fd %d\n", poll_list[l].fd);
            poll_list[l].fd = -1;        
			int index = dpl_get_index_of_element( conn_list, poll_node );
			assert(dplist_errno == DPLIST_NO_ERROR);
		    dpl_remove_at_index( conn_list, index, true );
			assert(dplist_errno == DPLIST_NO_ERROR);
		  }
		}
	  }
    }
  }while(check_flag == 0);
  
  fclose(fp_sensor_data);
  assert(tcp_close(&server)==TCP_NO_ERROR);
}

void remove_inactive_peer()
{ 
  sensor_ts_t now_time;
  for( int j = 0; j < dpl_size(conn_list); j++ )
  {
	sensor_node_packet_t * packet_data = (sensor_node_packet_t*)dpl_get_element_at_index( conn_list, j );
	assert(dplist_errno == DPLIST_NO_ERROR);
	sensor_ts_t last_time_seen = packet_data -> last_time_stamp;
	
	time(&now_time);
	int difftime = now_time - last_time_seen;
	if( difftime >= TIMEOUT )
	{
	  tcpsock_t * client_inactive = packet_data->sensor_tcp_socket;
	  assert(tcp_close( &client_inactive )==TCP_NO_ERROR);
	  dpl_remove_at_index( conn_list, j, true );
	  assert(dplist_errno == DPLIST_NO_ERROR);
	  poll_list[packet_data->sensor_sockfd].events = 0;
	}
  }
}

void connmgr_free(){
  dpl_free(&conn_list);
}

void element_free(void**element)
{
  sensor_node_packet_t* sock_packet = (sensor_node_packet_t*)(*element);
  FREE(sock_packet->sensor_tcp_socket);
  FREE(*element);
  
}

void *element_copy(void* element)
{
  sensor_node_packet_t* sock_packet = (sensor_node_packet_t*) malloc(sizeof(sensor_node_packet_t));
  assert(sock_packet != NULL);
  sock_packet->last_time_stamp = ((sensor_node_packet_t *)element)->last_time_stamp;
  sock_packet->sensor_tcp_socket = ((sensor_node_packet_t *)element)->sensor_tcp_socket;
  sock_packet->sensor_sockfd = ((sensor_node_packet_t *)element)->sensor_sockfd;
  return (void*)sock_packet;
}



int element_compare(void* first_element, void* second_element)
{
if(((struct sensor_node_packet*)first_element)->sensor_sockfd == ((struct sensor_node_packet*)second_element)->sensor_sockfd ){
	  return 0;
	}
	  return 1;
}

