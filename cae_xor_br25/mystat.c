//*****************************************************************************************
//Billy Rodriguez	mystat		CS333		
//
//
//A program that allows it to display the inode meta data for each data file given on the
//command line.
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


int main(int argc, char *argv[])
{

	//no given paths
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
		exit(EXIT_FAILURE);
	}


	//traversing through given paths


	for(int i = 1; i < argc; i++)
	{
		//variables
		struct stat sb;			//access to inode values
		struct group *grp_entry;	//access to group info
		struct passwd *pw_entry;	//access to owner info
		char time_buffer_a[100];	//time buffer
		char time_buffer_m[100];	//time buffer
		char time_buffer_c[100];	//time buffer
		struct tm *tm_info_a;		//pointer to the tm_a
		struct tm *tm_info_m;		//pointer to the tm_m
		struct tm *tm_info_c;		//pointer to the tm_c

		char gtime_buffer_a[100];	//time buffer
		char gtime_buffer_m[100];	//time buffer
		char gtime_buffer_c[100];	//time buffer
		struct tm *gm_info_a;		//pointer to the gm_a
		struct tm *gm_info_m;		//pointer to the gm_m
		struct tm *gm_info_c;		//pointer to the gm_c

		//lstat error
		if(lstat(argv[i], &sb) == -1)
		{
			perror("lstat");
			continue;
		}
		
		printf("File: %s\n", argv[i]);

		printf("  File type:		");
		
		//symbolic link handling
		if(S_ISLNK(sb.st_mode))		
		{
			char link_target[1024];
			ssize_t len = readlink(argv[i], link_target, sizeof(link_target) - 1);
			if(len != -1)
			{
				link_target[len] = '\0';	//null terminate string
				if(access(link_target, F_OK) == -1)
				{
					//dangling symbolic link
					printf("	Symbolic link - with dangling destination\n");
				}
				else
				{
					//valid symbolic link
					printf("	Symbolic link -> %s\n", link_target);
				}
			}
					else
					{
						//error reading symbolic link
						printf("	Symbolic link - error reading link\n");
					}
		}

		//other file types handling
		else
		{
			switch(sb.st_mode &S_IFMT)
			{
				case S_IFBLK: printf("	block device\n");	break;
				case S_IFCHR: printf("	character device\n");	break;
				case S_IFDIR: printf("	directory\n");		break;
				case S_IFIFO: printf("	FIFO/pipe\n");		break;
				case S_IFLNK: printf("	symlink\n");		break;
				case S_IFREG: printf("	regular file\n");	break;
				case S_IFSOCK: printf("	socket\n");		break;
				default: printf("	unknown?\n");		break;
			}
		}

		//stat info
		printf("  Device ID number:		%ju\n", (uintmax_t) sb.st_dev);
		printf("  I-node number:		%ju\n", (uintmax_t) sb.st_ino);

		//Special Cases
			switch(sb.st_mode &S_IFMT)
			{
				case S_IFBLK: printf("  Mode:				b");	break;
				case S_IFCHR: printf("  Mode:				c");	break;
				case S_IFDIR: printf("  Mode:				d");	break;
				case S_IFIFO: printf("  Mode:				p");	break;
				case S_IFLNK: printf("  Mode:				l");	break;
				case S_IFSOCK: printf("  Mode:				s");	break;
				case S_IFREG: printf("  Mode:				-");	break;
				default: printf("Mode:				");	break;
			}

		//Readable Strings
		//Owner permissions
		printf((sb.st_mode & S_IRUSR) ? "r" : "-");
		printf((sb.st_mode & S_IWUSR) ? "w" : "-");
		printf((sb.st_mode & S_IXUSR) ? "x" : "-");

		//Group permissions
		printf((sb.st_mode & S_IRGRP) ? "r" : "-");
		printf((sb.st_mode & S_IWGRP) ? "w" : "-");
		printf((sb.st_mode & S_IXGRP) ? "x" : "-");

		//Other permissions
		printf((sb.st_mode & S_IROTH) ? "r" : "-");
		printf((sb.st_mode & S_IWOTH) ? "w" : "-");
		printf((sb.st_mode & S_IXOTH) ? "x" : "-");	

		printf("	(%03jo in octal)\n", (uintmax_t) sb.st_mode & 0777);
		printf("  Link count:			%ju\n", (uintmax_t) sb.st_nlink);
		printf("  Owner Id:			");

		//point to owner id
		pw_entry = getpwuid(sb.st_uid);

		//Readable strings
		//success
		if(pw_entry != NULL)
			printf("%s", pw_entry->pw_name);
		//fail
		else
		{
			fprintf(stderr, "Warning: Could not find owner name for UID %d. Printing UID instead\n", sb.st_uid);
			printf("Owner ID: %d\n", sb.st_uid);
		}

		printf("	(UID = %ju)\n", (uintmax_t) sb.st_uid);
		printf("  Group Id:			");	

		//point to group info
		grp_entry = getgrgid(sb.st_gid);	

		//Readable Strings		
		//success
		if(grp_entry != NULL)
			printf("%s", grp_entry->gr_name);
		//fail
		else
		{
			fprintf(stderr, "Warning: Could not find group name for GID %d. Printing GID instead\n", sb.st_gid);
			printf("Group ID: %d\n", sb.st_gid);
		}

		printf("	(GID = %ju)\n", (uintmax_t) sb.st_gid);
		printf("  Preferred I/O block size:	%ju bytes\n", (uintmax_t) sb.st_blksize);
		printf("  File size:			%ju bytes\n", (uintmax_t) sb.st_size);
		printf("  Blocks allocated:		%ju\n", (uintmax_t) sb.st_blocks);

		//epoch timestamp
		printf("  Last file access:		%ld (seconds since the epoch)\n", sb.st_atime);
		printf("  Last file modification:	%ld (seconds since the epoch)\n", sb.st_mtime);
		printf("  Last status change:		%ld (seconds since the epoch)\n", sb.st_ctime);


		//time buffers (local)

		//access
		tm_info_a = localtime(&sb.st_atime);
		strftime(time_buffer_a, sizeof(time_buffer_a), "%Y-%m-%d %H:%M:%S %z (%Z) %a (local)", tm_info_a);
		//modification
		tm_info_m = localtime(&sb.st_mtime);
		strftime(time_buffer_m, sizeof(time_buffer_m), "%Y-%m-%d %H:%M:%S %z (%Z) %a (local)", tm_info_m);	
		//change
		tm_info_c = localtime(&sb.st_ctime);
		strftime(time_buffer_c, sizeof(time_buffer_c), "%Y-%m-%d %H:%M:%S %z (%Z) %a (local)", tm_info_c);

		//local timestamp
		printf("  Last file access:		%s\n", time_buffer_a);
		printf("  Last file modification:	%s\n", time_buffer_m);
		printf("  Last status change:		%s\n", time_buffer_c);


		//time buffers (GMT)

		//access
		gm_info_a = gmtime(&sb.st_atime);
		strftime(gtime_buffer_a, sizeof(gtime_buffer_a), "%Y-%m-%d %H:%M:%S %z (%Z) %a (GMT)", gm_info_a);
		//modification
		gm_info_m = gmtime(&sb.st_mtime);
		strftime(gtime_buffer_m, sizeof(gtime_buffer_m), "%Y-%m-%d %H:%M:%S %z (%Z) %a (GMT)", gm_info_m);	
		//change
		gm_info_c = gmtime(&sb.st_ctime);
		strftime(gtime_buffer_c, sizeof(gtime_buffer_c), "%Y-%m-%d %H:%M:%S %z (%Z) %a (GMT)", gm_info_c);
	
		//UTD timestamp
		printf("  Last file access:		%s\n", gtime_buffer_a);
		printf("  Last file modification:	%s\n", gtime_buffer_m);
		printf("  Last status change:		%s\n", gtime_buffer_c);

	}


return EXIT_SUCCESS;
}


