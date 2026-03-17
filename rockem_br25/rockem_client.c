//*****************************************************************************************
//Billy Rodriguez	rockem_client		CS333		
//
//
//A program to allow access to an open server with its matching port and ip address. If
//the connection is successful, the commands given will execute (get, put, dir) and if
//stated flag -u, it will add 1000 microseconds to the duration the client sleeps after
//each read/write operation on file data (put or get).
//*****************************************************************************************

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>				//for open permissions

#include "rockem_hdr.h"

//global variables
static short is_verbose = 0;
static int   usleep_time = 0;

static char  ip_addr[50] = {DEFAULT_IP};
static short ip_port     = DEFAULT_SERVER_PORT;

//struct to pass arguments to the client transfer thread
typedef struct
{
	char  ip_addr[50];
	int   port;
	cmd_t cmd_struct;
} client_thread_args_t;

//prototypes
int   get_socket(const char *addr, int port);
void  transfer_file_data(int sockfd, cmd_t *cmd, const char *local_filename);
void *thread_transfer_worker(void *info);

//*****************************************************************************************
//thread_transfer_worker
//Handles the entire lifecycle for a single file transfer (GET/PUT) or DIR command.
//*****************************************************************************************
void *thread_transfer_worker(void *info)
{
	client_thread_args_t *args = (client_thread_args_t *) info;
	int                   sockfd = -1;

	//create socket and connect to server
	sockfd = get_socket(args->ip_addr, args->port);
	if (sockfd < 0)
	{
		fprintf(stderr, "Transfer worker failed to connect.\n");
		goto cleanup;
	}

	//write the command struct to the server
	if (write(sockfd, &args->cmd_struct, sizeof(cmd_t)) == -1)
	{
		perror("client write command failed");
		goto cleanup;
	}

	if (usleep_time > 0)
	{
		usleep(usleep_time);
	}

	//handle data transfer based on the command
	if (strcmp(args->cmd_struct.cmd, CMD_GET) == 0)
	{
		//client receives file, write to local file
		transfer_file_data(sockfd, &args->cmd_struct, args->cmd_struct.name);
	}
	else if (strcmp(args->cmd_struct.cmd, CMD_PUT) == 0)
	{
		//client sends file, read local file and write to socket
		transfer_file_data(sockfd, &args->cmd_struct, args->cmd_struct.name);
	}
	else if (strcmp(args->cmd_struct.cmd, CMD_DIR) == 0)
	{
		//client receives directory listing, write to stdout
		transfer_file_data(sockfd, &args->cmd_struct, NULL);
	}

cleanup:
	if (sockfd >= 0)
	{
		close(sockfd);
	}

	free(args);				//free heap allocated arguments
	pthread_exit(NULL);
}

//*****************************************************************************************
//transfer_file_data
//Handles both PUT (read local file, write to socket) and GET/DIR (read socket, write to
//file/stdout).
//*****************************************************************************************
void transfer_file_data(int sockfd, cmd_t *cmd, const char *local_filename)
{
	char    buffer[MAXLINE];
	ssize_t n_rw;
	int     source_fd;
	int     target_fd;

	//PUT: source is local file, target is socket
	if (strcmp(cmd->cmd, CMD_PUT) == 0)
	{
		source_fd = open(local_filename, O_RDONLY);
		target_fd = sockfd;

		if (source_fd < 0)
		{
			perror("client open file for PUT failed");
			return;
		}

		//loop reading from local file, writing to socket
		while ((n_rw = read(source_fd, buffer, MAXLINE)) > 0)
		{
			if (usleep_time > 0)
			{
				usleep(usleep_time);
			}

			if (write(target_fd, buffer, n_rw) != n_rw)
			{
				perror("client write to socket failed (PUT)");
				break;
			}
		}

		if (n_rw == -1)
		{
			perror("client read from file error");
		}

		if (source_fd >= 0)
		{
			close(source_fd);
		}
	}
	//GET or DIR: source is socket, target is local file or stdout
	else
	{
		source_fd = sockfd;

		if (local_filename == NULL || strcmp(cmd->cmd, CMD_DIR) == 0)
		{
			//DIR or unnamed output, write to stdout
			target_fd = STDOUT_FILENO;
		}
		else
		{
			//GET: open local file for write
			target_fd = open(local_filename,
					O_WRONLY | O_CREAT | O_TRUNC,
					S_IRUSR | S_IWUSR);

			if (target_fd == -1)
			{
				perror("client open local file failed (GET/DIR)");
				return;
			}
		}

		//loop reading from socket, writing to target
		while ((n_rw = read(source_fd, buffer, MAXLINE)) > 0)
		{
			if (usleep_time > 0)
			{
				usleep(usleep_time);
			}

			if (write(target_fd, buffer, n_rw) != n_rw)
			{
				perror("client write to target failed (GET/DIR)");
				break;
			}
		}

		if (n_rw == -1)
		{
			perror("client read from socket error");
		}

		if (target_fd != STDOUT_FILENO)
		{
			close(target_fd);
		}
	}
}

//*****************************************************************************************
//get_socket
//Configure and create a new socket and connect to the server.
//*****************************************************************************************
int get_socket(const char *addr, int port)
{
	int                 sockfd;
	struct sockaddr_in  servaddr;

	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("socket creation failed");
		return -1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(port);		//convert port to network byte order

	//convert IP string to network address
	if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <= 0)
	{
		fprintf(stderr, "Invalid address: %s\n", addr);
		close(sockfd);
		return -1;
	}

	//connect to server
	if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) != 0)
	{
		perror("connection failed");
		close(sockfd);
		return -1;
	}

	if (is_verbose)
	{
		fprintf(stderr, "Client: Connected to %s:%d\n", addr, port);
	}

	return sockfd;
}

//*****************************************************************************************
//main
//Parses command line options and spawns threads for GET/PUT or runs DIR.
//*****************************************************************************************
int main(int argc, char *argv[])
{
	char   cmd_input[MAXLINE] = {'\0'};
	char  *cmd_str_ptr        = NULL;
	char  *primary_command    = NULL;
	char  *file_list[argc];			//max possible files is argc
	int    file_count         = 0;
	client_thread_args_t *args;
	pthread_t            tid;

	//parse options
	int opt = 0;
	while ((opt = getopt(argc, argv, CLIENT_OPTIONS)) != -1)
	{
		switch (opt)
		{
			case 'i':
				//IPv4 address of the server
				strncpy(ip_addr, optarg, sizeof(ip_addr) - 1);
				ip_addr[sizeof(ip_addr) - 1] = '\0';
				break;

			case 'p':
				//port on which the server will listen
				ip_port = (short) atoi(optarg);
				break;

			case 'c':
				//copy command string and save pointer for strtok
				strncpy(cmd_input, optarg, MAXLINE - 1);
				cmd_input[MAXLINE - 1] = '\0';
				cmd_str_ptr = cmd_input;
				break;

			case 'v':
				//enable verbose output
				is_verbose++;
				break;

			case 'u':
				//add 1000 microseconds to usleep_time
				usleep_time += USLEEP_INCREMENT;
				break;

			case 'h':
				fprintf(stderr, "Usage: %s ...\n\tOptions: %s\n",
						argv[0], CLIENT_OPTIONS);
				fprintf(stderr, "\t-i str\t\tIPv4 address of the server (default %s)\n",
						ip_addr);
				fprintf(stderr, "\t-p #\t\tport on which the server will listen (default %hd)\n",
						DEFAULT_SERVER_PORT);
				fprintf(stderr, "\t-c str\t\tcommand to run (one of %s, %s, or %s)\n",
						CMD_GET, CMD_PUT, CMD_DIR);
				fprintf(stderr, "\t-u\t\tnumber of thousands of microseconds the client will "
						"sleep between read/write calls (default %d)\n",
						0);
				fprintf(stderr, "\t-v\t\tenable verbose output. Can occur more than once\n");
				fprintf(stderr, "\t-h\t\tshow this help message\n");
				exit(EXIT_SUCCESS);
				break;

			default:
				fprintf(stderr, "*** Invalid option ***\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	//require -c command
	if (cmd_str_ptr == NULL)
	{
		fprintf(stderr, "ERROR: -c command argument required.\n");
		exit(EXIT_FAILURE);
	}

	//tokenize command string (first token is the command)
	primary_command = strtok(cmd_str_ptr, " ");
	if (primary_command == NULL)
	{
		fprintf(stderr, "ERROR: Invalid command specified.\n");
		exit(EXIT_FAILURE);
	}

	//remaining argv arguments are filenames
	for (int i = optind; i < argc; i++)
	{
		file_list[file_count++] = argv[i];
	}

	//DIR uses a single connection/thread
	if (strcmp(primary_command, CMD_DIR) == 0)
	{
		if (file_count > 0)
		{
			fprintf(stderr,
					"WARNING: files specified with DIR command are ignored.\n");
		}

		args = malloc(sizeof(client_thread_args_t));
		if (args == NULL)
		{
			perror("malloc failed");
			exit(EXIT_FAILURE);
		}

		strncpy(args->ip_addr, ip_addr, sizeof(args->ip_addr) - 1);
		args->ip_addr[sizeof(args->ip_addr) - 1] = '\0';

		args->port = ip_port;

		strncpy(args->cmd_struct.cmd, CMD_DIR, CMD_LEN);
		args->cmd_struct.cmd[CMD_LEN - 1] = '\0';

		memset(args->cmd_struct.name, 0, NAME_LEN);

		//single DIR thread
		if (pthread_create(&tid, NULL, thread_transfer_worker, args) != 0)
		{
			perror("pthread_create failed for DIR");
			free(args);
			exit(EXIT_FAILURE);
		}

		pthread_join(tid, NULL);
	}
	//GET or PUT with file arguments: concurrent connections
	else if ((strcmp(primary_command, CMD_GET) == 0 ||
				strcmp(primary_command, CMD_PUT) == 0) &&
			file_count > 0)
	{
		pthread_t threads[file_count];

		for (int i = 0; i < file_count; i++)
		{
			args = malloc(sizeof(client_thread_args_t));
			if (args == NULL)
			{
				perror("malloc failed");
				exit(EXIT_FAILURE);
			}

			//fill in thread args
			strncpy(args->ip_addr, ip_addr, sizeof(args->ip_addr) - 1);
			args->ip_addr[sizeof(args->ip_addr) - 1] = '\0';

			args->port = ip_port;

			strncpy(args->cmd_struct.cmd, primary_command, CMD_LEN);
			args->cmd_struct.cmd[CMD_LEN - 1] = '\0';

			strncpy(args->cmd_struct.name, file_list[i], NAME_LEN - 1);
			args->cmd_struct.name[NAME_LEN - 1] = '\0';

			if (is_verbose)
			{
				fprintf(stderr, "Main: spawning thread %d for %s.\n",
						i + 1, file_list[i]);
			}

			if (pthread_create(&threads[i], NULL,
						thread_transfer_worker, args) != 0)
			{
				perror("pthread_create failed");
				free(args);
				exit(EXIT_FAILURE);
			}
		}

		//wait for all threads
		for (int i = 0; i < file_count; i++)
		{
			pthread_join(threads[i], NULL);
		}
	}
	else
	{
		fprintf(stderr, "ERROR: command '%s' requires file arguments.\n",
				primary_command);
		exit(EXIT_FAILURE);
	}

	pthread_exit(NULL);
}

