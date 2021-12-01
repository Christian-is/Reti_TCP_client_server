/*
 ============================================================================
 Name        : Reti_server_esonero_1.c
 Author      : Christian Giacovazzo
 Description : primo esonero di "Reti di calcolatori 2021/2022" - client tcp
 ============================================================================
 */

///portabilità unix/windows
#if defined WIN32
	#include <winsock.h>
#else
	#define closesocket close
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

//print an error message
void errorhandler(char *errorMessage) {
	printf("%s", errorMessage);
}

//clear socket if WIN32
void clearwinsock() {
	#if defined WIN32
	WSACleanup();
	#endif
}

//sets message structure to default value
void clearMsg(message* msg){
	msg->operation = '0'; //default
	msg->operatorA = 0; //default
	msg->operatorB = 0; //default
	msg->result = 0; //default
	msg->resultR = 0; //default
	msg->resultNaN = '0'; //default
}

//return 1 if str is not a number, 0 otherwise
int isNotNumber(const char* str){
	int len = strlen(str);
	char number[len+1];
	strcpy(number, str);
	int i = 0;

	if(number[i] == '+' || number[i] == '-'){
		i++;
	}
	while(number[i] != '\0'){
		if(number[i] < '0' || number[i] > '9'){
			return 1;
		}
		i++;
	}

	return 0;
}

//parse str into tokens divided by 'space', and saves them in msg. If invalid str, return default msg
message parseString(char* str) {
	message msg;
	char *token1; //token for the operation
	char *token2; //token for the first operator
	char *token3; //token for the second operator
	char *tokenNull = NULL; //flag error token. If not NULL, there is an error
	clearMsg(&msg);

	if(str[0] != '\n'){ //if str is new line, error
		str[strcspn(str, "\n")] = 0; //remove \n from str
		//extract tokens
		token1 = strtok(str, " ");
		token2 = strtok(NULL, " ");
		token3 = strtok(NULL, " ");
		tokenNull = strtok(NULL, " "); //if this token is not null, input error (too many operators)

		if(strcmp(token1, "=") == 0 && token2 == NULL){ //quit flag
			msg.operation = *token1;
			return msg;
		}

		if(tokenNull != NULL || token1 == NULL || token2 == NULL || token3 == NULL){ //input error
			clearMsg(&msg);
			return msg;
		}

		//assign tokens if valid
		//first token
		if(strcmp(token1, "+") != 0 && strcmp(token1, "-") != 0 && strcmp(token1, "x") != 0 && strcmp(token1, "/") != 0){
			clearMsg(&msg);
			return msg;
		}else{
			msg.operation = *token1;
		}

		//second token
		if(isNotNumber(token2)){
			clearMsg(&msg);
			return msg;
		}else{
			msg.operatorA = strtol(token2, (char **)NULL, 10);
		}

		//third token
		if(isNotNumber(token3)){
			clearMsg(&msg);
			return msg;
		}else{
			msg.operatorB = strtol(token3, (char **)NULL, 10);
		}

		return msg;
	}else{
		clearMsg(&msg);
		return msg;
	}
}

//calculator function
void calculatorClient(int c_socket){
	int LEN = 50;
	char input[LEN]; //string input from user
	int errorFlag = 0; //1 if invalid input, 0 otherwise
	message msg; //message struct that must be sent to the server

	printf("\nInsert one of the following operation in brackets { + - x / } followed by two integer (operators).\n");
	printf("Each value must be separated by a space (Example: + 23 45).\n");
	printf("If wrong input format is used, retry on new line. Insert { = } to quit.\n");

	while(1){
		clearMsg(&msg); //set msg to default
		//get input (single string input)
		printf("Insert new input: ");
		do{
			errorFlag = 0;
			fflush(stdout); //may not be needed for linux
			fflush(stdin); //may not be needed for linux
			fgets(input, LEN, stdin); //need this because 'space' is a valid char
			msg = parseString(input);
			if(msg.operation == '=') {
				//send quit message to server, then quit
				//msg.operation = htonl(msg.operation); //char values don't need conversion htonl/ntohl
				send(c_socket, (char*) &msg, sizeof(msg), 0);
				free(input); //free memory
				return;
			}
			if(msg.operation == '0'){ //if msg is in default state, errorFlag = 1
				errorFlag = 1;
				errorhandler("ERROR. Insert valid input: ");
			}
		}while(errorFlag);

		//send message
		//msg.operation = htonl(msg.operation); //char values don't need conversion htonl/ntohl
		msg.operatorA = htonl(msg.operatorA);
		msg.operatorB = htonl(msg.operatorB);
		msg.result = htonl(msg.result);
		msg.resultR = htonl(msg.resultR);
		//msg.resultNaN = htonl(msg.resultNaN); //char values don't need conversion htonl/ntohl
		send(c_socket, (char*) &msg, sizeof(msg), 0);

		//receive message
		recv(c_socket, (char*) &msg, sizeof(msg), 0);
		//msg.operation = ntohl(msg.operation); //char values don't need conversion htonl/ntohl
		msg.operatorA = ntohl(msg.operatorA);
		msg.operatorB = ntohl(msg.operatorB);
		msg.result = ntohl(msg.result);
		msg.resultR = ntohl(msg.resultR);
		//msg.resultNaN = ntohl(msg.resultNaN); //char values don't need conversion htonl/ntohl

		//print result
		if(msg.resultNaN == '1'){
			printf("Result: %ld %c %ld = %s\n", msg.operatorA, msg.operation, msg.operatorB, "nan");
		}else if(msg.operation != '/'){
			printf("Result: %ld %c %ld = %ld\n", msg.operatorA, msg.operation, msg.operatorB, msg.result);
		}else{
			printf("Result: %ld %c %ld = quotient %ld remainder %ld\n", msg.operatorA, msg.operation, msg.operatorB, msg.result, msg.resultR);
		}
	}
}

int main(int argc, char *argv []) {
	int MAX_IP_LENGHT = 16; //ip address in char dotted form are 15 character long (+1 for \0)
	char ip_address[MAX_IP_LENGHT];
	int port;
	strcpy(ip_address, PROTOIP); //set default ip
	port = PROTOPORT; //set default port
	char new_ip_address[MAX_IP_LENGHT];
	int new_port;

	//typing ip and port number of the server, (use default PROTOIP and PROTOPORT)
	printf("Welcome. Please connect to the server.\n");
	printf("(NOTE: Default server IP is %s , default server port is %d)\n", PROTOIP, PROTOPORT);
	printf("Insert server IP: ");
	fflush(stdout); //may not be needed for linux
	scanf("%s", new_ip_address);
	fflush(stdin); //may not be needed for linux
	printf("Insert port number: ");
	fflush(stdout); //may not be needed for linux
	scanf("%d", &new_port);
	fflush(stdin); //may not be needed for linux
	///delete 'if statement' if you want a non-local server connection (if exists)
	if(strcmp(new_ip_address, PROTOIP) != 0 || new_port != PROTOPORT || new_port < 0) {
		printf("Bad ip address or port number. Using default server: %s:%d \n", PROTOIP, PROTOPORT);
	}else{
		printf("Using server: %s:%d \n", new_ip_address, new_port);
		strcpy(ip_address, new_ip_address);
		port = new_port;
	}///


	#if defined WIN32
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2 ,2), &wsa_data);

	if (result != 0) {
		errorhandler("error at WSASturtup\n");
		return -1;
	}
	#endif

	//SOCKET CREATION
	int c_socket;
	c_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (c_socket < 0) {
		errorhandler("socket creation failed.\n");
		closesocket(c_socket);
		clearwinsock();
		return -1;
	}

	// SERVER ADDRESS CONSTRUCTION
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr(ip_address); //server ip
	sad.sin_port = htons(port); //server port

	//SERVER CONNECTION
	if (connect(c_socket, (struct sockaddr *)&sad, sizeof(sad))< 0) {
		errorhandler( "Failed to connect.\n" );
		closesocket(c_socket);
		clearwinsock();
		return -1;
	}

	//client request
	calculatorClient(c_socket);

	// CLOSE CONNECTION
	closesocket(c_socket);
	clearwinsock();

	printf(" \n");
	system("pause");
	return(0);
}
