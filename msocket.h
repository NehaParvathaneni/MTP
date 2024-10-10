#ifndef MSOCKET_H
#define MSOCKET_H
#endif

#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/types.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<fcntl.h>
#include<time.h>
#include<pthread.h>
#include<sys/time.h>
#include<sys/wait.h>
#include<signal.h>

#define SOCK_MTP 1
#define T 5
#define DATA 26
#define ACK 15
#define DUP 41
#define KB 1024
#define P 0

typedef struct {
    int cnt;
    int front;
    int size;
    clock_t lastsent;
    int seqno;
    int winstart;
    int winsize;
    int winsent;
    int winseq;
} sendwindow;

typedef struct {
    int cnt;
    int front;
    int rear;
    int size;
    int nospace;
    int seqno;
    int winstart;
    int winsize;
} recvwindow;

typedef struct SM{
    int smvalid;
    int pid;
    int sockid;
    char sip[INET_ADDRSTRLEN];
    int sport;
    char dip[INET_ADDRSTRLEN];
    int dport;
    char s[10][1024];
    char r[5][1024];
    int seqs[10];
    int valid[5];
    sendwindow swnd;
    recvwindow rwnd;
    int isclosed;
    int no_pacs_sent;
} SM;

typedef struct sockinfo{
    int sockid;
    char ip[INET_ADDRSTRLEN];
    int port;
    int err;
} sockinfo;

int m_socket(int domain, int type, int protocol);
int m_bind(int sockfd, const struct sockaddr *saddr, socklen_t saddrlen, const struct sockaddr *daddr, socklen_t daddrlen);
int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int m_close(int sockfd);
int dropMessage(float p);

