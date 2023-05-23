#include "csapp.h"
/*rio 흐름?
1. 서버와 통신이 시작된 뒤, 우리가 입력을 넣으면 (네트워크용)버퍼에 저장. (fgets())
2. 버퍼에 저장된 데이터를 fd(파일 디스크립터)로 상대방 버퍼에 넘긴다.
(그러면 서버는 이 데이터를 받고 무언가 작업을 한 뒤, 결과 텍스트를 넘겨줄 것이다. 그리고 fd를 통해서 서버로부터 온 데이터들이 클라이언트 쪽 버퍼에 저장된다.)
3. 버퍼에 저장되어있는, 서버로부터 온 데이터를 읽는다.
4. 데이터를 출력한다. */


/*클라이언트와의 연결된 소캣으로부터 데이터를 읽어와 에코하는 역할을 수행한다.*/
void echo(int connfd){
    size_t n;
    char buf[MAXLINE];
    rio_t rio; 

    Rio_readinitb(&rio, connfd); //rio를 초기화 한다, rio는 connfd 소켓과 연결되어 입출력 작업을 수행할 수 있게 됨
    while((n = Rio_readlineb(&rio,buf, MAXLINE)) != 0){
        printf("server received %d bytes\n",(int)n); // 읽은 바이트 수를 프린트
        Rio_writen(connfd,buf,n); // buf에 있는 데이터를 connfd 소캣으로 전송한다. 이를 통해 "클라이언트에게 데이터를 에코한다?" -->뭔말이고 ㅠ
    }
}