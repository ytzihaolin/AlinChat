#include<sys/socket.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<string.h>//gets,puts
#include<stdlib.h>
#include<pthread.h> //To create multi-thread
#include<unistd.h>//close
#include<sys/types.h>
#include<signal.h>

char message[200];
int flag=0;

void Logout(int );

void *recvmes(void *soc_desc){//recv thread

    char MessageFromServer[100];
    int connectionflag=0,sizeofmessage;
    int ClientSocket= *(int *)soc_desc;


    do{
        bzero(MessageFromServer,sizeof(MessageFromServer));
        sizeofmessage=recv(ClientSocket,MessageFromServer,2000,0);
        puts(MessageFromServer);
        if(sizeofmessage<=0){
            flag=1;
            break;
        }
    }while(1);
    close(ClientSocket);
    pthread_exit(NULL);
}


void *sendmes(void *soc_desc){//send thread


    int connectionflag=0,sizeofmessage;
    int ClientSocket= *(int *)soc_desc;


    do{
        bzero(message,sizeof(message));
        gets(message);

        if(send(ClientSocket,message,strlen(message),0)<0){
            puts("send failed");
            }
    }while(1);
    close(ClientSocket);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]){
    int ClientSocket,threadflag;
    struct sockaddr_in server;
    struct sockaddr_in client;


    ClientSocket=socket(PF_INET,SOCK_STREAM,0);

    client.sin_family=PF_INET;
    client.sin_addr.s_addr=inet_addr(argv[1]);

    if (bind(ClientSocket,(struct sockaddr*) &client,sizeof(client)) < 0){
         perror("ERROR on binding");
         return 0;
    }


    pthread_t trd;


    if(ClientSocket<0){
        puts("Cannot create socket");
    }

    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_family=PF_INET;
    server.sin_port=htons(atol(argv[2]));

    puts("Welcom to ALinChat vClient");
    printf("Your Current IP is %s\n",inet_ntoa(client.sin_addr));

    if(connect(ClientSocket,(struct sockaddr *)&server, sizeof(server))<0){
        perror("connect error");
        return 1;
    }
    puts("connected");

    threadflag=pthread_create(&trd,NULL,recvmes,(void *)&ClientSocket);
        if(threadflag==-1){
            perror("cannot create thread");

        }

    threadflag=pthread_create(&trd,NULL,sendmes,(void *)&ClientSocket);
        if(threadflag==-1){
            perror("cannot create thread");

        }

    while(flag==0){
        if(signal(SIGINT,Logout)!=SIG_ERR)
        signal(SIGINT,Logout);
    }
    close(ClientSocket);
    puts("\nGoodBye!");
}

void Logout(int l){//close server, disconnect all online users
    flag=1;
}







