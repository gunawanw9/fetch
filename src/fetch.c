/* fetch.c
 
	fetch - An easy to use, tweakable HTTP download tool that utilizes the
		HTTP Fetcher library (http://cs.nmu.edu/~lhanson/http_fetcher/)
 
	Copyright (C) 2001 Lyle Hanson (lhanson@cs.nmu.edu)

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	See included LICENSE file for details 

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <http_fetcher.h>


void printUsage()
	{
	printf("Usage:  fetch [OPTIONS] URL\n\n");
	printf("Downloads a file using the GET method of the HTTP protocol\n\n");
	printf("  -A\t\t\tdo not present default User Agent to server\n");
	printf("  -a, --agent\t\tUser Agent to present to the server\n");
	printf("  -f, --filename\tlocal filename to save the url to\n");
	printf("  -h, --help\t\tdisplay this help message\n");
	printf("  -n, --nosave\t\thit the url, but don't save it to disk\n");
	printf("  -r, --referer\t\treferer URL to present to the server\n");
	printf("\t\t\t\t(no referer presented by default)\n");
	printf("  -t, --timeout\t\tnumber of seconds without data before giving up\n");
	printf("\t\t\t\t(less than 0 means don't time out)\n");
	printf("  -v, --version\t\toutput version information and exit\n\n");
	printf("Lyle Hanson (lhanson@cs.nmu.edu)\n");
	
	exit(1);
	}



void printVersion()
	{
	printf("fetch %s\n\n", VERSION);
	printf("Copyright (C) 2001 Lyle Hanson (lhanson@cs.nmu.edu)\n");
	exit(0);
	}



int main(int argc, char *argv[])
	{
	char *url = NULL,
		 *downloaded_file = NULL,
		 *filename = NULL,
		 *user_agent = NULL,
		 *referer = NULL;
	
	int	ret = 0,
		c,
		fd,
		file_length = 0,
		digit_optind = 0,
		nosave = 0,
		suppress_User_Agent = 0,
		timeout = 0,
		timeoutSet = 1;
	
	if(argc < 2)
		printUsage();
	
	/* Use getopt to process command line arguments. */
	while(1)
		{
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] =
			{
			{"agent", 1, 0, 'a'},
			{"filename", 1, 0, 'f'},
			{"help", 0, 0, 'h'},
			{"nosave", 0, 0, 'n'},
			{"referer", 1, 0, 'r'},
			{"timeout", 1, 0, 't'},
			{"version", 0, 0, 'v'},
			{0, 0, 0, 0}
			};

		c = getopt_long(argc, argv, "Aa:f:hnr:t:v", long_options,&option_index);
		if(c == -1)
			break;

		switch(c)
			{
			case 0:
				printf("option %s", long_options[option_index].name);
				if(optarg)
					printf(" with arg %s", optarg);
				printf("\n");
				break;
			
			case '0':
			case '1':
			case '2':
				if (digit_optind != 0 && digit_optind != this_option_optind)
					printf ("digits occur in two different argv-elements.\n");
				digit_optind = this_option_optind;
				printf ("option %c\n", c);
				break;    

			case '?':
				break;

			case 'A':
				suppress_User_Agent = 1;
				break;

			case 'a':
				user_agent = optarg;
				break;
				
			case 'f':
				if(nosave != 0)
					{
					printf("'filename' and 'nosave' switches incompatible.\n"
							"Try 'fetch -h' for help.\n");
					exit(1);
					}
				filename = optarg;
				break;

			case 'h':
				printUsage();
				break;			/* (not reached */
				
			case 'n':
				if(filename != NULL)
					{
					printf("'filename' and 'nosave' switches incompatible.\n"
							"Try 'fetch -h' for help.\n");
					exit(1);
					}
				nosave = 1;
				break;
				
			case 'r':
				referer = optarg;
				break;

			case 't':
				timeoutSet = 1;
				timeout = atoi(optarg);
				break;

			case 'v':
				printVersion();
				break;
				
			default:
				printf("?? getopt returned character code 0%o (%c) ??\n", c, c);
			}
		}
	
	if(optind < argc)
		{
		if(argc - optind != 1)
			{
			printf("Too many arguments.\nTry 'fetch -h' for help.\n");
			exit(1);
			}
		else
			url = argv[optind];
		}
	else if(optind == argc)
		{
		printf("Download URL not specified.\nTry 'fetch -h' for help.\n");
		exit(1);
		}
	
	/* 
	 * Allright, now that all that optarg junk is out of the way, I'll
	 *	illustrate use of HTTP Fetcher.
	 * Functions that begin with http_ are part of HTTP Fetcher.
	 */

	/* Set up HTTP Fetcher with the options that the user has chosen: */

	/* REFERER */
	if(referer != NULL)
		{
		ret = http_setReferer(referer);
		if(ret == -1)
			{
			http_perror("http_setReferer");
			exit(1);
			}
		}
	
	/* USER AGENT*/
	if(suppress_User_Agent)
		ret = http_setUserAgent(NULL);
	else if(user_agent != NULL)
		ret = http_setUserAgent(user_agent);
	if(ret == -1)
		{
		http_perror("http_setUserAgent");
		exit(1);
		}

	/* TIMEOUT */
	if(timeoutSet)
		http_setTimeout(timeout);
	

	/*
	 * Now let's actually download the page!  Note that THIS function can be
	 *	used to request files all by itself, without setting up any of the
	 *	options above.
	 *
	 * Calling http_fetch() with a NULL buffer tells HTTP Fetcher to just
	 *	download the url and toss it away.  Passing it a pointer gives you
	 *	access to the downloaded url.
	 */
	if(nosave)
		file_length = http_fetch(url, NULL);
	else
		file_length = http_fetch(url, &downloaded_file);

	if(file_length == -1)
		{
		http_perror("http_fetch");

		/* You could also get a pointer to the error description string:
		 *	char *tmp = http_strerror(); printf("Error is: %s\n", tmp);
		 */

		exit(1);
		}
	printf("Downloaded %d bytes\n", file_length);

	
	/* If the user wanted the file saved, do it.  Otherwise they probably
	 *	just wanted to register a hit on the server */
	if(!nosave)
		{
		/* Write the file to disk */
		if(filename == NULL)
			{
			/* User didn't specify a filename to save to, so we'll try to parse
			 *	it out of the url */
			ret = http_parseFilename(url, &filename);
			if(ret == 1)
				{
				/* url contains NO end filename.  We'll go with 'index.html'. */
				filename = (char *)malloc(strlen("index.html") + 1);
				if(filename == NULL)
					{
					perror("allocating memory for filename");
					exit(1);
					}
			
				strncpy(filename, "index.html", strlen("index.html"));
				filename[strlen("index.html")] = 0;	/* NULL terminate it */
				}
			else if(ret == -1)
				{
				http_perror("http_parseFilename");
				exit(1);
				}
			}
printf("Writing to file: %s\n", filename);

		fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
		if(fd == -1)
			{
			perror("opening file for writing");
			exit(1);
			}
	
		ret = write(fd, downloaded_file, file_length);
		if(ret == -1)
			{
			perror("writing to file");
			exit(1);
			}
		else if(ret != file_length)
			printf("warning: %d bytes written out of %d downloaded\n\n",
														ret, file_length);

		/* Free the buffer holding the file */
		free(downloaded_file);
		}	/* end of if(!nosave) */	

	exit(0);
	}
