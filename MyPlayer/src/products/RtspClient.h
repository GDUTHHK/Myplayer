#pragma once
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <string>
#include <iostream>
#include <thread>
#include"internal/DigestAuthentication.h"
#include "internal/IProtocolHandler.h"
#include "internal/STRTPSession.h"
#include "internal/Share.h"

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

typedef bool (ST_APICALL* STRTSPAuthCallback)(int _channelId, void* _channelPtr, const char** _username, const char** _password);

typedef enum {
	NONE,
	OPTIONS,
	DESCRIBE,
	SETUP,
	PLAY,
	TEARDOWN
}RTSPCommand;

typedef struct {
	std::string userName;
	std::string sessionId;
	std::string sessionVersion;
	std::string networkType;
	std::string addressType;
	std::string address;
}Origion;

typedef struct SessionActiveTime {	
	SessionActiveTime() :startTime(0), stopTime(0) {}
	unsigned long long startTime;
	unsigned long long stopTime;
}SAT;

typedef std::string attributeName;
typedef std::string attributeValue;
typedef std::string mediaName;
typedef struct MediaDescription {
	std::string name;
	std::string port;
	std::string protocol;
	std::string fmt;
	std::map<attributeName, attributeValue> mediaAttributeMap;
}MediaDescription;

typedef struct SessionDescriptionProtocol {
	SessionDescriptionProtocol() :v(0) {}
	int v;
	Origion origion;
	SAT sat;
	std::map<attributeName, attributeValue> sessionAttributeMap;
	std::map<mediaName, struct MediaDescription> mediaMap;
}SDP;

class RtspClient : public IProtocolHandler
{
public:
	RtspClient(const Properties pProperties);
	~RtspClient();
	virtual bool initialize() override;
	virtual int OpenStream() override;
	virtual int CloseStream() override;
	virtual void SetCallback(FrameCallback cb) override;
	virtual bool updateProperties(Properties* _properties) override;
private:
	int SendTeardownCommand();
	int sendOptionsCommand();
	int sendDescribeCommand();
	int sendSetupCommand();
	int sendPlayCommand();
	int sendRTSP(const std::string& _msg);
	int recvRTSP(std::string& _msg);
	int check200();
	std::string createSessionString();
	std::string createRequest(const std::string& _command, const void*);
	std::string createAuthorizationString(const std::string& _cmd, const std::string& _url);
	int createSocket();
	int connectToServer();
	int handleResponseBytes();
	int parseSessionField(const std::string& _msg);
	int parsePublicField(const std::string& _msg);
	int parseTransportField(const std::string& _msg);
	int parse3W_AuthenticateField(const std::string& _msg);
	int parseResponseStatusLine(const std::string& _msg);
	int parseSDP();
	bool parseSDPLine_o(const std::string& _sdpLine);
	bool parseSDPLine_v(const std::string& _sdpLine);
	bool parseSDPLine_t(const std::string& _sdpLine);
	bool parseSDPLine_control(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap);
	bool parseSDPLine_rtpmap(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap);
	bool parseSDPLine_framerate(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap);
	bool parseSDPLine_fmtp(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap);
	int parseSDPLine_m(const std::string& _sdpLine, struct MediaDescription& mediaDescription);
	void SetAuthenticatorCallback(STRTSPAuthCallback _callback);
	void setAuthenticatorcallback(std::function<bool(const std::string&, const std::string&)> _callback);
	void SetAuthorization(const std::string& _username, const std::string& _password);

	int closeSocket();
private:
	Properties m_Properties;
	std::string m_strRTSPResponse;	//response
	std::string m_strRTSPResponseStatusCode;
	std::string m_strRTSPResponseStatusDescription;
	std::string m_strUrl;
	std::string m_strSession;	//session id
	unsigned m_uiCSeq;	//cseq
	RTSPCommand m_eRTSPCommand;	//command
	SockType m_iSocketFd;
	unsigned char m_u8SourceIdOverTCP;	//source id over tcp
	unsigned short m_usClientPortNum;	//client port num
	unsigned short m_usServerPortNum;	//server port num
	SDP m_sdp;
	bool m_bStreamOverTCP;
	std::string m_strSDP;	//sdp
	std::set<std::string> m_setSupportedCommands;
	STRTPSession* m_strtpsession;
	//STRTSPSourceCallBack m_rtspsourcecallback;
	FrameCallback m_framecallback;
	STRTSPExceptionCallback m_rtspexceptioncallback;
	std::thread m_recDataThread;
	void* m_pUserPtr;
	int m_iUserChannelId;
	ST_MEDIA_INFO m_stMediaInfo;
	int m_iVideoWidth;
	int m_iVideoHeight;
	std::stringstream m_sstreamRequestMag;
	Authenticator fCurrentAuthenticator;
	bool m_bNeedAuthentication;
	std::function<bool(const std::string&, const std::string&)> Authenticatorcallback;
	std::atomic<bool> m_running;
};

