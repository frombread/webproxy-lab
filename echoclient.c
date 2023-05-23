#include "csapp.h"

int main(int argc, char **argv){

    int clientfd;
    char *host, *port,buf[MAXLINE];
    rio_t rio; 

    if (argc != 3){ //명령행 인수의 개수가 3이 아닌 경우, 출력 -> 프로그램 종료
        fprintf(stderr, "usage: %s <host> <port> \n" ,argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = open_clientfd(host,port); //open_clientfd 함수 호출해서, 서버에 연결된 클라이언트 소캣의 파일 디스크립터 가져옴.
    Rio_readinitb(&rio, clientfd); //Rio_readinitb 함수로 rio 구조체를 초기화 한다. rio 구조체는 버퍼링된 I/O를 지원하는 라이브러리

    while (Fgets(buf, MAXLINE, stdin) != NULL){
        Rio_writen(clientfd,buf, strlen(buf)); //buf의 내용을 서버로 보냄
        Rio_readlineb(&rio, buf ,MAXLINE); // 서버로부터 응답을 읽어옴
        Fputs(buf, stdout); //표준출력에 출력한다.????? 뭔말인지 모르겠
    }
    Close(clientfd); // 클라이언트 소캣을 닫는다
    exit(0); //종료
}