#include "wordle/wordle.h"

#define CLIENT "bwisemaSgood"

char *message_to_json(Message *msg, Type type) {
	cJSON *json = cJSON_CreateObject();
	cJSON *items = cJSON_CreateObject();

	switch (type) {
		case JOIN:	
			cJSON_AddStringToObject(items, "Name", msg->name);
			cJSON_AddStringToObject(items, "Client", msg->client);
			cJSON_AddItemToObject(json, "join", items); 
			break;
		case CHAT:
			cJSON_AddStringToObject(items, "Name", msg->name);
			cJSON_AddStringToObject(items, "Text", msg->text);
			cJSON_AddItemToObject(json, "chat", items);
		    break;	
		default:
			puts("bad type");
			return NULL;
	}

	char *str = cJSON_Print(json);
	cJSON_Delete(json);

	return str;
}

cJSON *get_message_type(cJSON *object, Message *msg) {

	// JOIN
	if(cJSON_HasObjectItem(object, "join")) {
		msg->type = JOIN;

		return cJSON_GetObjectItem(object, "join");
	}

	// CHAT
	if(cJSON_HasObjectItem(object, "Chat")) {

		msg->type = CHAT;
		return cJSON_GetObjectItem(object, "Chat");
	}
	
	// JOIN_RESULT
	if(cJSON_HasObjectItem(object, "joinResult")) {
		msg->type = JOIN_RESULT;
		return cJSON_GetObjectItem(object, "joinResult");
	}

	// START_GAME
	if(cJSON_HasObjectItem(object, "startGame")) {
		msg->type = START_GAME;
		return cJSON_GetObjectItem(object, "startGame");
	}

	// JOIN_GAME
	if(cJSON_HasObjectItem(object, "joinGame")) {
		msg->type = JOIN_GAME;
		return cJSON_GetObjectItem(object, "joinGame");
	}

	// JOIN_GAME_RESULT
	if(cJSON_HasObjectItem(object, "joinGameResult")) {
		msg->type = JOIN_GAME_RESULT;
		return cJSON_GetObjectItem(object, "joinGameResult");
	}

	// BEGIN_GAME - ask about name
	if(cJSON_HasObjectItem(object, "beginGame")) {
		msg->type = BEGIN_GAME;
		return cJSON_GetObjectItem(object, "beginGame");
	}

	// START_ROUND
	if(cJSON_HasObjectItem(object, "startRound")) {
		msg->type = START_ROUND;
		return cJSON_GetObjectItem(object, "startRound");
	}

	// PROMPT
	if(cJSON_HasObjectItem(object, "promptForGuess")) {
		msg->type = PROMPT;
		return cJSON_GetObjectItem(object, "promptForGuess");
	}

	// GUESS
	if(cJSON_HasObjectItem(object, "guess")) {
		msg->type = GUESS;
		return cJSON_GetObjectItem(object, "guess");
	}

	// GUESS_RESPONSE
	if(cJSON_HasObjectItem(object, "guessResponse")) {
		msg->type = GUESS_RESPONSE;
		return cJSON_GetObjectItem(object, "guessResponse");
	}

	// GUESS_RESULT
	if(cJSON_HasObjectItem(object, "guessResult")) {
		msg->type = GUESS_RESULT;
		return cJSON_GetObjectItem(object, "guessResult");
	}

	// END_ROUND
	if(cJSON_HasObjectItem(object, "endRound")) {
		msg->type = END_ROUND;
		return cJSON_GetObjectItem(object, "endRound");
	}

	// END_GAME
	if(cJSON_HasObjectItem(object, "endGame")) {
		msg->type = END_GAME;
		return cJSON_GetObjectItem(object, "endGame");
	}


	return NULL;

}

Message *message_from_command(char *commands) {
	
	Message *msg = calloc(1, sizeof(Message));

	char *cmd = strtok(commands, " ");

	if (!strcmp(cmd, "join")) {
		char *name = strtok(NULL, "\n");
		strcpy(msg->name, name);
		strcpy(msg->client, CLIENT);
		msg->type = JOIN;
	
	} else if (!strcmp(cmd, "chat")) {
		puts("chat");
		char *name = strtok(NULL, " ");
		char *text = strtok(NULL, "\n");
		strcpy(msg->name, name);
		strcpy(msg->text, text);
		msg->type = CHAT;
		puts("out");
	} else {
		return NULL;
	}

	return msg;


}

Message *message_from_json(char *json) {
	Message *msg = calloc(1, sizeof(Message));

	cJSON *message = cJSON_Parse(json);
	cJSON *item = get_message_type(message, msg);

	cJSON *value = NULL;

	value = cJSON_GetObjectItem(item, "name");
	if (value && value->valuestring) {
		strcpy(msg->name, value->valuestring);	
	}

	value = cJSON_GetObjectItem(item, "client");
	if (value && value->valuestring) {
		strcpy(msg->client, value->valuestring);	
	}

	value = cJSON_GetObjectItem(item, "guess");
	if (value && value->valuestring) {
		strcpy(msg->guess, value->valuestring);	
	}

	value = cJSON_GetObjectItem(item, "text");
	if (value && value->valuestring) {
		strcpy(msg->text, value->valuestring);	
	}
	
	value = cJSON_GetObjectItem(item, "result");
	if (value && value->valuestring) {
		if(strstr(value->string, "es")) {
			msg->Result = true;
		} else {
			msg->Result = false;
		}	
	}

	value = cJSON_GetObjectItem(item, "client");
	if (value && value->valuestring) {
		strcpy(msg->client, value->valuestring);	
	}

	value = cJSON_GetObjectItem(item, "server");
	if (value && value->valuestring) {
		strcpy(msg->server, value->valuestring);	
	}

	value = cJSON_GetObjectItem(item, "port");
	if (value && value->valuestring) {
		strcpy(msg->port, value->valuestring);	
	}
	
	value = cJSON_GetObjectItem(item, "nonce");
	if (value) {
		msg->nonce = value->valueint;
	}
	
	value = cJSON_GetObjectItem(item, "number");
	if (value) {
		msg->number = value->valueint;	
	}
	
	value = cJSON_GetObjectItem(item, "players");
	if (value) {
		msg->players = value;	
	}
	
	value = cJSON_GetObjectItem(item, "wordlength");
	if (value) {
		msg->word_length = value->valueint;	
	}
	
	value = cJSON_GetObjectItem(item, "round");
	if (value) {
		msg->round = value->valueint;	
	}
	
	value = cJSON_GetObjectItem(item, "roundsRemaining");
	if (value) {
		msg->rounds_remaining = value->valueint;	
	}
	
	value = cJSON_GetObjectItem(item, "guessNumber");
	if (value) {
		msg->guess_number = value->valueint;	
	}
	
	value = cJSON_GetObjectItem(item, "accepted");
	if (value) {
		if(strstr(value->valuestring, "es")) {
			msg->Accepted = true;
		} else {
			msg->Accepted = false;
		}		
	}
	
	value = cJSON_GetObjectItem(item, "guessNumber");
	if (value) {
		if(strstr(value->valuestring, "es")) {
			msg->Winner = true;
		} else {
			msg->Winner= false;
		}	
	}
	
	cJSON_Delete(message);


	return msg;
}
