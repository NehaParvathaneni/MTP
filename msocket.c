#include"msocket.h"

int m_socket(int domain, int type, int protocol)
{
    if(type != SOCK_MTP){
        errno = EPROTOTYPE;
        perror("socket creation.");
        return errno;
    }
    int smshm = shmget(ftok(".", 3000), 25*sizeof(SM), 0777|IPC_CREAT);
    int sishm = shmget(ftok(".", 4000), sizeof(sockinfo), 0777|IPC_CREAT);
    sockinfo *si = (sockinfo *)shmat(sishm, 0, 0);
    int sem1 = semget(ftok(".", 5000), 1, 0777|IPC_CREAT);
    int sem2 = semget(ftok(".", 6000), 1, 0777|IPC_CREAT);
    struct sembuf sop, vop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = 0;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    SM *sm;
    sm = (SM *)shmat(smshm, 0, 0);
    int i;
    for(i=0; i<25; i++){
        if(sm[i].smvalid == 0) break;
    }
    if(i==25){
        errno = ENOBUFS;
        return -1;
    }
    semop(sem1, &vop, 1);
    semop(sem2, &sop, 1);
    if(si->sockid == -1){
        si->sockid = 0;
        si->port = 0;
        strcpy(si->ip, "");
        errno = si->err;
        si->err = 0;
        perror("socket creation.");
        return errno;
    }
    sm[i].smvalid = 1;
    sm[i].sockid = si->sockid;
    si->sockid = 0;
    si->port = 0;
    strcpy(si->ip, "");
    shmdt(si);
    shmdt(sm);
    return i;
}

int m_bind(int sockfd, const struct sockaddr *saddr, socklen_t saddrlen, const struct sockaddr *daddr, socklen_t daddrlen)
{
    int smshm = shmget(ftok(".", 3000), 25*sizeof(SM), 0777|IPC_CREAT);
    int sishm = shmget(ftok(".", 4000), sizeof(sockinfo), 0777|IPC_CREAT);
    sockinfo *si = (sockinfo *)shmat(sishm, 0, 0);
    int sem1 = semget(ftok(".", 5000), 1, 0777|IPC_CREAT);
    int sem2 = semget(ftok(".", 6000), 1, 0777|IPC_CREAT);
    struct sembuf sop, vop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = 0;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    SM *sm;
    sm = (SM *)shmat(smshm, 0, 0);
    si->sockid = sm[sockfd].sockid;
    si->port = ntohs(((struct sockaddr_in *)saddr)->sin_port);
    strcpy(si->ip,inet_ntoa(((struct sockaddr_in *)saddr)->sin_addr));
    semop(sem1, &vop, 1);
    semop(sem2, &sop, 1);
    if(si->err != 0){
        errno = si->err;
        si->err = 0;
        si->sockid = 0;
        si->port = 0;
        strcpy(si->ip, "");
        perror("bind");
        return errno;
    }
    sm[sockfd].pid = getpid();
    sm[sockfd].sport = si->port;
    strcpy(sm[sockfd].sip, si->ip);
    sm[sockfd].dport = ntohs(((struct sockaddr_in *)daddr)->sin_port);  
    strcpy(sm[sockfd].dip, inet_ntoa(((struct sockaddr_in *)daddr)->sin_addr));
    sm[sockfd].swnd.cnt = 0;
    sm[sockfd].swnd.front = 0;
    sm[sockfd].swnd.size = 10;
    //sm[sockfd].swnd.rear = 0;
    sm[sockfd].swnd.lastsent = 0;
    sm[sockfd].swnd.seqno = 0;
    sm[sockfd].swnd.winstart = 0;
    sm[sockfd].swnd.winsent = -1;
    sm[sockfd].swnd.winsize = 5;
    sm[sockfd].rwnd.cnt = 0;
    sm[sockfd].rwnd.front = 0;
    sm[sockfd].rwnd.size = 5;
    sm[sockfd].rwnd.rear = 0;
    sm[sockfd].rwnd.nospace = 0;
    sm[sockfd].isclosed = 0;
    sm[sockfd].no_pacs_sent = 0;
    for(int i=0; i<5; i++) sm[sockfd].valid[i] = 0;
    si->err = 0;
    si->sockid = 0;
    si->port = 0;
    strcpy(si->ip, "");
    shmdt(si);
    shmdt(sm);
    return 0;
}

int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    int smshm = shmget(ftok(".", 3000), 25*sizeof(SM), 0777|IPC_CREAT);
    int sendsem = semget(ftok(".", 8000), 1, 0777|IPC_CREAT);
    SM *sm;
    sm = (SM *)shmat(smshm, 0, 0);
    if(sm[sockfd].pid != getpid() || sm[sockfd].isclosed){
        errno = EPERM;
        perror("sendto");
        return -1;
    }
    if((sm[sockfd].dport != ntohs(((struct sockaddr_in *)dest_addr)->sin_port))&&(strcmp(sm[sockfd].dip, inet_ntoa(((struct sockaddr_in *)dest_addr)->sin_addr))==0)){
        errno = ENOTCONN;
        perror("sendto");
        return -1;
    }
    if(sm[sockfd].swnd.cnt == sm[sockfd].swnd.size){
        errno = ENOBUFS;
        perror("sendto");
        return -1;
    }
    struct sembuf sop, vop;
    sop.sem_num = sockfd;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = sockfd;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    //printf("Sending packet %d\n", sm[sockfd].swnd.seqno);
    semop(sendsem, &sop, 1);
    memset(sm[sockfd].s[sm[sockfd].swnd.front], 0, KB);
    sm[sockfd].seqs[sm[sockfd].swnd.front] = sm[sockfd].swnd.seqno;
    strcpy(sm[sockfd].s[sm[sockfd].swnd.front], (char *)buf);
    sm[sockfd].swnd.seqno = (sm[sockfd].swnd.seqno+1)%16;
    sm[sockfd].swnd.front = (sm[sockfd].swnd.front+1)%sm[sockfd].swnd.size;
    sm[sockfd].swnd.cnt++;
    semop(sendsem, &vop, 1);
    shmdt(sm);
    return strlen((char *)buf);
}

int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    int smshm = shmget(ftok(".", 3000), 25*sizeof(SM), 0777|IPC_CREAT);
    int recvsem = semget(ftok(".", 7000), 1, 0777|IPC_CREAT);
    SM *sm;
    sm = (SM *)shmat(smshm, 0, 0);
    if(sm[sockfd].pid != getpid() || sm[sockfd].isclosed){
        errno = EPERM;
        perror("recvfrom");
        return -1;
    }
    struct sembuf sop, vop;
    sop.sem_num = sockfd;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    vop.sem_num = sockfd;
    vop.sem_op = 1;
    vop.sem_flg = 0;
    semop(recvsem, &sop, 1);
    if(sm[sockfd].rwnd.cnt == 0 || sm[sockfd].valid[sm[sockfd].rwnd.rear] == 0){
        errno = ENOMSG;
        semop(recvsem,&vop,1);
        //perror("recvfrom");
        return -1;
    }
    strcpy((char *)buf, sm[sockfd].r[sm[sockfd].rwnd.rear]);
    memset(sm[sockfd].r[sm[sockfd].rwnd.rear], 0, KB);
    sm[sockfd].valid[sm[sockfd].rwnd.rear] = 0;
    sm[sockfd].rwnd.rear = (sm[sockfd].rwnd.rear+1)%sm[sockfd].rwnd.size;
    sm[sockfd].rwnd.cnt--;
    sm[sockfd].rwnd.nospace = 0;
    semop(recvsem, &vop, 1);
    shmdt(sm);
    return strlen((char *)buf);
}

int m_close(int sockfd){
    int smshm = shmget(ftok(".", 3000), 25*sizeof(SM), 0777|IPC_CREAT);
    SM *sm;
    sm = (SM *)shmat(smshm, 0, 0);
    if(sm[sockfd].pid != getpid() || sm[sockfd].isclosed){
        errno = EPERM;
        perror("close");
        return -1;
    }
    sm[sockfd].isclosed = 1;
    shmdt(sm);
    return 0;
}

int dropMessage(float p){
    float r = (float)rand()/(float)RAND_MAX;
    if(r < p){
        return 1;
    }
    return 0;
}
