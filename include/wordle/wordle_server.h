#ifndef WORDLE_SERVER_H
#define WORDLE_SERVER_H

#include "wordle/wordle.h"

/* definitions */

#define MAX_PLAYERS 10
#define MAX_WORDS 100
#define MAX_WORD_LENGTH 20

typedef struct Client_info {
	int fd;
	int Tscore;
	int Rscore;
	bool inGame;
	bool isTurn;
	char name[256];
	int nonce;
	int number;
	int guesses;
	double time;
	bool correct;
	bool canGuess;
	char guess[256];
	char byg[256];
} Client_info; 

typedef struct Game_info {
	bool started;
	int word_length;
	char *word;
	bool winner;
	char Win[256];
	bool over;
	int players;
	int guessed_players;
	int rounds;
	char *gport;
	bool debug;
} Game_info;


void send_message(char *str, char *name, int fd);
cJSON *get_players(Type type);
int get_words(char W[MAX_WORDS][MAX_WORD_LENGTH], char *);

#endif
