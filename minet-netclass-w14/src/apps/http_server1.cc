#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;


#define BUFSIZE 383484
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
  int rc,fd;

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
   rc = minet_listen(fd,1);
   if (rc < 0)
   {
		cerr << "Can't listen on socket.\n";
		minet_perror("reason:");
		die(fd);
	} else {
		cerr << "Socket listened.\n";
	}

  /* connection handling loop */
  while(1)
  {
    /* handle connections */
    rc = handle_connection(fd);
  }
}

int handle_connection(int sock)  //sock = cfd 
{
	cout << "Handling connection. \n";
  char filename[FILENAMESIZE+1];
  int rc;
  int fd2;
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
  bool ok=true;
  char tmp_header[BUFSIZE];
  string output;
  fd_set* set;
	cout << "Accepting socket. \n";
  fd2 = minet_accept(sock, &client_sa);//not sure if this should be sa or sa2 -jg; should be sa, aka the client one, not the server - lm
  /* first read loop -- get request and headers*/
  if (fd2 < 0) {
	cerr << "Accepting socket failed.";
  }
  
  cout << "Reading from socket."  << endl;
  rc = minet_read(fd2,buf,BUFSIZE+1);
  if (rc<0){
	cerr << "Read failed.\n";
	minet_perror("reason:");
	die(fd2);
	}
  if (rc==0){
	cerr << "Done.\n";
	}

/*  
	cout << "Setting stuff for select\n";
	set = (fd_set*)malloc(sizeof(BUFSIZE));
	
	FD_ZERO(set);
	FD_SET(sock, set);
	if (minet_select(sock+1, set, NULL, NULL, NULL) > 0) {
		cout << "Reading from socket\n";
		rc = minet_read(fd2,buf,BUFSIZE+1);
		cout << "read from socket\n";
	}
  if (rc<0){
	cerr << "Read failed.\n";
	minet_perror("reason:");
	die(fd2);
	}
  if (rc==0){
	cerr << "Done.\n";
	}
	*/
	
  // /* parse request to get file name */
	string mybuf = (string)buf;
	unsigned pos1 = mybuf.find("GET "); //position is greater than string length
	//GET path HTTP/1.0\r\n\r\n
	//   pos1           pos2 
	//int bufLength = mybuf.length();
	
	if(pos1<0) {
		
		cerr << "First substring position not right" << endl;
		strcpy(tmp_header, notok_response);
		die(fd2);
	}
	else {
		mybuf = mybuf.substr(pos1/*, (bufLength+1)*/);
		cerr << mybuf << endl;
		//unsigned pos2 = pos1+4;
		unsigned pos3= mybuf.find(" HTTP");
		cout << "First substring ok" << endl;
		if (pos3 < 0) {
			cerr << "Second substring position not right" << endl;
			strcpy(tmp_header, notok_response);
			die(fd2);
		}
		else {
			
			string path = mybuf.substr(pos1+4, pos3-4);
			cout << "Second substring ok." << endl;
			cerr << path << endl;
			/*char * mypath;
			strcpy(mypath,path.c_str());
			
			//cout << "Path is " << path << endl;
			
			/* Assumption: this is a GET request and filename contains no spaces
			
			
			/* try opening the file 
			cerr << "Opening file\n";
			if((fd2=open(mypath, O_RDONLY)) < 0) {
				ok = false;
			}*/
			string line;
			fstream myfile;
			myfile.open(path);
			if (myfile.is_open()) {
				cerr << "file opened\n";
				while (getline(myfile, line)) {
					
					output+= line;
					output+="\n";
				}
				strcpy(tmp_header, ok_response_f);
			}
			
			else{
				cerr << "file not opened\n";
				strcpy(tmp_header, notok_response);
				ok = false;
			}
			if (output.length() > 0) {
				cerr << output << endl;
			}
		}
	}
	
  /* send response */
  char * outbuf = new char[output.length()+1];
  
  cout << "About to write." << endl;
  cout << ok << endl;
  if (ok)
  {
    /* send headers */
	minet_write(fd2, tmp_header, BUFSIZE);

    /* send file */
	strcpy(outbuf,output.c_str());
	minet_write(fd2, outbuf, BUFSIZE);
  }
  else // send error response
  {
	cout << "is it going to work?" << endl;
	if (minet_write(fd2, tmp_header, strlen(tmp_header)) < 0) {
		cerr <<"Write failed" << endl;
		minet_perror("reason: ");
		die(fd2);
	}
	cout << "YESSSSSS" <<endl;
  }
	
  /* close fd2et and free space */
	delete [] outbuf;
	die(fd2);

// cerr << "JUST WRITE." << endl;
// minet_write(fd2, notok_response, BUFSIZE);
// cerr << "WRITTEN." << endl;


  if (ok)
    return 0;
  else
    return -1;
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
