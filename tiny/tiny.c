/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); //듣기 소켓을 오픈
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept // 반복적으로 연결 요청을 접수하고
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,0); 
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit //트랜잭션 시작
    Close(connfd);  // line:netp:tiny:close //연결 끊음
  }
}
void doit(int fd)
{
  int is_static; 
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd); //rio 구조체를 초기화 시킨다
  Rio_readlineb(&rio, buf, MAXLINE); //요청헤드를 buf에 읽어온다
  printf("Request headers: \n"); 
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri,version); // 요청 헤더에서 메스드, url및 버전을 추출한다
  if(strcasecmp(method, "GET"))
  {
    clienterror(fd,filename,"404","Not found","Tiny couldn't find this file"); //404오류를 클라이언트에게 반환하고 함수를 종료합니다
    return;
  }
  if (is_static) // 정적파일이면 
  {
    if(!(S_ISREG(sbuf.st_mode))|| !(S_IRUSR &sbuf.st_mode)) //sbuf의 파일 모드가 일반파일이 아니거나 사용자에게 읽을 권한이 없는 경우
    {
      clienterror(fd,filename,"403","Forbidden","Tiny couldn't read the file");
      return;
    }
    serve_dynamic(fd,filename,cgiargs); // 동적 파일처리
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg){
  char buf[MAXLINE],body[MAXBUF];
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

/* http 요청 헤더를 읽어들이는 함수 */
/* doit함수에서 호출되어 요청 헤더를 읽어들이는 부분을 담당 */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE]; //요청 헤더라인을 저장한느 문자열 버퍼
  Rio_readlineb(rp,buf,MAXLINE); //rp가 가리키는 rio_t 구조체로부터 요청 헤더라인을 읽어 buf에 저장한다.
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp,buf,MAXLINE); //Rio_readlineb 함수를 사용하여 다음 요청 헤더라인을 읽음
    printf("%s",buf) // 읽어들인 요청 헤더 라인을 화면에 출력
  }
}

/*doit 함수에서 호출되어 요청된 url를 파싱하여 파일 경로와 cgl인자를 추출하는 역할을 한다.*/
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  if (!strstr(uri, "cgi-bin")){
    strcpy(cgiargs, "");
    strcpy(filename,"."); // .로 초기화
    strcat(filename,uri); // uri를 이어 붙인다.
    if (uri[strlen(uri) - 1] =='/')
      strcat(filename, home.html) // 마지막 문자가 '/'면 home.html을 이어 붙인다.
    return 1;
  }
  else{
    ptr = index(uri ,'?'); // ?를 찾는다
    if(ptr){// 문자를 찾는다면
      strcpy(cgiargs,ptr +1); // cgiargs에 ptr 다음 문자부터 끝까지의 문자열을 복사한다.
      *ptr ='\0'; //ptr \0로 설정하여 uri문자열을 종료한다.
    }
    else
      strcpy(cgiargs,""); // 빈문자로 초기화 해버림

    strcpy(filename,".");
    strcat(filename,uri); // 이어 붙인다.
    return 0;
  }
}
void serve_static(int fd, char *filename, int filesize){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename,filetype);

  /*sprintf --> 함수는 형식화된 문자열을 생성하여 지정된 버퍼에 저장, 
  생성된 문자열은 지정된 버퍼에 저장되기 때문에 특정 파일이나 화면에 직접 출력되지 않습니다.
  대신, 문자열을 변수에 저장하거나 다른 함수로 전달하는 용도로 사용*/

  /*client에게 헤더 전송*/
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server \r\n", buf);
  sprintf(buf, "%sConnection: close\r\n",buf);
  sprintf(buf, "%sContent-length: %d\r\n",buf,filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n",buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s",buf);
  
  /*client에게 body 전송*/
  srcfd = Open(filename, O_RDONLY,0);
  srcp = Mmap(0,filesize,PROT_READ,MAP_PRIVATE,srcfd,0);
  Close(srcfd);
  Rio_writen(fd,srcp,filesize);
  Munmap(srcp,filesize);
}
void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename,".png"))
    strcpy(filetype,"image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype , "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] ={NULL};

  sprintf(buf, "HTTP/1.0 200 OK \r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd,buf,strlen(buf));

  if(Fork()==0){
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd,STDERR_FILENO);
    Execve(filename , emptylist, environ);
  }
  Wait(NULL);
}
