#include "msocket.h"

int main(){
    int sd = m_socket(AF_INET, SOCK_MTP, 0);
    printf("Socket created with id: %d\n", sd);
    struct sockaddr_in saddr, daddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8002);
    saddr.sin_addr.s_addr = INADDR_ANY;
    daddr.sin_family = AF_INET;
    daddr.sin_port = htons(8003);
    daddr.sin_addr.s_addr = INADDR_ANY;
    int bd = m_bind(sd, (struct sockaddr *)&saddr, sizeof(saddr), (struct sockaddr *)&daddr, sizeof(daddr));
    printf("Bind status: %d\n", bd);
    FILE *fp = fopen("Frankenstein.txt", "r");
    char buf[1024];
    int n = 0;
    while(!feof(fp)){
        memset(buf, 0, 1024);
        fread(buf, sizeof(char), 1023, fp);
        n++;
        int sb = m_sendto(sd, buf, strlen(buf), 0, (struct sockaddr *)&daddr, sizeof(daddr));
        while(sb == -1){
            sleep(3);
            sb = m_sendto(sd, buf, strlen(buf), 0, (struct sockaddr *)&daddr, sizeof(daddr));
        }
        printf("Send status: %d %d\n", sb,n);
    }
    if(m_close(sd) == -1){
        perror("Socket closed\n");
    }
    printf("Total messages sent on socket %d: %d\n",sd, n);
    fclose(fp);
    return 0;
}