#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>

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
    struct sockaddr_in sa;
    FILE * wheretoprint = stdout;
    struct hostent * site = NULL;
    char * req = NULL;

    char buf[BUFSIZE + 1];
    char * bptr = NULL;
    char * bptr2 = NULL;
    char * endheaders = NULL;
   
    struct timeval timeout;
	int fd = 0;
    fd_set set;

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
	bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    memcpy((void *)(&(sa.sin_addr)), site->h_addr, site->h_length);
    sa.sin_port = htons(atoi(argv[3]));

    /* connect socket */
	if (minet_bind(fd, sa) < 0) {
		cerr << "Can't bind socket." << endl;
		minet_perror("reason:");
		die(fd);
    }
	
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
	buf = "GET "+server_path+" HTTP/1.0\r\n"+"Host: "+server_name+"\r\n"+"Connection: keep-alive\r\nUser-agent: Chrome/32.0.1700.76 m\r\nAccept-language: en\r\n";
	if (minet_write(fd, buf, BUFSIZE) < 0) {
	    cerr << "Write failed." << endl;
	    minet_perror("reason:");
		die(fd);
	    break;
	}

    /* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */
	
		//minet_select() - lm
    
    /* first read loop -- read headers */
		
		//minet_read() - lm
    
    /* examine return code */   
		
		//returns either bytes read from rocket or -1 - lm
			
		//Skip "HTTP/1.0"
		//remove the '\0'
		// Normal reply has return code 200

    /* print first part of response */
		
		//print char buffer - lm

    /* second read loop -- print out the rest of the response */
	
		//print char buffer - lm
    
    /*close socket and deinitialize */


    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

int write_n_bytes(int fd, char * buf, int count) {
    int rc = 0;
    int totalwritten = 0;

    while ((rc = minet_write(fd, buf + totalwritten, count - totalwritten)) > 0) {
	totalwritten += rc;
    }
    
    if (rc < 0) {
	return -1;
    } else {
	return totalwritten;
    }
}


