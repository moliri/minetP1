#include "minet_socket.h"
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <ctype.h>
#include <string.h>

using namespace std;

//bind to client, connect to server

#define BUFSIZE 5454545 //should start at 1024 and dynamically allocate with malloc

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
	//int fd = 0;  fd in main function should actually be the variable sock, declared above
    fd_set* set;
	int sock2 = 0;
	int listenSock = 0;

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
	sock = minet_socket(SOCK_STREAM);
	if (sock < 0) {
		cerr << "Can't create socket." << endl;
		minet_perror("reason:");
		die(sock);
    } 
	cerr << "Socket created." << endl;
	
    // Do DNS lookup
    /* Hint: use gethostbyname() */
	site = gethostbyname(server_name);
	
	if (site == 0) {
		cerr << "Unknown host." << endl;
		die(sock);
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
	if (minet_bind(sock, &client_sa) < 0) {
		cerr << "Can't bind socket." << endl;
		minet_perror("reason:");
		die(sock);
    }
	 cerr << "Socket bound" << endl;
	if (minet_connect(sock, &server_sa) < 0) {
		cerr << "Can't connect socket." << endl;
		minet_perror("reason:");
		die(sock);
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
	strcpy(mybuf, buf);
	strcat(mybuf, "GET ");
	strcat(mybuf, server_path); 
	strcat(mybuf, " HTTP/1.1\r\n"); 
	strcat(mybuf, "Host: ");
	strcat(mybuf,  server_name);
	strcat(mybuf,  "\r\nConnection: close");
	strcat(mybuf, "\r\nUser-agent: Mozilla/4.0\r\n");
	strcat(mybuf, "Accept-language: en\r\n\r\n");
	
	//cout << "\n\nGet request stored in mybuf\n\n";
	//cout << mybuf << endl;
	if (minet_write(sock, mybuf, BUFSIZE) < 0) {
	    cerr << "Write failed." << endl;
	    minet_perror("reason:");
		die(sock);
	}
	
	//cout << "\n\nwrote mybuf to the socket\n\n";
	/* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */
	//cout << mybuf;
	
	set = (fd_set*)malloc(sizeof(BUFSIZE)); //should change from BUFSIZE
	FD_ZERO(set);
	FD_SET(sock, set);
	
	while(1) {
		int readSet = (minet_select(sock+1, set, NULL, NULL, NULL)); //holds number of sockets ready to go
		listenSock = sock;
		if (readSet < 0)
			cerr << "Select failed." << endl;
		//process sockets that are ready
		for(int i=0; i<=sock; i++) {
			if(!(FD_ISSET(i, set))) //check to see if socket is in set, if not in set, move on
				continue;
			//for the accept socket, add accepted connection to connections
			if(i == listenSock) { //if at end of loop 
				sock2 = minet_accept(listenSock, &server_sa); //server accepting connection to socket before exchanging data with client via read/write, sock2 now holds connection to client, sock goes back to listening/not part of connection
				if (sock2 > sock) {
					//update sock
					sock = sock2;
				}
				FD_SET(sock2, set); 
			}
			else {
				rc = minet_read(sock, mybuf, BUFSIZE); //buffer receives incoming data
				FD_CLR(i, set);
				if(i == sock)
					sock = sock2;
			}
		}
	}
    /* print first part of response */
	//print char buffer - lm
	//buf should have the characters in it after using minet_read
	//cout << "\n\nread from mybuf after selecting socket, first loop\n\n";
	//cout << mybuf;//print the headers to the error
	

    /* second read loop -- print out the rest of the response */
	//print char buffer - lm
	//put the rest of the response into buf
	while (1) {
		if ((rc = minet_read(sock, buf, BUFSIZE)) < 0) {
			cerr << "Read failed." << endl;
			minet_perror("reason:");
			break;
		}

		if (rc == 0) {
			cerr << "Done." << endl;
			break;
		}

		if (minet_write(sock, mybuf, rc) < 0) {
			cerr << "Write failed." << endl;
			minet_perror("reason:");
			break;
		}
    }
	string strng = (string) mybuf;
	unsigned pos = strng.find("<!DOCTYPE html>");
	strng = strng.substr(pos);
	cout << strng << endl;//the body of the reply
    
    /*close socket and deinitialize */
	die(sock);
	
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


