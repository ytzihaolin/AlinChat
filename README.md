# AlinChat
ALinChat Developed by zl2402 Zihao Lin

1.Description
	This is TCP based chatting application, using multi-thread to handle multi-users, containing a server version and a client version. This application have all the functionalities required by the assignment, and other extra features. Change variables like "LAST_HOUR" in the #define area of server.cpp, if want to add more users, don't forget to define Maxuser in server.cpp, and type in username and passwd in user_pass.txt

2.Development Environment
	This application is built with C# in Linux(Ubantu14.04) 

3.The folder contains:
	source code for client and server:Client.cpp Server.cpp
	makefile
	user_pass.txt
To run the application, simply type "%make" in terminal, then invoke the Server by typing "%./Server portnumber", invoke the Client by typing "%./Client IPaddress portnumberofserver"
Sample:
	%make
	%./Server 4119
	%.........(server version will be invoked)

	%./Client 127.0.0.2 4119
	%.........(client version will be invoked)
port number should be the same, use valid IP address or binding in client will fail

4.
	CommandLine:       	function
	%whoelse           	displays name of other connected users
	%wholasthr	   	displays name of those who were online within one hour
	%<broadcast>message     send message to all the online users
	%<message><user>message private message to a user,if offline then delivered message when he comes online
	%help			show all the commandline and function
	%logout			log user out
don't forget to input"<",">" for broadcast ,message and user, these symbols are used in the command line, or will causing server not recognizing this command


5.Extra functions:
(1).offline message. Just send private message, if offline, message will be sent to user when comes online
(2).wholasthr will display log in time as well as log off time(for those who logged off within one hour)
(3).have help manual

6.Problem:
1. Don't know which IP will be considered valid. Some will work, some won't
2. It works well in linux, but if run in Mac, there will be problem, like sentence "Command:" will be sent twice from server.
3. Not be tested on different hosts
4. starter for makefile, don't know if there is any problems.
5. If any question, please contact me at zl2402@columbia.edu, thanks
