#ifndef WORDLE_H
#define WORDLE_H

#include <stdio.h>

#include "wordle/client_socket.h"
#include <pthread.h>
#include "wordle/cJSON.h"
#include <stdbool.h>

/* Functions */

#define SMALL_BUFSIZ 256

typedef enum {
	CHAT,
	JOIN,
	JOIN_RESULT,
	START_INSTANCE,
	JOIN_INSTANCE,
	JOIN_INSTANCE_RESULT,
	START_GAME,
	START_ROUND,
	PROMPT,
	GUESS,
	GUESS_RESPONSE,
	GUESS_RESULT,
	END_ROUND,
	END_GAME,
} Type;

typedef struct Message {
	char name[SMALL_BUFSIZ];
	Type type;
	char guess[SMALL_BUFSIZ];
	char text[BUFSIZ];
	char result[10];
	char client[SMALL_BUFSIZ];
	char server[SMALL_BUFSIZ];
	char port[SMALL_BUFSIZ];
	int nonce;
	int number;
	cJSON *players;
	int word_length;
	int round;
	int rounds;
	int rounds_remaining;
	int guess_number;
	char accepted[10];
	char winner[10];
} Message;


char *message_to_json(Message *, Type);
void get_message_type(char *, Message *);
Message *message_from_command(char *, char *);
Message *message_from_json(char *);
#endif
