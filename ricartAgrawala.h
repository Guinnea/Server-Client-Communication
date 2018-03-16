#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <limits.h>

#include "globals.h"

#define MAX_SHARED_MEMORY_SEGMENTS 16

struct server_address
{
    char host_name [MAX_LINE_SIZE];
    int port_no;
};

int segment_id;
key_t segment_key;
int segment_permissions = SHM_R | SHM_W | IPC_CREAT | IPC_EXCL;
size_t segment_size;

pthread_t *tids;

void *server_thread (void *args);
void *client_thread (void *args);

sem_t *sem;
int *my_server_ready = 0;
int *my_ticket_no = 0;
int *my_highest_ticket_no = 0;
int *my_replies = 0;
int *my_request = 0;
char *my_deferred_table_address;
struct server_address *my_deferred_table;
int *my_deferred_count = 0;

void allocate_shared_memory (int segment_ids [], int *segment_counter);
void generate_ftok_parameter (char parameter []);
void generate_ftok_file_name (char file_name [], char parameter []);
void initialize_semaphore ();
void create_client_and_server_threads ();
void wait_for_threads_to_finish ();
void destroy_semaphore ();
void deallocate_shared_memory (int segment_ids [], int *segment_counter);
