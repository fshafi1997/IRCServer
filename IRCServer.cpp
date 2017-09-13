
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "IRCServer.h"
using namespace std;

vector<string> userVec;
vector<string> passVec;
vector<string> roomVec;

int numRooms = 0;
fstream passwordFile;

int QueueLength = 5;

//test

int
IRCServer::open_server_socket(int port) {

    // Set the IP address and port for this server
    struct sockaddr_in serverIPAddress; 
    memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
    serverIPAddress.sin_family = AF_INET;
    serverIPAddress.sin_addr.s_addr = INADDR_ANY;
    serverIPAddress.sin_port = htons((u_short) port);

    // Allocate a sockt
    int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
    if ( masterSocket < 0) {
	perror("socket");
	exit( -1 );
    }

    // Set socket options to reuse port. Otherwise we will
    // have to wait about 2 minutes before reusing the sae port number
    int optval = 1; 
    int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
	    (char *) &optval, sizeof( int ) );

    // Bind the socket to the IP address and port
    int error = bind( masterSocket,
	    (struct sockaddr *)&serverIPAddress,
	    sizeof(serverIPAddress) );
    if ( error ) {
	perror("bind");
	exit( -1 );
    }

    // Put socket in listening mode and set the 
    // size of the queue of unprocessed connections
    error = listen( masterSocket, QueueLength);
    if ( error ) {
	perror("listen");
	exit( -1 );
    }

    return masterSocket;
}

    void
IRCServer::runServer(int port)
{
    int masterSocket = open_server_socket(port);

    initialize();

    while ( 1 ) {

	// Accept incoming connections
	struct sockaddr_in clientIPAddress;
	int alen = sizeof( clientIPAddress );
	int slaveSocket = accept( masterSocket,
		(struct sockaddr *)&clientIPAddress,
		(socklen_t*)&alen);

	if ( slaveSocket < 0 ) {
	    perror( "accept" );
	    exit( -1 );
	}

	// Process request.
	processRequest( slaveSocket );		
    }
}

    int
main( int argc, char ** argv )
{
    // Print usage if not enough arguments
    if ( argc < 2 ) {
	fprintf( stderr, "%s", usage );
	exit( -1 );
    }

    // Get the port from the arguments
    int port = atoi( argv[1] );

    IRCServer ircServer;

    // It will never return
    ircServer.runServer(port);

}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

    void
IRCServer::processRequest( int fd )
{
    // Buffer used to store the comand received from the client
    const int MaxCommandLine = 1024;
    char commandLine[ MaxCommandLine + 1 ];
    int commandLineLength = 0;
    int n;

    // Currently character read
    unsigned char prevChar = 0;
    unsigned char newChar = 0;

    //
    // The client should send COMMAND-LINE\n
    // Read the name of the client character by character until a
    // \n is found.
    //

    // Read character by character until a \n is found or the command string is full.
    while ( commandLineLength < MaxCommandLine &&
	    read( fd, &newChar, 1) > 0 ) {

	if (newChar == '\n' && prevChar == '\r') {
	    break;
	}

	commandLine[ commandLineLength ] = newChar;
	commandLineLength++;

	prevChar = newChar;
    }

    // Add null character at the end of the string
    // Eliminate last \r
    commandLineLength--;
    commandLine[ commandLineLength ] = 0;

    printf("RECEIVED: %s\n", commandLine);

    //printf("The commandLine has the following format:\n");
    //printf("COMMAND <user> <password> <arguments>. See below.\n");
    //printf("You need to separate the commandLine into those components\n");
    //printf("For now, command, user, and password are hardwired.\n");

    char * tCommand = (char*)malloc(MaxCommandLine*sizeof(char));
    char * tUser = (char*)malloc(MaxCommandLine*sizeof(char));
    char * tPassword = (char*)malloc(MaxCommandLine*sizeof(char));
    char * tArgs = (char*)malloc(MaxCommandLine*sizeof(char));
//    char * tInput = (char*)malloc(MaxCommandLine*sizeof(char));

    int nRead = sscanf(commandLine, "%s %s %s %[^\n]", tCommand, tUser, tPassword, tArgs);

   // strcpy(tInput,commandLine);

   /* int index = 0;
    while(*tInput!=' ' && *tInput!='\0'){
	tCommand[index] = *tInput;
	tInput++;
	index++;
    }

    index = 0;

    tInput++;

    while(*tInput!=' ' && *tInput!='\0'){
	tUser[index] = *tInput;
	tInput++;
	index++;
    }

    index = 0;

    tInput++;

    while(*tInput!=' ' && *tInput!='\0'){
	tPassword[index] = *tInput;
	tInput++;
	index++;
    }

    index = 0;

    tInput++;

    tArgs = tInput;*/

    const char * command = tCommand;
    const char * user = tUser;
    const char * password = tPassword;
    const char * args = tArgs;

    printf("command=%s\n", command);
    printf("user=%s\n", user);
    printf( "password=%s\n", password );
    printf("args=%s\n", args);

    if (!strcmp(command, "ADD-USER")) {
	addUser(fd, user, password, args);
    }
    else if (!strcmp(command, "ENTER-ROOM")) {
	enterRoom(fd, user, password, args);
    }
    else if (!strcmp(command, "LEAVE-ROOM")) {
	leaveRoom(fd, user, password, args);
    }
    else if (!strcmp(command, "SEND-MESSAGE")) {
	sendMessage(fd, user, password, args);
    }
    else if (!strcmp(command, "GET-MESSAGES")) {
	getMessages(fd, user, password, args);
    }
    else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
	getUsersInRoom(fd, user, password, args);
    }
    else if (!strcmp(command, "GET-ALL-USERS")) {
	getAllUsers(fd, user, password, args);
    }
    else if(!strcmp(command, "CREATE-ROOM")){
	createRoom(fd,user,password,args);
    }
    else if(!strcmp(command, "LIST-ROOMS")){
	listRooms(fd,user,password,args);
    }
    else if(!strcmp(command, "CHECK-USER-EXIST-IN-ROOM")){
	checkUserExistInRoom(fd, user,password,args);
    }
    else if(!strcmp(command, "GET-MESSAGES2")){
	getMessages2(fd, user, password, args);
    }
    else if (!strcmp(command, "LOG-IN")) {
	logIn(fd, user, password);
    }
    else {
	const char * msg =  "UNKNOWN COMMAND\r\n";
	write(fd, msg, strlen(msg));
    }

    // Send OK answer
    //const char * msg =  "OK\n";
    //write(fd, msg, strlen(msg));

    close(fd);	
}

/*void llist_add(ListUsers * list,const char * user, const char * password) {
    Members * n = (Members *) malloc(sizeof(Members));
    n->username = user;
    n->password = password;

    n->next = list->head;
    list->head = n;
}*/

void
IRCServer::initialize()
{
    // Open password file (ASK TA about this)
    // Initialize users in room
    // Initalize message list

    userList.head = NULL;
    roomList.head = NULL;

    messageList.head = NULL;
    messageCounter = 0;


    passwordFile.open(PASSWORD_FILE);
    string line;
    int n = 1;

    if(passwordFile.is_open()){
	while(getline (passwordFile,line)) {
	    if((n % 3 == 1)) {
		Members * newUser = (Members*)malloc(sizeof(Members));

		userVec.push_back(line);
		newUser->username = line.c_str();
		n++;
		//printf("username%s/n",line.c_str());

		getline (passwordFile,line);
		passVec.push_back(line);
		newUser->password = line.c_str();
		n++;
		newUser->inRoom = NULL;
		newUser->next = NULL;

		if (userList.head == NULL){
	    		userList.head = newUser;
		}
		else if (strcmp(newUser->username, userList.head->username) < 0){
		    newUser->next = userList.head;
		    userList.head = newUser;
		}

		else{
		    Members * tempoUser = userList.head;
		    while (tempoUser->next != NULL){
			if (strcmp(newUser->username, tempoUser->next->username) < 0){
			    newUser->next = tempoUser->next;
			    tempoUser->next = newUser;
			}
			else tempoUser = tempoUser->next;
		    }
		    if(newUser->next==NULL) tempoUser->next = newUser;
		}

	    }
	    else{
		n++;
	    }
	}
	passwordFile.close();
    }
}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
    // Here check the password

    Members * n = userList.head;
    while(n->next!=NULL){
	if(strcmp(n->username,user)==0){
	    if(strcmp(n->password,password)==0){
		return true;
	    }
	}
	n = n->next;
    }
    if(strcmp(n->username,user)==0){
	if(strcmp(n->password,password)==0){
	    return true;
	}
    }
    return false;
}

void
IRCServer::logIn(int fd, const char * user, const char * password){
    bool e = false;
    for(int i = 0; i < passVec.size(); i++) {
	cout << "Already here = " << userVec[i] << '\n';
	if((passVec[i].compare(password) == 0) && (userVec[i].compare(user) == 0)) {
	    const char * msg =  "OK\r\n";
	    write(fd, msg, strlen(msg));
	    e = true;
	}
    }

    if(!e) {
	const char * msg2 =  "LOGIN not SUCCESSFUL\r\n";
	write(fd, msg2, strlen(msg2));
    }
}

    bool
    IRCServer::checkRoom(int fd, const char * user, const char * password, const char * args) {


    Room * e;
    if (roomList.head == NULL){
	return false;
    }
    e = roomList.head;
    while (e->next != NULL){
	if (strcmp(args, e->rName) == 0){
	    return true;
	}
	e = e->next;
    }
    if (strcmp(args, e->rName) == 0){
	return true;
    }
    return false;
}

    bool
IRCServer::checkUserExistInRoom(int fd, const char * user, const char * password, const char * args)
{
    members * e;
    Room * g;
    if (userList.head == NULL){
	const char * msg =  "false\r\n";
	write(fd, msg, strlen(msg));
	return false;
    }
    e = userList.head;
    while(e->next != NULL){
	if (strcmp(e->username, user) == 0){
	    g = e->inRoom;
	    if (g != NULL){
		while (g->next != NULL){
		    if (strcmp(g->rName, args) == 0){
			const char * msg =  "true\r\n";
			write(fd, msg, strlen(msg));
			return true;
		    }
		    g = g->next;
		}
		if (strcmp(g->rName, args) == 0){
		    const char * msg =  "true\r\n";
		    write(fd, msg, strlen(msg));
		    return true;
		}
	    }
	}
	e = e->next;
    }
    if (strcmp(e->username, user) == 0){
	g = e->inRoom;
	if (g != NULL){
	    while (g->next != NULL){
		if (strcmp(g->rName, args) == 0){
		    const char * msg =  "true\r\n";
		    write(fd, msg, strlen(msg));
		    return true;
		}
		g = g->next;
	    }
	    if (strcmp(g->rName, args) == 0){
		const char * msg =  "true\r\n";
		write(fd, msg, strlen(msg));
		return true;
	    }
	}
    }
}
bool
IRCServer::checkUserExistInRoom2(int fd, const char * user, const char * password, const char * args)
{
    members * e;
    Room * g;
    if (userList.head == NULL){
	return false;
    }
    e = userList.head;
    while(e->next != NULL){
	if (strcmp(e->username, user) == 0){
	    g = e->inRoom;
	    if (g != NULL){
		while (g->next != NULL){
		    if (strcmp(g->rName, args) == 0){
			return true;
		    }
		    g = g->next;
		}
		if (strcmp(g->rName, args) == 0){
		   return true;
		}
	    }
	}
	e = e->next;
    }
    if (strcmp(e->username, user) == 0){
	g = e->inRoom;
	if (g != NULL){
	    while (g->next != NULL){
		if (strcmp(g->rName, args) == 0){
		   return true;
		}
		g = g->next;
	    }
	    if (strcmp(g->rName, args) == 0){
		return true;
	    }
	}
    }
}

bool
IRCServer::userExists(const char * user) {
    for(int i = 0; i<userVec.size();i++){
	if((userVec[i].compare(user) == 0)){
	    return true;
	}
    }
    return false;
} 

    void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
    // Here add a new user. For now always return OK.

   if(userExists(user)){
	const char * msg = "LOGIN DENIED \r\n";
	write(fd, msg, strlen(msg));
    }
    else{
	passwordFile.open(PASSWORD_FILE, std::fstream::in | std::fstream::out | std::fstream::app);
	passwordFile << user << '\n' << password << "\n\n";
	passwordFile.close();
	passVec.push_back(password);
	userVec.push_back(user);

	Members * newUser = (Members*)malloc(sizeof(Members));
	newUser->password = password;
	newUser->username = user;
	newUser->args = args;
	newUser->inRoom = NULL;
	newUser->next = NULL;

	if (userList.head == NULL){
	    userList.head = newUser;
	    const char * msg =  "OK\r\n";
	    write(fd, msg, strlen(msg));
	    return;		
	}

	if (strcmp(user, userList.head->username) < 0){
	    newUser->next = userList.head;
	    userList.head = newUser;
	    const char * msg =  "OK\r\n";
	    write(fd, msg, strlen(msg));
	    return;
	}

	Members * tempoUser = userList.head;
	while (tempoUser->next != NULL){
	    if (strcmp(user, tempoUser->next->username) < 0){
		newUser->next = tempoUser->next;
		tempoUser->next = newUser;
		const char * msg = "OK\r\n";
		write(fd, msg, strlen(msg));
		return;
	    }
	    tempoUser = tempoUser->next;
	}
	tempoUser->next = newUser;


	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));

	return;		
    }
}

void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
       Members * e;
    if (userList.head == NULL){
	const char * message = "DENIED \r\n";
	write(fd, message, strlen(message));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * message = "ERROR (Wrong password)\r\n";
	write(fd, message, strlen(message));
	return;
    }
    char * rName = (char*)args;
    if (!checkRoom(fd, user, password, args)){
	const char * message = "ERROR (No room)\r\n";
	write(fd, message, strlen(message));
	return;
    }
    Room * g = (Room*)malloc(sizeof(Room));
    g->next = NULL;
    g->rName = rName;
    g->messageCounter = 0;

    e = userList.head;
    while (e->next != NULL){
	if (strcmp(user, e->username) == 0){
	    if (e->inRoom == NULL){
		e->inRoom = g;
	    }
	    else if (strcmp(e->inRoom->rName, rName) == 0){
	    }
	    else{
		e->inRoom->next = g;
	    }
	    const char * msg = "OK\r\n";
	    write(fd, msg, strlen(msg));
	    return;
	}
	e = e->next;
    }
    if (strcmp(user, e->username) == 0){
	if (e->inRoom == NULL){
	    e->inRoom = g;
	}
	else if (strcmp(e->inRoom->rName, rName) == 0){
	}
	else{
	    e->inRoom->next = g;
	}
	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
}

    void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
    Members * e;
    Room * g;
    Room * s;
    if (userList.head == NULL){
	const char * msg = "DENIED (NO USERS).\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkRoom(fd, user, password, args)){
	const char * msg = "Error (Room DNE)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkUserExistInRoom2(fd, user, password, args)){
	const char * msg = "ERROR (No user in room)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    g = roomList.head;
    while(g->next != NULL){
	if (strcmp(g->rName, args) == 0){
	    break;
	}
	g = g->next;
    }

    e = userList.head;
    while (e->next != NULL){
	if (strcmp(e->username, user) == 0){
	    while (e->inRoom->next != NULL){
		if (strcmp(e->inRoom->rName, args) == 0){
		    e->inRoom = NULL;
		    const char * msg = "OK\r\n";
		    write(fd, msg, strlen(msg));
		    return;
		}
		e->inRoom = e->inRoom->next;
	    }
	    if (strcmp(e->inRoom->rName, args) == 0){
		e->inRoom = NULL;
		const char * msg = "OK\r\n";
		write(fd, msg, strlen(msg));
		return;
	    }
	}
	e = e->next;
    }
    if (strcmp(e->username, user) == 0){
	while (e->inRoom->next != NULL){
	    if (strcmp(e->inRoom->rName, args) == 0){
		e->inRoom = NULL;
		const char * msg = "OK\r\n";
		write(fd, msg, strlen(msg));
		return;
	    }
	    e->inRoom = e->inRoom->next;
	}
	if (strcmp(e->inRoom->rName, args) == 0){        
	    e->inRoom = NULL;
	    const char * msg = "OK\r\n";
	    write(fd, msg, strlen(msg));
	    return;
	}
    }

}

    void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
    Members * e;
    if (userList.head == NULL){
	const char * msg = "DENIED (NO USERS).\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    e = userList.head;
    while (e->next != NULL){
	if (strcmp(user, e->username) == 0){
	    break;
	}
	e = e->next;
    }

    char * tempoRoom = (char*)malloc(sizeof(char)*100);
    char * rName = tempoRoom;
    char * tempoMessage = (char*)malloc(sizeof(char)*1000);
    char * tempoArgs = (char*)args;
    while (*tempoArgs != ' ' && *tempoArgs != '\0'){
	*tempoRoom = *tempoArgs;
	tempoArgs++;
	tempoRoom++;
    }
    tempoArgs ++;
    tempoMessage = tempoArgs;

    Message * newMessage = (Message*)malloc(sizeof(Message));
    newMessage->message = tempoMessage;
    newMessage->messageFrom = e;
    newMessage->room = rName;
    newMessage->next = NULL;

    Room * r;
    if (roomList.head == NULL){
	const char * msg = "ERROR (NO ROOMS)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    r = roomList.head;
    while (r->next != NULL){
	if (strcmp(r->rName, rName) == 0){
	    newMessage->messageNumber = r->messageCounter;
	    r->messageCounter ++;
	}
	r = r->next;
    }
    if (strcmp(r->rName, rName) == 0){
	newMessage->messageNumber = r->messageCounter;
	r->messageCounter ++;
    }

    if (!checkUserExistInRoom2(fd, user, password, newMessage->room)){
	const char * msg = "ERROR (user not in room)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    Message * f;
    if (messageList.head == NULL){
	newMessage->next;
	messageList.head = newMessage;
	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    f = messageList.head;
    while (f->next != NULL){
	f = f->next;
    }
    f->next = newMessage;

    const char * msg = "OK\r\n";
    write(fd, msg, strlen(msg));
    return;
}

    void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
    Members * e;
    if (userList.head == NULL){
	const char * msg = "ERROR (No users)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    e = userList.head;
    while (e->next != NULL){
	if (strcmp(e->username, user) == 0){
	    break;
	}
	e = e->next;
    }
    int tempoMessageCount = atoi(args);
    while (*args != ' '){
	args++;
    }
    args ++;

    char * rName = (char*)args;

    if (!checkUserExistInRoom2(fd, user, password, rName)){
	const char * msg = "ERROR (User not in room)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    Message * m;
    Message * n;
    int lastMessageNumber = 0;
    if (messageList.head == NULL){
	const char * msg = "NO-NEW-MESSAGES\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    m = messageList.head;
    n = messageList.head;
    while (n->next != NULL){
	if (strcmp(n->room, rName) == 0){
	    lastMessageNumber = n->messageNumber;
	}
	n = n->next;
    }
    if (strcmp(n->room, rName) == 0){
	lastMessageNumber = n->messageNumber;
    }

    if (lastMessageNumber <= tempoMessageCount){
	const char * msg = "NO-NEW-MESSAGES\r\n";
	write(fd, msg, strlen(msg));
	return;
    }

    while (m->next != NULL){
	if (strcmp(m->room, rName) == 0 && tempoMessageCount < m->messageNumber){
	    char * messageOut = (char*)malloc(sizeof(char)*2000);

	    char buffer[50];
	    sprintf(buffer, "%d ", m->messageNumber);
	    char * message1 = (char*)buffer;
	    char * message2 = (char*)m->messageFrom->username;
	    char * message3 = (char*)m->message;
	    char * message4 = (char*)"\r\n";
	    
	    strcat(messageOut, message1);
	    strcat(messageOut, message2);
	    strcat(messageOut, " ");
	    strcat(messageOut, message3);
	    strcat(messageOut, message4);

	    write(fd, messageOut, strlen(messageOut));
	}
	m = m->next;
    }
    if (strcmp(m->room, rName) == 0 && tempoMessageCount <= m->messageNumber){
	char * messageOut = (char*)malloc(sizeof(char)*2000);

	char buffer[50];
	sprintf(buffer, "%d ", m->messageNumber);
	char * message1 = (char*)buffer;
	char * message2 = (char*)m->messageFrom->username;
	char * message3 = (char*)m->message;
	char * message4 = (char*)"\r\n";

	strcat(messageOut, message1);
	strcat(messageOut, message2);
	strcat(messageOut, " ");
	strcat(messageOut, message3);
	strcat(messageOut, message4);

	write(fd, messageOut, strlen(messageOut));
    }
    char * ending = (char*)"\r\n";
    write(fd, ending, strlen(ending));

}
void
IRCServer::getMessages2(int fd, const char * user, const char * password, const char * args)
{
    string message;
    Members * e;
    if (userList.head == NULL){
	const char * msg = "ERROR (No users)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    e = userList.head;
    while (e->next != NULL){
	if (strcmp(e->username, user) == 0){
	    break;
	}
	e = e->next;
    }
    int tempoMessageCount = atoi(args);
    while (*args != ' '){
	args++;
    }
    args ++;

    char * rName = (char*)args;

    if (!checkUserExistInRoom2(fd, user, password, rName)){
	const char * msg = "ERROR (User not in room)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    Message * m;
    Message * n;
    int lastMessageNumber = 0;
    if (messageList.head == NULL){
	const char * msg = "NO-NEW-MESSAGES\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    m = messageList.head;
    n = messageList.head;
    while (n->next != NULL){
	if (strcmp(n->room, rName) == 0){
	    lastMessageNumber = n->messageNumber;
	}
	n = n->next;
    }
    if (strcmp(n->room, rName) == 0){
	lastMessageNumber = n->messageNumber;
    }

    if (lastMessageNumber <= tempoMessageCount){
	const char * msg = "NO-NEW-MESSAGES\r\n";
	write(fd, msg, strlen(msg));
	return;
    }

    while (m->next != NULL){
	if (strcmp(m->room, rName) == 0 && tempoMessageCount < m->messageNumber){
	    char * messageOut = (char*)malloc(sizeof(char)*2000);

	    char buffer[50];
	    sprintf(buffer, "%d ", m->messageNumber);
	    char * message1 = (char*)buffer;
	    char * message2 = (char*)m->messageFrom->username;
	    char * message3 = (char*)m->message;
	    char * message4 = (char*)"\r\n";
	    char * colon = (char*)":";
	    
	    strcat(messageOut, message2);
	    strcat(messageOut, colon);
	    strcat(messageOut, " ");
	    strcat(messageOut, message3);
	    strcat(messageOut, message4);

	    message = message + string(messageOut);

	    //write(fd, messageOut, strlen(messageOut));
	}
	m = m->next;
    }
    if (strcmp(m->room, rName) == 0 && tempoMessageCount <= m->messageNumber){
	char * messageOut = (char*)malloc(sizeof(char)*2000);

	char buffer[50];
	sprintf(buffer, "%d ", m->messageNumber);
	char * message1 = (char*)buffer;
	char * message2 = (char*)m->messageFrom->username;
	char * message3 = (char*)m->message;
	char * message4 = (char*)"\r\n";
	char * colon = (char*)":";


	strcat(messageOut, message2);
	strcat(messageOut, colon);
	strcat(messageOut, " ");
	strcat(messageOut, message3);
	strcat(messageOut, message4);

	message = message + string(messageOut);

	//write(fd, messageOut, strlen(messageOut));
    }
    char * ending = (char*)"\r\n";
    message = message + string(ending);
    write(fd, message.c_str(), message.length());

}


    void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
    Members * e;
    Room * f;
    string message;

    if (userList.head == NULL){
	const char * msg = "DENIED (NO USERS).\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    e = userList.head;
    char * roomName = (char*)args;
    while (e->next != NULL){
	f = e->inRoom;
	if (f != NULL){
	    while (f->next != NULL){
		if (strcmp(f->rName, roomName) == 0){
		    char * msg1 = (char*)e->username;
		    //char * msg2 = (char*)"\r\n";
		    //write(fd, msg1, strlen(msg1));
		    message = message + string(msg1) + string("\r\n");
		}
		f = f->next;
	    }
//	    write(fd, message.c_str(), message.length());
	    if (strcmp(f->rName, roomName) == 0){
		char * msg1 = (char*)e->username;
		//char * msg2 = (char*)"\r\n";
		//write(fd, msg1, strlen(msg1));
		message = message + string(msg1) + string("\r\n");
	    }
	}
	e = e->next;
    }
  //  write(fd, message.c_str(), message.length());
    f = e->inRoom;
    if (f != NULL){
	while (f->next != NULL){
	    if (strcmp(f->rName, roomName) == 0){
		char * msg1 = (char*)e->username;
		//char * msg2 = (char*)"\r\n";
		//write(fd, msg1, strlen(msg1));
		message = message + string(msg1) + string("\r\n");
	    }
	    f = f->next;
	}
	//write(fd, message.c_str(), message.length());
	if (strcmp(f->rName, roomName) == 0){
	    char * msg1 = (char*)e->username;
	    //char * msg2 = (char*)"\r\n";
	    //write(fd, msg1, strlen(msg1));
	    //write(fd, msg2, strlen(msg2));
	    message = message + string(msg1) + string("\r\n");
	}
    }
    char * msg2 = (char*)"\r\n";
    message = message + string(msg2);
    //write(fd, msg1, strlen(msg1));
    write(fd, message.c_str(), message.length());
}

    void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
    string message;
    Members * e;
    if (userList.head == NULL){
	const char * msg = "DENIED (NO USERS).\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    e = userList.head;
    while (e->next != NULL){
	char * msg1 = (char*)e->username;
	/*char * msg2 = (char*)"\r\n";
	write(fd, msg1, strlen(msg1));
	write(fd, msg2, strlen(msg2));*/
	message = message + string(msg1) + string("\r\n");
	e = e->next;
	printf("user: %s\n", msg1);
    }
    char * msg1 = (char*)e->username;
    //char * msg2 = (char*)"\r\n\r\n";
    message = message + string(msg1) + string("\r\n\r\n");
    //write(fd, msg1, strlen(msg1));
    write(fd, message.c_str(), message.length());
    printf("user: %s\n", msg1);
    return;

}
    void
IRCServer::createRoom(int fd, const char * user, const char * password,const  char * args)
{

    if (userList.head == NULL){
	const char * msg = "DENIED (NO USERS).\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    if (!checkPassword(fd, user, password)){
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    /*if(!checkRoom(fd, user, password,args)){
      roomVec.push_back(args);
      }*/
    Room * newRoom = (Room*)malloc(sizeof(Room));
    newRoom->rName = args;
    newRoom->messageCounter = 0;
    newRoom->next = NULL;
    if (roomList.head == NULL){
	roomList.head = newRoom;
	roomVec.push_back(args);
	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
	return;
    }
    roomVec.push_back(args);
    newRoom->next = roomList.head;
    roomList.head = newRoom;

    const char * msg = "OK\r\n";
    write(fd, msg, strlen(msg));
    return;

}

    void
IRCServer::listRooms(int fd, const char * user, const char * password,const  char * args)
{
    string message;
    /*members * e;
    Room * g;
    if (userList.head == NULL){
    const char * msg = "DENIED (NO USERS).\r\n";
    write(fd, msg, strlen(msg));
    return;
    }
    if (!checkPassword(fd, user, password)){
    const char * msg = "ERROR (Wrong password)\r\n";
    write(fd, msg, strlen(msg));
    return;
    }
    for(int i = 0; i < roomVec.size(); i++) {
    const char * msg1 = roomVec[i].c_str();
    message = message + string(msg1) + string("\r\n");
    printf("room: %s\n", msg1);
    }

    g = roomList.head;
    while (g->next != NULL){
    char * msg1 = (char*)g->rName;
    message = message + string(msg1) + string("\r\n");
    g = g->next;
    printf("room: %s\n", msg1);
    }
    char * msg1 = (char*)g->rName;
    message = message + string(msg1) + string("\r\n\r\n");
    write(fd, message.c_str(), message.length());
    printf("room: %s\n", msg1);
    return;*/
    if(checkPassword(fd, user, password)) {
	for(int i = 0; i < roomVec.size(); i++) {
	    string msg = roomVec[i].c_str();
	    message = message + msg + string("\r\n");
	}
	if(message!=""){
	    int x = message.length();
	    write(fd, message.c_str(), x);
	}

    } else {
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
    }
}

/*	for(int i = 0; i < roomVec.size(); i++) {
	const char * msg = roomVec[i].c_str();
	const char * msg2 = "\r\n";
	message = message + string(msg) + string(msg2);
	printf("Rooms are%s\n",message.c_str());
	write(fd, message.c_str(), message.length());
	}
	}
	else{
	const char * msg = "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return;
	}
	}

	g = roomList.head;
	while (g->next != NULL){
	char * msg1 = (char*)g->rName;
	char * msg2 = (char*)"\r\n";
	write(fd, msg1, strlen(msg1));
	write(fd, msg2, strlen(msg2));
	message = message + string(msg1) + string("\r\n");
	g = g->next;
	printf("room: %s\n",msg1);
	}
	char * msg1 = (char*)g->rName;
//   char * msg2 = (char*)"\r\n\r\n";
message = message + string(msg1) + string("\r\n");
//   write(fd, msg1, strlen(msg1));
write(fd, message.c_str(), message.length());
printf("room: %s\n",msg1);
return;*/

