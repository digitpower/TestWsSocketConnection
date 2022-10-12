#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <mutex>
#include "globals.h"
#include "WsClientLib.h"
#include "BlockingCollection.h"

class Data;

struct WebSocketClient {
	static void OnErrorCallback(const WsClientLib::WSError& err, void* pUserData);
	WebSocketClient();
	bool reconnectSocket(WsClientLib::WebSocket::pointer& pWebsock, const std::string& wsAddress);
	void startSendData(const std::string& strWsAddress);
	std::string m_wsAddress;
	std::thread* m_threadStartWsClient;
	std::thread* m_threadUdpClient;
	void endCommunication();
	bool m_stopThread = false;
};

extern code_machina::BlockingCollection<std::string>* g_blockingCollectionWSocket;
extern WebSocketClient* g_wsClient;
#endif
