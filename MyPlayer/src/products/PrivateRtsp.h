#pragma once
#include <iostream>
#include <thread>
#include <deque>
#include <regex>
#include <mutex>
#include <condition_variable>

#include "IProtocolHandler.h"
#include "share.h"
#include "internal/EventDispatcher.h"
#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

enum class ParserState {
	Synchronizing,
	ReadingHead,
	ReadingBody
};

class PrivateRtsp :public IProtocolHandler {
public:
	PrivateRtsp(const Properties pProperties);
	~PrivateRtsp();
	bool initialize() override;
	int OpenStream() override;
	int CloseStream()override;
	//int getIdentity() override;
	void SetCallback(FrameCallback cb) override;
	bool updateProperties(Properties* _properties) override;
private:
	void parserLoop();
	int createSocket();
	int connectToServer();
	int sendCommand(const void* command, const int size);
	int pollReadFd(const int& iFd, const unsigned int& uiTimeOut);
	int receiveData(const int& iFd, char* buf, const int& iBufLen, const unsigned int& uiTimeOut);
	int closeSocket();
private:
	Properties m_Properties;
	std::string m_strUrl;
	bool m_bStreamOverTCP;
	SockType m_iSocketFd;
	std::thread m_thread;
	FrameCallback m_framecallback;
	std::deque<unsigned char> m_receiveBuffer;
	ParserState m_parserState;
	std::vector<char> m_buffer;
	std::atomic<bool> m_running;
};