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

#define EXIT_CMD "$q"

char login[MAX_LEN_LOGIN];
int active_client = 1;
int server;

void *pt_recv_msg();
void *pt_send_msg();

int main(int argc, char *argv[])
{
	struct sockaddr_in server_addr;

	int cnt_read;
	char msg[MAX_LEN_MSG];

	/* Connect to server */
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0)
        error_exit("Fail of create socket!");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	printf("Сonnect with the server...\n");
	if (connect(server, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
		error_exit("Fail of connect for server");
	sleep(1);
	printf("Server connected!\n");


	/* Authorization on server */
	printf("Enter your login, please: ");
	fflush(stdout);
	cnt_read = read(0, login, MAX_LEN_LOGIN);
	login[cnt_read-1] = '\0';
	send(server, login, strlen(login), 0);
	recv(server, msg, MAX_LEN_MSG, 0);
	char *result = strstr(msg, "res=");
	if (!result || result[strlen("res=")] != '1') {
		errno = 0;
		error_exit("Fail in authorization! \n");
	}
	printf("Welcome from OIChat, <%s>!\n", login);


	/* Thread for input message */
	pthread_t input_msg;
	if (pthread_create(&input_msg, NULL, pt_recv_msg, NULL))
		error_exit("Thread for input message don`t create! ");

	/* Thread for input message */
	pthread_t output_msg;
	if (pthread_create(&output_msg, NULL, pt_send_msg, NULL))
		error_exit("Thread for output message don`t create! ");

	/* Close client */
	while(active_client);
	pthread_kill(output_msg, 0);
	pthread_kill(input_msg, 0);
	close(server);
	printf("Conection is closed! Thank you for using OIChat. Goodbye!\n");
    return 0;
}

void *pt_recv_msg()
{
	char msg[MAX_LEN_MSG];
	int cnt_recv;
	while (1) {
		cnt_recv = recv(server, msg, MAX_LEN_MSG, 0);
		if (strncmp(msg, EXIT_CMD, strlen(EXIT_CMD)) == 0 || cnt_recv == -1) {
			active_client = 0;
			break;
		}
		printf("\n%s", msg);
		printf("\n%s: ", login);
		fflush(stdout);
		sleep(1);
	}
	printf("\n");
	pthread_exit(NULL);
}

void *pt_send_msg()
{
	char msg[MAX_LEN_MSG];
	int cnt_read;
	while (1) {
		printf("%s: ", login);
		fflush(stdout);
		cnt_read = read(0, msg, MAX_LEN_MSG);
		send(server, msg, MAX_LEN_MSG, 0);
		if (strncmp(msg, EXIT_CMD, strlen(EXIT_CMD)) == 0) {
			active_client = 0;
			break;
		}
		memset(msg, 0, cnt_read);
	}
	pthread_exit(NULL);
}

