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


	using std::string;
	using std::cout;
	using std::endl;
	string filename("d:\\mixdown.wav");
	std::fstream file;
	file.open(filename, std::ios_base::in | std::ios::binary);
	if (!file.is_open()) {
		//file.clear();
		//file.open(filename, std::ios_base::out | std::ios_base::binary);
		//file.close();
		//file.open(filename, std::ios_base::in | std::ios_base::binary);
		cout << "File cannot opened!" << endl;
	}
	else if (file.good()) {
		cout << "file already exists." << endl;
		cout << "file was opened on the first try!" << endl;


		g_blockingCollectionWSocket = new code_machina::BlockingCollection<Data*>();

		//Start WebSocket Thread

		g_wsClient = new WebSocketClient();
#ifdef USE_IPC
		g_wsClient->startUdpClient();
#else
		std::string strWebSocket = "wss://rtst.autoscript.ai/ws";
		//std::string strWebSocket = "ws://127.0.0.1:5000/ws";
		g_wsClient->startSendData(strWebSocket);
#endif

		//Wait for 5 secs
		std::this_thread::sleep_for(5000ms);




		//Start Reading Data From File
		int counter = 0;

		
		while (!file.eof() /* && counter < 30 * 5 * / /** 30 secs*/)
		{
			Data* dt = new Data();
			dt->bufferLength = AUDIO_BUFFER_LENGTH;
			
			dt->node_id = 123;
			file.read(dt->buffer, AUDIO_BUFFER_LENGTH);


			g_blockingCollectionWSocket->add(dt);
			//cout << "Number of bytes read:" << audio_chunk_size << " counter: " << counter << "\n";
			counter++;
			std::this_thread::sleep_for(200ms);
		}
		int c = getchar();

		g_wsClient->endCommunication();
	}
	file.close();
	return 0;
}