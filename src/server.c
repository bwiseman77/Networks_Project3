#include "wordle/wordle.h"
#include "wordle/server_socket.h"

#define MAX_PLAYERS 50

pthread_t threads[MAX_PLAYERS];
int Clients[MAX_PLAYERS];
int Players = 0;

void *client_thread(void *arg) {
	int *fd = (int *)arg;
	char buffer[BUFSIZ];

	/* recieve messages, and send to everyone if CHAT */

	while(1) {
		if (!recv(*fd, buffer, BUFSIZ, 0)) return 0;
		printf("from fd %d: %s\n", *fd, buffer);
		
		/* send to everyone but sender */
		for (int i = 0; i < Players; i++) {	
			if (Clients[i] != *fd) {
				send(Clients[i], buffer, BUFSIZ, 0);
			}
		}

	}
}	



int main(int argc, char *argv[]) {
	/* set up server */
	if (argc != 2) { puts("error"); return 0; }
	
	char *port = argv[1];
	int server_fd;

	if ((server_fd = socket_listen(port)) < 0) return 0;

	printf("listening on port %s\n", port);
	
	char buffer[BUFSIZ];
	
	/* main loop */
	while (1) {

		/* gather clients */
		int client_fd = socket_accept(server_fd);	
		
		recv(client_fd, buffer, BUFSIZ, 0);
		puts("got message");
		Message *msg = message_from_json(buffer);
		printf("%s\n", msg->name);
		free(msg);

		/* add to list of clients, and start thread */
		Clients[Players] = client_fd;	
		pthread_create(&threads[Players], NULL, client_thread, &Clients[Players]);

		/* add to players */	
		Players++;

		/* start game */
		if (Players >= 3) {
			send(client_fd, "good to go\0", BUFSIZ, 0);
			return 0;
		}
	}
	
	return 0;
}
