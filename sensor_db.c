/***********************************************************************
** FILENAME: sensor_db.c                                              **
** Author: Konjit Sileshi                                             **
** An implementation that reads sensor id, temp value & timestamp     **
** from a binary file sensor_data and inserts to sqlite db Sensor.db. **
** Resource: http://zetcode.com/db/sqlitec/                           **
***********************************************************************/

#define _GNU_SOURCE         
#include <stdio.h>
#include <assert.h>
#include <sqlite3.h>
#include "config.h"
#include "sensor_db.h"
#include "config.h"

#define FILE_OPEN_ERROR(fp) 								\
		do {												\
			if ( (fp) == NULL )								\
			{												\
				perror("File open failed");					\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)

#define FILE_CLOSE_ERROR(err) 								\
		do {												\
			if ( (err) == -1 )								\
			{												\
				perror("File close failed");				\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)



#define SENSOR_DATA "sensor_data"

typedef int(*callback_t)(void *, int, char **, char **);
int finish_with_error( FILE * fp_sensor_data );
int finish_with_err( DBCONN * dbconn );

DBCONN * init_connection( char clear_up_flag )
{
  DBCONN * dbconn;
  char * error_msg = 0, * sql_smt;

  int rc = sqlite3_open( "Sensor.db", &dbconn );
  
  if( rc != SQLITE_OK )
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(dbconn));
    sqlite3_close(dbconn);
    return NULL;
  }
  
  if( clear_up_flag == '1' && dbconn != NULL )
  {
    sql_smt = "DROP TABLE IF EXISTS SensorData;";
    rc = sqlite3_exec( dbconn, sql_smt, 0, 0, &error_msg ); 
    if (rc != SQLITE_OK ) 
    {
      fprintf( stderr, "SQL error: %s\n", error_msg );
      sqlite3_free(error_msg);        
      sqlite3_close(dbconn);
      return  NULL;
    }
  }
  
  sql_smt="CREATE TABLE SensorData(Id INTEGER PRIMARY KEY , sensor_id INT, sensor_value INT, timestamp INT);";		
  rc = sqlite3_exec( dbconn, sql_smt, 0, 0, &error_msg );
   
  if (rc != SQLITE_OK ) 
  {
    fprintf( stderr, "SQL error: %s\n", error_msg );        
    sqlite3_free(error_msg);        
    sqlite3_close(dbconn);     
    return NULL;
  }
  
  return dbconn;
}

void disconnect( DBCONN * dbconn )
{
  sqlite3_close(dbconn);
}

int insert_sensor( DBCONN * dbconn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts )
{
  char * error_msg = 0, * sql_smt;
  char time_buff[20];
  strftime(time_buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&ts));
  int res = asprintf( &sql_smt, "INSERT INTO SensorData(sensor_id, sensor_value, timestamp) VALUES(%hd, %g, '%s')", id, value, time_buff);
  assert( res != -1 );
  
  int rc = sqlite3_exec( dbconn, sql_smt, 0, 0, &error_msg );
  
  if( rc != SQLITE_OK )
  {
    printf("INSERT INTO SensorData(sensor_id, sensor_value, timestamp) VALUES failed \n");
    fprintf(stderr, "SQL error: %s\n", error_msg); 
    sqlite3_free(error_msg);        
    sqlite3_close(dbconn);
    return -1;
  }
  sqlite3_free(error_msg);
  
  return 0;
}

int insert_sensor_from_file( DBCONN * dbconn, FILE * fp_sensor_data )
{
  sensor_data_t data;
  int result;
  long filesize;
  
  fp_sensor_data = fopen( SENSOR_DATA, "r" );
  FILE_OPEN_ERROR(fp_sensor_data);
  
  result  = fseek( fp_sensor_data, 0, SEEK_END );
  if ( result == -1 ) 
  {
    perror("File fseek failed: ");
    result = fclose(fp_sensor_data);
    FILE_CLOSE_ERROR(result);
    return -1;
  }

  filesize = ftell(fp_sensor_data);
  rewind(fp_sensor_data);
  
  while( ftell(fp_sensor_data) < filesize )
  {	
    result = fread( &(data.id), sizeof(sensor_id_t), 1, fp_sensor_data );
    if(result < 1 ) finish_with_error( fp_sensor_data );
    
    result = fread( &(data.value), sizeof(sensor_value_t), 1, fp_sensor_data );
    if(result < 1 ) finish_with_error( fp_sensor_data );

    result = fread( &(data.ts), sizeof(sensor_ts_t), 1, fp_sensor_data );
    if(result < 1 ) finish_with_error( fp_sensor_data );
	
    insert_sensor( dbconn, data.id, data.value, data.ts );
  }
  
  if ( fclose(fp_sensor_data) == EOF ) 
  {
	perror("File close failed: ");
	return -1;
  }
  
  return 1;
}

// getters functions 
int rc;
char * error_msg = 0;
char * sql_smt;

int find_sensor_all(DBCONN * dbconn, callback_t f)
{
  rc = sqlite3_open("Sensor.db", &dbconn);
  if( rc != SQLITE_OK )
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(dbconn));
    sqlite3_close(dbconn);
    return -1;
  }
  
  sql_smt = "SELECT * FROM SensorData";
  rc=sqlite3_exec(dbconn, sql_smt, f, 0, &error_msg);
  
  if( rc != SQLITE_OK )
  {
    finish_with_err(dbconn);
  }
  sqlite3_close(dbconn);
  
  return 0;
}

int find_sensor_by_value( DBCONN * dbconn, sensor_value_t value, callback_t f )
{
  rc = sqlite3_open("Sensor.db", &dbconn);
  if( rc != SQLITE_OK )
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(dbconn));
    sqlite3_close(dbconn);
    return -1;
  }
 // sql_smt = "SELECT * FROM SensorData WHERE sensor_value = value";
  char * query;
	int size = asprintf(&query, "SELECT * FROM SensorData WHERE sensor_value=%g", value);
	if(size == -1){
		return -1;
	}
  rc = sqlite3_exec(dbconn, query, f, 0, &error_msg);

  if( rc != SQLITE_OK )
  {
	finish_with_err(dbconn);
  }
  sqlite3_close(dbconn);
 
  return 0;
}

int find_sensor_exceed_value( DBCONN * dbconn, sensor_value_t value, callback_t f )
{
  rc = sqlite3_open("Sensor.db", &dbconn);
  if( rc != SQLITE_OK )
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(dbconn));
    sqlite3_close(dbconn);
    return -1;
  }
  sql_smt = "SELECT * FROM SensorData WHERE sensor_value > value";
  rc = sqlite3_exec(dbconn, sql_smt, f, 0, &error_msg);

  if( rc != SQLITE_OK )
  {
	finish_with_err(dbconn);
  }
  sqlite3_close(dbconn);
 
  return 0;
}

int find_sensor_by_timestamp( DBCONN * dbconn, sensor_ts_t ts, callback_t f )
{
  rc = sqlite3_open("Sensor.db", &dbconn);
  if( rc != SQLITE_OK )
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(dbconn));
    sqlite3_close(dbconn);
    return -1;
  }
  sql_smt = "SELECT * FROM SensorData WHERE timestamp = ts";
  rc = sqlite3_exec(dbconn, sql_smt, f, 0, &error_msg);
  
  if( rc != SQLITE_OK )
  {
	finish_with_err(dbconn);
  }
  sqlite3_close(dbconn);
 
  return 0;
}

int find_sensor_after_timestamp( DBCONN * dbconn, sensor_ts_t ts, callback_t f )
{
  rc = sqlite3_open("Sensor.db", &dbconn);
  if( rc != SQLITE_OK )
  {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(dbconn));
    sqlite3_close(dbconn);
    return -1;
  }
  sql_smt = "SELECT * FROM SensorData WHERE timestamp > ts";
  rc = sqlite3_exec(dbconn, sql_smt, f, 0, &error_msg);
  
  if( rc != SQLITE_OK )
  {
	finish_with_err(dbconn);
  }
  sqlite3_close(dbconn);
 
  return 0;
}

int f(void * NotUsed, int argc, char **argv, char **azColName) 
{
  NotUsed = 0;
  for( int i = 0; i < argc; i++ ) 
  {
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

// error handler functions
int finish_with_error( FILE * fp_sensor_data )
{
  perror("File read failed: ");
  int res = fclose(fp_sensor_data);
  FILE_CLOSE_ERROR(res);
  return -1;
}

int finish_with_err( DBCONN * dbconn )
{
  fprintf(stderr, "Failed to select data\n");
  fprintf(stderr, "SQL error: %s\n", error_msg);
  sqlite3_free(error_msg);
  sqlite3_close(dbconn);
  return 1;
}
