#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <mutex>
#include "globals.h"
#include "WsClientLib.h"
#include "BlockingCollection.h"

class Data;

struct WebSocketClient {
	static void OnErrorCallback(const WsClientLib::WSError& err, void* pUserData);
	static void CleanupSocket();
	WebSocketClient();
	void startSendData(const std::string& strWsAddress);
	std::string m_wsAddress;
	std::thread* m_threadStartWsClient;
};

extern code_machina::BlockingCollection<Data*>* g_blockingCollectionWSocket;
#endif
