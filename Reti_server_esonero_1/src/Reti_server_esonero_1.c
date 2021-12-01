/*
 ============================================================================
 Name        : Reti_server_esonero_1.c
 Author      : Christian Giacovazzo
 Description : primo esonero di "Reti di calcolatori 2021/2022" - server tcp
 ============================================================================
 */

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
#include <math.h>
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

//addition function
long signed int add(long signed int a, long signed int b) {
	return a + b;
}

//subtraction function
long signed int sub(long signed int a, long signed int b) {
	return a - b;
}

//multiplication function
long signed int mult(long signed int a, long signed int b) {
	return a * b;
}

//division function
div_t division(long signed int a, long signed int b) {
	return div(a,b);
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

//sets div_t structure to default value
void clearDiv_t(div_t* d){
	d->quot = 0; //default
	d->rem = 0; //default
}

//calculator function
void calculatorServer(int client) {
	message msg;
	div_t divResult; //division result

	while(1) {
		clearMsg(&msg); //clear struct and set default values, just to be safe

		//receive message
		recv(client, (char*) &msg, sizeof(msg), 0);
		//msg.operation = ntohl(msg.operation); //char values don't need conversion htonl/ntohl
		msg.operatorA = ntohl(msg.operatorA);
		msg.operatorB = ntohl(msg.operatorB);
		msg.result = ntohl(msg.result);
		msg.resultR = ntohl(msg.resultR);
		//msg.resultNaN = ntohl(msg.resultNaN); //char values don't need conversion htonl/ntohl

		//compute request
		if(msg.operation == '+') {
			//addition
			msg.result = add(msg.operatorA, msg.operatorB);
		}else if(msg.operation == '-') {
			//subtraction
			msg.result = sub(msg.operatorA, msg.operatorB);
		}else if(msg.operation == 'x') {
			//multiplication
			msg.result = mult(msg.operatorA, msg.operatorB);
		}else if(msg.operation == '/') {
			//division
			clearDiv_t(&divResult); //clear struct and set default values, just to be safe
			if(msg.operatorB != 0){
				divResult = division(msg.operatorA, msg.operatorB);
				msg.result = divResult.quot;
				msg.resultR = divResult.rem;
			}else{
				msg.resultNaN = '1';
			}
		}else if(msg.operation == '='){
			return;
		}

		//send result message
		//msg.operation = htonl(msg.operation); //char values don't need conversion htonl/ntohl
		msg.operatorA = htonl(msg.operatorA);
		msg.operatorB = htonl(msg.operatorB);
		msg.result = htonl(msg.result);
		msg.resultR = htonl(msg.resultR);
		//msg.resultNaN = htonl(msg.resultNaN); //char values don't need conversion htonl/ntohl
		send(client, (char*) &msg, sizeof(msg), 0);
	}
}

int main(int argc, char *argv []) {
	//selecting a port number for the server (use default PROTOPORT)
	int port = PROTOPORT;
/*
	if (argc > 1) {
		port = atoi(argv[1]);
	} else {
		port = PROTOPORT;
	}
*/
	if (port < 0) {
		printf("Bad port number %s , using default %d\n", argv[1], PROTOPORT);
		port = PROTOPORT;
	}

	//WSAStartup only for windows
	#if defined WIN32
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != 0) {
		errorhandler("Error at WSAStartup().\n");
		return -1;
	}
	#endif

	//SOCKET CREATION
	int my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //tcp protocol
	if (my_socket < 0) {
		errorhandler("socket creation failed.\n");
		clearwinsock();
		return -1;
	}

	//ASSIGNING AN ADDRESS TO THE SOCKET
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad)); //clear space
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr(PROTOIP); //loopback IP (local);
	sad.sin_port = htons(port);
	if (bind(my_socket, (struct sockaddr *)&sad, sizeof(sad))< 0) {
		errorhandler("bind() failed.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	//SETTING SOCKET TO LISTEN
	if (listen(my_socket, QLEN) < 0) {
		errorhandler("listen() failed.\n");
		closesocket(my_socket);
		clearwinsock();
		return -1;
	}

	//ACCEPT CONNECTION FROM CLIENT
	struct sockaddr_in cad; // structure for the client address
	int client_socket;	// socket descriptor for the client
	int client_len; // the size of the client address
	printf("Server %s:%hu listening. Waiting for a client to connect...\n", inet_ntoa(sad.sin_addr), ntohs(sad.sin_port));
	while (1) { //endless listening loop
		client_len = sizeof(cad); // set the size of the client	address
		if ((client_socket = accept(my_socket, (struct sockaddr*) &cad,	&client_len)) < 0) {
			// CONNECTION FAILED
			errorhandler("accept() failed.\n");
		} else {
			//CONNECTION SUCCEEDED
			printf("Connection established with %s:%hu\n", inet_ntoa(cad.sin_addr), ntohs(cad.sin_port));
			//computing request from client
			calculatorServer(client_socket);
			printf("Client %s:%hu served\n", inet_ntoa(cad.sin_addr), ntohs(cad.sin_port));
		}
		// CLOSE CONNECTION WITH CLIENT
		closesocket(client_socket);
		//clearwinsock(); //server don't need this, unless you want to close the whole server
	}

	system("pause");
	return 0;
}
