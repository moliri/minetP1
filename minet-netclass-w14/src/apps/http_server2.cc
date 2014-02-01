//AUTHORS: Jordan Geltner and Leesha Maliakal
#include "minet_socket.h"
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

#define BUFSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int);
int writenbytes(int,char *,int);
int readnbytes(int,char *,int);

void die(int fd) {
    minet_close(fd);
    minet_deinit();
    exit(0);
}

int main(int argc,char *argv[])
{
  int server_port;
  int sock,sock2;
  struct sockaddr_in server_sa,client_sa;
  int rc,i,fd;
  fd_set readlist;
  fd_set connections;
  int maxfd;

  /* parse command line args */
  if (argc != 3)
  {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }
  
  if (toupper(argv[1][0])=='K') {
    cerr << "Using kernel stack.\n";
    if (minet_init(MINET_KERNEL)<0) {
      cerr << "Stack initialization failed.\n";
      die(0);
    } else {
      cerr << "Stack initialized.\n";
    }
  } else {
    cerr << "Using Minet User Level Stack.\n";
    if (minet_init(MINET_USER)<0) {
      cerr << "Stack initialization failed.\n";
      die(0);
    } else {
      cerr << "Stack initialized.\n";
    }
  }
  
  server_port = atoi(argv[2]);
  if (server_port < 1500)
  {
    fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
    exit(-1);
  }

  /* initialize and make socket */
  fd = minet_socket(SOCK_STREAM);
  
	if (fd<0) {
		cerr << "Can't create socket.\n";
		minet_perror("reason:");
		die(fd);
     } else {
     cerr << "Socket created.\n";
	}
  /* set server address*/
  bzero(&server_sa,sizeof(server_sa));
  server_sa.sin_family=AF_INET;
  server_sa.sin_addr.s_addr=htonl(INADDR_ANY);
  server_sa.sin_port=htons(atoi(argv[2]));
  
  /* bind listening socket */
    if (minet_bind(fd,&server_sa)<0) {
		cerr << "Can't bind socket.\n";
		minet_perror("reason:");
		die(fd);
    } else {
		cerr << "Socket bound.\n";
    }

  /* start listening */
   rc = minet_listen(fd,5);
   if (rc < 0)
   {
		cerr << "Can't listen on socket.\n";
		minet_perror("reason:");
		die(fd);
	} else {
		cerr << "Socket listened.\n";
	}
  /* connection handling loop */
	maxfd = fd;
	FD_ZERO(&connections);
	FD_SET(fd, &connections);
	while(1)  {
    /* create read list */
	readlist = connections;	

    /* do a select */
	int rc = minet_select(maxfd+1, &readlist, NULL, NULL, NULL);
	if (rc<0) {
		cerr << "Select error. \n";
	}

    /* process sockets that are ready */
	for(i = 0; i<=maxfd; i++) {
		if(!(FD_ISSET(i, &readlist))) {
			continue;
		}
      /* for the accept socket, add accepted connection to connections */
		if (i == fd) {
			sock2 = minet_accept(fd, &client_sa);
			if(sock2 > maxfd)
			maxfd++;
			FD_SET(sock2, &connections);
		}
		else {/* for a connection socket, handle the connection */
			rc = handle_connection(i);
			FD_CLR(i, &readlist);
			FD_CLR(i, &connections);
			if(i == maxfd);
			maxfd++;
		}
	}
  }
}

int handle_connection(int sock) {
  char filename[FILENAMESIZE+1];
  int rc;
  int fd,fd2; //ends up holding client sock
  struct stat filestat;
  struct sockaddr_in client_sa;
  char buf[BUFSIZE+1];
  char *headers;
  char *endheaders;
  char *bptr;
  int datalen=0;
  char *ok_response_f = "HTTP/1.0 200 OK\r\n"\
                      "Content-type: text/plain\r\n"\
                      "Content-length: %d \r\n\r\n";
  char ok_response[100];
  char *notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"\
                         "Content-type: text/html\r\n\r\n"\
                         "<html><body bgColor=black text=white>\n"\
                         "<h2>404 FILE NOT FOUND</h2>\n"
                         "</body></html>\n";
  char notok_response_f[200];
  bool ok=true;
  char tmp_header[BUFSIZE];
  string output;
  char * mypath;
  fd_set* set;
  /* first read loop -- get request and headers*/
  
  //////cout << "Reading from socket."  << endl;
  rc = minet_read(sock,buf,BUFSIZE+1);
  if (rc<0){
	minet_perror("reason:");
	minet_close(sock);
	return -1;
	}
  if (rc==0){
	cerr << "Done.\n";
	}
	
	/* parse request to get file name */
	string mybuf = (string)buf;
	unsigned pos1 = mybuf.find("GET "); //position is greater than string length
	char readfile[BUFSIZE];
//	readfile  = (char*)malloc(sizeof(BUFSIZE));
	
	//we had some substring errors so we checked their positions
	if(pos1<0) {
		cerr << "First substring position not right" << endl;
		strcpy(tmp_header, notok_response);
	}
	else {
		mybuf = mybuf.substr(pos1/*, (bufLength+1)*/);
		cerr << mybuf << endl;
		//unsigned pos2 = pos1+4;
		unsigned pos3= mybuf.find(" HTTP");
		//////cout << "First substring ok" << endl;
		if (pos3 < 0) {
			cerr << "Second substring position not right" << endl;
			strcpy(tmp_header, notok_response);
			minet_close(sock);
		}
		else {
			//if all the substrints are right, we can take the path from the get request
			string path = mybuf.substr(pos1+4, pos3-4);
			//////cout << "Second substring ok." << endl;
			//cerr << path << endl;
			mypath  = (char*)malloc(100);
			strcpy(mypath,path.c_str());
						
			/* Assumption: this is a GET request and filename contains no spaces
			
			/* try opening the file */
			if((fd=open(mypath, O_RDONLY)) < 0) {
				cerr << "Opening failed.\n";
				//put the bad response into the header buffer
				strcpy(tmp_header, notok_response);
				ok = false;
			}else {
				//put the good response into the header buffer
				strcpy(tmp_header, ok_response_f);
				cerr << "Reading " << path << endl;
				//read from the file into the body buffer
				readnbytes(fd, readfile, BUFSIZE);
				//////cout << mypath << " read\n";
				//////cout << readfile << endl;
			}
		}
	}
	
  /* send response */
  
  //////cout << "About to write." << endl;
  ////////cout << ok << endl;
  if (ok)
  {
    /* send headers */
	//////cout << "Sending good header\n";
	sprintf(ok_response,ok_response_f,300);
	minet_write(sock, ok_response, strlen(ok_response));

	//////cout << "good header sent, sending " << mypath << endl;
    /* send file */
	minet_write(sock, readfile, strlen(readfile));
  }
  else // send error response
  {
	//////cout << "Sending bad header\n";
	sprintf(notok_response_f,notok_response,300);
	if (minet_write(sock, notok_response_f, strlen(notok_response_f)) < 0) {
		cerr <<"Write failed" << endl;
		minet_perror("reason: ");
		minet_close(sock);
		return -1;
	}
	//////cout << "Sent bad header to client\n";
  }
  //////cout << "Write finished\n\n\n";//, closing socket\n";
	
  /* close socket and free space */
	////////cout << "socket closed, returning\n";
	close(fd);
	minet_close(sock);

  if (ok)
  {
	//minet_close(fd2);
    return 0;
}
  else{
    //minet_close(fd2);
    return -1;
	}
}

int readnbytes(int fd,char *buf,int size)
{
  int rc = 0;
  int totalread = 0;
  while ((rc = minet_read(fd,buf+totalread,size-totalread)) > 0)
    totalread += rc;

  if (rc < 0)
  {
    return -1;
  }
  else
    return totalread;
}

int writenbytes(int fd,char *str,int size)
{
  int rc = 0;
  int totalwritten =0;
  while ((rc = minet_write(fd,str+totalwritten,size-totalwritten)) > 0)
    totalwritten += rc;

  if (rc < 0)
    return -1;
  else
    return totalwritten;
}

