/*
 * protocol.h
 *
 *  Created on: 11 nov 2021
 *      Author: cgiac
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

//default IP address
#define PROTOIP "127.0.0.1"
//default protocol port number
#define PROTOPORT 27015
//max number of clients
#define QLEN 5


//struct of a message
typedef struct{
	char operation; //+ - x / or =
	long signed int operatorA; //signed integer
	long signed int operatorB; //signed integer
	long signed int result; //result of operations + - x and int quot of /
	long signed int resultR; //rem part of /
	char resultNaN; //'1' if result of operation is not a number, '0' otherwise
}message;


#endif /* PROTOCOL_H_ */
