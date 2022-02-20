/*
* Brief - Implements a "Native Socket Server" as mentioned in assignmented 5 part 1
* 
* Author - Deekshith Reddy Patil, patil.deekshithreddy@colorado.edu
*
* Reference -https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
*           -https://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux
*           -https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
*/

#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <arpa/inet.h>


#define PORT_NO             9000

int sockfd=-1, clientfd=-1; //file descriptors of client and the server's socket
int fd=-1; //file descriptor for /var/tmp/aesdsocketdata

char *temp_buff = NULL; //pointer used to store recieved data through socket

int daemonize = 0; //Used to check if the "-d" flag is set. If set, the process is daemonised

//Function that is used to daemonize this process if -d argument was passed
static void daemonize_process();

/*
* Brief - Signal handler for SIGINT and SIGTERM
*/
void signal_handler(int signo)
{
    //Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received
    syslog(LOG_USER,"Caught signal, exiting\n");

    if(clientfd > 0)
    {
        close(clientfd);
    }

    if(sockfd > 0)
    {
        shutdown(clientfd,SHUT_RDWR);
        close(sockfd);
    }

    if(fd > 0)
    {
        //deleting the file /var/tmp/aesdsocketdata
        remove("/var/tmp/aesdsocketdata");
        close(fd);
    }

    if(temp_buff != NULL)
    {
        free(temp_buff);
    }


    exit(-1);
}

int main(int argc, char *argv[])
{

    int n;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    //Paramters used in recieving data
    char temp;
    unsigned long idx=0;

    //Parameters related to signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM,signal_handler);

    //Setup syslog
    openlog("aesdsocket",0,LOG_USER);

    //Check if process needs to be daemonised
    if(argc > 2)
    {
        printf("Invalid arguments to main\nUsage: ./main <-d>\n");
        exit(-1);
    }

    if(argc == 2)
    {
        if((strcmp(argv[1],"-d")) == 0)
        {
            daemonize = 1;
        }

        else
        {
            printf("Invalid arguments to main\nUsage: ./main <-d>\n");
            exit(-1);
        }
    }

    //Start the servers socket and bind
    sockfd = socket(AF_INET,SOCK_STREAM, 0); //open a socket with IPV4, stream type (tcp) and the protocol is TCP (0)
    printf("sockfd = %d\n",sockfd);
    if(sockfd < 0)
    {
        perror("Error opening socket");
        exit(-1);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    memset((void *)&serv_addr,0,sizeof(serv_addr)); //Set serv_addr to 0
    
    //Set the server characteristics 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; //This is an IP address that is used when we don't bind a socket to any specific IP. 
                                            //When we don't know the IP address of our machine, we can use the special IP address INADDR_ANY.
    serv_addr.sin_port = htons(PORT_NO); //host to network short

    int ret = bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    if(ret < 0)
     {
         perror("Binding failed");
         exit(-1);
     }

    if(daemonize == 1)
     {
         //-d argument was passed to main, now the process has to daemonise itself
         daemonize_process();
        // daemon(0,0);
     }

     //file needed for append (/var/tmp/aesdsocketdata), creating this file if it doesn’t exist
     fd = open("/var/tmp/aesdsocketdata",O_RDWR | O_TRUNC | O_CREAT, 0666);

     if(fd == -1)
     {
         perror("Error opening file");
         exit(-1);
     }

    while(1)
    {
        //start listening on the created socket
        listen(sockfd, 5);

        clilen = sizeof(cli_addr);
     
        clientfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if(clientfd < 0)
        {
            perror("Error on accept");
            exit(-1);
        }

        //Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client
        syslog(LOG_USER,"Accepted connection from %s\n",inet_ntoa(cli_addr.sin_addr));

        //Initialise a temp buffer to read data
        temp_buff = (char *)malloc(sizeof(char));

        //Start receiving the packets from socket
        while(1)
        {

            n = recv(clientfd,&temp,1,0);
            
            if(n < 1)
            {
                //Error in recv()
                perror("recv failed");
                exit(-1);
            }

            temp_buff[idx] = temp;
            idx++;
            temp_buff = (char *)realloc(temp_buff,sizeof(char) * (idx + 1));

            if(temp == '\n')
            {
                //write buffer to the file of len 'idx'
                n = write(fd,temp_buff,idx);
                idx = 0;
                free(temp_buff);
                temp_buff = NULL;
                break;
            }


        }

        //read the entire contents of the file and transfer it over the socket
        lseek(fd,0,SEEK_SET);

        while(1)
        {
            n = read(fd,&temp,1);
            

            if(n==0)
            {

                //EOF file reached
                break;
            }
            write(clientfd,&temp,1);


        }
        
        //Syslog: “Closed connection from XXX” where XXX is the IP address of the connected client.
        syslog(LOG_USER,"Closed connection from %s\n",inet_ntoa(cli_addr.sin_addr));

        //Close the connection with the client and start listening for incoming connections again
        close(clientfd);

    }

    //Unreachable code
    return 0;

}


static void daemonize_process()
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
