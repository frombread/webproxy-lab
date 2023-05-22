#include "csapp.h"

/*클라이언트와의 연결된 소캣으로부터 데이터를 읽어와 에코하는 역할을 수행한다.*/
void echo(int connfd){
    size_t n;
    char buf[MAXLINE];
    rio_t rio; // 버퍼링된 I/O를 지원하는 rio_t구조체 --> 진짜뭔지 모름

    Rio_readinitb(&rio, connfd); //rio를 초기화 한다, rio는 connfd 소켓과 연결되어 입출력 작업을 수행할 수 있게 됨
    while((n = Rio_readlineb(&rio,buf, MAXLINE)) != 0){
        printf("server received %d bytes\n",(int)n); // 읽은 바이트 수를 프린트
        Rio_writen(connfd,buf,n); // buf에 있는 데이터를 connfd 소캣으로 전송한다. 이를 통해 "클라이언트에게 데이터를 에코한다?" -->뭔말이고 ㅠ
    }
}