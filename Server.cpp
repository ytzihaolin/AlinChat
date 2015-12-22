#include<sys/socket.h>
#include<stdio.h>
#include<arpa/inet.h>//inet_ntoa
#include<string.h>//gets,puts
#include<stdlib.h>
#include<pthread.h> //pthread
#include<unistd.h>//close
#include <sys/types.h>
#include<time.h>//time()
#include<signal.h>//read ctrl+C

#define Maxuser 9 //define max user number
#define LAST_HOUR 3600 //define last_hour for wholasthr (seconds)
#define TIME_OUT 1800//define timeout kick for Client I/O  (seconds)
#define LOGIN_TIMEOUT 30//define timeout kick for client who is in login process(second)
#define BLOCK_OUT 30//define block out time for three wrong password input
struct timeval timeout={TIME_OUT,0};

//User information
struct User{
    char userid[50];
    char userpasswd[50];
    int User_socket;
    int User_status;
    time_t logintime;
    time_t logouttime;
    time_t attempttime;
    char attemptaddress[100];
    char offlinemes[2000];

};

User User[Maxuser];

void *Userthread(void *);
void Broadcast(char *,int);
int PrivateMessage(char *,int);
int UserLogin(int *);
int CheckCommand(char * );
void CommandManul(int );
int Endflag;
void Logout(int );

int main(int argc, char *argv[]){
    int SocketClient,ListeningSocket;
    int i=0,j=0,c,threadflag,flag=0;
    struct sockaddr_in serveraddress;
    char *mes;
    long argl=atol(argv[1]);

    //loading user id and password
    FILE *fp=fopen("user_pass.txt","r");
    i=0;
    while(!feof(fp)&&i<Maxuser){
        fscanf(fp,"%s",User[i].userid);
        fscanf(fp,"%s*[^\n]",User[i].userpasswd);
        i++;
    }
    //init user information
    for(i=0;i<Maxuser;i++){
        User[i].User_socket=0;
        User[i].User_status=0;
        User[i].logintime=0;
        User[i].logouttime=0;
        User[i].attempttime=0;
        bzero(User[i].offlinemes,sizeof(User[i].offlinemes));
    }

//create and bind listen socket
    if((ListeningSocket=socket(PF_INET,SOCK_STREAM,0))<0){
        puts("Cannot create socket");
    }
    serveraddress.sin_family=PF_INET;
    serveraddress.sin_addr.s_addr=INADDR_ANY;
    serveraddress.sin_port=htons(argl);

    if (bind(ListeningSocket,(struct sockaddr*) &serveraddress,sizeof(serveraddress)) < 0){
         perror("ERROR on binding");
         return 0;
    }
    puts("bind successful");


    //start listening
    listen(ListeningSocket,Maxuser);

     //tell the listening port number
    socklen_t len = sizeof(serveraddress);
    if (getsockname(ListeningSocket, (struct sockaddr *)&serveraddress, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", ntohs(serveraddress.sin_port));


    i=0;
    puts("Welcome to ALinChat vServer");

    //assign multi-thread to handle users
    while(i<Maxuser){
        //create thread to solve multi-users
        pthread_t trd;
        threadflag=pthread_create(&trd,NULL,Userthread,(void *)&ListeningSocket);
        if(threadflag==-1){
            perror("cannot create thread");
            break;
        }
        i++;
    }

    //keep looping untill read Ctrl+C from console,then shutdown
    Endflag=0;
    while(Endflag==0){
        if(signal(SIGINT,Logout)!=SIG_ERR)
        signal(SIGINT,Logout);
    }
    close(ListeningSocket);
}

void Logout(int l){//close server, disconnect all online users
    int i;
    for(i=0;i<Maxuser;i++){
        if(User[i].User_status==1){
            send(User[i].User_socket,"Server is closed",sizeof("Server is closed"),0);
            close(User[i].User_socket);
        }
    }
    Endflag=1;
}

//login verification, still lack timeout kick
int UserLogin(int *Client_Soc,char *address){
    char ClientMessage[2000],name[2000];
    int sizeofmessage;
    int i=0,j=0;
    struct timeval logintimeout={LOGIN_TIMEOUT,0};

    while(1){//keep ask username until right input or timeout kick
        bzero(ClientMessage,sizeof(ClientMessage));
        send(*Client_Soc,"UserName",strlen("UserName"),0);
        //set socket to kick user out if no input from user within LOGIN_TIMEOUT
        setsockopt(*Client_Soc,SOL_SOCKET,SO_RCVTIMEO,&logintimeout,sizeof(logintimeout));
        sizeofmessage=recv(*Client_Soc,ClientMessage,2000,0);
        if(sizeofmessage<=0){//check if client close the connection
            puts("Client disconnected");
            return -1;
        }
        //check one by one if user is in the database
        for(i=0;i<Maxuser;i++){
            if(strcmp(ClientMessage,User[i].userid)==0){
                if(User[i].User_status==1){//prevent mutiple log in using same userid
                    send(*Client_Soc,"This user has already logged in \n",strlen("This user has already logged in \n"),0);
                    break;
                }
                //check if user is still being blocked
                if(difftime(time(NULL),BLOCK_OUT)<User[i].attempttime&&User[i].attempttime!=0&&strcmp(User[i].attemptaddress,address)==0) return -1;
                for(j=0;j<3;j++){//valid user, check passwd
                    send(*Client_Soc,"Passwd",strlen("Passwd"),0);
                    bzero(ClientMessage,sizeof(ClientMessage));
                    sizeofmessage=recv(*Client_Soc,ClientMessage,2000,0);
                    if(sizeofmessage<=0){
                        puts("Client disconnected");
                        return -1;
                    }
                    if(strcmp(ClientMessage,User[i].userpasswd)!=0){
                        send(*Client_Soc,"Wrong password\n\n",strlen("Wrong password\n\n"),0);
                    }else{//storing information
                            User[i].attempttime=0;
                            User[i].User_status=1;
                            User[i].User_socket=*Client_Soc;
                            User[i].logintime=time(NULL);
                            User[i].logouttime=0;
                            return i;
                    }
                }
                return i+Maxuser;//this user is being blocked
            }
        }
        if(i==Maxuser) send(*Client_Soc,"No Such User\n",strlen("No Such User\n"),0);
    }
}
//thread that handle user business
void *Userthread(void *soc_desc){//thread that handle User I/O

    char ClientMessage[2000],message[200];
    int sizeofmessage;
    int ListeningSocket= *(int *)soc_desc,ClientSocket;
    int i=0,j=0,ClientNum;
    int mm;
    struct sockaddr_in clientaddress;
    time_t Now=0;

    mm= sizeof(struct sockaddr_in);
    ClientSocket=accept(ListeningSocket,(struct sockaddr *)&clientaddress,(socklen_t *)&mm);


    ClientNum=UserLogin((int *)&ClientSocket,inet_ntoa(clientaddress.sin_addr));//user verification
    if(ClientNum!=-1&&ClientNum<=Maxuser){//Valid user
        if(strlen(User[ClientNum].offlinemes)!=0){
            send(ClientSocket,User[ClientNum].offlinemes,sizeof(User[ClientNum].offlinemes),0);
            bzero(User[ClientNum].offlinemes,sizeof(User[ClientNum].offlinemes));
        }else send(ClientSocket,"Welcome to EasyChat\n\nCommand:",sizeof("Welcome to EasyChat\n\nCommand:"),0);

        printf("New Client %s Connected, His IP is:%s\n",User[ClientNum].userid,inet_ntoa(clientaddress.sin_addr));
        //show online users
        puts("those clients are online:");
        for(j=0;j<Maxuser;j++){
            if(User[j].User_status==1){
                puts(User[j].userid);
            }
        }

    }else if(ClientNum!=-1){//user that are being blocked
            User[ClientNum-Maxuser].attempttime=time(NULL);
            bzero(User[ClientNum-Maxuser].attemptaddress,sizeof(User[ClientNum-Maxuser].attemptaddress));
            strcpy(User[ClientNum-Maxuser].attemptaddress,inet_ntoa(clientaddress.sin_addr));
            send(ClientSocket,"You are blocked",sizeof("You are blocked"),0);
            close(ClientSocket);
            pthread_exit(NULL);
            }else {//
                    send(ClientSocket,"You are blocked",sizeof("You are blocked"),0);
                    close(ClientSocket);
                    pthread_exit(NULL);
                  }
    //reading from client
    struct timeval timeout={TIME_OUT,0};
    while(1){//I/O
        //set timeout kick
        setsockopt(ClientSocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        send(ClientSocket,"Command:",strlen("Command:"),0);
        bzero(ClientMessage,sizeof(ClientMessage));

        //check if client is disconnected
        sizeofmessage=recv(ClientSocket,ClientMessage,2000,0);
        if(sizeofmessage<=0){
            printf("Client %s Disconnected\n",User[ClientNum].userid);
            break;
        }
        //check command from user
        switch(CheckCommand(ClientMessage)){
            case 1:{//whoelse
                    i=0;j=0;
                    bzero(message,sizeof(message));
                    for(i=0;i<Maxuser;i++){
                        if(User[i].User_status==1&&i!=ClientNum){
                            j=1;
                            strcat(message,User[i].userid);
                            strcat(message,"\n");
                        }
                    }
                    if(j==0) strcat(message,"There are no other users\n");
                    strcat(message,"\nCommand:");
                    send(ClientSocket,message,sizeof(message),0);
                    break;
                    }
            case 2:{//who last hour
                    bzero(message,sizeof(message));
                    Now=time(NULL);
                    for(i=0;i<Maxuser;i++){
                            if(User[i].logintime!=0&&User[i].logouttime==0&&i!=ClientNum){
                                strcat(message,User[i].userid);
                                strcat(message,"  last log in time: ");
                                strcat(message,ctime(&User[i].logintime));
                                strcat(message,"\n");
                            }
                            if(User[i].logouttime!=0&&difftime(Now,User[i].logouttime)<LAST_HOUR&&i!=ClientNum){
                                strcat(message,User[i].userid);
                                strcat(message,"  last log in time: ");
                                strcat(message,ctime(&User[i].logintime));
                                strcat(message,"    last log out time: ");
                                strcat(message,ctime(&User[i].logouttime));
                                strcat(message,"\n");
                            }
                        }
                    if(message[0]=='\0') strcat(message,"No one is online within one hour\n");
                    strcat(message,"\nCommand:");
                    send(ClientSocket,message,sizeof(message),0);
                    break;
                    }
            case 3:{//broadcast
                    Broadcast(ClientMessage,ClientNum);
                    break;
                    }
            case 4:{//private messaging:
                    PrivateMessage(ClientMessage,ClientNum);
                    break;
                    }
            case 5: {//show help
                     CommandManul(ClientNum);
                     break;
                     }
            case 6:{
                    printf("Client %s Disconnected\n",User[ClientNum].userid);
                    User[ClientNum].User_status=0;
                    User[ClientNum].User_socket=0;
                    User[ClientNum].logouttime=time(NULL);
                    close(ClientSocket);
                    pthread_exit(NULL);
                    }
            case 7: send(ClientSocket,"unknown command\n\nCommand:",sizeof("unknown command\n\nCommand:"),0);//unkonwn command
        }
    }//end while means user logout

    //reset user information,close socket,close thread
    User[ClientNum].User_status=0;
    User[ClientNum].User_socket=0;
    User[ClientNum].logouttime=time(NULL);
    close(ClientSocket);
    pthread_exit(NULL);
}
//verify commandline
int CheckCommand(char message[2000]){
    if(strcmp(message,"whoelse")==0) return 1;
    else if(strcmp(message,"wholasthr")==0) return 2;
    else if(strncmp(message,"<broadcast>",11)==0) return 3;
    else if(strncmp(message,"<message>",9)==0) return 4;
    else if(strcmp(message,"help")==0) return 5;
    else if(strcmp(message,"logout")==0) return 6;
    else return 7;

}

void Broadcast(char *message,int ClientNum){
    int i,j;
    char broadcastmes[200];

    bzero(broadcastmes,sizeof(broadcastmes));
    strcat(broadcastmes,User[ClientNum].userid);
    strcat(broadcastmes,"(broadcast): ");
    message+=11;
    strcat(broadcastmes,message);
    strcat(broadcastmes,"\nCommand:");
    puts(broadcastmes);
    for(i=0;i<Maxuser;i++){
        if((User[i].User_status==1)&&(i!=ClientNum)){
            send(User[i].User_socket,broadcastmes,sizeof(broadcastmes),0);
        }
    }
    send(User[ClientNum].User_socket,"\nCommand:",sizeof("\nCommand:"),0);
}

int PrivateMessage(char *message, int ClientNum){
    int i,j;
    char privatemes[2000];

    for(i=0;i<Maxuser;i++){
        if((strncmp(User[i].userid,message+10,strlen(User[i].userid))==0)&&(i!=ClientNum)&&(User[i].User_status==1)){

             bzero(privatemes,sizeof(privatemes));
             strcat(privatemes,User[ClientNum].userid);
             strcat(privatemes,": ");
             message+=11;
             message+=strlen(User[i].userid);
             strcat(privatemes,message);
             strcat(privatemes,"\nCommand");
             send(User[i].User_socket,privatemes,sizeof(privatemes),0);
             send(User[ClientNum].User_socket,"Command:",sizeof("Command:"),0);
             return 1;
        }else if((strncmp(User[i].userid,message+10,strlen(User[i].userid))==0)&&(i!=ClientNum)&&(User[i].User_status==0)){

                if(User[i].offlinemes[0]=='\0')
                    strcat(User[i].offlinemes,"Welcome to EasyChat\n\nYou have offline messages:\n");

                bzero(privatemes,sizeof(privatemes));
                strcat(privatemes,User[ClientNum].userid);
                strcat(privatemes,": ");
                message+=11;
                message+=strlen(User[i].userid);
                strcat(privatemes,message);
                strcat(User[i].offlinemes,privatemes);
                strcat(User[i].offlinemes,"\n");
                send(User[ClientNum].User_socket,"User Not Online:\nCommand:",sizeof("User Not Online:\nCommand:"),0);
                return 1;
                }//offline message

    }
    send(User[ClientNum].User_socket,"Wrong User:\nCommand:",sizeof("Wrong User:\nCommand:"),0);

    return 0;
}

void CommandManul(int ClientNum){//help manual

    int i;
    char cmdline[1000]={ "whoelse--->Print out every other online users\nwholasthr--->Print out users that logged in within last hour\n<broadcast>message--->Broadcast your message to all online users\n<message><user>message--->Send message to the chosen user(save to offline message if not online)\nlogout--->logout\n\nCommand:"};
    send(User[ClientNum].User_socket,cmdline,sizeof(cmdline),0);

}




//thanks


