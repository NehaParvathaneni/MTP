#include "msocket.h"

int main(){
    int sd = m_socket(AF_INET, SOCK_MTP, 0);
    printf("Socket created with id: %d\n", sd);
    struct sockaddr_in saddr, daddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8003);
    saddr.sin_addr.s_addr = INADDR_ANY;
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(8002);
    daddr.sin_addr.s_addr = INADDR_ANY;
    int bd = m_bind(sd, (struct sockaddr *)&saddr, sizeof(saddr), (struct sockaddr *)&daddr, sizeof(daddr));
    printf("Bind status: %d\n", bd);
    char buf[1024];
    FILE *fp = fopen("Frankenstein_dup.txt", "w");
    int n = 0;
    while(1){
        memset(buf, 0, 1024);
        int len = sizeof(saddr);
        int rb = m_recvfrom(sd, buf, 1024, 0, (struct sockaddr *)&saddr, &len);
        while( rb == -1){
            sleep(3);
            rb = m_recvfrom(sd, buf, 1024, 0, (struct sockaddr *)&saddr, &len);
        }
        if(rb != -1){
            printf("Recv status: %d\n", rb);
            fwrite(buf, sizeof(char), strlen(buf), fp);
            n++;
            if(strlen(buf) < 1023){
                break;
            }
        }
    }
    fclose(fp);
    printf("Total messages received on socket %d: %d\n",sd, n);
    if(m_close(sd) == -1){
        perror("Socket closed\n");
    }
}
