#include "wordle/wordle.h"
#include "wordle/server_socket.h"
#include <stdlib.h>

#define MAX_PLAYERS 50

void send_message(char *str, char *name, int fd) {
	//printf("%s|\n", str);
	char *s;
	Message *msg = message_from_command(str, name);
	//printf("s: %d\n", msg->type);
	s = message_to_json(msg, msg->type);
	//printf("%s\n", s);
	send(fd, s, strlen(s) + 1, 0);
	free(s);
	free(msg);
}

/* structures */

typedef struct Client_info {
	int fd;
	int score;
	bool inGame;
	bool isTurn;
	char name[256];
	int nonce;
} Client_info; 

/* Globals */

pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t threads[MAX_PLAYERS];
Client_info Clients[MAX_PLAYERS];
int Players = 0;
bool Game = false;

/* Thread Functions */

void *client_thread(void *arg) {
	Client_info *client = (Client_info *)arg;
	char buffer[BUFSIZ];

	/* recieve messages, and send to everyone if CHAT */
	while(1) {
		if (!recv(client->fd, buffer, BUFSIZ, 0)) {
			printf("client %s quit\n", client->name);
			close(client->fd);
			client->inGame = false;	
			return 0;
		}
		// buffer is json
		printf("from fd %d: %s\n", client->fd, buffer);
		
		Message *msg = message_from_json(buffer);
		//Message response;
		printf("%d\n", msg->type);	
		/* send to everyone but sender */
		if (msg->type == CHAT) {
			for (int i = 0; i < Players; i++) {
				//printf("%d\n", i);
				if (client->inGame && Clients[i].fd != client->fd ) {
					send(Clients[i].fd, buffer, BUFSIZ, 0);
				}
			}
		}

		free(msg);
	}
	puts("leave client thread");
}	

void * game_thread(void * arg) {
	int game_fd = socket_listen("43000");
	char buff[BUFSIZ];
	puts("into game thread");
	int players_in_game = 0;
	Game = true;

	/* collect players */
	while(players_in_game < Players) {
		int new_fd = socket_accept(game_fd);
		recv(new_fd, buff, BUFSIZ, 0);
		Message *msg = message_from_json(buff);
		char *s = message_to_json(msg, msg->type);
		printf("in game: %s\n", s);
		if (msg->type == JOIN_INSTANCE) {
			for (int i = 0; i < Players; i++) {
				if (msg->nonce == Clients[i].nonce) {
					Clients[i].fd = new_fd;
					Clients[i].inGame = true;
					pthread_create(&threads[i], NULL, client_thread, &Clients[i]);
				}
			}
		}

		free(s);
		free(msg);
		players_in_game++;
	}

	/* play game */
	int MaxRounds = 1;
	int currRound = 1;
	int word_length = 5;
	char *word = "pizza";
	bool winner = false;
	char *str;

	for (int i = 0; i < Players; i++) {
		sprintf(buff, "startGame %d\n", MaxRounds);
		send_message(buff, NULL, Clients[i].fd);	

	}


	// TODO: start game
	/* rounds */
	while (currRound <= MaxRounds) {
		winner = false;
		/* curr round */
		// TODO start round
		while(!winner) {
			for (int i = 0; i < Players; i++) {
				//send(Clients[i].fd, buffer, BUFSIZ, 0);
			winner = true;

			}


		}



	}	

	return NULL;
}

int main(int argc, char *argv[]) {
	/* set up server */
	if (argc != 3) { puts("error"); return 0; }
	
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
			puts("join");
			if (Players >= num_players) {	

				/* send No response to extra players */
				/*
				strcpy(response.name, msg->name);
				strcpy(response.result, "No");	
				str = message_to_json(&response, JOIN_RESULT);
				send(client_fd, str, strlen(str) + 1, 0);
				free(str);
				*/
				
				sprintf(buffer, "joinResult No\n");
				send_message(buffer, msg->name, client_fd);

			} else { 
		
				/* send Yes reponse to players */
				/*strcpy(response.name, msg->name);
				strcpy(response.result, "Yes");	
				str = message_to_json(&response, JOIN_RESULT);
				send(client_fd, str, strlen(str) + 1, 0);
				free(str);
				*/
				sprintf(buffer, "joinResult Yes\n");
				send_message(buffer, msg->name, client_fd);


				/* add to list of clients, and start thread */
				Clients[Players].fd = client_fd;
				Clients[Players].inGame = true;	
				Clients[Players].isTurn = false;
				Clients[Players].score = 0;
				Clients[Players].nonce = Players;
				strcpy(Clients[Players].name, msg->name);
				
				pthread_create(&threads[Players], NULL, client_thread, &Clients[Players]);
	
				free(msg);

				/* add to players */	
				Players++;

				/* start game */
				if (Players >= num_players) {

					pthread_t game;
					pthread_create(&game, NULL, game_thread, NULL);

					while(!Game);

					/* send clients to game port */
					
					Message start;
					strcpy(start.server, "localhost");
					strcpy(start.port, "43000");
					start.type = START_INSTANCE;
					for (int i = 0; i < Players; i++) {
						start.nonce = Clients[i].nonce;
						char *s = message_to_json(&start, START_INSTANCE);
					   	puts("try");	
						if (Clients[i].inGame) {
							puts("send join");
							send(Clients[i].fd, s, strlen(s) + 1, 0);
						}

						free(s);
					}

					for (int i = 0; i < Players; i++) {
							pthread_join(threads[i], NULL);
					}
				
				}

			}
		}
	}

	return 0;
}
