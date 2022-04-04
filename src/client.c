#include "wordle/wordle.h"

/* Thread Functions */

void *msg_thread(void *arg) {
	char buffer[BUFSIZ];
	int *fd = (int *) arg;
	while (1) {
		fgets(buffer, BUFSIZ, stdin);
		puts("sending");
		Message *msg = message_from_command(buffer);

		char *str = message_to_json(msg, msg->type);
		printf("%s\n", str);
		send(*fd, str, strlen(str) + 1, 0);
		free(str);
		free(msg);
	}
}

int main(int argc, char *argv[]) {
	if (argc != 3) { puts("Error"); return 0; }

	char *host = argv[1];
	char *port = argv[2];



	char buffer[BUFSIZ];

	int fd = socket_dial(host, port);
	pthread_t thread;
	pthread_create(&thread, NULL, msg_thread, &fd);

	while (1) {
		if (!recv(fd, buffer, BUFSIZ, 0)) exit(0);
		if (strstr(buffer, "good")) {
			puts("enter game");
			return 0;
		}
		printf("response: \n%s\n", buffer);
	}
	return 0;
}
