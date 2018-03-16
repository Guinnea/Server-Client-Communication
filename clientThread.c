#include "clientServerThreads.h"

void myReplyClient(char *host_name1, int port_num1, char *reply);
void myRequestClient(char *host_name1, int port_num1, int *my_ticket_num1, char *request);

void *client_thread (void *args)
{
    int socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *hp;
    char host_name [MAX_LINE_SIZE];
    int host_port_no;
    struct server_address server_table [MAX_SERVERS];
    int server_count;
    char send_line [MAX_LINE_SIZE];
    char work_c_string [MAX_LINE_SIZE];
    char caller [MAX_LINE_SIZE];
    int i;

    gethostname (host_name, sizeof (host_name));
    sprintf (caller, "    Client [%s]", host_name);
    printf ("    Client [HOST=%s PID=%d]: ready\n", host_name, getpid ());

    /* Don't start until server is ready */
    while (SERVER_NOT_READY)
    {
        mutex_lock (caller);
        if (*my_server_ready)
        {
            break;
        }
        mutex_unlock (caller);
    }
    /* Don't start until client is ready */
    server_count = get_server_addresses (server_table);
    for (i = 0; i < server_count; i ++)
    {
        if (strcmp (host_name, server_table [i].host_name) == 0)
        {
            host_port_no = server_table [i].port_no;
            break;
        }
    }
	
    mutex_unlock (caller);

    srand (host_port_no);

    while (LOOP_FOREVER)
    {
        printf ("    Client [HOST=%s PID=%d]: ENTERING NON-CRITICAL SECTION\n", host_name, getpid ());
        usleep (2000 * (1 + rand () % 1000));
        printf ("    Client [HOST=%s PID=%d]: LEAVING NON-CRITICAL SECTION\n", host_name, getpid ());

        /* THE REQUEST PART OF YOUR PRE-PROTOCOL CODE GOES HERE! */
		 mutex_lock(caller);
		*my_request = 1;
		*my_highest_ticket_no = *my_highest_ticket_no + 1;
		*my_ticket_no = *my_highest_ticket_no;
		*my_replies = 0;
		*my_deferred_count = 0;
		for (i = 0; i < server_count; i++)
		{
			/*
			send request messages to all other nodes
			*/
			if(server_table[i].port_no != host_port_no)
			{	
				myRequestClient(host_name, host_port_no, my_ticket_no, send_line);
				printf ("    Client [HOST=%s PID=%d]: PREPARING TO SEND THE MESSAGE: [%s] TO [%d]\n", host_name, getpid (), send_line, server_table[i].port_no);
				if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
				{
					printf ("socket ERROR in main");
					exit (1);
				}
					memset (&server_addr, 0, sizeof (server_addr));
					server_addr.sin_family = AF_INET;
					hp = gethostbyname (server_table [i].host_name);
				if (hp == (struct hostent *) NULL)
				{
					printf ("gethostbyname ERROR in main: %s does not exist", server_table [i].host_name);
					exit (1);
				}
					memcpy (&server_addr.sin_addr, hp -> h_addr, hp -> h_length);
					server_addr.sin_port = htons (server_table [i].port_no);
				if (connect (socket_fd, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0)
				{
					printf ("connect ERROR in main");
					exit (1);
				}
				
				
				send_message(socket_fd, send_line, caller);
				printf ("    Client [HOST=%s PID=%d]: SENT MESSAGE [%s] TO [%d]\n", host_name, getpid (), send_line, server_table[i].port_no);
				close (socket_fd);
			}
		}
		
		mutex_unlock(caller);
        printf ("    Client [HOST=%s PID=%d]: WAITING TO ENTER CRITICAL SECTION\n", host_name, getpid ());
        while (AWAIT_REPLIES)
        {

            /* THE AWAIT REPLIES PART OF YOUR PRE-PROTOCOL CODE GOES HERE! */
			
			//printf("    Client [HOST=%s PID=%d]: MY REPLYS: [%d] VS SERVER_COUNT: [%d]\n", host_name, getpid (), *my_replies, server_count); 
			mutex_lock(caller);
			if(*my_replies == server_count - 1)
			{
				*my_request = 0;
				mutex_unlock(caller);
				break;
			}
			mutex_unlock(caller);
			
			
        }
        printf ("    Client [HOST=%s PID=%d]: ENTERING CRITICAL SECTION\n", host_name, getpid ());
        write_to_history_file (host_name, host_port_no, my_ticket_no);
        printf ("    Client [HOST=%s PID=%d]: LEAVING CRITICAL SECTION\n", host_name, getpid ());

        /* THE DEFERRED REPLIES OF YOUR POST-PROTOCOL CODE GOES HERE! */
		mutex_lock(caller);
		printf("\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~DEFERRED SECTION~~~~~~~~~~~~~~~~%d~~~~~~~\n\n", *my_deferred_count);
		for(i = 0; i < *my_deferred_count; i++)
		{		
				myReplyClient(my_deferred_table[i].host_name, my_deferred_table[i].port_no, send_line);
				printf ("    Client [HOST=%s PID=%d]: PREPARING TO SEND THE DEFERRED MESSAGE: [%s] TO [%d]\n", host_name, getpid (), send_line, my_deferred_table[i].port_no);
				if ((socket_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
				{
					printf ("socket ERROR in main");
					exit (1);
				}
					memset (&server_addr, 0, sizeof (server_addr));
					server_addr.sin_family = AF_INET;
					hp = gethostbyname (my_deferred_table [i].host_name);
				if (hp == (struct hostent *) NULL)
				{
					printf ("gethostbyname ERROR in main: %s does not exist", my_deferred_table [i].host_name);
					exit (1);
				}
					memcpy (&server_addr.sin_addr, hp -> h_addr, hp -> h_length);
					server_addr.sin_port = htons (my_deferred_table [i].port_no);
				if (connect (socket_fd, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0)
				{
					printf ("connect ERROR in main");
					exit (1);
				}
				
				
				send_message(socket_fd, send_line, caller);
				printf ("    Client [HOST=%s PID=%d]: SENT DEFERRED MESSAGE [%s] TO [%d]\n", host_name, getpid (), send_line, my_deferred_table[i].port_no);
				close (socket_fd);
		}
		*my_deferred_count = 0;
		mutex_unlock(caller);
		
    }

    exit (0);
}

int get_server_addresses (struct server_address server_table [])
{
    FILE *server_address_file;
    char char_port_no [MAX_LINE_SIZE];
    int int_port_no;
    int server_count = 0;
    int lock;
    struct flock key;

    key.l_type = F_WRLCK;
    key.l_whence = SEEK_SET;
    key.l_start = 0;
    key.l_len = 0;
    key.l_pid = getpid ();
    lock = open ("server_address_file_lock", O_WRONLY);
    fcntl (lock, F_SETLKW, &key);

    server_address_file = fopen ("serverAddressFile", "r");
    if (server_address_file == (FILE *) NULL)
    {
        printf ("    Client: fopen failed for serverAddressFile read");
        exit (1);
    }

    while (fscanf (server_address_file, "%s", server_table [server_count].host_name) == 1)
    {
        fscanf (server_address_file, "%s", char_port_no);
        int_port_no = atoi (char_port_no);
        server_table [server_count].port_no = int_port_no;
        server_count ++;
    }

    fclose (server_address_file);

    key.l_type = F_UNLCK;
    fcntl (lock, F_SETLK, &key);
    close (lock);

    return (server_count);
}

void write_to_history_file (char host_name [], int host_port_no, int *my_ticket_no)
{

    FILE *history_file;

    history_file = fopen ("historyFile", "a");
    if (history_file == (FILE *) NULL)
    {
        printf ("    Client: fopen failed for historyFile append");
        exit (1);
    }

    fprintf (history_file, "%s\n", host_name);
    fprintf (history_file, "%d\n", host_port_no);
    fprintf (history_file, "%d\n\n", *my_ticket_no);

    fclose (history_file);

    return;
}

void myReplyClient(char *host_name1, int port_num1, char *reply)
{
	sprintf(reply, "reply %s %d\0", host_name1, port_num1);  
}

void myRequestClient(char *host_name1, int port_num1, int *my_ticket_num1, char *request)
{
	sprintf(request, "request %s %d %d\0", host_name1, port_num1, *my_ticket_num1);  
}


