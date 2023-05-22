# include "csapp.h"

void main(int argc, char **argv){

    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage : %s <port> \n", argv[0]);
        exit(0);
    }

    listenfd = open_listenfd(argv[1]); //지정된 포트 번호를 가진 소캣을 생성하고, 연결 대기 상태로 설정한 후, 리스닝 소캣의 파일 디스크립터를 가져옴
    while (1){
        clientlen = sizeof(struct sockaddr_storage); 
        connfd = Accept(listenfd ,(SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); //클라이언트의 호스트 이름과 포트 번호를 받아옴
        printf("Connected to (%s, %s)\n", client_hostname,client_port); // 위에서 받아온 호스트 이름이랑 포트 번호를 받아옴
        echo(connfd); // echo 함수 호출, 클라이언트와의 에코통신을 수행한다.
        Close(connfd); // 클라이언트와의 통신이 완료되면 클라이언트 소캣을 닫는다.
    }
    exit(0); //종료
}