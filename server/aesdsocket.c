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
int server_socket;       //socket_fd
int client_socket;       //client_fd
//char *recv_buf = NULL;

void daemonise_process();

/*
Signal handler to gracefully terminate when signals SIGINT and SIGTERM arrive
parameter: signal number
*/
void graceful_shutdown(int signo)
{
    syslog(LOG_DEBUG, "CAUGHT SIGNAL, EXITING GRACEFULLY \n");
    remove("/var/tmp/aesdsocketdata");
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
    //if(recv_buf != NULL)
    	//free(recv_buf);
    exit(0);
}

//Application entry point
int main(int argc, char *argv[])
{
    //signal fucntions
    signal(SIGINT, graceful_shutdown);
    signal(SIGTERM, graceful_shutdown);
    
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
            //printf("Invalid arguments\n");
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
    store_fd = open("/var/tmp/aesdsocketdata", O_RDWR|O_CREAT|O_TRUNC, 0666);
    if(store_fd < 0)
    {
        perror("open");
    }

    //Server socket is established, next Client-Server connection should be made
    while(1){
    listen(server_socket, 5);
    
    
    addr_size = sizeof(cl_addr);
    client_socket = accept(server_socket, (struct sockaddr *)&cl_addr, &addr_size);
    struct sockaddr_in* pv4Addr = (struct sockaddr_in*)&cl_addr;
    struct in_addr ipAddr = pv4Addr->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr,str, INET_ADDRSTRLEN);
    syslog(LOG_DEBUG, "Accepted connection to %s\n", str);

    //Client-Server connection is made.
    char *recv_buf = (char *)malloc(1024);
    int realloc_cnt = 1;
    int recv_bytes = 0;
    int recv_buf_size = 0;
    int written_bytes = 0;
    if(recv_buf == NULL)
    {
        //printf("Error: allocating memory\n");
        close(client_socket);
        exit(-1);
    }

    
    //read bytes
    while(1)
    {
    //f("Entering read loop\n");
        recv_bytes = recv(client_socket, &recv_buf[recv_buf_size], 1024, 0);
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
                //f("error allocating memory\n");
                //free(recv_buf);
                close(client_socket);
                exit(1);
            }            
        }

    }

    lseek(store_fd, 0, SEEK_SET);
    store_fd = open("/var/tmp/aesdsocketdata", O_RDWR|O_CREAT|O_TRUNC, 0666);
    syslog(LOG_DEBUG,"Entering socket write loop\n");
    while(1)
    {
        char temp;
        int read_bytes = read(store_fd,&temp,1);

        if(read_bytes < 0)
        {
            perror("read");
            exit(-1);
        }

        if(read_bytes == 0)
        {
            break;
        }

        write(client_socket,&temp,1);

    }
    syslog(LOG_DEBUG,"Exiting write loop!\n");

    shutdown(client_socket,SHUT_RDWR);
    close(client_socket);
    syslog(LOG_DEBUG, "Closing connection from %s \n",str);
    if(recv_buf != NULL)
    	free(recv_buf);
    //printf("Closed connection\n");
    }
    close(server_socket);
    close(store_fd);

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
