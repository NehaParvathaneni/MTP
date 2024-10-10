#include "msocket.h"

int main(){
    int sd = m_socket(AF_INET, SOCK_MTP, 0);
    printf("Socket created with id: %d\n", sd);
    struct sockaddr_in saddr, daddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8000);
    saddr.sin_addr.s_addr = INADDR_ANY;
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(8001);
    daddr.sin_addr.s_addr = INADDR_ANY;
    int bd = m_bind(sd, (struct sockaddr *)&saddr, sizeof(saddr), (struct sockaddr *)&daddr, sizeof(daddr));
    printf("Bind status: %d\n", bd);
    for(int i=0;i<50;i++){
        char buf[100];
        memset(buf, 0, 100);
        sprintf(buf, "Hello %d", i);
        int sb = m_sendto(sd, buf, strlen(buf), 0, (struct sockaddr *)&daddr, sizeof(daddr));
        while(sb == -1){
            sleep(3);
            sb = m_sendto(sd, buf, strlen(buf), 0, (struct sockaddr *)&daddr, sizeof(daddr));
        }
        printf("Send status: %d\n", sb);
    }
    if(m_close(sd) == -1){
        perror("Socket closed\n");
    }
    return 0;
}