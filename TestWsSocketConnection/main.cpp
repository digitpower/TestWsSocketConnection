#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <signal.h>
#include "include/zlog.h"
#include "Base64DecEnc.h"
#include "WebSocketClient.h"
#include "globals.h"

using namespace std::chrono_literals;


void SignalHandler(int signal)
{
	if (signal == SIGINT) {
		// abort signal handler code
		std::cout << "SIGINT signal received\n";
	}
	else {
		// ...
	}
}


int startReceive()
{
    // Initialize Winsock version 2.2
    WSADATA wsaData;
    SOCKADDR_IN SenderAddr;
    int SenderAddrSize = sizeof(SenderAddr);
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Server: WSAStartup failed with error: %ld\n", WSAGetLastError());
        return -1;
    }
    else {
        printf("Server: The Winsock DLL status is: %s.\n", wsaData.szSystemStatus);
    }

    SOCKET ReceivingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ReceivingSocket == INVALID_SOCKET) {
        printf("Server: Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }
    else {
        printf("Server: socket() is OK!\n");
    }

    int Port = 8888;
    SOCKADDR_IN ReceiverAddr;
    ReceiverAddr.sin_family = AF_INET;
    ReceiverAddr.sin_port = htons(Port);
    ReceiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(ReceivingSocket, (SOCKADDR*)&ReceiverAddr, sizeof(ReceiverAddr)) == SOCKET_ERROR)
    {
        printf("Server: Error! bind() failed!\n");
        closesocket(ReceivingSocket);
        WSACleanup();
        return -1;
    }
    else {
        printf("Server: bind() is OK!\n");
    }

    // Print some info on the receiver(Server) side...
    //getsockname(ReceivingSocket, (SOCKADDR*)&ReceiverAddr, (int*)sizeof(ReceiverAddr));

    const int BufLength = 10000;
    char ReceiveBuf[BufLength];
    while (1) { // Server is receiving data until you will close it.(You can replace while(1) with a condition to stop receiving.)
        int ByteReceived = recvfrom(ReceivingSocket, ReceiveBuf, BufLength, 0, (SOCKADDR*)&SenderAddr, &SenderAddrSize);

        if (ByteReceived > 0) { //If there are data
            //printf("Total Bytes received: %d\n", ByteReceived);
            std::string str(ReceiveBuf, ByteReceived);
            g_blockingCollectionWSocket->add(str);
        }
        else if (ByteReceived <= 0) { //If the buffer is empty
            //Print error message
            printf("Server: Connection closed with error code: %ld\n", WSAGetLastError());
        }
        else { //If error
            //Print error message
            printf("Server: recvfrom() failed with error code: %d\n", WSAGetLastError());
        }
    }

    getpeername(ReceivingSocket, (SOCKADDR*)&SenderAddr, &SenderAddrSize);

    if (closesocket(ReceivingSocket) != 0) {
        printf("Server: closesocket() failed! Error code: %ld\n", WSAGetLastError());
    }
    else {
        printf("Server: closesocket() is OK\n");
    }

    if (WSACleanup() != 0) {
        printf("Server: WSACleanup() failed! Error code: %ld\n", WSAGetLastError());
    }
    else {
        printf("Server: WSACleanup() is OK\n");
    }
    return 0;
}


int main()
{

	typedef void (*SignalHandlerPointer)(int);

	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGINT, SignalHandler);


	int rc = zlog_init("log.conf");
	if (rc) {
		printf("init failed\n");
		return -1;
	}


    g_blockingCollectionWSocket = new code_machina::BlockingCollection<std::string>();
    std::thread* receiverThread = new std::thread(startReceive);
    WebSocketClient* g_wsClient = new WebSocketClient();
    std::string strWebSocket = "wss://rtst.autoscript.ai/ws";
    //std::string strWebSocket = "ws://127.0.0.1:5000/ws";
    g_wsClient->startSendData(strWebSocket);

    receiverThread->join();

    return 0;
}