
#ifndef IRC_SERVER
#define IRC_SERVER

#define PASSWORD_FILE "password.txt"

class IRCServer {
	// Add any variables you need

	struct Room{
	    const char * rName;
	    int messageCounter;
	    Room * next;
	};
	typedef struct Room Room;

	struct ListRooms{
	    Room * head;
	};
	typedef struct ListRooms ListRooms;

	struct Members{
	    const char * password;
	    const char * username;
	    const char * args;
	    Room * inRoom;
	    struct Members * next;
	};
	typedef struct Members members;

	struct ListUsers{
	    Members * head;
	};
	typedef struct ListUsers ListUsers;

	struct Message{
	    const char * message;
	    const char * room;
	    int messageNumber;
	    Members * messageFrom;
	    Message * next;
	};
	typedef struct Message Message;

	struct ListMessage{
	    Message * head;
	};
	typedef struct ListMessage ListMessage;

private:
	int open_server_socket(int port);
	ListUsers userList;
	ListMessage messageList;
	ListRooms roomList;
	int messageCounter;


public:
	void initialize();
	bool checkPassword(int fd, const char * user, const char * password);
	void processRequest( int socket );
	void addUser(int fd, const char * user, const char * password, const char * args);
	void enterRoom(int fd, const char * user, const char * password, const char * args);
	void leaveRoom(int fd, const char * user, const char * password, const char * args);
	void sendMessage(int fd, const char * user, const char * password, const char * args);
	void getMessages(int fd, const char * user, const char * password, const char * args);
	void getMessages2(int fd, const char * user, const char * password, const char * args);
	void getUsersInRoom(int fd, const char * user, const char * password, const char * args);
	void getAllUsers(int fd, const char * user, const char * password, const char * args);
	void createRoom(int fd, const char * user, const char * password, const char * args);
	void listRooms(int fd, const char * user, const char * password, const char * args);
	bool checkRoom(int fd, const char * user, const char * password, const char * args);
	bool checkUserExistInRoom(int fd, const char * user, const char * password, const char * args);
	bool userExists(const char * user);
	bool checkUserExistInRoom2(int fd, const char * user, const char * password, const char * args);
	void logIn(int fd, const char * user, const char * password);
	void runServer(int port);
};

#endif
