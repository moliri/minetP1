#include "minet_socket.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <ctype.h>

using namespace std;

//bind to client, connect to server

#define BUFSIZE 1024

int write_n_bytes(int fd, char * buf, int count);

void die(int fd) {
    minet_close(fd);
    minet_deinit();
    exit(0);
}

int main(int argc, char * argv[]) {
    char * server_name = NULL;
    int server_port = 0;
    char * server_path = NULL;

    int sock = 0;
    int rc = -1;
    int datalen = 0;
    bool ok = true;
    sockaddr_in client_sa;
	sockaddr_in server_sa;
    FILE * wheretoprint = stdout;
    struct hostent * site = NULL;
    char * req = NULL;

    char buf[BUFSIZE + 1];
    char * bptr = NULL;
    char * bptr2 = NULL;
    char * endheaders = NULL;
   
    struct timeval timeout;
	int fd = 0;
    fd_set* set;

    /*parse args */
    if (argc != 5) {
	fprintf(stderr, "usage: http_client k|u server port path\n");
	exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];



    /* initialize minet */
    if (toupper(*(argv[1])) == 'K') { 
	minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') { 
	minet_init(MINET_USER);
    } else {
	fprintf(stderr, "First argument must be k or u\n");
	exit(-1);
    }
    

	/* create socket */
	fd = minet_socket(SOCK_STREAM);
	if (fd < 0) {
		cerr << "Can't create socket." << endl;
		minet_perror("reason:");
		die(fd);
    } 
	cerr << "Socket created." << endl;
	
    // Do DNS lookup
    /* Hint: use gethostbyname() */
	site = gethostbyname(server_name);
	
	if (site == 0) {
		cerr << "Unknown host." << endl;
		die(fd);
    }
	
	/* set address */
	bzero(&server_sa, sizeof(server_sa));
    server_sa.sin_family = AF_INET;
    memcpy((void *)(&(server_sa.sin_addr)), site->h_addr, site->h_length);
    server_sa.sin_port = htons(atoi(argv[3]));
	
	bzero(&client_sa, sizeof(client_sa));
    client_sa.sin_family = AF_INET;
    client_sa.sin_addr.s_addr = htonl(INADDR_ANY);
    client_sa.sin_port = htons(0);

    /* connect socket */
	if (minet_bind(fd, &client_sa) < 0) {
		cerr << "Can't bind socket." << endl;
		minet_perror("reason:");
		die(fd);
    }
	 cerr << "Socket bound" << endl;
	if (minet_connect(fd, &server_sa) < 0) {
		cerr << "Can't connect socket." << endl;
		minet_perror("reason:");
		die(fd);
    }
    
	cerr << "Socket connected." << endl;
    /* send request */
	
	/* 
		SAMPLE HTTP MESSAGE
		GET /somedir/page.html HTTP/1.1
		Host: www.someschool.edu
		Connection: close
		User-agent: Mozilla/5.0
		Accept-language: fr
	*/
	char mybuf[BUFSIZE];
	//strcpy(mybuf, buf);
	strcat(mybuf, "GET ");
	strcat(mybuf, server_path); 
	strcat(mybuf, " HTTP/1.0\r\n"); 
	strcat(mybuf, "Host: ");
	strcat(mybuf,  server_name);
	strcat(mybuf,  "\r\nConnection: close \r\n"); //after connection type, User-agent: Chrome/32.0.1700.76 m\r\nAccept-language: en
	
	if (minet_write(fd, mybuf, BUFSIZE) < 0) {
	    cerr << "Write failed." << endl;
	    minet_perror("reason:");
		die(fd);
	}
	/* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */
	
	set = (fd_set*)malloc(sizeof(BUFSIZE));
	int filedes = 0;
	FD_SET(filedes, set);
	if (minet_select(fd+1, set, set, set, NULL) > 0) {
   
	/* first read loop -- read headers */
	/* examine return code */ 
		//returns either bytes read from socket or -1
		//Skip "HTTP/1.0"
		//remove the '\0'
		// Normal reply has return code 200
		minet_read(fd, buf, BUFSIZE);

    }

    /* print first part of response */
	//print char buffer - lm
	//buf should have the characters in it after using minet_read
	//i'm not sure how to separate the first and second parts of the response. idk what they mean
	cout << mybuf;
	

    /* second read loop -- print out the rest of the response */
	//print char buffer - lm
	//put the rest of the response into buf
	while (1) {
		if ((rc = minet_read(fd, buf, BUFSIZE)) < 0) {
			cerr << "Read failed." << endl;
			minet_perror("reason:");
			break;
		}

		if (rc == 0) {
			cerr << "Done." << endl;
			break;
		}

		if (minet_write(fd, mybuf, rc) < 0) {
			cerr << "Write failed." << endl;
			minet_perror("reason:");
			break;
		}
    }
	cout << buf;
    
    /*close socket and deinitialize */
	die(fd);
	
    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

int write_n_bytes(int fd, char * mybuf, int count) {
    int rc = 0;
    int totalwritten = 0;

    while ((rc = minet_write(fd, mybuf + totalwritten, count - totalwritten)) > 0) {
	totalwritten += rc;
    }
    
    if (rc < 0) {
	return -1;
    } else {
	return totalwritten;
    }
}


