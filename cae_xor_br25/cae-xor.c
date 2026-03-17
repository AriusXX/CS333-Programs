//*****************************************************************************************
//Billy Rodriguez	Cae-Xor		CS333		
//
//
//A program to encipher input with both Caesar-like cipher and Xor cipher
//*****************************************************************************************


//library calls
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

//MACROS
#define OPTIONS "edc:x:Dh"
#define BUFSIZE 4096


//global values
int encrypt_mode = 1;	//default flag
int debug_mode = 0;	//debug flag


//protocall functions
void caesar_cipher(char *buf, ssize_t n, const char *key, int decrypt);
void xor_cipher(char *buf, ssize_t n, const char *key);
void help(void);


//Main
int main(int argc, char *argv[])
{
	//processing buffers
	char buffer[BUFSIZE];
	ssize_t bytes_read = 0;

	//get-opt variables
	int opt = 0;
	char *caesar_key = NULL;
	char *xor_key = NULL;

	//get-opt function
	while((opt = getopt(argc, argv, OPTIONS)) != -1)
	{
		switch(opt)
		{
			case 'e':
				encrypt_mode = 1;
				break;
			case 'd':
				encrypt_mode = 0;
				break;
			case 'c':
				caesar_key = optarg;	
				break;
			case 'x':
				xor_key = optarg;	
				break;
			case 'D':
				debug_mode = 1;
				break;
			case 'h':
				help();
				exit(EXIT_SUCCESS);
			default:
				help();
				exit(EXIT_FAILURE);
		}
	}


	//Terminal Command Work
	while((bytes_read = read(STDIN_FILENO, buffer, BUFSIZE)) > 0)
	{
		//debug mode
		if(debug_mode)
		{
			fprintf(stderr, "Debugging: Processing buffer of size %ld\n", bytes_read);
			fprintf(stderr, "Debugging: Encrypt mode is %d\n", encrypt_mode);
			fprintf(stderr, "Debugging: Using Caesar key '%s'\n", caesar_key ? caesar_key: "NULL");
			fprintf(stderr, "Debugging: Using Xor key '%s'\n", xor_key ? xor_key: "NULL");
		}

		if(encrypt_mode)
		{
			if(caesar_key)
				caesar_cipher(buffer, bytes_read, caesar_key, 0);
			if(xor_key)
				xor_cipher(buffer, bytes_read, xor_key);
		}
		else
		{
			if(xor_key)
				xor_cipher(buffer, bytes_read, xor_key);
			if(caesar_key)
				caesar_cipher(buffer, bytes_read, caesar_key, 1);
		}


		write(STDOUT_FILENO, buffer, bytes_read);
	}	

	//failure reading bytes
	if(bytes_read < 0)
	{
		perror("read");
		exit(EXIT_FAILURE);
	}
			
	return EXIT_SUCCESS;
}

//Caesar-Cipher Function
void caesar_cipher(char *buf, ssize_t n, const char *key, int decrypt)
{

	//variables
	size_t key_len = 0;
	static size_t key_index = 0;
	ssize_t i = 0;
	unsigned char ch = 0;
	int shift = 0;

	//error input
	if(!key || !*key)
	{
		fprintf(stderr, "Error: Caesar Cipher key is invalid.\n");
		return;
	}

	key_len = strlen(key);

	//debug mode
	if(debug_mode)
	{
		fprintf(stderr, "Debugging: Caesar cipher started with key '%s'\n", key);
	}


	for(i = 0; i < n; i++)
	{
		ch = buf[i];

		//shitable printable ASCII (' ' to '~')
		if(ch >= 32 && ch <= 126)
		{
			shift = key[key_index] - ' ';
			
			//whenever a -d flag is set in terminal
			if(decrypt)
				shift = -shift;
		
			//debug mode
			if(debug_mode)
				fprintf(stderr, "Debugging: Char '%c' (ASCII: %d) -> Shift: %d\n", ch, ch, shift);
			
			//algorithym
			ch = ((ch - 32 + shift + 95) % 95) + 32;	
			buf[i] = ch;	

			key_index = (key_index + 1) % key_len;
		}
		
	}
}

//Xor-Cipher Function
void xor_cipher(char *buf, ssize_t n, const char *key)
{
	//variables
	size_t key_len;
	static size_t key_index = 0;
	ssize_t i = 0;

	//error input
	if(!key || !*key) 
	{
		fprintf(stderr, "Error: XOR cipher key is invalid.\n");
		return;
	}

	key_len = strlen(key);

	//debug mode
	if(debug_mode)
		fprintf(stderr, "Debugging: XORing byte %d with key byte '%c' (ASCII: %d)\n", buf[i], key[key_index], key[key_index]);

	for(i = 0; i < n; i++)
	{
		buf[i] = buf[i] ^ key[key_index];
		key_index = (key_index + 1) % key_len;
	}
}

//Help Function
void help(void)
{
	fprintf(stderr, 
			"   	-e		encript text (default)\n"
			"	-d		decript text\n"
			"	-c str	string to use for caesar cipher algorithm (default NULL)\n"
			"	-x str	string to use for xor cipher algorithm (default NULL)\n"
			"	-D		enable diagnostic output\n"
			"	-h		show this info again and exit\n");
}

