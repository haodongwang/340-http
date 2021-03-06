#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <iostream>

#define BUFSIZE 1024

using namespace std;

int write_n_bytes(int fd, char * buf, int count);

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
    //fprintf(stderr, "before socket");
    /* create socket */
    sock=minet_socket(SOCK_STREAM);
    // Do DNS lookup
    /* Hint: use gethostbyname() */
    //fprintf(stderr, "a");
    site = gethostbyname(server_name);
    if (site == NULL) {
        minet_close( sock);
        minet_deinit();
        fprintf(stderr, "gethostbyname");
        return -1;
    }
    /* set address */
    memset (& sa, 0, sizeof( sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(server_port);
    sa.sin_addr.s_addr = *( unsigned long *) site-> h_addr_list[0];
    /* connect socket */
    if (minet_connect(sock, &sa) != 0) {
        minet_close(sock);
        fprintf(stderr, "connect");
        return -1;
    }
    fprintf(stderr, "b");
    
    int fdmax = 0;
    fdmax = (fdmax>sock)? fdmax: sock;
    fprintf(stderr, "c");
    int n=0;
    //assemble request
    req = buf;
    sprintf(req,"GET %s HTTP/1.0\r\n\r\n",server_path);
    /* send request */
    n = write_n_bytes(sock, req, strlen(buf));
    if (n < 0) {
        fprintf(stderr, "ERROR writing to socket");
        return -1;
    }
    /* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */
    FD_SET(0, &set);
    FD_SET(sock, &set);
    fprintf(stderr, "d");
    if ((rc=minet_select(fdmax+1, &set, NULL, NULL, NULL)) == -1) {
        fprintf(stderr, "select");
        return -1;
    }
    //moved sending message out of the loop for now

    /* first read loop -- read headers */
    fprintf(stderr, "f");
    bzero(buf, BUFSIZE);
    string headers="";
    string response="";
    size_t index;
    while((n = minet_read(sock, buf, BUFSIZE))>0){
	buf[n]='\0';
	headers+=buf;
	if((index=headers.find("\r\n\r\n"))!=string::npos)
	{
	//while there is response left to read
	//read until the end of the headers (two newlines in a row)
	    response = headers.substr(index+4);
	    headers = headers.substr(0,index);
	//store buf in a variable to save the header
	//fprintf(wheretoprint,"%s",buf);	
            break;
	}
    }
    
    /* examine return code */   
    if((index=headers.find("200 OK"))==string::npos){
	//not ok
	ok = false;
	wheretoprint=stderr;
	fprintf(wheretoprint,headers.c_str());
	fprintf(wheretoprint,"\r\n\r\n%s",response.c_str());
	}
    else{
	ok = true;
	fprintf(wheretoprint,headers.c_str());
	fprintf(wheretoprint,"\r\n\r\n%s",response.c_str());
	}
    //Skip "HTTP/1.0"
    //remove the '\0'
    // Normal reply has return code 200

    /* print first part of response */

    /* second read loop -- print out the rest of the response */
    while((n = minet_read(sock, buf, BUFSIZE))>0){
	buf[n]='\0';
	//while there is response left to read
	//read until the end of the headers
	if(ok){
	//store buf in a variable to save the header
		fprintf(wheretoprint,"%s",buf);	
            //break;
        }else{
		fprintf(stderr,"%s",buf);
	}
    }

    /*close socket and deinitialize */
    //fprintf(stderr, "g");
    if(minet_close(sock)!=0)
    {
        fprintf(stderr, "close");
        ok=false;
    }
    if(minet_deinit()!=0)
    {
        fprintf(stderr, "deinit");
        ok=false;
    }

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


