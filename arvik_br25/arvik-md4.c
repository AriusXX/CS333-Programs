//*****************************************************************************************
//Billy Rodriguez	arvik-md4	CS333		
//
//
//A program to read and write archive files
//*****************************************************************************************

//library calls
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sysmacros.h>
#include <inttypes.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <md4.h>
#include "arvik.h"

#define BUFSIZE 4096

//prototype functions
void validate(const char *filename);
void table_of_contents(const char *filename, int v_flag);
void extract(const char *filename, int v_flag);
void calculate_md4(int file_descriptor, unsigned char out[MD4_DIGEST_LENGTH]);
void create(const char *filename, int v_flag, int argc, char *argv[]);
void help(void);


int main(int argc, char *argv[])
{

	//variables
	int v_flag = 0;			//verbose activated
	int c_flag = 0;			//create activated
	int x_flag = 0;			//extract activated
	int t_flag = 0;			//toc activated
	int V_flag = 0;			//validate activated
	char *filename = NULL;		//file



	//get-opt function
	{
		int opt = 0;
		while((opt = getopt(argc, argv, ARVIK_OPTIONS)) != -1)
		{
			switch(opt)
			{
				//extract members from arvik file
				case 'x':
					x_flag = 1;
					break;

				//create an arvik style archive file
				case 'c':
					c_flag = 1;
					break;

				//table of contents
				case 't':
					t_flag = 1;
					break;

				//specify the name of the arvik file to operate on
				case 'f':
					filename = optarg;
					break;

				//help
				case 'h':
					help();
					break;

				//verbose process
				case 'v':
					v_flag = 1;
					break;

				//validate md4 value for header and data for each archive member
				case 'V':
					V_flag = 1;
					break;

				//default
				default:
					help();
					break;
			}
		}

	}

	//command flags
	if(c_flag)
		create(filename, v_flag, argc, argv);
	if(x_flag)
		extract(filename, v_flag);
	if(t_flag)
		table_of_contents(filename, v_flag);
	if(V_flag)
		validate(filename);

	return EXIT_SUCCESS;

}

//validate
void validate(const char *filename)
{
	int iarch = STDIN_FILENO;                               //file-descriptor
	char tagbuf[64];                                        //tag buffer
	ssize_t nread;                                          //byte reader
	arvik_header_t header;                                  //arvik header struct
	arvik_footer_t footer;                                  //arvik footer struct
	char name[ARVIK_NAME_LEN + 1];                          //member file name
	char *p;                                                //pointer for name term
	long datasz, remain;                                    //data size / remaining bytes
	ssize_t chunk;                                          //chunk size
	char pad, buf[BUFSIZE];                                 //padding and buffer
	MD4_CTX ctx;                                            //md4 context
	unsigned char md4_header_hash[MD4_DIGEST_LENGTH];       //header hash
	unsigned char md4_data_hash[MD4_DIGEST_LENGTH];         //data hash
	char hex_header[MD4_DIGEST_LENGTH * 2];                 //header hex string
	char hex_data[MD4_DIGEST_LENGTH * 2];                   //data hex string
	int j, rc = 0;                                          //loop index / return code flag

	//open file
	if(filename) 
		iarch = open(filename, O_RDONLY);
	if(iarch < 0)
	{ 
		fprintf(stderr,"Cannot open %s\n", filename ? filename: "(stdin)"); 
		exit(EXIT_FAILURE); 
	}

	//validate tag
	nread = read(iarch, tagbuf, strlen(ARVIK_TAG));
	if(nread!=(ssize_t)strlen(ARVIK_TAG) || strncmp(tagbuf, ARVIK_TAG, strlen(ARVIK_TAG)) !=0)
	{
		fprintf(stderr,"Invalid arvik file\n"); 	
		if(filename != NULL)
			close(iarch); 
		exit(EXIT_FAILURE);
	}

	//loop through each member
	while((nread=read(iarch,&header,sizeof(header)))==(ssize_t)sizeof(header))
	{
		int header_ok, data_ok;		


		strncpy(name,header.arvik_name,ARVIK_NAME_LEN); 
		name[ARVIK_NAME_LEN]='\0';
		p = strchr(name,ARVIK_NAME_TERM); 
		if(p)*p='\0';

		MD4Init(&ctx);
		MD4Update(&ctx,(unsigned char*)&header,sizeof(header)); 
		MD4Final(md4_header_hash,&ctx);

		for(j = 0; j < MD4_DIGEST_LENGTH; j++)
			sprintf(&hex_header[j*2],"%02x",md4_header_hash[j]);

		datasz = strtol(header.arvik_size,NULL,10); 
		if(datasz < 0) 
			datasz = 0;

		MD4Init(&ctx); 
		remain = datasz;
		while(remain>0)
		{ 
			chunk = (remain > BUFSIZE) ? BUFSIZE:remain; 
			nread = read(iarch,buf,chunk);
			if(nread <= 0)
			{
				fprintf(stderr,"[V] %s: unexpected EOF in data\n",name); 
				rc = 1;
				break;
			}

			MD4Update(&ctx,(unsigned char*)buf,(unsigned)nread); 
			remain -= nread; 
		}

		MD4Final(md4_data_hash,&ctx);
		for(j = 0; j < MD4_DIGEST_LENGTH; j++) 
			sprintf(&hex_data[j*2],"%02x",md4_data_hash[j]);

		if(remain > 0) 
			break;

		if(datasz % 2 != 0 && read(iarch, &pad, 1) !=1)
		{ 
			fprintf(stderr,"[V] %s: EOF on pad\n", name); 
			rc = 1; 
			break; 
		}
		if(read(iarch, &footer, sizeof(footer)) != (ssize_t)sizeof(footer))
		{ 
			fprintf(stderr,"[V] %s: EOF in footer\n", name); 
			rc = 1; 
			break; 
		}

		header_ok = (memcmp(hex_header, footer.md4sum_header, MD4_DIGEST_LENGTH * 2) ==0);
		data_ok = (memcmp(hex_data,footer.md4sum_data, MD4_DIGEST_LENGTH * 2) == 0);

		printf("[V] %s: header %s, data %s\n", name, header_ok ? "OK": "FAIL", data_ok ? "OK": "FAIL");
		if(!header_ok||!data_ok) 
			rc = 1;
	}

	if(nread < 0)
	{ 
		perror("read header"); 
		rc = 1; 
	}
	close(iarch);
	if(rc) 
		exit(EXIT_FAILURE);
}



//table of contents
void table_of_contents(const char *filename, int v_flag)
{
	//variables
	int iarch = STDIN_FILENO;                	//file descriptor
	char tagbuf[64];                          	//tag buffer
	ssize_t nread;                            	//read count
	arvik_header_t md;                        	//header struct
	arvik_footer_t footer;                    	//footer struct
	char name[ARVIK_NAME_LEN + 1];            	//member name
	char *p;                                  	//temporary pointer
	long datasz;                              	//member file size
	long remain;                              	//bytes remaining
	ssize_t chunk;                            	//chunk size
	char buf[BUFSIZE];                        	//buffer
	char pad;   					//padding byte
	mode_t mode;					//mode
	time_t t;					//time
	char tbuf[64];					//time buff
	struct tm * tm_info;				//struct time

	//open archive
	if(filename != NULL)
		iarch = open(filename, O_RDONLY);

	//error opening file
	if(iarch < 0)
	{
		fprintf(stderr, "Cannot open %s\n", filename ? filename : "(stdin)");
		exit(EXIT_FAILURE);
	}

	//validate tag
	nread = read(iarch, tagbuf, strlen(ARVIK_TAG));

	if(nread != (ssize_t)strlen(ARVIK_TAG) || strncmp(tagbuf, ARVIK_TAG, strlen(ARVIK_TAG)) != 0)
	{
		fprintf(stderr, "Invalid arvik file\n");
		if(filename != NULL)
			close(iarch);
		exit(BAD_TAG);
	}

	//loop through each member file
	while((nread = read(iarch, &md, sizeof(arvik_header_t))) == (ssize_t)sizeof(arvik_header_t))
	{
		//***************READ MEMBER NAME*****************
		strncpy(name, md.arvik_name, ARVIK_NAME_LEN);
		name[ARVIK_NAME_LEN] = '\0';
		p = strchr(name, ARVIK_NAME_TERM);
		if(p) *p = '\0';
		//************************************************

		//***************READ FILE METADATA***************
		datasz = strtol(md.arvik_size, NULL, 10);
		//************************************************


		//***************SKIP FILE DATA*******************
		remain = datasz;
		while(remain > 0)
		{
			chunk = (remain > BUFSIZE) ? BUFSIZE : (ssize_t)remain;
			nread = read(iarch, buf, chunk);
			if(nread <= 0)
			{
				fprintf(stderr, "Unexpected EOF in data for %s\n", name);
				if(filename != NULL)
					close(iarch);
				exit(EXIT_FAILURE);
			}
			remain -= nread;
		}
		//************************************************




		//***************SKIP PADDING BYTE****************
		if((datasz % 2) != 0)
			read(iarch, &pad, 1);
		//************************************************

		//***************READ FOOTER**********************
		read(iarch, &footer, sizeof(arvik_footer_t));
		//************************************************

		//***********PRINT FILE INFO (VERBOSE)************
		if (v_flag) 
		{
			struct passwd *pw;
			struct group  *gr;

			t = strtol(md.arvik_date, NULL, 10);
			tm_info = localtime(&t);
			strftime(tbuf, sizeof(tbuf), "%b %e %H:%M %Y", tm_info);

			mode = (mode_t)strtol(md.arvik_mode, NULL, 8);

			pw = getpwuid((uid_t)strtoul(md.arvik_uid, NULL, 10));
			gr = getgrgid((gid_t)strtoul(md.arvik_gid, NULL, 10));

			printf("file name: %s\n", name);

			//permissions
			printf("    mode:       ");
			printf((mode & 0400) ? "r" : "-");
			printf((mode & 0200) ? "w" : "-");
			printf((mode & 0100) ? "x" : "-");
			printf((mode & 0040) ? "r" : "-");
			printf((mode & 0020) ? "w" : "-");
			printf((mode & 0010) ? "x" : "-");
			printf((mode & 0004) ? "r" : "-");
			printf((mode & 0002) ? "w" : "-");
			printf((mode & 0001) ? "x" : "-");
			printf("\n");

			//uid/gid with fallback
			printf("    uid: %16u  %s\n",
					(unsigned)strtoul(md.arvik_uid, NULL, 10),
					pw ? pw->pw_name : "br25");

			printf("    gid: %16u  %s\n",
					(unsigned)strtoul(md.arvik_gid, NULL, 10),
					gr ? gr->gr_name : "them");

			//size + mtime
			printf("    size: %15ld  bytes\n", (long)datasz);
			printf("    mtime:      %s\n", tbuf);

			//hashes (exactly 32 hex chars for MD4)
			printf("    header md4: %.32s\n", footer.md4sum_header);
			printf("    data md4:   %.32s\n", footer.md4sum_data);
			fflush(stdout);
		} 
		else 
		{
			printf("%s\n", name);
		}
		//************************************************

	}

	//close file
	if(filename != NULL)
		close(iarch);
}


//extract archive file
void extract(const char *filename, int v_flag)
{
	//variables
	int iarch = STDIN_FILENO;
	int ofd = -1;
	ssize_t nread = 0, nw = 0;
	long remain = 0, data_size = 0;
	mode_t mode = 0;
	time_t mtime = 0;
	char name[ARVIK_NAME_LEN + 1], buf[BUFSIZE], junk = 0;
	struct timespec ts[2];
	arvik_header_t md;
	arvik_footer_t footer;
	char date_s[ARVIK_DATE_LEN + 1];
	char mode_s[ARVIK_MODE_LEN + 1];
	char size_s[ARVIK_SIZE_LEN + 1];
	char *p = NULL;
	ssize_t footer_read = 0;

	//open archive
	if (filename)
	{
		iarch = open(filename, O_RDONLY);
		if (iarch < 0)
		{
			fprintf(stderr, "Cannot open %s\n", filename);
			exit(CREATE_FAIL);
		}
	}

	//verbose
	if (v_flag)
	{
		fprintf(stderr, "[x] extracting from: %s\n",
				filename ? filename : "stdin");
	}

	//validate tag
	nread = read(iarch, buf, strlen(ARVIK_TAG));
	if (nread != (ssize_t)strlen(ARVIK_TAG) ||
			strncmp(buf, ARVIK_TAG, strlen(ARVIK_TAG)) != 0)
	{
		fprintf(stderr, "Invalid ARVIK TAG\n");
		if (filename)
			close(iarch);
		exit(BAD_TAG);
	}

	//loop members
	while (1)
	{
		ssize_t header_read = read(iarch, &md, sizeof(md));
		if (header_read == 0)
			break;
		if (header_read < (ssize_t)sizeof(md))
			break;

		//parse name
		memset(name, 0, sizeof(name));
		strncpy(name, md.arvik_name, ARVIK_NAME_LEN);
		if ((p = strchr(name, ARVIK_NAME_TERM)))
			*p = '\0';

		//parse fields
		memcpy(date_s, md.arvik_date, ARVIK_DATE_LEN);
		date_s[ARVIK_DATE_LEN] = '\0';
		memcpy(mode_s, md.arvik_mode, ARVIK_MODE_LEN);
		mode_s[ARVIK_MODE_LEN] = '\0';
		memcpy(size_s, md.arvik_size, ARVIK_SIZE_LEN);
		size_s[ARVIK_SIZE_LEN] = '\0';

		mtime = (time_t)strtol(date_s, NULL, 10);
		mode = (mode_t)strtol(mode_s, NULL, 8);
		data_size = strtol(size_s, NULL, 10);
		if (data_size < 0)
			break;

		//verbose
		if (v_flag)
		{
			fprintf(stderr,
					"[x] member: name=%s size=%ld mode=%o mtime=%ld\n",
					name, data_size, mode, (long)mtime);
		}

		//open output file
		ofd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (ofd < 0)
		{
			continue;
		}

		//copy data only
		remain = data_size;
		while (remain > 0)
		{
			ssize_t chunk = (remain > BUFSIZE) ? BUFSIZE : remain;
			nread = read(iarch, buf, chunk);
			if (nread <= 0)
				break;

			nw = write(ofd, buf, nread);
			if (nw != nread)
			{
				perror("write output");
				close(ofd);
				if (filename)
					close(iarch);
				exit(EXIT_FAILURE);
			}
			remain -= nread;
		}

		//align
		if (data_size % 2 != 0)
		{
			read(iarch, &junk, 1);
		}

		//skip footer safely
		footer_read = read(iarch, &footer, sizeof(footer));
		if (footer_read != (ssize_t)sizeof(footer))
			break;

		//restore perms and time
		fchmod(ofd, mode);
		ts[0].tv_sec  = mtime; ts[0].tv_nsec  = 0;
		ts[1].tv_sec  = mtime; ts[1].tv_nsec  = 0;
		futimens(ofd, ts);

		if (v_flag)
		{
			fprintf(stderr, "[x] extracted %s\n", name);
		}

		close(ofd);
	}

	if (filename)
	{
		close(iarch);
	}

	return;
}





void calculate_md4(int file_descriptor, unsigned char out[MD4_DIGEST_LENGTH])
{
	//variables
	MD4_CTX ctx;
	unsigned char buffer[BUFSIZE];
	ssize_t bytes_read;

	//initialize MD4 context
	MD4Init(&ctx);	

	//read file and update MD4 hash
	while((bytes_read = read(file_descriptor, buffer, BUFSIZE)) > 0)
	{
		MD4Update(&ctx, buffer, bytes_read);
	}

	//finalize MD4 calculation
	MD4Final(out, &ctx);
}	

//create archive file
void create(const char *filename, int v_flag, int argc, char *argv[])
{

	//variables
	arvik_header_t md;								//struct variable
	arvik_footer_t footer;								//struct variable
	ssize_t tag_length = strlen(ARVIK_TAG);						//tag length
	struct stat sb;									//stat structure
	int iarch = STDOUT_FILENO;							//file-descriptor
	int member_iarch = 0;								//member file-descriptor
	char buf[BUFSIZE] = {'\0'};							//buffer
	ssize_t bytes_read = 0;								//byte reader
	unsigned char md4_header_hash[MD4_DIGEST_LENGTH];				//header hash
	unsigned char md4_data_hash[MD4_DIGEST_LENGTH];					//header hash
	char tmp_date[32], tmp_uid[16], tmp_gid[16], tmp_mode[16], tmp_size[32]; 	//temporaries
	size_t nlen;									//name length
	char pad = '\n';								//padding
	MD4_CTX ctx;									//ctx

	//****************************************************************************************

	//opening file processs
	if(filename != NULL)
	{
		//open file
		iarch = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);

		//verbose
		if(v_flag)
		{
			if(filename)
				fprintf(stderr, "[c] creating archive file: %s\n", filename);
			else
				fprintf(stderr, "[c] creating archive on stdout\n");
		}

		//error opening file
		if(iarch < 0)
		{
			fprintf(stderr, "Cannot create %s", filename);
			exit(CREATE_FAIL);
		}
	}

	//*************writing arvik tag********************
	if(write(iarch, ARVIK_TAG, tag_length) != tag_length)
	{
		fprintf(stderr, "Invalid tag");
		if(filename != NULL)
			close(iarch);
		exit(BAD_TAG);
	}

	//verbose
	if(v_flag)
		fprintf(stderr, "[c] wrote archive tag\n");
	//**************************************************

	//no member files
	if(optind == argc)
	{
		if(filename != NULL)
			close(iarch);
		return;
	}

	//                       MEMBER FILE PROCCESS
	//**************************************************************************************
	//process through each member file
	for(int i = optind; i < argc; i++)
	{
		//open member file
		member_iarch = open(argv[i], O_RDONLY);

		//check if member file opened correctly
		if(member_iarch < 0)
		{
			fprintf(stderr, "Cannot open %s", argv[i]);
			continue;
		}

		//verbose
		if(v_flag)
			fprintf(stderr, "[c] Adding %s to archive...\n", argv[i]);

		//fstat initialization
		if(fstat(member_iarch, &sb) == -1)
		{
			perror("fstat");
			exit(EXIT_FAILURE);
		}


		//clear header
		memset(&md, ' ', sizeof(md));

		//**********************WRITING METADATA************************************
		//*****************WRITING NAME************************
		{

			nlen = strlen(argv[i]);

			if (nlen > (size_t)(ARVIK_NAME_LEN - 1)) 
				nlen = ARVIK_NAME_LEN - 1;            /* leave room for '<' */

			memcpy(md.arvik_name, argv[i], nlen);
			md.arvik_name[nlen] = ARVIK_NAME_TERM;
		}
		//****************************************************


		//*****************WRITING DATE***********************
		sprintf(tmp_date, "%ld", (long)sb.st_mtime);
		memcpy(md.arvik_date, tmp_date, strlen(tmp_date));
		//****************************************************

		//*****************WRITING UID************************
		sprintf(tmp_uid, "%u", (unsigned)sb.st_uid);
		memcpy(md.arvik_uid,  tmp_uid,  strlen(tmp_uid));
		//****************************************************

		//*****************WRITING GID************************
		sprintf(tmp_gid, "%u", (unsigned)sb.st_gid);
		memcpy(md.arvik_gid,  tmp_gid,  strlen(tmp_gid));
		//****************************************************

		//*****************WRITING MODE***********************
		sprintf(tmp_mode, "%o", (unsigned)sb.st_mode);
		memcpy(md.arvik_mode, tmp_mode, strlen(tmp_mode));
		//****************************************************

		//*****************WRITING FILE SIZE******************
		sprintf(tmp_size, "%lu", (unsigned long)sb.st_size);
		memcpy(md.arvik_size, tmp_size, strlen(tmp_size));
		//****************************************************

		//*****************WRITING TERMINATOR*****************
		memcpy(md.arvik_term, ARVIK_TERM, ARVIK_TERM_LEN);
		//****************************************************

		memset(&footer, 0, sizeof(footer));

		//MD4 header process
		{
			MD4Init(&ctx);
			//hash raw bytes of header struct
			MD4Update(&ctx, (unsigned char *)&md, sizeof(arvik_header_t));
			MD4Final(md4_header_hash, &ctx);	
		}

		//convert binary to hex and store in footer
		for(int j = 0; j < MD4_DIGEST_LENGTH; j++)
		{
			sprintf(&footer.md4sum_header[j * 2], "%02x", md4_header_hash[j]);
		}


		//*****************WRITING HEADER*********************
		if (write(iarch, &md, sizeof(arvik_header_t)) != (ssize_t)sizeof(arvik_header_t)) 
		{
			perror("write header");
			close(member_iarch);
			if (filename != NULL) 
				close(iarch);
			exit(CREATE_FAIL);
		}
		//***************************************************



		//calculate MD4 checksum for member file's content
		lseek(member_iarch, 0, SEEK_SET);
		calculate_md4(member_iarch, md4_data_hash);


		//store MD4 checksum into file data's footer
		for(int j = 0; j < MD4_DIGEST_LENGTH; j++)
		{
			sprintf(&footer.md4sum_data[j * 2], "%02x", md4_data_hash[j]);
		}	


		//footer.md4sum_header[MD4_DIGEST_LENGTH * 2] = '\0';
		//footer.md4sum_data[MD4_DIGEST_LENGTH * 2] = '\0';	

		//verbose
		if(v_flag)
		{
			fprintf(stderr, "[c] md4 header=%.*s\n", (int)(MD4_DIGEST_LENGTH*2), footer.md4sum_header);
			fprintf(stderr, "[c] md4 data  =%.*s\n", (int)(MD4_DIGEST_LENGTH*2), footer.md4sum_data);
		}

		//store footer terminator
		memcpy(footer.arvik_term, ARVIK_TERM, ARVIK_TERM_LEN);

		//verbose
		if(v_flag)
			fprintf(stderr, "[c] wrote footer for %s\n", argv[i]);

		//rewind and store file's raw bytes into archive
		lseek(member_iarch, 0, SEEK_SET);
		while((bytes_read = read(member_iarch, buf, BUFSIZE)) > 0)
		{
			if(write(iarch, buf, bytes_read) != bytes_read)
			{
				perror("write member data");
				close(member_iarch);
				if(filename != NULL)
					close(iarch);
				exit(CREATE_FAIL);
			}
		}

		//2-byte alignment if data size is odd
		if(sb.st_size % 2 != 0)
		{
			if(write(iarch, &pad, 1) != 1)
			{
				perror("error pad");
				close(member_iarch);
				if(filename != NULL)
					close(iarch);	
				exit(EXIT_FAILURE);
			}
		}
		//write footer struct into archive
		if(write(iarch, &footer, sizeof(arvik_footer_t)) != sizeof(arvik_footer_t))
		{
			perror("write footer");
			close(member_iarch);
			if(filename != NULL)
				close(iarch);
			exit(CREATE_FAIL);
		}


		close(member_iarch);

	}

	//*************************************************************************************

	//closing file process
	if(filename != NULL)
		close(iarch);
}

//help text
void help(void)
{
	printf("Usage: arvik-md4 -[cxtvVf:h] archive-file file...\n");
	printf("        -c           create a new archive file\n");
	printf("        -x           extract members from an existing archive file\n");
	printf("        -t           show the table of contents of archive file\n");
	printf("        -f filename  name of archive file to use\n");
	printf("        -V           Validate the md4 values for the header and data\n");
	printf("        -v           verbose output\n");
	printf("        -h           show help text\n");
}

