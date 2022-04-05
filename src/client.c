#include "wordle/wordle.h"

/* Thread Functions */

typedef struct Client_info {
	char name[256];
	int fd;

} Client_info; 

void *msg_thread(void *arg) {
	char buffer[BUFSIZ];
	Client_info *ci = (Client_info *) arg;
	while (1) {
		fgets(buffer, BUFSIZ, stdin);
		//printf("%s|\n", buffer);
		Message *msg = message_from_command(buffer, ci->name);

		if (msg) {
			char *str = message_to_json(msg, msg->type);
			send(ci->fd, str, strlen(str) + 1, 0);
			free(str);
			free(msg);
		}
	}
}

int main(int argc, char *argv[]) {

	if (argc != 4) { puts("Error"); return 0; }

	char *name = argv[1];
	char *host = argv[2];
	char *port = argv[3];

	Client_info client_info;
	strcpy(client_info.name, name);
	

	char buffer[BUFSIZ];

	int fd = socket_dial(host, port);

	/* send join request */
	Message init;
	strcpy(init.name, name);
	strcpy(init.client, "bwisemaSgood");
	init.type = JOIN;

	char *str = message_to_json(&init, JOIN);

	send(fd, str, strlen(str) + 1, 0);
   	free(str);
	


	client_info.fd = fd;
	pthread_t thread;
	pthread_create(&thread, NULL, msg_thread, &client_info);

	while (1) {
		if (recv(fd, buffer, BUFSIZ, 0) <= 0) { puts("error"); exit(0); }
		//printf("%s|\n", buffer);
		Message *msg = message_from_json(buffer);
		if (msg->type == CHAT) {
			printf("(%s): %s\n", msg->name, msg->text);
		}

		if (msg->type == JOIN_RESULT) {
			if (strcmp(msg->result, "No")) {
				puts("You have joined the game");
			} else {
				puts("failed to join");
				exit(0);
			}
		}
		
		free(msg);
		
	}
	return 0;
}
