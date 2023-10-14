// CS 2690 Program 1 
// Simple Windows Sockets Echo Client (IPv6)
// Last update: 2/12/19
//
// Usage: WSEchoClientv6 <server IPv6 address> <server port> <"message to echo">
// Companion server is WSEchoServerv6
// Server usage: WSEchoServerv6 <server port>
//
// This program is coded in conventional C programming style, with the 
// exception of the C++ style comments.
//
// I declare that the following source code was written by me or provided
// by the instructor. I understand that copying source code from any other 
// source or posting solutions to programming assignments (code) on public
// Internet sites constitutes cheating, and that I will receive a zero 
// on this project if I violate this policy.
// ----------------------------------------------------------------------------

#pragma comment(lib, "ws2_32.lib")

// Minimum required header files for C Winsock program
#include <stdio.h>       // for print functions
#include <stdlib.h>      // for exit() 
#include <winsock2.h>	 // for Winsock2 functions
#include <ws2tcpip.h>    // adds support for getaddrinfo & getnameinfo for v4+6 name resolution
#include <Ws2ipdef.h>    // optional - needed for MS IP Helper

// #define ALL required constants HERE, not inline 
// #define is a macro, don't terminate with ';'  For example...
#define RCVBUFSIZ 50

// declare any functions located in other .c files here
void DisplayFatalErr(char *errMsg); // writes error message before abnormal termination

void main(int argc, char *argv[])   // argc is # of strings following command, argv[] is array of ptrs to the strings
{
	// Declare ALL variables and structures for main() HERE, NOT INLINE (including the following...)
	WSADATA wsaData;                // contains details about WinSock DLL implementation
	//struct sockaddr_in6 serverInfo;	// standard IPv6 structure that holds server socket info

	struct addrinfo hints, * addrInfoResult, * ptrAddrInfo;
	SOCKET clientSock = INVALID_SOCKET;
	
	// Verify correct number of command line arguments, else do the following:
	if (argc != 4)
	{
		fprintf(stderr, "Program requires three arguments at the command line.\nExpected: IP address of server, Listening port on server, Message to send to server.\n");
		exit(1);	  // ...and terminate with abnormal termination code (1)
	}
	
	// Retrieve the command line arguments. (Sanity checks not required, but port and IP addr will need
	// to be converted from char to int.  See slides 11-15 & 12-3 for details.)
	char* serverIPaddr = argv[1];
	char* serverPort = argv[2];
	char* message = argv[3];
	size_t messageLen = strlen(message);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//IPv6 used
	/*memset(&serverInfo, 0, sizeof(serverInfo));

	serverInfo.sin6_family = AF_INET6;
	serverInfo.sin6_port = htons(serverPort);
	inet_pton(AF_INET6, serverIPaddr, &serverInfo.sin6_addr);*/

	// Initialize Winsock 2.0 DLL. Returns 0 if ok. If this fails, fprint error message to stderr as above & exit(1).
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		DisplayFatalErr("WSAStartup failed");
	}

	//resolve what kind of IP address is being used
	if (getaddrinfo(serverIPaddr, serverPort, &hints, &addrInfoResult) != 0)
	{
		DisplayFatalErr("getaddrinfo failed. Could not resolve IP address");
	}
   
	for (ptrAddrInfo = addrInfoResult; ptrAddrInfo != NULL; ptrAddrInfo = ptrAddrInfo->ai_next)
	{
		clientSock = socket(ptrAddrInfo->ai_family, ptrAddrInfo->ai_socktype, ptrAddrInfo->ai_protocol);
		if (clientSock == INVALID_SOCKET)
		{
			DisplayFatalErr("Invalid socket returned when trying to create socket");
		}

		// Attempt connection to the server.  If it fails, call DisplayFatalErr() with appropriate message,
		// otherwise printf() confirmation message
		if (connect(clientSock, ptrAddrInfo->ai_addr, (int)ptrAddrInfo->ai_addrlen) == SOCKET_ERROR)
		{
			closesocket(clientSock);
			clientSock = INVALID_SOCKET;
			continue;
		}
		else
		{
			printf("Connection established\n");
			break;
		}
	}
	freeaddrinfo(addrInfoResult);

	if (clientSock == INVALID_SOCKET)
	{
		DisplayFatalErr("Failed to connect to server");
	}
        
	// Send message to server (without '\0' null terminator). Check for null msg (length=0) & verify all bytes were sent...
	// ...else call DisplayFatalErr() with appropriate message as before
	if (messageLen != 0)
	{
		size_t sendResult = (size_t)send(clientSock, message, messageLen, 0);
		if (sendResult != messageLen)
		{
			if (sendResult == -1)
			{
				DisplayFatalErr("Socket Error. Failed to transfer message");
			}
			else
			{
				DisplayFatalErr("Did not send all the bytes in the message");
			}
		}
	}
	else
	{
		printf("Supplied message is empty, nothing to send. Exiting\n");
		WSACleanup();
		exit(1);
	}
	printf("Message sent to server\n");
 
 	// Retrieve the message returned by server.  Be sure you've read the whole thing (could be multiple segments). 
	// Manage receive buffer to prevent overflow with a big message.
 	// Call DisplayFatalErr() if this fails.  (Lots can go wrong here, see slides.)
	char rcvBuffer[RCVBUFSIZ];
	size_t bytesReceived = 0;
	size_t totalBytesReceived = 0;

	printf("Listening for echo from server\n");
	// Display ALL of the received message, in printable C string format.
	do
	{
		bytesReceived = recv(clientSock, rcvBuffer, RCVBUFSIZ - 1, 0);
		if (bytesReceived > 0)
		{
			totalBytesReceived += bytesReceived;
			rcvBuffer[bytesReceived] = '\0';
			printf("%s",rcvBuffer);
		}
		else
		{
			DisplayFatalErr("recv failed with error: ");
			break;
		}
	} while (totalBytesReceived < messageLen);
	printf("\n");
	printf("Message received from echo server\nBytes transmitted: %zu Bytes received: %zu\n", messageLen, totalBytesReceived);
    
 	// Close the TCP connection (send a FIN) & print appropriate message.
	if (closesocket(clientSock) == SOCKET_ERROR)
	{
		DisplayFatalErr("closesocket failed with error: ");
	}
	printf("Connection closed\n");

	// Release the Winsock DLL
	WSACleanup();
	
	exit(0);
}
