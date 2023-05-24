/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
	doit(connfd);                                             //line:netp:tiny:doit
	Close(connfd);                                            //line:netp:tiny:close
    }
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) 
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) //rio에서 최대 MAXLINE만큼의 문자열을 읽어 buf에 저장
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);  //buf에서 method, uri, version 
    if (strcasecmp(method, "GET")) {          //method가 GET이 아니면
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method"); // 클라이언트한테 전송하고 함수 종료
        return;
    }                                                    //line:netp:doit:endrequesterr
    read_requesthdrs(&rio);    // rio를 통해 요청 헤더를 읽고 처리

    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);      
    if (stat(filename, &sbuf) < 0) {                    //권한여부 확인
	clienterror(fd, filename, "404", "Not found",
		    "Tiny couldn't find this file");
	return;
    }                                                    //line:netp:doit:endnotfound

    if (is_static) { /* Serve static content */          
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	    return;
	}
	serve_static(fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
    }
    else { /* Serve dynamic content */
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { //line:netp:doit:executable
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	    return;
	}
	serve_dynamic(fd, filename, cgiargs);            //line:netp:doit:servedynamic
    }
}
/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE]; 

    Rio_readlineb(rp, buf, MAXLINE); //rp에서 최대라인만큼 문자열을 읽어 buf에 저장
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) { //http요청 헤더의 끝이랑 buf랑 다르다면 계속해서 헤더를 읽고 출력반복
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */ 
	strcpy(cgiargs, "");                             
	strcpy(filename, ".");      // .로 초기화        
	strcat(filename, uri);      // uri를 이어 붙인다.
	if (uri[strlen(uri)-1] == '/')                  
	    strcat(filename, "home.html");  // 마지막 문자가 '/'면 home.html을 이어 붙인다.         
	return 1;
    }
    else {  /* Dynamic content */                        
	ptr = index(uri, '?');   // ?를 찾는다                 
	if (ptr) {// 문자를 찾는다면
	    strcpy(cgiargs, ptr+1);// cgiargs에 ptr 다음 문자부터 끝까지의 문자열을 복사한다.
	    *ptr = '\0'; //ptr \0로 설정하여 uri문자열을 종료한다.
	}
	else 
	    strcpy(cgiargs, "");    // 빈문자로 초기화 해버림 
	strcpy(filename, ".");                          
	strcat(filename, uri);      // 이어 붙인다.     
	return 0;
    }
}
/* $end parse_uri */

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize) 
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
 
    /* Send response headers to client */
    /*응답 라인과 헤더 작성*/
    get_filetype(filename, filetype);      // 파일 타입 찾아오기
    sprintf(buf, "HTTP/1.0 200 OK\r\n");    // 응답 라인 작성
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf); // 응답 헤더 작성
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);

    /*응답 라인과 헤더를 클라이언트에게 보냄*/
    Rio_writen(fd, buf, strlen(buf));       //connfd를 통해 clientfd에게 보냄
    printf("Response headers:\n");   
    printf("%s", buf); // 서버측에서도 출력한다.

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);    //filename의 이름을 갖는 파일을 읽기 권한으로 불러온다.
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);//line:netp:servestatic:mmap
    srcp = (char *)Malloc(filesize); //파일 크기만큼의 메모리를 동적할당한다.
    Rio_readn(srcfd,srcp,filesize); // filename 내용을 동적할당한 메모리에 쓴다.
    Close(srcfd); //파일을 닫는다                           //line:netp:servestatic:close
    Rio_writen(fd, srcp, filesize); // 해당 메모리에 있는 파일 내용들을 클라이언트에 보낸다.         //line:netp:servestatic:write
    // Munmap(srcp, filesize);                 //line:netp:servestatic:munmap
    free(srcp);
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else if (strstr(filename,".mpeg"))
    strcpy(filetype,"video/MPG");
    else
	strcpy(filetype, "text/plain");
}  
/* $end serve_static */

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    if (Fork() == 0) { /* Child */ //line:netp:servedynamic:fork
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
	Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
	Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}
/* $end serve_dynamic */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */
