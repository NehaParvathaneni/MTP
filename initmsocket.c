#include "msocket.h"

sockinfo *si;
SM *sm;

int recvsem;
int sendsem;
int stop = 0;
int sem1,sem2;

void sighandler(){
    printf("\nStopping server...\n");
    stop = 1;
    struct sembuf sop, vop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = 0;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    semop(sem1, &vop, 1);
}

void *G_Collector(){
    while(1){
        sleep(T/2);
        if(stop){
            for(int i=0;i<25;i++){
                if(sm[i].smvalid){
                    kill(sm[i].pid, SIGINT);
                    close(sm[i].sockid);
                }
            }
            pthread_exit(NULL);
        }
        for(int i=0;i<25;i++){
            if(sm[i].smvalid == 1){
                if(kill(sm[i].pid, 0) != 0){
                    if(sm[i].isclosed == 0 || (sm[i].isclosed && (sm[i].swnd.cnt == 0 || (sm[i].swnd.cnt != 0 && (time(NULL) - sm[i].swnd.lastsent) >= 20*T)))){
                        sm[i].isclosed = 1;
                        printf("closing socket %d\n",i);
                        printf("No of packets sent on socket %d: %d\n",i, sm[i].no_pacs_sent);
                        close(sm[i].sockid);
                        sm[i].smvalid = 0;
                        memset(sm[i].sip, 0, INET_ADDRSTRLEN);
                        sm[i].sport = 0;
                        memset(sm[i].dip, 0, INET_ADDRSTRLEN);
                        sm[i].dport = 0;
                        memset(sm[i].s, 0, 10*1034);
                        memset(sm[i].r, 0, 5*1024);
                        memset(sm[i].valid, 0, 5);
                        sm[i].swnd.cnt = 0;
                        sm[i].swnd.front = 0;
                        //sm[i].swnd.rear = 0;
                        sm[i].swnd.lastsent = 0;
                        sm[i].swnd.seqno = 0;
                        sm[i].swnd.winstart = 0;
                        memset(&sm[i],0,sizeof(SM));
                    }
                }
            }
        }
    }
}

void *R_routine(){
    struct sembuf sop,vop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = 0;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    int spaceavl[25];
    for(int i = 0;i<25;i++){
        spaceavl[i] = 0;
    }
    while(1){
        int i;
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        fd_set readfds;
        FD_ZERO(&readfds);
        int max = 0;
        for(i=0; i<25; i++){
            if(sm[i].smvalid == 1){
                if(sm[i].dport != 0){
                    FD_SET(sm[i].sockid, &readfds);
                    if(sm[i].sockid > max){
                        max = sm[i].sockid;
                    }
                }
            }
        }
        int sret = select(max+1, &readfds, NULL, NULL, &timeout);
        if(stop){
            pthread_exit(NULL);
        }
        if(sret == -1){
            perror("select");
        }
        for(i=0; i<25; i++){
            if(sm[i].smvalid == 1){
                if(sm[i].dport != 0){
                    if(FD_ISSET(sm[i].sockid, &readfds)){
                        printf("Received packet on socket %d ", sm[i].sockid);
                        struct sockaddr_in saddr;
                        int len = sizeof(saddr);
                        char buff[KB + 10];
                        memset(buff, 0, KB + 10);
                        int rb = recvfrom(sm[i].sockid, buff, KB+10, 0, (struct sockaddr *)&saddr, &len);
                        if(dropMessage(P)){
                            printf("Dropped packet.\n");
                            continue;
                        }
                        if(rb == -1){
                            perror("recvfrom");
                        }else{
                            int type = -1;
                            int seqno = -1;
                            char *data;
                            data = (char *)malloc(KB);
                            memset(data,0,1024);
                            sscanf(buff, "%d %d ", &type, &seqno);
                            int j;
                            int k = 0;
                            for(j=0; j<strlen(buff); j++){
                                if(buff[j] == ' '){
                                    k++;
                                }
                                if(k == 2){
                                    break;
                                }
                            }
                            strcpy(data, buff+j+1);
                            if(type == DATA){
                                printf("%d %d %ld\n",type, seqno, strlen(data));
                                sop.sem_num = i;
                                semop(recvsem, &sop, 1);
                                if(seqno == sm[i].rwnd.seqno && sm[i].rwnd.nospace == 0){
                                    spaceavl[i] = 0;
                                    strcpy(sm[i].r[sm[i].rwnd.front], data);
                                    sm[i].valid[sm[i].rwnd.front] = 1;
                                    while(1){
                                        sm[i].rwnd.front = (sm[i].rwnd.front+1)%sm[i].rwnd.size;
                                        sm[i].rwnd.seqno++;
                                        sm[i].rwnd.seqno = sm[i].rwnd.seqno%16;
                                        sm[i].rwnd.cnt = sm[i].rwnd.cnt+1;
                                        if(sm[i].rwnd.cnt == sm[i].rwnd.size){
                                            spaceavl[i] = 1;
                                            sm[i].rwnd.nospace = 1;
                                            break;
                                        }
                                        if(sm[i].valid[sm[i].rwnd.front] == 0){
                                            break;
                                        }
                                    }
                                    //printf("%d %d %d %d %d\n", sm[i].rwnd.seqno, sm[i].rwnd.front, sm[i].rwnd.rear, sm[i].rwnd.cnt, sm[i].rwnd.nospace);
                                    memset(data, 0, KB);
                                    int n_emp = 0;
                                    for(int ze=0;ze<5;ze++){
                                        if(sm[i].valid[ze]) continue;
                                        n_emp++;
                                    }
                                    sprintf(data, "%d %d %d", ACK, (sm[i].rwnd.seqno + 15)%16, n_emp);
                                    int sb = sendto(sm[i].sockid, data, strlen(data), 0, (struct sockaddr *)&saddr, sizeof(saddr));
                                    if(sb == -1){
                                        perror("sendto ack");
                                    }
                                }else{
                                    int temp = sm[i].rwnd.front;
                                    for(int iter = 1;iter<5;iter++){
                                        if((seqno == (sm[i].rwnd.seqno+iter)%16)&&(sm[i].valid[(temp+iter)%5] == 0)){
                                            strcpy(sm[i].r[(temp+iter)%5], data);
                                            sm[i].valid[(temp+iter)%5] = 1;
                                            spaceavl[i] = 0;
                                            break;
                                        }
                                    }
                                    int n_emp = 0;
                                    for(int ze=0;ze<5;ze++){
                                        if(sm[i].valid[ze]) continue;
                                        n_emp++;
                                    }
                                    memset(data, 0, KB);
                                    sprintf(data, "%d %d %d", ACK, (sm[i].rwnd.seqno + 15)%16,n_emp);
                                    int sb = sendto(sm[i].sockid, data, strlen(data), 0, (struct sockaddr *)&saddr, sizeof(saddr));
                                    if(sb == -1){
                                        perror("sendto out of order msg ack");
                                    }
                                }
                                vop.sem_num = i;
                                semop(recvsem, &vop, 1);
                            } else if(type == ACK){
                                printf("%s\n", buff);
                                sop.sem_num = i;
                                int winlen;
                                sscanf(buff,"%d %d %d",&type,&seqno,&winlen);
                                semop(sendsem, &sop, 1);
                                int sum = 0;
                                int flag = 0;
                                for(int iter = 0;iter<5;iter++){
                                    sum++;
                                    if(seqno == (sm[i].swnd.winseq+iter)%16){
                                        flag = 1;
                                        break;
                                    }
                                }
                                if(flag){
                                sm[i].swnd.winstart = (sm[i].swnd.winstart+sum)%sm[i].swnd.size;
                                sm[i].swnd.cnt = sm[i].swnd.cnt - sum;
                                sm[i].swnd.winseq = (sm[i].swnd.winseq+sum)%16;
                                sm[i].swnd.winsize = winlen;
                                }
                                vop.sem_num = i;
                                semop(sendsem, &vop, 1);
                            }
                            else if(type == DUP){
                                printf("%s\n", buff);
                                sop.sem_num = i;
                                int winlen;
                                sscanf(buff, "%d %d %d", &type, &seqno, &winlen);
                                semop(sendsem, &sop, 1);
                                sm[i].swnd.winsize = winlen;
                                vop.sem_num = i;
                                semop(sendsem, &vop, 1);
                                spaceavl[seqno] = 0;
                            }
                        }
                    }
                }
            }
        }
        for(i=0;i<25;i++){
            if(spaceavl[i]){
                sop.sem_num = i;
                semop(recvsem,&sop,1);
                if(!sm[i].rwnd.nospace && sm[i].smvalid == 1 && sm[i].dport != 0){
                    struct sockaddr_in saddr;
                    memset(&saddr, 0, sizeof(saddr));
                    saddr.sin_family = AF_INET;
                    saddr.sin_port = htons(sm[i].dport);
                    inet_aton(sm[i].dip, &saddr.sin_addr);
                    char sendit[KB];
                    memset(sendit, 0, KB);
                    int n_emp = 0;
                    for(int ze=0;ze<5;ze++){
                        if(sm[i].valid[ze]) continue;
                        n_emp++;
                    }
                    sprintf(sendit, "%d %d %d",DUP, i, n_emp);
                    int sb = sendto(sm[i].sockid, sendit, strlen(sendit), 0, (struct sockaddr *)&saddr, sizeof(saddr));
                    if(sb == -1){
                        perror("sendto dup ack");
                    }
                }
                vop.sem_num = i;
                semop(recvsem,&vop,1);
            }
        }
    }
}

void *S_routine(){
    struct sembuf sop,vop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = 0;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    char temporbuf[1034];
    while(1){
        sleep(T/2);
        if(stop){
            pthread_exit(NULL);
        }
        int i;
        for(i=0; i<25; i++){
            if(sm[i].smvalid == 1){
                if(sm[i].dport != 0){
                    if(sm[i].swnd.cnt == 0) continue;
                    time_t t;
                    time(&t);
                    sop.sem_num = i;
                    semop(sendsem, &sop, 1);
                    if(sm[i].swnd.lastsent == 0 || (t - sm[i].swnd.lastsent) >= T){
                        struct sockaddr_in daddr;
                        int sb;
                        daddr.sin_family = AF_INET;
                        daddr.sin_port = htons(sm[i].dport);
                        inet_aton(sm[i].dip, &daddr.sin_addr);
                        int j;
                        for(j=0; j < sm[i].swnd.winsize; j++){
                            int k = (sm[i].swnd.winstart + j)%sm[i].swnd.size;
                            if(j<sm[i].swnd.cnt){
                                memset(temporbuf, 0, 1034);
                                sprintf(temporbuf, "%d %d %s", DATA, sm[i].seqs[k], sm[i].s[k]);
                                sb = sendto(sm[i].sockid, temporbuf, strlen(temporbuf), 0, (struct sockaddr *)&daddr, sizeof(daddr));
                                if(sb == -1){
                                     perror("sendto");
                                }else{
                                    sm[i].swnd.winsent = k;
                                    time(&sm[i].swnd.lastsent);
                                    sm[i].no_pacs_sent++;
                                }     
                            }
                        }
                    }else{
                        struct sockaddr_in daddr;
                        int sb;
                        daddr.sin_family = AF_INET;
                        daddr.sin_port = htons(sm[i].dport);
                        inet_aton(sm[i].dip, &daddr.sin_addr);
                        int j;
                        for(j=0; j< sm[i].swnd.winsize; j++){
                            int k = (sm[i].swnd.winstart + j)%sm[i].swnd.size;
                            if(k != (sm[i].swnd.winsent + 1)%sm[i].swnd.size){
                                continue;
                            }
                            if(j<sm[i].swnd.cnt){
                                memset(temporbuf, 0, 1034);
                                sprintf(temporbuf, "%d %d %s", DATA, sm[i].seqs[k], sm[i].s[k]);
                                sb = sendto(sm[i].sockid, sm[i].s[k], strlen(sm[i].s[k]), 0, (struct sockaddr *)&daddr, sizeof(daddr));
                                if(sb == -1){
                                     perror("sendto");
                                }else{
                                    sm[i].swnd.winsent = k;
                                    time(&sm[i].swnd.lastsent);
                                    sm[i].no_pacs_sent++;
                                }     
                            }
                        }
                    }
                    sm[i].swnd.winseq = sm[i].seqs[sm[i].swnd.winstart];
                    vop.sem_num = i;
                    semop(sendsem, &vop, 1);
                }
            }
        }
    }
}


int main(){
    int smshm = shmget(ftok(".", 3000), 25*sizeof(SM), 0777|IPC_CREAT);
    int sishm = shmget(ftok(".", 4000), sizeof(sockinfo), 0777|IPC_CREAT);
    printf("Server started.\n");
    si = (sockinfo *)shmat(sishm, 0, 0);
    sm = (SM *)shmat(smshm, 0, 0);
    memset(sm, 0, 25*sizeof(SM));
    int i;
    for(i=0; i<25; i++){
        sm[i].smvalid = 0;
    }
    si->sockid = 0;
    strcpy(si->ip,"");
    si->port=0;
    si->err=0;
    sem1 = semget(ftok(".", 5000), 1, 0777|IPC_CREAT);
    sem2 = semget(ftok(".", 6000), 1, 0777|IPC_CREAT);
    recvsem = semget(ftok(".", 7000), 25, 0777|IPC_CREAT);
    sendsem = semget(ftok(".", 8000), 25, 0777|IPC_CREAT);
    semctl(sem1, 0, SETVAL, 0);
    semctl(sem2, 0, SETVAL, 0);
    for(int i=0;i<25;i++){
    semctl(recvsem, i, SETVAL, 1);
    semctl(sendsem, i, SETVAL, 1);
    }
    struct sembuf sop, vop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = 0;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    pthread_t R, S, G;
    pthread_create(&R, NULL, R_routine, NULL);
    pthread_create(&S, NULL, S_routine, NULL);
    pthread_create(&G, NULL, G_Collector, NULL);
    signal(SIGINT, sighandler);
    while(1){
        semop(sem1, &sop, 1);
        if(stop){
            pthread_join(R, NULL);
            pthread_join(S, NULL);
            pthread_join(G, NULL);
            shmdt(si);
            shmdt(sm);
            semctl(sem1, 0, IPC_RMID, 0);
            semctl(sem2, 0, IPC_RMID, 0);
            semctl(recvsem, 0, IPC_RMID, 0);
            semctl(sendsem, 0, IPC_RMID, 0);
            shmctl(smshm, IPC_RMID, 0);
            shmctl(sishm, IPC_RMID, 0);
            break;
        }
            if((si->sockid == 0)&&(si->port==0)){
                int sid = socket(AF_INET, SOCK_DGRAM, 0);
                si->sockid = sid;
                if(sid == -1){
                    si->err = errno;
                    perror("socket creation.");
                }

            }else{
                struct sockaddr_in saddr;
                saddr.sin_family = AF_INET;
                saddr.sin_port = htons(si->port);
                saddr.sin_addr.s_addr = inet_addr(si->ip);
                if((bind(si->sockid, (struct sockaddr *)&saddr, sizeof(saddr))) != 0){
                    si->err = errno;
                    perror("bind");
                }
            }
        semop(sem2, &vop, 1);
    }
    printf("Server stopped.\n");
    return 0;
}