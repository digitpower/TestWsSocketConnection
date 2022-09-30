#include <windows.h>
#include <iostream>
//#include <zlog.h>
#include "Base64DecEnc.h"
#include "globals.h"
#include "WebSocketClient.h"

void OnErrorCallback(const WsClientLib::WSError& err, void* pUserData)
{
	std::cout << "OnErrorCallback: " 
		<< "err:" << err.message
		<< __FILE__ << __LINE__
		<< "\n";
	//zlog_category_t* zc = zlog_get_category("ws_socket_cat");
	//if (zc != nullptr) {
	//	char output[100];
	//	sprintf(output, "Err: OnErrorCallback: code: %d", err);
	//	zlog_info(zc, output);
	//}
}

void CleanupSocket()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

WebSocketClient::WebSocketClient() {}

void WebSocketClient::startSendData(const std::string& strWsAddress)
{
	m_wsAddress = strWsAddress;
	m_threadStartWsClient = new std::thread([this]() {
		while (true)
		{

#ifdef _WIN32
			WSADATA wsaData;
			INT rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (rc) {
				printf("WSAStartup Failed.\n");
				Sleep(1000);
				continue;
			}
#endif
			WsClientLib::WebSocket::pointer pWebsock = WsClientLib::WebSocket::from_url(m_wsAddress, "");
			if (pWebsock == nullptr)
			{
				Sleep(1000);
				continue;
			}

			pWebsock->connect();

			//auto connectionStartTime = std::chrono::system_clock::now();

			while (!g_blockingCollectionWSocket->is_completed())
			{
				int sz = g_blockingCollectionWSocket->size();



				// take will block if there is no data to be taken
				Data* dt;
				auto waitDuration = std::chrono::duration<double>(5);
				auto status = g_blockingCollectionWSocket->try_take(dt, waitDuration);

				WsClientLib::WebSocket::readyStateValues state = pWebsock->getReadyState();

				if (status == code_machina::BlockingCollectionStatus::TimedOut)
				{
					if (state != WsClientLib::WebSocket::readyStateValues::CLOSED/* 1*/) {
						pWebsock->poll(0, ::OnErrorCallback, nullptr);
						pWebsock->sendPing();
					}
				}
				else if (status == code_machina::BlockingCollectionStatus::Ok)
				{
					//CLOSING, CLOSED, CONNECTING, OPEN
					if (state != WsClientLib::WebSocket::readyStateValues::CLOSED/* 1*/) {
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
				}
				//CLOSING, CLOSED, CONNECTING, OPEN
				if (state == WsClientLib::WebSocket::readyStateValues::CLOSED/* 1*/)
				{
					//zlog_category_t* zc = zlog_get_category("ws_socket_cat");
					//if (zc != nullptr) {
					//	char output[100];
					//	sprintf(output, "Err: Trying to reconnect: queue size is %d. Clearing queue.", sz);
					//	g_blockingCollectionWSocket.flush();
					//	zlog_info(zc, output);
					//}
					std::cout << "Err: Trying to reconnect" << __FILE__ << __LINE__;
					pWebsock->connect();
				}
			}
		}
	});
}

code_machina::BlockingCollection<Data*>* g_blockingCollectionWSocket;
