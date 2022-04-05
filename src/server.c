#include "wordle/wordle.h"
#include "wordle/server_socket.h"

#include <stdlib.h>
#define MAX_PLAYERS 50



pthread_t threads[MAX_PLAYERS];
int Clients[MAX_PLAYERS];
int Players = 0;

void *client_thread(void *arg) {
	int *fd = (int *)arg;
	char buffer[BUFSIZ];

	/* recieve messages, and send to everyone if CHAT */
	while(1) {
		if (!recv(*fd, buffer, BUFSIZ, 0)) {
			printf("client from %d quit\n", *fd);
			close(*fd);	
			return 0;
		}
		// buffer is json
		printf("from fd %d: %s\n", *fd, buffer);
		
		Message *msg = message_from_json(buffer);
		//Message response;
		


		/* send to everyone but sender */
		if (msg->type == CHAT) {
			for (int i = 0; i < Players; i++) {
				//printf("%d\n", i);
				if (Clients[i] != *fd) {
					send(Clients[i], buffer, BUFSIZ, 0);
				}
			}
		}
/*
		if (msg->type == JOIN) {
			strcpy(response.name, msg->name);
			response.Result = true;	
			char *str = message_to_json(&response, JOIN_RESULT);
			send(*fd, str, strlen(str) + 1, 0);
			free(str);
		}
*/
		
		free(msg);
	}
}	



int main(int argc, char *argv[]) {
	/* set up server */
	if (argc != 2) { puts("error"); return 0; }
	
	char *port = argv[1];
	int server_fd;
	int num_players = atoi(argv[2]);

	if ((server_fd = socket_listen(port)) < 0) { puts("error"); return 0; }

	printf("listening on port %s\n", port);
	
	char buffer[BUFSIZ];
	
	/* main loop */
	while (1) {

		/* gather clients */
		int client_fd = socket_accept(server_fd);	
		
		/* accept clients */
		recv(client_fd, buffer, BUFSIZ, 0);
		Message *msg = message_from_json(buffer);
		char *s = message_to_json(msg, msg->type);
		printf("request: %s\n", s);
		free(s);
		
		if (msg->type == JOIN) {
	
			Message response;
			char *str;

			if (Players >= num_players) {	

				/* send No response to extra players */
				strcpy(response.name, msg->name);
				strcpy(response.result, "No");	
				str = message_to_json(&response, JOIN_RESULT);
				send(client_fd, str, strlen(str) + 1, 0);
				free(str);

			} else { 
		
				/* send Yes reponse to players */
				strcpy(response.name, msg->name);
				strcpy(response.result, "Yes");	
				str = message_to_json(&response, JOIN_RESULT);
				send(client_fd, str, strlen(str) + 1, 0);
				free(str);


				/* add to list of clients, and start thread */
				Clients[Players] = client_fd;	
				pthread_create(&threads[Players], NULL, client_thread, &Clients[Players]);
	
				free(msg);

				/* add to players */	
				Players++;

				/* start game */
				if (Players >= num_players) {
					Message start;
					strcpy(start.server, "Server");
					strcpy(start.port, "9999");
					start.type = START_GAME;
					for (int i = 0; i < Players; i++) {
						start.nonce = i;
						char *s = message_to_json(&start, START_GAME); 
						send(Clients[i], s, BUFSIZ, 0);
					}
				
				}

			}
		}
	}

	return 0;
}
