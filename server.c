#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "error_handle.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2622
#define SERVER_NAME "OIChat"
#define MAX_LEN_LOGIN 64
#define MAX_LEN_MSG 1024
#define MAX_CNT_CLIENTS 10

#define EXIT_CMD "$q"

int server;
struct clients {
	int status;
	int id;
	int socket;
	pthread_t thread;
	char login[MAX_LEN_LOGIN];
} clients[MAX_CNT_CLIENTS];
int cnt_clients;

void close_server();
void *connect_clients();
void *input_command();
void *work(void *number);

pthread_t input_server;
pthread_t server_connect;

int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr;

	int cnt_read;
	char msg[MAX_LEN_MSG];

	/* Bind server */
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0)
        error_exit("Fail of create socket!");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	printf("Start of server...\n");
	if (bind(	server, (struct sockaddr *) &server_addr, 
				sizeof(server_addr)) == -1)
		error_exit("Fail of start for server");
	printf("Server started!\n");


	/* Thread for listen clients to connect */
	if (pthread_create(&server_connect, NULL, connect_clients, NULL))
		error_exit("Thread for listen server don`t create! ");

	/* Thread for input command to server */
	if (pthread_create(&input_server, NULL, input_command, NULL))
		error_exit("Thread for listen server don`t create! ");

	pthread_join(input_server, NULL);
    return 0;
}


void *connect_clients()
{
	cnt_clients = 0;
	printf("Wait connect clients...");
	fflush(stdout);
	listen(server, 1);
	while (1) {
		clients[cnt_clients].socket = accept(server, NULL, NULL);
		if (cnt_clients >= MAX_CNT_CLIENTS) {
			printf("Count of clients is max! I have ignor next conection!");
			printf("\n%s: ", SERVER_NAME);
			continue;
		}
		clients[cnt_clients].id = cnt_clients;
		if (pthread_create(&clients[cnt_clients].thread, NULL, 
							work, &clients[cnt_clients].id))
			error_exit("Thread for client don`t create! ");
		char new_login[MAX_LEN_MSG];
		recv(clients[cnt_clients].socket, new_login, MAX_LEN_LOGIN, 0);
		if (strlen(new_login) != 0 && !exist(new_login)) {
			strcpy(clients[cnt_clients].login, new_login);
			send(clients[cnt_clients].socket, "res=1", 5, 0);
			clients[cnt_clients].status = 1;
			printf("Client <%s> was connected!", clients[cnt_clients].login);
			printf("\n%s: ", SERVER_NAME);
			fflush(stdout);
			cnt_clients++;
		}
		else {
			send(clients[cnt_clients].socket, "res=0", 5, 0);
		}
	}
	pthread_exit(NULL);
}

void *input_command()
{
	char msg[MAX_LEN_MSG];
	int cnt_read;
	while (1) {
		printf("\n%s: ", SERVER_NAME);
		fflush(stdout);
		cnt_read = read(0, msg, MAX_LEN_MSG);
		if (strncmp(msg, EXIT_CMD, strlen(EXIT_CMD)) == 0) {
			/* Close server */
			printf("%s: Closing server...\n", SERVER_NAME);
			pthread_kill(server_connect, NULL);
			close_server();
			break;
		}
		memset(msg, 0, cnt_read);
	}
	pthread_exit(NULL);
}

void *work(void *number)
{
	int N = *(int *) number;
	char msg_recv[MAX_LEN_MSG];
	char msg_send[MAX_LEN_MSG];
	int cnt_read;
	int dst_id;

	while (clients[N].status == 1) {
		cnt_read = recv(clients[N].socket, msg_recv, MAX_LEN_MSG, 0);
		if (strncmp(msg_recv, EXIT_CMD, strlen(EXIT_CMD)) == 0) {
			clients[N].status = 0;
			cnt_clients--;
			close(clients[N].socket);
			break;
		}
		dst_id = acos_clients_id(msg_recv);
		if (dst_id != -1) {
			printf(	"<%s> -> <%s>: %s",	clients[N].login, clients[dst_id].login, 
					msg_recv + strlen(clients[dst_id].login) + 2);
			memset(msg_send, 0, MAX_LEN_MSG);
			sprintf(msg_send, 	"<%s>: %s", 
								clients[N].login,
								msg_recv + strlen(clients[N].login) + 2);
			send(clients[dst_id].socket, msg_send, MAX_LEN_MSG, 0);
		} else {
			printf("<%s>: %s", clients[N].login, msg_recv);
			memset(msg_send, 0, MAX_LEN_MSG);
			sprintf(msg_send, "%s: That user is not connected", SERVER_NAME);
			send(clients[N].socket, msg_send, MAX_LEN_MSG, 0);
		}
		printf("\n%s: ", SERVER_NAME);
		fflush(stdout);
	}
	printf("User <%s> exit", clients[N].login);
	printf("\n%s: ", SERVER_NAME);
	fflush(stdout);
	pthread_exit(NULL);
}

void close_server()
{
	cnt_clients--;
	while (cnt_clients > -1) {
		pthread_kill(clients[cnt_clients].thread, 0);
		send(clients[cnt_clients].socket, EXIT_CMD, strlen(EXIT_CMD), 0);
		close(clients[cnt_clients].socket);
		cnt_clients--;
	}
	close(server);
	return;
}

int exist(char login[MAX_LEN_LOGIN])
{
	int i;
	for(i = 0; i < MAX_CNT_CLIENTS; i++) {
		if (strcmp(clients[i].login, login) == 0)
			return 1;
	}
	return 0;
}

int acos_clients_id(char msg[MAX_LEN_MSG])
{
	int i;
	for(i = 0; i < MAX_CNT_CLIENTS; i++) {
		if (strncmp(msg, clients[i].login, strlen(clients[i].login)) == 0 &&
			clients[i].status == 1)
			return i;
	}
	return -1;
}
