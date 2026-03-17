//*****************************************************************************************
//Billy Rodriguez	thread_hash		CS333		
//
//
//A program to decrypt hash values with the specified amount of threads you want to use
//*****************************************************************************************

//library calls
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <crypt.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include "thread_hash.h"

//macros
#define BUFSIZE 4096
#define MAX_HASH_FILE 200000
#define MAX_DICT_FILE 5000000
#define MAX_THREADS 24
#define MAX_FAILED 200000

//prototype functions
hash_algorithm_t detect_algorithm(const char *hash);
void cleanup(void);
void *worker(void *arg);
void get_salts(void);
void run_threads(int num_threads);
void remove_newspace(void);
void load_files(const char *hash_file, const char *dict_file);
void help(void);

//structs
typedef struct thread_stats_s
{
	double seconds;
	int count[ALGORITHM_MAX];
	int total;
	int failed;
	int failed_count;
	char *failed_hashes[MAX_FAILED];
}thread_stats_t;


//variables
char dest_hash[MAX_HASH_FILE];					//exact copy of hashs file
char dest_dict[MAX_DICT_FILE];					//exact copy of dictionary file
char *hashes[60000];						//array of pointers to each hash
char *dictionary[300000];					//array of pointers to each dict
char *salts[60000];						//array of pointers to each salt
int hash_count;							//keep track of hash count
int dict_count;							//keep track of dict count
int next_word_index = 0;					//keep track of index (for each thread)
int v_flag = 0;							//verbose mode
int n_flag = 0;							//nice mode
FILE *ofd_c;							//open file descriptor
thread_stats_t thread_stats[MAX_THREADS];			//each individual thread stats
thread_stats_t total_stats;					//total thread stats
static pthread_mutex_t word_lock = PTHREAD_MUTEX_INITIALIZER;	//initialize mutex


int main(int argc, char *argv[])
{

	//variables
	int t_num = 1;
	char *i_file = NULL;
	char *o_file = NULL;
	char *d_file = NULL;

	//get-opt function
	{
		int opt = 0;
		while((opt = getopt(argc, argv, OPTIONS)) != -1)
		{
			switch(opt)
			{
				//specify name of input file (contains hashed password values to crack)
				case 'i':
					i_file = optarg;
					break;

				//specify name of output file (default to stdout)
				case 'o':
					o_file = optarg;
					break;

				//specify name of directory file that contains plain-text words to crack the passwords
				case 'd':
					d_file = optarg;
					break;

				//specify number of threads to use, up to 24 (default to 1 thread)
				case 't':
					t_num = atoi(optarg);
					break;

				//verbose process
				case 'v':
					v_flag = 1;
					break;

				//help
				case 'h':
					help();
					break;

				//apply nice(), 10 allows a lower porability that your process will be scheduled
				case 'n':
					n_flag = 1;
					nice(10);
					break;

				//default
				default:
					help();
					break;
			}
		}
	}

	//open file
	if(o_file)
		ofd_c = fopen(o_file, "w");
	else
		ofd_c = stdout;

	load_files(i_file, d_file);
	remove_newspace();
	get_salts();
	run_threads(t_num);
	cleanup();

	//close file
	if(ofd_c != stdout)
		fclose(ofd_c);

	return EXIT_SUCCESS;
}


//detect algorithym used
hash_algorithm_t detect_algorithm(const char *hash)
{
	if(hash[0] != '$') return DES;
	if(hash[1] == '1') return MD5;
	if(hash[1] == '3') return NT;
	if(hash[1] == '5') return SHA256;
	if(hash[1] == '6') return SHA512;
	if(hash[1] == 'y') return YESCRYPT;
	if(hash[1] == 'g') return GOST_YESCRYPT;
	if(hash[1] == '2') return BCRYPT;

	return ALGORITHM_MAX; 
}

//free anything
void cleanup(void)
{
	for(int i = 0; i < hash_count; i++)
	{
		free(salts[i]);
	}

}

//does everything the program needs to do
void *worker(void *arg)
{
	int tid = *(int *)arg;			//unique thread id
	struct crypt_data data;           	//crypt data struct
	char *crypt_return = NULL;        	//pointer returned by crypt_rn
	int cracked_flag = 0;             	//flag to signal cracked hash
	int hash_index = 0;			//keep track of thread hash
	thread_stats_t *stats;			//keep track of thread stats
	hash_algorithm_t alg;			//access to .h struct
	struct timeval start, end;		//keep track of thread running time
	

	//initialize crypt struct to 0
	memset(&data, 0, sizeof(data));

	stats = &thread_stats[tid];

	//start time
	gettimeofday(&start, NULL);

	//crypt work
	while(1)
	{
		//lock thread
		pthread_mutex_lock(&word_lock);
		hash_index = next_word_index++;
		pthread_mutex_unlock(&word_lock);

		if(hash_index >= hash_count)
			break;

		//verbose
		if(v_flag)
			fprintf(stderr, "thread:  %i cracking  %s\n", tid, hashes[hash_index]);

		//keep track of which ones been cracked
		cracked_flag = 0;

		//track algorithm counts
		alg = detect_algorithm(hashes[hash_index]);
		stats->count[alg]++;

		//crack hashes
		for(int j = 0; j < dict_count; j++)
		{
			crypt_return = crypt_rn(dictionary[j], salts[hash_index], &data, sizeof(data));

			if(strcmp(crypt_return, hashes[hash_index]) == 0)
			{
				fprintf(ofd_c, "cracked  %s  %s\n", dictionary[j], hashes[hash_index]);
				cracked_flag = 1;
				break;
			}
		}

		//failed cracks
		if(!cracked_flag)
		{
			stats->failed++;
			stats->failed_hashes[stats->failed_count] = hashes[hash_index];
			stats->failed_count++;
			fprintf(ofd_c, "*** failed to crack  %s\n", hashes[hash_index]);
		}

		//accumalate total
		stats->total++;
	}

	//end time
	gettimeofday(&end, NULL);

	stats->seconds = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec) / 1000000.0;
	return NULL;
}



//attain salts from each hash
void get_salts(void)
{
	//variables
	int dollar_count = 0;		//$ count
	char des_salt[3];		//buffer for DES
	char *salt;			//store salt
	char *h;			//hash pointer
	int j = 0;			//iterating

	for(int i = 0; i < hash_count; ++i)
	{
		//set pointer to first index
		h = hashes[i];

		//DES condition
		if(h[0] != '$')
		{
			des_salt[0] = h[0];
			des_salt[1] = h[1];
			des_salt[2] = '\0';
			salts[i] = strdup(des_salt);	//store des buf into salts
			continue;
		}

		//reset counters
		dollar_count = 0;
		j = 0;	

		//$ hashing conditions
		while(h[j] != '\0')
		{
			if(h[j] == '$')
			{
				dollar_count++;

				//all cases besides md5 and bcrypt
				if(dollar_count == 4)
				{
					j++;		//include $ in buffer
					break;
				}

				//md5crypt case
				if(h[1] == '1' && dollar_count == 3)
				{
					j++;
					break;
				}

			}

			//bcrypt case
			if(j == 28 && h[1] == '2')
			{
				j++;
				break;
			}	

			j++;
		}

		//transfer salt string into salts
		salt = malloc(j + 1);			//allocate memory for salt storage
		strncpy(salt, h, j);			//copy salt string into salt storage
		salt[j] = '\0';				//set null terminator for salt storage
		salts[i] = salt;			//store salt into salts array
	}	

}

//create threads and joins them
void run_threads(int num_threads)
{
	//variables
	pthread_t threads[num_threads];
	int thread_ids[num_threads];
	pthread_mutex_init(&word_lock, NULL);

	//set thread ids
	for(int i = 0; i < num_threads; i++)
		thread_ids[i] = i;

	//allocate thread stats
	for(int i = 0; i < num_threads; i++)
		memset(&thread_stats[i], 0, sizeof(thread_stats_t));

	//allocate thread total stats
	memset(&thread_stats, 0, sizeof(thread_stats_t));

	//create threads
	for(int i = 0; i < num_threads; i++)
		pthread_create(&threads[i], NULL, worker, &thread_ids[i]);

	//wait for threads to finish (join)
	for(int i = 0; i < num_threads; i++)
		pthread_join(threads[i], NULL);


	//accumalate total thread stats
	for (int i = 0; i < num_threads; i++)
	{
		thread_stats_t *s = &thread_stats[i];

		if(thread_stats[i].seconds > total_stats.seconds)
			total_stats.seconds = s->seconds;

		total_stats.total   += s->total;
		total_stats.failed  += s->failed;

		for (int a = 0; a < ALGORITHM_MAX; a++)
			total_stats.count[a] += s->count[a];

		//Accumulate failed hashes
		for (int h = 0; h < s->failed_count; h++) 
		{
			total_stats.failed_hashes[ total_stats.failed_count++ ] = s->failed_hashes[h];
		}
	}

	//print stats
	for (int i = 0; i < num_threads; i++) 
	{
		thread_stats_t *s = &thread_stats[i];

		printf("thread: %d  %8.2f sec  ", i, s->seconds);

		//print algorithm counts using algorithm_string[]
		for (int alg = 0; alg < ALGORITHM_MAX; alg++) 
		{
			printf("%s:%-6d ", algorithm_string[alg], s->count[alg]);
		}

		printf(" total:%-6d  failed:%-6d\n", s->total, s->failed);

	}

	//print total stats
	printf("\ntotal: %d  %8.2f sec", num_threads, total_stats.seconds);
	printf("   DES:%-6d NT:%d\t MD5:%-6d SHA256:%-6d SHA512:%-6d YESCRYPT:%-6d GOST:%-6d BCRYPT:%-6d",
			total_stats.count[DES], total_stats.count[NT], total_stats.count[MD5], total_stats.count[SHA256],
			total_stats.count[SHA512], total_stats.count[YESCRYPT], total_stats.count[GOST_YESCRYPT],
			total_stats.count[BCRYPT]);

	printf("  total:%-6d  failed:%-6d\n", total_stats.total, total_stats.failed);


	pthread_mutex_destroy(&word_lock);
}


//split each word in hash and dictionary array
void remove_newspace(void)
{
	//variables
	char *h = dest_hash;
	char *d = dest_dict;
	hash_count = 0;
	dict_count = 0;

	//hashes
	while(*h != '\0')
	{
		hashes[hash_count++] = h;

		while(*h != '\0' && *h != '\n')
			h++;
		if(*h == '\n')
		{
			*h = '\0';
			h++;
		}
	}


	//dictionary
	while(*d != '\0')
	{
		dictionary[dict_count++] = d;

		while(*d != '\0' && *d != '\n')
			d++;
		if(*d == '\n')
		{
			*d = '\0';
			d++;
		}
	}

	//verbose
	if(v_flag)
	{
		fprintf(stderr, "word count: %i\n", hash_count);
		fprintf(stderr, "word count: %i\n", dict_count);
	}

}

//load both the hashed and dictionary files (-i and -d)
void load_files(const char *hash_file, const char *dict_file)
{
	//variables
	int ofd = -1;
	ssize_t n;


	//hash file
	ofd = open(hash_file, O_RDONLY);
	if(ofd < 0)
	{
		fprintf(stderr, "Cannot open hash file");
		exit(EXIT_FAILURE);
	}

	n = read(ofd, dest_hash, MAX_HASH_FILE);
	close(ofd);

	if(n < 0)
	{
		perror("read hash_file");
		exit(EXIT_FAILURE);
	}

	dest_hash[n] = '\0';


	//dictionary file
	ofd = open(dict_file, O_RDONLY);
	if(ofd < 0)
	{
		fprintf(stderr, "Cannot open dictionary file");
		exit(EXIT_FAILURE);
	}

	n = read(ofd, dest_dict, MAX_DICT_FILE);
	close(ofd);

	if(n < 0)
	{
		perror("read hash_file");
		exit(EXIT_FAILURE);
	}

	dest_dict[n] = '\0';
}

//help text
void help(void)
{
	printf("help text\n");
	printf("        ./thread_hash ...\n");
	printf("        Options: i:o:d:hvt:n\n");
	printf("                -i file         hash file name (required)\n");
	printf("                -o file         output file name (default stdout)\n");
	printf("                -d file         dictionary file name (required)\n");
	printf("                -t #            number of threads to create (default == 1)\n");
	printf("                -n              renice to 10\n");
	printf("                -v              enable verbose mode\n");
	printf("                -h              helpful text\n");

}

