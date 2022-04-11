#include "wordle/wordle_server.h"
#include "wordle/server_socket.h"
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>

/**
 * Return current timestamp as a double.
 *
 * Utilizes gettimeofday for precision (accounting for seconds and
 * microseconds) and falls back to time (only account for seconds) if that
 * fails.
 **/
double timestamp() {
    // set timeval struct
    struct timeval tv;

    // check of fails
    if(gettimeofday(&tv, NULL) < 0) {
       return time(NULL);
    }
    return tv.tv_sec/1.0 + ( tv.tv_usec / 1000000.0 );

}

/* Globals */

pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t threads[MAX_PLAYERS];
Client_info Clients[MAX_PLAYERS];
Game_info Game = {.started = false, .winner = false, .guessed_players = 0};
int Players = 0;
int players_in_game = 0;
pthread_t game;

/* Thread Functions */

void *client_thread(void *arg) {
	Client_info *client = (Client_info *)arg;
	char buffer[BUFSIZ];

	/* recieve messages, and send to everyone if CHAT */
	while(1) {

		if (recv(client->fd, buffer, BUFSIZ, 0) <= 0) {
			printf("client %s quit\n", client->name);
			close(client->fd);
			client->inGame = false;
			players_in_game--;
			return 0;
		}

		// buffer is json
		printf("from fd %d: %s\n", client->fd, buffer);
		Message *msg = message_from_json(buffer);
		//Message response;
		//printf("type: %d\n", msg->type);	
		/* send to everyone but sender */
		if (msg->type == CHAT) {
			for (int i = 0; i < Players; i++) {
				//printf("%d\n", i);
				if (client->inGame && Clients[i].fd != client->fd ) {
					send(Clients[i].fd, buffer, BUFSIZ, 0);
				}
			}
		}

		if (msg->type == GUESS) {

			if (!Clients[client->nonce].canGuess || strlen(msg->guess) > Game.word_length) {
				sprintf(buffer, "guessResponse %s No\n", msg->guess);
				send_message(buffer, client->name, client->fd);

			} else {
				double time = timestamp();
				Clients[client->nonce].guesses++;

				sprintf(buffer, "guessResponse %s Yes\n", msg->guess);
				send_message(buffer, client->name, client->fd);
	
				printf("%d %s\n", client->nonce, msg->guess);

				char byg[Game.word_length];
				byg[Game.word_length] = '\0';
				char *w = Game.word;
				char *g = msg->guess;
				int score = 0;
				for (int i = 0; i < Game.word_length; i++) {
					// check green
					if (w[i] == g[i]) {
						byg[i] = 'G';
						score += 5/Clients[client->nonce].guesses;
						continue;
					}
				
					// check yellow and black
					bool B = true;
					for (int j = 0; j < Game.word_length; j++) {
						if (w[j] == g[i]) {
							byg[i] = 'Y';
							score += 2/Clients[client->nonce].guesses;
							B = false;
						}
					}

					if (B) {
						byg[i] = 'B';
					}
				}
			
				printf("guess: %s word: %s result: %s\n", g, w, byg);
				pthread_mutex_lock(&Lock);
				if (!strcmp(g, w)) {
					puts("correct guess");
					Game.winner = true;
					Clients[client->nonce].correct = true;
				}
				/* update guess for client */
				strcpy(Clients[client->nonce].byg, byg);
				Clients[client->nonce].canGuess = false;
				Clients[client->nonce].Rscore += score;
				Clients[client->nonce].time = time;

				// TODO update time
				Game.guessed_players++;
				pthread_mutex_unlock(&Lock);
			}

		}

		free(msg);
	}
}	

void *game_thread(void * arg) {
	int game_fd = socket_listen("43000");
	char buff[BUFSIZ];
	puts("into game thread");

	Game.players = 0;
	Game.word_length = 5;
	Game.word = "pizza";
	Game.winner = false;
	Game.guessed_players = 0;
	Game.started = true;

	/* collect players */
	while(Game.players < Players) {
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

					/* ADD TO guesses info array */
					strcpy(Clients[i].name, msg->name);
				    Clients[i].number = msg->nonce;
					Clients[i].correct	= false;
					Clients[i].canGuess = false;						

					pthread_create(&threads[i], NULL, client_thread, &Clients[i]);
				}
			}
		}

		free(s);
		free(msg);
		Game.players++;
	}

	/* play game */
	int MaxRounds = 2;
	int currRound = 1;
	Game.word = "pizza";
	/* send start game */
	for (int i = 0; i < Players; i++) {
		sprintf(buff, "startGame %d\n", MaxRounds);
		send_message(buff, NULL, Clients[i].fd);	
	}

	/* rounds */
	while (currRound <= MaxRounds) {
		

		/* send start round */
		for (int i = 0; i < Players; i++) {
			sprintf(buff, "startRound %d %d %d\n", Game.word_length, currRound, MaxRounds - currRound + 1);
			send_message(buff, NULL, Clients[i].fd);
		}
		

	
		Game.winner = false;
		/* curr round */

		while(!Game.winner) {


			/* prompt for guess */
			for (int i = 0; i < Players; i++) {
					Clients[i].canGuess = true;
				sprintf(buff, "prompt %d %d", Game.word_length, Clients[i].guesses);
				send_message(buff, Clients[i].name, Clients[i].fd);
				}


			/* wait for players to guess */
			while(Game.guessed_players < Players);
			Game.guessed_players = 0;
	
			// send guess_result
			for (int i = 0; i < Players; i++) {
				sprintf(buff, "guessResult\n");
				send_message(buff, NULL, Clients[i].fd);
			}
		}


		/* end round */
		// update score
		for (int i = 0; i < Players; i++) {
			Clients[i].Tscore += Clients[i].Rscore;
			Clients[i].guesses = 0;
		}

		for (int i = 0; i < Players; i++) {
			sprintf(buff, "endRound %d\n", MaxRounds - currRound);
			send_message(buff, NULL, Clients[i].fd);
			
			Clients[i].Rscore = 0;
		}
		
		currRound++;
		Game.word = "hello";
		
	}

	/* winner */
	int w = 0;
	for (int i = 0; i < Players; i++) {
		if(Clients[i].Tscore > w) {
			w = Clients[i].Tscore;
			strcpy(Game.Win, Clients[i].name);
		}
	}


	for (int i = 0; i < Players; i++) {
		sprintf(buff, "endGame %s\n", Game.Win);
		send_message(buff, NULL, Clients[i].fd);
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
		if (Game.started) {
			if (players_in_game <= 0) {
				break;
			}
		}
		/* gather clients */
		int client_fd = socket_accept(server_fd);	
		
		/* accept clients */
		recv(client_fd, buffer, BUFSIZ, 0);
		//printf("%s\n", buffer);
		Message *msg = message_from_json(buffer);
		char *s = message_to_json(msg, msg->type);
		printf("request: %s\n", s);
		free(s);
		
		if (msg->type == JOIN) {
			if (Players >= num_players) {	

				/* send No response to extra players */				
				sprintf(buffer, "joinResult No\n");
				send_message(buffer, msg->name, client_fd);

			} else { 
		
				/* send Yes reponse to players */
				sprintf(buffer, "joinResult Yes\n");
				send_message(buffer, msg->name, client_fd);

				/* add to list of clients, and start thread */ 
				Clients[Players].fd = client_fd;
				Clients[Players].inGame = true;	
				Clients[Players].isTurn = false;
				Clients[Players].Tscore = 0;
				Clients[Players].Rscore = 0;
				Clients[Players].nonce = Players;
				Clients[Players].guesses = 0;
				strcpy(Clients[Players].name, msg->name);
				
				pthread_create(&threads[Players], NULL, client_thread, &Clients[Players]);
	
				free(msg);

				/* add to players */	
				Players++;
				players_in_game++;

				/* start game */
				if (Players >= num_players) {

					

	
					pthread_create(&game, NULL, game_thread, NULL);

					while(!Game.started);

					/* send clients to game port */			
					Message start;
					strcpy(start.server, "localhost");
					strcpy(start.port, "43000");
					start.type = START_INSTANCE;
					for (int i = 0; i < Players; i++) {
						start.nonce = Clients[i].nonce;
						char *s = message_to_json(&start, START_INSTANCE);

						if (Clients[i].inGame) {
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

	for (int i = 0; i < Players; i++) {
		pthread_join(threads[i], NULL);
	}

	pthread_join(game, NULL);

	
	return 0;
}
