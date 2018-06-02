/* Name: Yuntao Wang 
 * ID: yuntaow
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_REQUEST 1000
#define NUM_THREAD 50
#define BUFFER_SIZE 5000
/*holds thread info, including the thread number and the connection file descriptor*/
struct thread {			
	int thread_num;			
	int connection;				
};
/*root directory of the service*/
char web_root[100];
/*holds all the threads*/
int thread_list[NUM_THREAD] = { 0 };
/*gets request service*/
void *get(void *arg);
int get_empty_thread();
pthread_mutex_t lock;