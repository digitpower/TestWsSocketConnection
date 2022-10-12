#include <windows.h>
#include <iostream>
#include "include/zlog.h"
#include "Base64DecEnc.h"
#include "globals.h"
#include "WebSocketClient.h"

#define SERVER "127.0.0.1"
#define PORT 8888

void OnErrorCallback(const WsClientLib::WSError& err, void* pUserData)
{
	std::cout << "OnErrorCallback: " 
		<< "err:" << err.message
		<< __LINE__
		<< "\n";
	//zlog_category_t* zc = zlog_get_category("ws_socket_cat");
	//if (zc != nullptr) {
	//	char output[100];
	//	sprintf(output, "Err: OnErrorCallback: code: %d", err);
	//	zlog_info(zc, output);
	//}
}

WebSocketClient::WebSocketClient() {}

void WebSocketClient::endCommunication()
{
	m_stopThread = true;
	//g_blockingCollectionWSocket->complete_adding();
	m_threadStartWsClient->join();
	std::cout << "Thread finished\n";
}

#ifdef USE_IPC
void WebSocketClient::startUdpClient()
{
	m_threadStartWsClient = new std::thread([this]() {
		// initialise winsock
		WSADATA ws;
		printf("Initialising Winsock...");
		if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
		{
			printf("Failed. Error Code: %d", WSAGetLastError());
			return;
		}
		printf("Initialised.\n");

		// create socket
		sockaddr_in server;
		int client_socket;
		if ((client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) // <<< UDP socket
		{
			printf("socket() failed with error code: %d", WSAGetLastError());
			return;
		}

		// setup address structure
		memset((char*)&server, 0, sizeof(server));
		server.sin_family = AF_INET;
		server.sin_port = htons(PORT);
		server.sin_addr.S_un.S_addr = inet_addr(SERVER);

		int counter = 0;
		while (!g_blockingCollectionWSocket->is_completed())
		{
			int sz = g_blockingCollectionWSocket->size();
			// take will block if there is no data to be taken
			Data* dt;
			auto waitDuration = std::chrono::duration<double>(5);
			auto status = g_blockingCollectionWSocket->try_take(dt, waitDuration);

			if (status == code_machina::BlockingCollectionStatus::Ok)
			{
				std::string strUserName;
				std::string strJson = "{ "
					//"\"StarTime\" : " + std::to_string(msecs) + ", "
					//"\"EndTime\" : " + std::to_string(msecs) + ", "
					"\"SpeakerLabel\" : \"" + (!strUserName.empty() ? strUserName : std::to_string(dt->node_id)) + "\", ";
				std::string ba64EncodedAudio = Base64DecEnc::b64encode(dt->buffer, dt->bufferLength);
				strJson += "\"base64Buffer\" : \"" + ba64EncodedAudio + "\"";
				strJson += "}";

				int length = strJson.length();
				// send the message
				if (sendto(client_socket, strJson.c_str(), length, 0, (sockaddr*)&server, sizeof(sockaddr_in)) == SOCKET_ERROR)
				{
					printf("sendto() failed with error code: %d", WSAGetLastError());
					return;
				}
				printf("Sent packet with length: %d. packet_count: %d\n", length, counter);
				counter++;
			}
		}
		if (sendto(client_socket, "{\"code\" : \"FINISH\"}", 19, 0, (sockaddr*)&server, sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code: %d", WSAGetLastError());
			return;
		}
	});
}
#endif

bool WebSocketClient::reconnectSocket(WsClientLib::WebSocket::pointer& pWebsock, const std::string& wsAddress)
{
	if (pWebsock != nullptr)
	{
		std::cout << "pWebsock: " << pWebsock << " " << __LINE__ << "\n";
		pWebsock->close();
		delete pWebsock;
	}
	pWebsock = WsClientLib::WebSocket::from_url(wsAddress, "");
	if (pWebsock == nullptr)
		return false;
	std::cout << "Now opening the connection\n";
	pWebsock->connect();
	WsClientLib::WebSocket::readyStateValues readyState;
	while ((readyState = pWebsock->getReadyState()) == WsClientLib::WebSocket::readyStateValues::CONNECTING) {
		pWebsock->poll(100, ::OnErrorCallback, nullptr);
		std::cout << "IN CONNECTING MODE\n";
	}
	std::cout << "Now readyState is: " << readyState << "\n";
	return readyState == WsClientLib::WebSocket::readyStateValues::OPEN;
}

void WebSocketClient::startSendData(const std::string& strWsAddress)
{
	m_wsAddress = strWsAddress;
	m_threadStartWsClient = new std::thread([this]() {

		WsClientLib::WebSocket::pointer pWebsock = nullptr;
		while (true)
		{
			if (m_stopThread)
				return;

			WsClientLib::WebSocket::pointer oldPointer = pWebsock;
			bool res = reconnectSocket(pWebsock, m_wsAddress);
			if (!res)
			{
				Sleep(5000);
				continue;
			}

			while (true)
			{
				if (m_stopThread)
					break;
				int sz = g_blockingCollectionWSocket->size();
				// take will block if there is no data to be taken
				Data* dt;
				auto waitDuration = std::chrono::duration<double>(2);
				std::cout << "Try Take:" << g_blockingCollectionWSocket->size() << " " << __LINE__ << "\n";
				auto status = g_blockingCollectionWSocket->try_take(dt, waitDuration);
				std::cout << "Try Take:" << g_blockingCollectionWSocket->size() << " " << __LINE__ << "\n";
				WsClientLib::WebSocket::readyStateValues wsState = pWebsock->getReadyState();
				if (status == code_machina::BlockingCollectionStatus::TimedOut)
				{
					if (wsState != WsClientLib::WebSocket::readyStateValues::CLOSED/* 1*/) {
						std::cout << "PING:******************\n";
						pWebsock->poll(0, ::OnErrorCallback, nullptr);
						pWebsock->sendPing();
					}
					else
					{
						bool res = reconnectSocket(pWebsock, m_wsAddress);
						if (!res)
						{
							Sleep(5000);
							continue;
						}
					}
				}
				else if (status == code_machina::BlockingCollectionStatus::Ok)
				{
					//CLOSING, CLOSED, CONNECTING, OPEN
					if (wsState != WsClientLib::WebSocket::readyStateValues::CLOSED/* 1*/) {
						auto begindt = std::chrono::system_clock::now();
						pWebsock->poll(0, ::OnErrorCallback, nullptr);
						


						std::string strUserName;
						std::string strJson = "{ "
							//"\"StarTime\" : " + std::to_string(msecs) + ", "
							//"\"EndTime\" : " + std::to_string(msecs) + ", "
							"\"SpeakerLabel\" : \"" + (!strUserName.empty() ? strUserName : std::to_string(dt->node_id)) + "\", ";
						std::string ba64EncodedAudio = Base64DecEnc::b64encode(dt->buffer, dt->bufferLength);
						strJson += "\"base64Buffer\" : \"" + ba64EncodedAudio + "\"";
						strJson += "}";




						pWebsock->send(strJson);
						std::cout << "pWebsock: " << pWebsock << " " << "oldPointer: " << oldPointer << "SEND DATA:*******************************" << __LINE__ << "\n";
						auto enddt = std::chrono::system_clock::now();
						std::chrono::duration<double> elapsed_time = enddt - begindt;
#if 0
						zlog_category_t* zc = zlog_get_category("ws_socket_cat");
						if (zc != nullptr) {
							char output[100];
							sprintf(output, "Data sent to ws socket: elapsed: %lf.", elapsed_time);
							zlog_info(zc, output);
						}
#endif
					}
					else
					{
						bool res = reconnectSocket(pWebsock, m_wsAddress);
						if (!res)
						{
							Sleep(5000);
							continue;
						}
					}
				}
//				//CLOSING, CLOSED, CONNECTING, OPEN
				std::cout << "\n\n";
			}
		}
	});
}

code_machina::BlockingCollection<Data*>* g_blockingCollectionWSocket;
WebSocketClient* g_wsClient;
//Current Datetime
#if 0
					auto tm_now = std::chrono::system_clock::now();
					std::time_t tm = std::chrono::system_clock::to_time_t(tm_now);
					
					std::cout << "Err: Trying to reconnect:" << std::ctime(&tm) << __FILE__ << __LINE__;
#endif