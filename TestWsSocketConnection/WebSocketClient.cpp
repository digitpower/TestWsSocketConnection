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
}

WebSocketClient::WebSocketClient() {}

void WebSocketClient::endCommunication()
{
	m_stopThread = true;
	//g_blockingCollectionWSocket->complete_adding();
	m_threadStartWsClient->join();
	std::cout << "Thread finished\n";
}

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
	printf("Now opening the connection\n");
	pWebsock->connect();
	WsClientLib::WebSocket::readyStateValues readyState;
	while ((readyState = pWebsock->getReadyState()) == WsClientLib::WebSocket::readyStateValues::CONNECTING) {
		pWebsock->poll(100, ::OnErrorCallback, nullptr);
		printf("IN CONNECTING MODE\n");
	}
	printf("Now readyState is: %d\n", readyState);
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
				auto waitDuration = std::chrono::duration<double>(2);
				std::string strJson;
				auto status = g_blockingCollectionWSocket->try_take(strJson, waitDuration);
				if (status == code_machina::BlockingCollectionStatus::TimedOut)
				{
					WsClientLib::WebSocket::readyStateValues wsState = pWebsock->getReadyState();
					if (wsState != WsClientLib::WebSocket::readyStateValues::CLOSED/* 1*/) {
						printf("PING:******************\n");
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
					WsClientLib::WebSocket::readyStateValues wsState = pWebsock->getReadyState();
					//CLOSING, CLOSED, CONNECTING, OPEN
					if (wsState != WsClientLib::WebSocket::readyStateValues::CLOSED/* 1*/) {
						auto begindt = std::chrono::system_clock::now();
						pWebsock->poll(0, ::OnErrorCallback, nullptr);
						pWebsock->send(strJson);
						auto enddt = std::chrono::system_clock::now();
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

code_machina::BlockingCollection<std::string>* g_blockingCollectionWSocket;
WebSocketClient* g_wsClient;
//Current Datetime
#if 0
					auto tm_now = std::chrono::system_clock::now();
					std::time_t tm = std::chrono::system_clock::to_time_t(tm_now);
					
					std::cout << "Err: Trying to reconnect:" << std::ctime(&tm) << __FILE__ << __LINE__;
#endif