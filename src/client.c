#include "wordle/wordle.h"


/* definitions */

#define BORDER "------------------"

/* Thread Functions */

typedef struct Client_info {
	char name[256];
	int fd;

} Client_info; 

Client_info client_info;

pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;

void *msg_thread(void *arg) {
	char buffer[BUFSIZ];
	Client_info *ci = (Client_info *) arg;
	while (1) {
		fgets(buffer, BUFSIZ, stdin);
		//printf("%s|\n", buffer);
		Message *msg = message_from_command(buffer, ci->name);

		if (msg) {
			char *str = message_to_json(msg, msg->type);

			pthread_mutex_lock(&Lock);
			send(ci->fd, str, strlen(str) + 1, 0);
			pthread_mutex_unlock(&Lock);
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
	

	//printf("first fd %d\n", fd);
	client_info.fd = fd;
	pthread_t thread;
	pthread_create(&thread, NULL, msg_thread, &client_info);

	while (1) {
		int bytes = 0;
		if ((bytes = recv(fd, buffer, BUFSIZ, 0)) <= 0) { printf("error %d\n", fd); exit(0); }
		//printf("%s|\n", buffer);
		//printf("%d %ld\n", bytes, strlen(buffer));

		Message *msg = message_from_json(buffer);
		//printf("%d\n", msg->type);
		if (!msg) continue;

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
	
		if (msg->type == START_INSTANCE) {
			printf("join %s on port %s with id %d\n", msg->server, msg->port, msg->nonce);
			pthread_mutex_lock(&Lock);
			Message m;
			m.nonce = msg->nonce;
			strcpy(m.name, name);
			m.type = JOIN_INSTANCE;
			str = message_to_json(&m, JOIN_INSTANCE);
			close(fd);
			fd = socket_dial(msg->server, msg->port);
			client_info.fd = fd;
			//printf("new fd %d \n", fd);
			
			send(fd, str, strlen(str) + 1, 0);
			free(str);
			pthread_mutex_unlock(&Lock);
			//usleep(10000);
		}

		if (msg->type == START_GAME) {
			puts(BORDER);
			puts("starting game");
			//printf("%s\n", buffer);
			print_info(msg, START_GAME);
			puts(BORDER);
		}

		if (msg->type == START_ROUND) {
			puts(BORDER);
			printf("Word Length: %d Round: %d Rounds Left: %d\n", msg->word_length, msg->round, msg->rounds_remaining);
			print_info(msg, START_ROUND);
			puts(BORDER);	
		}
	
		if (msg->type == GUESS_RESPONSE) {
			if (strstr(msg->accepted, "es")) {
				puts("accepted guess");
			} else {
				puts("guess not accepted");
			}
		}

		if (msg->type == END_ROUND) {
			puts(BORDER);
			puts("round over");
			print_info(msg, END_ROUND);
			puts(BORDER);
		}

		if (msg->type == END_GAME) {
			puts(BORDER);
			puts("game over");
			printf("%s Wins!\n", msg->winnerName);
			print_info(msg, END_GAME);
			puts(BORDER);
		}

		if (msg->type == GUESS_RESULT) {
			puts(BORDER);
			print_info(msg, GUESS_RESULT);	
			puts(BORDER);
		}

		if (msg->type == PROMPT) {
			puts("make your guess");
		}
		free(msg);
		
	}
	return 0;
}
