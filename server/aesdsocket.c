/*
file: server.c
description: TCP/IP socket server code 
Author: Jahnavi Pinnamaneni; japi8358@colorado.edu
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <syslog.h>

#include <signal.h>

#include <pthread.h>
#include "queue.h"
#include <time.h>

int server_socket;       //socket_fd
int client_socket;       //client_fd

void daemonise_process();

/*
Signal handler to gracefully terminate when signals SIGINT and SIGTERM arrive
parameter: signal number
*/
void graceful_shutdown(int signo)
{
    syslog(LOG_DEBUG, "CAUGHT SIGNAL, EXITING GRACEFULLY \n");
    remove("/var/tmp/aesdsocketdata");
    // shutdown(client_socket, SHUT_RDWR);
    // close(client_socket);
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
    closelog(); 
    exit(0);
}
typedef struct thread_info
{
    //char ipv4_addr[50];
    pthread_t threadId;
    int client_fd;
    int complete_flag;
    TAILQ_ENTRY(thread_info) nodes;
}thread_info_t;

pthread_mutex_t mutex;

void * server_thread(void * thread_info)
{
    thread_info_t *th_info = (thread_info_t *)thread_info;
    //printf("client: %d\n",th_info->client_fd);
    int store_fd;
    store_fd = open("/var/tmp/aesdsocketdata", O_RDWR|O_APPEND, 0666);
    //printf("store_fd: %d\n",store_fd);
    //Client-Server connection is made.
        char *recv_buf = (char *)malloc(1024);
        int realloc_cnt = 1;
        int recv_bytes = 0;
        int recv_buf_size = 0;
        int written_bytes = 0;
        if(recv_buf == NULL)
        {
            close(client_socket);
            exit(-1);
        }

        //printf("thread initialization complete\n");
        //read bytes
        pthread_mutex_lock(&mutex);
        while(1)
        {
            //pthread_mutex_lock(mutex);
            recv_bytes = recv(th_info->client_fd, &recv_buf[recv_buf_size], 1024, 0);
            recv_buf_size += recv_bytes;
            if(!recv_bytes)
            {
                break;
            }
            if(recv_bytes < 1024)
            {
                if(recv_buf[recv_buf_size-1] == '\n')
                {
                    
                    written_bytes = write(store_fd, recv_buf, recv_buf_size);
                    //printf("recv_buf = %s\n", recv_buf);
                    //printf("written bytes = %d\n",written_bytes);
                    if(written_bytes < 0)
                    {
                        perror("write");
                    }
                    free(recv_buf);
                    recv_buf = NULL;
                    recv_buf_size = 0;
                    break;
                }
            }
            else
            {
                realloc_cnt++;
                recv_buf = realloc(recv_buf, realloc_cnt * 1024);
                if(recv_buf == NULL)
                {
                    close(client_socket);
                    free(recv_buf);
                    exit(1);
                }            
            }


        }

        lseek(store_fd, 0, SEEK_SET);
        while(1)
        {
            char temp;
            int read_bytes = read(store_fd,&temp,1);
            //printf("read_bytes %d\t",read_bytes);
            //printf("%c", temp);
            if(read_bytes < 0)
            {
                perror("read");
                exit(-1);
            }

            if(read_bytes == 0)
            {
                break;
            }

            write(th_info->client_fd,&temp,1);
            

        }
        
        pthread_mutex_unlock(&mutex);
        th_info->complete_flag = 1;

        close(store_fd);
        
        if(recv_buf != NULL)
            free(recv_buf);
    return 0;
}

void alarm_handler (int signum)
{
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);

    info = localtime(&rawtime);
    //printf("alarm handler\n");
    strftime(buffer,80, "timestamp: %Y %m %d %H %M %S\n",info);
    //printf("%s\n",buffer);
    pthread_mutex_lock(&mutex);
    int store_fd;
    store_fd = open("/var/tmp/aesdsocketdata", O_WRONLY|O_APPEND, 0666);  
    //printf("fd: %d",store_fd);  
    int write_ret = write(store_fd, buffer, 31);  
    //printf("bytes: %d\n",write_ret); 
    if(write_ret < 0)
        perror("write");
    close(store_fd);
    pthread_mutex_unlock(&mutex);
    alarm(10);
}

//Application entry point
int main(int argc, char *argv[])
{
    //printf("Inside main");
    TAILQ_HEAD(head_s, thread_info) head;
    // Initialize the head before use
    TAILQ_INIT(&head);


    pthread_mutex_init(&mutex, NULL);

    //signal fucntions
    signal(SIGINT, graceful_shutdown);
    signal(SIGTERM, graceful_shutdown);
    signal(SIGALRM, alarm_handler);
    
    
    //Initialization for syslog
    openlog(NULL, 0, LOG_USER);


    //create the server socket
    int get_fd;
    struct addrinfo hints;
    struct addrinfo *rp;
    struct sockaddr_storage cl_addr;
    socklen_t addr_size;

    memset(&hints, 0 , sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    get_fd = getaddrinfo(NULL, "9000", &hints, &rp);
    if(get_fd != 0)
    {
        perror("ERROR");
        exit(1);
    }
    
    int d_flag = 0;
  if(argc == 2)
    {
        if((strcmp(argv[1],"-d")) == 0)
        {
            d_flag = 1;
        }

        else
        {
            exit(EXIT_FAILURE);
        }
    }

    server_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if(server_socket == -1)
    {
        perror("ERROR");
        exit(1);
    }
    if(setsockopt(server_socket,SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))<0)
        perror("setsocket(SO_REUSEADDR) failed");

    //bind the socket to our specified IP and port
    if(bind(server_socket, rp->ai_addr, rp->ai_addrlen) != 0)
    {
        perror("ERROR");
        exit(1);
    }
    syslog(LOG_DEBUG, "Binded successfully");
    freeaddrinfo(rp);

    if(d_flag == 1)
    {
        daemonise_process();
    }
    
    //File to store the recieved data
    int store_fd;
    store_fd = open("/var/tmp/aesdsocketdata", O_CREAT|O_TRUNC, 0666);
    if(store_fd < 0)
    {
        perror("open");
    }
    alarm(10);
    //Server socket is established, next Client-Server connection should be made
    while(1)
    {
        
        listen(server_socket, 5);        
        
        addr_size = sizeof(cl_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&cl_addr, &addr_size);
        struct sockaddr_in* pv4Addr = (struct sockaddr_in*)&cl_addr;
        struct in_addr ipAddr = pv4Addr->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop( AF_INET, &ipAddr,str, INET_ADDRSTRLEN);
        syslog(LOG_DEBUG, "Accepted connection to %s\n", str);

        //Create a thread
        thread_info_t *th_info = NULL;
        th_info = malloc(sizeof(struct thread_info));
        if(th_info == NULL)
        {
            perror("thread malloc");
            exit(-1);
        }
        th_info->client_fd = client_socket;
        th_info->complete_flag = 0;
        int err = pthread_create(&(th_info->threadId),NULL, server_thread, (void *)th_info);
        if(err)
        {
            perror("pthread_create");
            free(th_info);
        }

        TAILQ_INSERT_TAIL(&head, th_info, nodes);

        TAILQ_FOREACH(th_info, &head, nodes)
        {
            //int join_ret = pthread_join(th_info->threadId,NULL);
            if(th_info->complete_flag)
            {
                shutdown(th_info->client_fd,SHUT_RDWR);
                close(th_info->client_fd);
                syslog(LOG_DEBUG, "Closing connection from %s \n",str);
                pthread_join(th_info->threadId,NULL);
                TAILQ_REMOVE(&head, th_info, nodes);
                free(th_info);
                break;
            }
        }        
    }
    close(server_socket);
    close(store_fd);
    closelog();
    return 0;
}


//used to daemonize the current process
void daemonise_process()
{
    pid_t pid;

    pid = fork();

    if(pid<0)
    {
        perror("fork");
        exit(-1);
    }

    if(pid > 0)
    {
        //Parent Process: has to exit
        printf("Parent Exiting!. Child PID = %d\n",pid);
        exit(0);
    }

    //Child process executes from here on, i.e pid = 0

    umask(0);

    //Create a new session and set the child as group leader
    pid = setsid();
    
    if(pid < 0)
    {
        perror("setsid");
        exit(-1);
    }

    //Change working directory to root directory
    chdir("/");

    //close all file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

