#include "csapp.h"

int main(int argc, char **argv){
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    int rc, flags;

    if (argc !=2){
        fprintf(stderr, "usage: %s <domain name>\n",argv[0]);
        exit(0);
    }

    memset(&hints, 0, sizeof(struct addrinfo)); //memset 함수를 사용하여 hints 구조체를 0으로 초기화한다.
    hints.ai_family = AF_INET; //ai_family를 AF_INET로 설정하여 "IPv4" 주소 체계를 사용
    hints.ai_socktype = SOCK_STREAM; // ai_socktype은 SOCK_STREAM으로 설정해서 TCP 소켓을 사용한다.
    if((rc = getaddrinfo(argv[1],NULL, &hints,&listp))!=0){ // getaddrinfo 함수를 호출하였고 argv[1]에 대한 ip주소 정보를 받아옴.
        // getaddrinfo 이거 에러 나면 error 출력
        fprintf(stderr, "getaddrinfo error: %s\n",gai_strerror(rc)); 
        exit(1);
    }
    flags = NI_NUMERICHOST; // 숫자형 호스트 주소를 출력하기 위한 것.

    for (p=listp; p; p= p->ai_next){ //listp 포인터를 순회하면서 각 IP주소에 대한 숫자형 호스트 주소를 받아온다.
        Getnameinfo(p->ai_addr,p->ai_addrlen, buf, MAXLINE ,NULL,0,flags); // ai_addr에서 호스트 주소를 얻어온다.
        printf("%s\n",buf); // 얻은 주소를 buf에 저장하고 출력
    }
    Freeaddrinfo(listp); //메모리 free 해버리기 
    exit(0); //종료
}