#include <iostream>
#include <regex>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#endif

#if defined(_MSC_VER)
#define SSCANF_SAFE sscanf_s
#else
#define SSCANF_SAFE sscanf
#endif

#include "RtspClient.h"
#include"internal/Base64.h"
#include "factories/GenericFactory.h"
//#include"internal/STRTPSession.h"

#include"internal/SpsParse.h"

#define DEFAULT_PORT    554
#define DEFAULT_RECV_RTSP_BUFFER_SIZE   1024

#if defined(__WIN32__) || defined(_WIN32) || defined(_QNX4)
#define _strncasecmp _strnicmp
#define snprintf _snprintf
#else
#define _strncasecmp strncasecmp

#endif

template<typename T>
static T getProperty(Properties m_Properties, const std::string& key, const T& defaultValue) {
    auto it = m_Properties.find(key);
    if (it == m_Properties.end()) {
        return defaultValue;
    }

    try {
        return std::any_cast<T>(it->second);
    }
    catch (const std::bad_any_cast& e) {
        std::cerr << "Warning: Property '" << key << "' has wrong type. Using default value. "
            << "Details: " << e.what() << std::endl;
        return defaultValue;
    }
}

static bool checkForHeader(char const* line, char const* headerName, unsigned headerNameLength, char const*& headerParams)
{
    if (_strncasecmp(line, headerName, headerNameLength) != 0) return false;

    // The line begins with the desired header name.  Trim off any whitespace, and return the header parameters:
    unsigned paramIndex = headerNameLength;
    while (line[paramIndex] != '\0' && (line[paramIndex] == ' ' || line[paramIndex] == '\t')) ++paramIndex;
    if (line[paramIndex] == '\0') return false; // the header is assumed to be bad if it has no parameters

    headerParams = &line[paramIndex];
    return true;
}

static char* getLine(char* startOfLine) {
    // returns the start of the next line, or NULL if none.  Note that this modifies the input string to add '\0' characters.
    for (char* ptr = startOfLine; *ptr != '\0'; ++ptr) {
        // Check for the end of line: \r\n (but also accept \r or \n by itself):
        if (*ptr == '\r' || *ptr == '\n') {
            // We found the end of the line
            if (*ptr == '\r') {
                *ptr++ = '\0';
                if (*ptr == '\n') ++ptr;
            }
            else {
                *ptr++ = '\0';
            }
            return ptr;
        }
    }
    return NULL;
}

static bool parseSDPLine(char const* inputLine, char const*& nextLine)
{
    // Begin by finding the start of the next line (if any):
    nextLine = NULL;
    for (char const* ptr = inputLine; *ptr != '\0'; ++ptr) {
        if (*ptr == '\r' || *ptr == '\n') {
            // We found the end of the line
            ++ptr;
            while (*ptr == '\r' || *ptr == '\n') ++ptr;
            nextLine = ptr;
            if (nextLine[0] == '\0') nextLine = NULL; // special case for end
            break;
        }
    }

    // Then, check that this line is a SDP line of the form <char>=<etc>
    // (However, we also accept blank lines in the input.)
    if (inputLine[0] == '\r' || inputLine[0] == '\n') return true;
    if (strlen(inputLine) < 2 || inputLine[1] != '=' || inputLine[0] < 'a' || inputLine[0] > 'z') {
        return false;
    }

    return true;
}

static std::string getIP(const std::string& _url)
{
    std::regex ip_regex("([a-zA-Z]+)://(\\d{1,3}(\\.\\d{1,3}){3})(:\\d+)?\\S*");
    std::smatch results;

    try
    {
        if (std::regex_match(_url, results, ip_regex))
        {
            return results[2].str();
        }
        else return std::string();
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return std::string();
    }
}

static int getPort(const std::string& _url)
{
    std::regex port_regex("([a-zA-Z]+)://(\\d{1,3}(\\.\\d{1,3}){3})(:(\\d+))?\\S*");
    std::smatch results;

    try
    {
        if (std::regex_match(_url, results, port_regex))
        {
            if (results[5].matched)
            {
                return std::stoi(results[5].str());
            }
            else return DEFAULT_PORT;
        }
        else return -1;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return -1;
    }
    catch (std::exception e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}

static int sendRTSP(SockType _sockfd, const std::string& _msg)
{
    const char* msg = _msg.c_str();
    size_t size = _msg.size();
    int sendResult = 0;
    int index = 0;
    int err = ST_NoErr;

    while (size > 0)
    {
        sendResult = send(_sockfd, msg + index, size, 0);
        if (sendResult < 0)
        {
            if (errno == EINTR) continue;
            else if (errno == EWOULDBLOCK || errno == EAGAIN) continue;
            else {
                err = ST_SendErr;
                break;
            }
        }
        else if (sendResult == 0)
        {
            err = ST_SendErr;
            break;
        }

        index += sendResult;
        size -= sendResult;
    }

    return err;
}

static int recvRTSP(SockType _sock, std::string& _msg)
{
    char* recvBuffer = new char[DEFAULT_RECV_RTSP_BUFFER_SIZE];
    if (recvBuffer == nullptr) return ST_MemoryErr;
    int recvResult = 0;
    int err = ST_NoErr;

    memset(recvBuffer, 0, DEFAULT_RECV_RTSP_BUFFER_SIZE);
    _msg.clear();

    do {
        recvResult = recv(_sock, recvBuffer, 4, MSG_PEEK);
        if (recvResult < 0)
        {
            if (errno == EINTR) continue;
            else if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                err = ST_RecvErr;;
                break;
            }
            else
            {
                err = ST_RecvErr;
                break;
            }
        }
        else if (recvResult == 0)
        {
            err = ST_RecvErr;
            break;
        }
        if (_strncasecmp(recvBuffer, "RTSP", 4) == 0) break;
        else if (recvBuffer[0] == 0x24) {
            unsigned int len = 0;
            len |= recvBuffer[2] << 8;
            len |= recvBuffer[3];
            recv(_sock, recvBuffer, len + 4, 0);
        }
        else return ST_RecvErr;
    } while (true);
    recvResult = 0;
    memset(recvBuffer, 0, DEFAULT_RECV_RTSP_BUFFER_SIZE);
    ////end

    do
    {
        recvResult = recv(_sock, recvBuffer, DEFAULT_RECV_RTSP_BUFFER_SIZE, 0);
        if (recvResult < 0)
        {
            if (errno == EINTR) continue;
            else if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                err = ST_RecvErr;;
                break;
            }
            else
            {
                err = ST_RecvErr;
                break;
            }
        }
        else if (recvResult == 0)
        {
            err = ST_RecvErr;
            break;
        }
        _msg.append(recvBuffer, recvResult);
        memset(recvBuffer, 0, recvResult);
    } while (recvResult == DEFAULT_RECV_RTSP_BUFFER_SIZE);

    delete[] recvBuffer;
    return err;
}

RtspClient::RtspClient(const Properties pProperties)
    :m_Properties(pProperties)
    ,m_uiCSeq(1)
    , m_eRTSPCommand(NONE)
    , m_iSocketFd(-1)
    , m_u8SourceIdOverTCP(0)
    , m_bStreamOverTCP(false)
    , m_strtpsession(nullptr)
    //, m_rtspsourcecallback(nullptr)
    , m_framecallback(nullptr)
    , m_rtspexceptioncallback(nullptr)
    , m_pUserPtr(nullptr)
    , m_iUserChannelId(-1)
    , m_iVideoWidth(1920)
    , m_iVideoHeight(1080)
    , m_bNeedAuthentication(false) 
{
}

RtspClient::~RtspClient() {
    while (m_strtpsession)
    {
        STRTPSession* toBeDestroyed = m_strtpsession;
        m_strtpsession = m_strtpsession->m_pNext;

        if (toBeDestroyed->IsActive())
        {
            toBeDestroyed->Destroy();
        }
        delete toBeDestroyed;
        toBeDestroyed = nullptr;
    }

    closeSocket();
}

int RtspClient::sendRTSP(const std::string& _msg)
{
    if (m_iSocketFd < 0)
        return ST_InvalidSocket;
#ifdef DEBUGINFO
    std::cout << "SEND MESSAGE(size:" << _msg.length() << "):\r\n"
        << _msg
        << std::endl;
#endif
    return ::sendRTSP(m_iSocketFd, _msg);
}

int RtspClient::recvRTSP(std::string& _msg)
{
    if (m_iSocketFd < 0)
        return ST_InvalidSocket;

    int ret = ::recvRTSP(m_iSocketFd, _msg);

#ifdef DEBUGINFO
    std::cout << "RECV MESSAGE(size:" << _msg.length() << "):\r\n"
        << _msg
        << std::endl;
#endif

    return ret;
}

int RtspClient::check200()
{
    if (m_strRTSPResponseStatusCode.compare("200") == 0) return ST_NoErr;
    else if (m_strRTSPResponseStatusCode.compare("401") == 0) return ST_AuthorizationErr;
    else if (m_strRTSPResponseStatusCode.compare("404") == 0) return ST_ResourceErr;
    else {
        return ST_ParseErr;
    }
}

int RtspClient::sendOptionsCommand()
{
    m_eRTSPCommand = OPTIONS;

    std::string msg = createRequest("OPTIONS", nullptr);

    ST_Error ret = ST_NoErr;

    ret = sendRTSP(msg);
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = recvRTSP(m_strRTSPResponse);
    if (ret != ST_NoErr)
    {
        return ret;
    }

    // 处理response
    ret = handleResponseBytes();

    return ret;
}

int RtspClient::sendDescribeCommand()

{
    m_eRTSPCommand = DESCRIBE;

    std::string msg = createRequest("DESCRIBE", nullptr);

    ST_Error ret = ST_NoErr;

    ret = sendRTSP(msg);
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = recvRTSP(m_strRTSPResponse);
    if (ret != ST_NoErr)
    {
        return ret;
    }

    // 处理response
    ret = handleResponseBytes();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = check200();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = parseSDP();

    return ret;
}

int RtspClient::sendSetupCommand()
{
    m_eRTSPCommand = SETUP;
    std::string msg;
    ST_Error ret = ST_NoErr;

    if (m_bStreamOverTCP)
    {
        bool hasSubSession = false;

        for (auto& mediaIt : m_sdp.mediaMap)
        {
            msg = createRequest("SETUP", &mediaIt.second);

            ret = sendRTSP(msg);
            if (ret != ST_NoErr)
            {
                return ret;
            }

            ret = recvRTSP(m_strRTSPResponse);
            if (ret != ST_NoErr)
            {
                return ret;
            }

            //����setup��response
            ret = handleResponseBytes();
            if (ret != ST_NoErr)
            {
                return ret;
            }

            ret = check200();
            if (ret != ST_NoErr)
            {
                return ret;
            }

            hasSubSession = true;
        }

        if (hasSubSession)
        {
            // 一个RTSP连接只创建一个Session
            STRTPSession* newSession = new STRTPSession();
            jrtplib::RTPSessionParams sessionParams;
            sessionParams.SetNeedThreadSafety(true);
            sessionParams.SetProbationType(jrtplib::RTPSources::NoProbation);
            sessionParams.SetMaximumPacketSize(1500);
            sessionParams.SetOwnTimestampUnit(1.0 / 1500);

            int error = newSession->Create(sessionParams, 0, jrtplib::RTPTransmitter::TCPProto);
            if (error < 0)
            {
                std::cerr << "STRTPSession Create Error" << std::endl;
                delete newSession;
                return ST_SessionErr;
            }
            // 将外部播放命令传递给Session
            //error = newSession->AddDestination(jrtplib::RTPTCPAddress(m_iSocketFd));
            //if (error < 0)

            //{
            //    std::cerr << "STRTPSession AddDestination Error" << std::endl;
            //    delete newSession;
            //    return ST_SessionErr;
            //}

            m_strtpsession = newSession;

            // 添加媒体流
            for (auto& mediaIt : m_sdp.mediaMap)
            {
                if (mediaIt.first == "video")
                {
                    m_strtpsession->AddPayloadType(std::stoi(mediaIt.second.mediaAttributeMap["payloadtype"]), ST_SDK_VIDEO_FRAME_FLAG, std::stoul(mediaIt.second.mediaAttributeMap["clockrate"]));
                }
                else if (mediaIt.first == "audio")
                {
                    m_strtpsession->AddPayloadType(std::stoi(mediaIt.second.mediaAttributeMap["payloadtype"]), ST_SDK_AUDIO_FRAME_FLAG, std::stoul(mediaIt.second.mediaAttributeMap["clockrate"]));
                }
            }

            // 设置媒体流信息
            m_strtpsession->m_stMediaInfo = &m_stMediaInfo;
            m_strtpsession->m_iVideoWidth = m_iVideoWidth;
            m_strtpsession->m_iVideoHeight = m_iVideoHeight;
            // 设置回调
            m_strtpsession->SetCallbackParams(m_pUserPtr, m_iUserChannelId);
            m_strtpsession->SetCallback(m_framecallback);
            m_strtpsession->SetExceptionCallback(m_rtspexceptioncallback);
        }
    }
    else //UDP
    {
        struct in_addr ipAddress;
        if (inet_pton(AF_INET, getIP(m_strUrl).c_str(), &ipAddress) == 0) {
            std::cerr << "Invalid IP address: " << getIP(m_strUrl).c_str() << std::endl;
            return ST_InvalidURL;
        }
        unsigned long serverIP = ntohl(ipAddress.s_addr);
        // 获取LocalIP
        struct sockaddr_in localAddress;
        socklen_t len = sizeof(localAddress);
        if (getsockname(m_iSocketFd, (struct sockaddr*)&localAddress, &len) < 0)
        {
            std::cerr << "Get Local IP error" << std::endl;
            return ST_InvalidSocket;
        }
        unsigned long clientIP = ntohl(localAddress.sin_addr.s_addr);

        for (auto& mediaIt : m_sdp.mediaMap)
        {
            STRTPSession* newSession = new STRTPSession();
            jrtplib::RTPSessionParams sessionParams;
            jrtplib::RTPUDPv4TransmissionParams transparams;
            // sessionParams.SetAcceptOwnPackets(false);
            sessionParams.SetNeedThreadSafety(true);
            sessionParams.SetProbationType(jrtplib::RTPSources::NoProbation);
            sessionParams.SetMaximumPacketSize(1500);
            sessionParams.SetUsePollThread(true);

            if (mediaIt.first == "video") {
                sessionParams.SetOwnTimestampUnit(1.0 / 90000.0);
            }
            else {
                sessionParams.SetOwnTimestampUnit(1.0 / 8000.0);
            }
            transparams.SetPortbase(0);
            transparams.SetBindIP(INADDR_ANY);

            int error = newSession->Create(sessionParams, &transparams);
            if (error < 0)
            {
                std::cerr << "STRTPSession Create Error" << std::endl;
                delete newSession;
                return ST_SessionErr;
            }
            // 获取实际分配的 RTP 端口号
            m_usClientPortNum = ((jrtplib::RTPUDPv4TransmissionInfo*)(newSession->GetTransmissionInfo()))->GetRTPPort();

#ifdef DEBUGINFO
            std::cout << "Client RTP Port: " << m_usClientPortNum << std::endl;
#endif

#ifdef _WIN32
            // 获取 socket 句柄用于其他操作（如果需要的话）
            // SocketType rtpSocket = ((jrtplib::RTPUDPv4TransmissionInfo*)(newSession->GetTransmissionInfo()))->GetRTPSocket();
#elif defined(__linux__)
            int udpsock = ((jrtplib::RTPUDPv4TransmissionInfo*)(newSession->GetTransmissionInfo()))->GetRTPSocket();
            int bytes_available = 0;
            if (ioctl(udpsock, FIONREAD, &bytes_available) == -1)
            {
                // 错误处理
                return -1;
            }
#elif defined(__APPLE__)
            // 设置UDP socket的接收缓冲区大小
            int udpsock = ((jrtplib::RTPUDPv4TransmissionInfo*)(newSession->GetTransmissionInfo()))->GetRTPSocket();
            int recv_buffer_size = 65535;
            setsockopt(udpsock, SOL_SOCKET, SO_RCVBUF, &recv_buffer_size, sizeof(recv_buffer_size));
#endif
            msg = createRequest("SETUP", &mediaIt.second);

            ret = sendRTSP(msg);
            if (ret != ST_NoErr)
            {
                return ret;
            }

            ret = recvRTSP(m_strRTSPResponse);
            if (ret != ST_NoErr)
            {
                return ret;
            }

            // 处理setup响应
            ret = handleResponseBytes();
            if (ret != ST_NoErr)
            {
                return ret;
            }

            ret = check200();
            if (ret != ST_NoErr)
            {
                return ret;
            }

            std::string serverIPStr = getIP(m_strUrl);
            bool isTargetPublic = !(serverIPStr.rfind("192.168.", 0) == 0 ||
                serverIPStr.rfind("10.", 0) == 0 ||
                (serverIPStr.rfind("172.", 0) == 0 && std::stoi(serverIPStr.substr(4, 2)) >= 16 && std::stoi(serverIPStr.substr(4, 2)) <= 31) ||
                serverIPStr.rfind("127.", 0) == 0);

            if (!m_bStreamOverTCP && isTargetPublic)
            {
                newSession->PunchHole(serverIPStr, m_usServerPortNum);
            }

            error = newSession->AddDestination(jrtplib::RTPIPv4Address(serverIP, m_usServerPortNum));
            if (error < 0)
            {
                std::cerr << "STRTPSession AddDestination Error" << std::endl;
                delete newSession;
                return ST_SessionErr;
            }

            if (mediaIt.first == "video")
            {
                newSession->AddPayloadType(std::stoi(mediaIt.second.mediaAttributeMap["payloadtype"]), ST_SDK_VIDEO_FRAME_FLAG, std::stoul(mediaIt.second.mediaAttributeMap["clockrate"]));
                newSession->m_iVideoWidth = m_iVideoWidth;
                newSession->m_iVideoHeight = m_iVideoHeight;
            }
            else if (mediaIt.first == "audio")
            {
                newSession->AddPayloadType(std::stoi(mediaIt.second.mediaAttributeMap["payloadtype"]), ST_SDK_AUDIO_FRAME_FLAG, std::stoul(mediaIt.second.mediaAttributeMap["clockrate"]));
            }

            //
            newSession->m_stMediaInfo = &m_stMediaInfo;
            // 设置回调
            newSession->SetCallbackParams(m_pUserPtr, m_iUserChannelId);
            newSession->SetCallback(m_framecallback);
            newSession->SetExceptionCallback(m_rtspexceptioncallback);

            //
            newSession->m_pNext = m_strtpsession;
            m_strtpsession = newSession;
        }
    }


    return ret;
}

int RtspClient::sendPlayCommand()
{
    m_eRTSPCommand = PLAY;

    std::string msg = createRequest("PLAY", nullptr);

    ST_Error ret = ST_NoErr;

    ret = sendRTSP(msg);
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = recvRTSP(m_strRTSPResponse);
    if (ret != ST_NoErr)
    {
        return ret;
    }

    // 处理play响应
    ret = handleResponseBytes();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = check200();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    if (ret == ST_NoErr && m_bStreamOverTCP)
    {
        int error = m_strtpsession->AddDestination(jrtplib::RTPTCPAddress(m_iSocketFd));
        if (error < 0)
        {
            std::cerr << "STRTPSession AddDestination Error" << std::endl;
            delete m_strtpsession;
            m_strtpsession = nullptr;
            return ST_SessionErr;
        }
    }

    return ret;
}

std::string RtspClient::createSessionString()
{
    if (!m_strSession.empty())
    {
        return "Session: " + m_strSession + "\r\n";
    }
    else return std::string();
}

std::string RtspClient::createAuthorizationString(const std::string& _cmd, const std::string& _url)
{
    const char* response = fCurrentAuthenticator.computeDigestResponse(_cmd.c_str(), _url.c_str());
    std::string str;
    str = "Authorization: Digest username=\"";
    str.append(fCurrentAuthenticator.username());
    str += "\", realm=\"";
    str.append(fCurrentAuthenticator.realm());
    str += "\", nonce=\"";
    str.append(fCurrentAuthenticator.nonce());
    str += "\", uri=\"";
    str.append(_url);
    str += "\", response=\"";
    str.append(response);
    str += "\"\r\n";
    fCurrentAuthenticator.reclaimDigestResponse(response);
    return str;
}

std::string RtspClient::createRequest(const std::string& _command, const void* data)
{
    const char* protocolStr = "RTSP/1.0";   //by default
    m_sstreamRequestMag.str("");

    std::string commandStr = _command;
    std::transform<std::string::const_iterator, std::string::iterator, int(int)>(_command.cbegin(), _command.cend(), commandStr.begin(), std::toupper);

    if (commandStr.compare("OPTIONS") == 0)
    {
        m_sstreamRequestMag << commandStr << " " << m_strUrl << " " << protocolStr << "\r\n";
        m_sstreamRequestMag << "CSeq: " << ++m_uiCSeq << "\r\n";
        if (m_bNeedAuthentication) {
            m_sstreamRequestMag << createAuthorizationString(commandStr, m_strUrl);
        }
        m_sstreamRequestMag << createSessionString();
        m_sstreamRequestMag << "\r\n";
    }
    else if (commandStr.compare("DESCRIBE") == 0)
    {
        m_sstreamRequestMag << commandStr << " " << m_strUrl << " " << protocolStr << "\r\n";
        m_sstreamRequestMag << "CSeq: " << ++m_uiCSeq << "\r\n";
        if (m_bNeedAuthentication) {
            m_sstreamRequestMag << createAuthorizationString(commandStr, m_strUrl);
        }
        m_sstreamRequestMag << "Accept: application/sdp" << "\r\n";
        m_sstreamRequestMag << "\r\n";
    }
    else if (commandStr.compare("SETUP") == 0)
    {
        //
        const char* baseURL;
        auto it = m_sdp.sessionAttributeMap.find("control");
        if (it != m_sdp.sessionAttributeMap.end() && !it->second.empty() && it->second.compare("*") != 0)
        {
            baseURL = it->second.c_str();
        }
        else
        {
            baseURL = m_strUrl.c_str();
        }

        struct MediaDescription* mediaDescription = (struct MediaDescription*)data;
        const char* suffixURL;
        const char* separator;
        auto mit = mediaDescription->mediaAttributeMap.find("control");
        if (mit != mediaDescription->mediaAttributeMap.end() && !mit->second.empty())
        {
            separator = "/";
            suffixURL = mit->second.c_str();
        }
        else
        {
            separator = "";
            suffixURL = "";
        }

        // 只支持RTP格式
        std::string transportFmt = "Transport: RTP/AVP%1%2%3=%4-%5\r\n";
        std::string transportTypeStr;
        std::string modeStr = /*streamOutgoing*/0 ? ";mode=receive" : "";
        std::string portTypeStr;
        std::string rtpNumber;
        std::string rtcpNumber;
        if (m_bStreamOverTCP)   // 使用UDP
        {
            transportTypeStr = "/TCP;unicast";
            portTypeStr = ";interleaved";
            rtpNumber = std::to_string(m_u8SourceIdOverTCP++);
            rtcpNumber = std::to_string(m_u8SourceIdOverTCP++);
        }
        else
        {
            transportTypeStr = ";unicast";
            portTypeStr = ";client_port";
            rtpNumber = std::to_string(m_usClientPortNum);
            rtcpNumber = std::to_string(m_usClientPortNum + 1);
        }
        {
            // 使用initialized_list
            std::vector<std::string> arg;
            arg.reserve(5);
            arg.push_back(transportTypeStr);
            arg.push_back(modeStr);
            arg.push_back(portTypeStr);
            arg.push_back(rtpNumber);
            arg.push_back(rtcpNumber);
            try
            {
                for (int i = 1; i <= arg.size(); ++i)
                {
                    std::regex regex("%" + std::to_string(i));
                    transportFmt = std::regex_replace(transportFmt, regex, arg[i - 1]);
                }
            }
            catch (std::regex_error e)
            {
                std::cerr << e.what() << '\t' << e.code() << std::endl;
            }
        }

        //
        m_sstreamRequestMag << commandStr << " " << baseURL << separator << suffixURL << " " << protocolStr << "\r\n";
        m_sstreamRequestMag << "CSeq: " << ++m_uiCSeq << "\r\n";
        if (m_bNeedAuthentication) {
            std::string uri;
            uri.append(baseURL).append(separator).append(suffixURL);
            m_sstreamRequestMag << createAuthorizationString(commandStr, uri);
        }
        m_sstreamRequestMag << transportFmt;
        m_sstreamRequestMag << createSessionString();
        m_sstreamRequestMag << "\r\n";

    }
    else if (commandStr.compare("PLAY") == 0)
    {
        m_sstreamRequestMag << commandStr << " " << m_strUrl << " " << protocolStr << "\r\n";
        m_sstreamRequestMag << "CSeq: " << ++m_uiCSeq << "\r\n";
        if (m_bNeedAuthentication) {
            m_sstreamRequestMag << createAuthorizationString(commandStr, m_strUrl);
        }
        m_sstreamRequestMag << createSessionString();
        m_sstreamRequestMag << "Range: " << "npt=0.000-" << "\r\n";
        m_sstreamRequestMag << "\r\n";
    }
    else if (commandStr.compare("TEARDOWN") == 0)
    {
        m_sstreamRequestMag << commandStr << " " << m_strUrl << " " << protocolStr << "\r\n";
        m_sstreamRequestMag << "CSeq: " << ++m_uiCSeq << "\r\n";
        if (m_bNeedAuthentication) {
            m_sstreamRequestMag << createAuthorizationString(commandStr, m_strUrl);
        }
        m_sstreamRequestMag << createSessionString();
        m_sstreamRequestMag << "\r\n";
    }
    return m_sstreamRequestMag.str();
}

int RtspClient::createSocket()
{
    // 创建socket
    static bool bHasWSAInit = false;
    if (!bHasWSAInit)
    {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cerr << "Failed to initialize Winsock." << std::endl;
            return ST_InvalidSocket;
        }
#endif
}
    SockType sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return ST_InvalidSocket;
    }

#ifdef _WIN32
    int timeout = 5000; // 设置超时时间
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof timeout);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
#else
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#endif

    m_iSocketFd = sock;
    return ST_NoErr;
}

int RtspClient::connectToServer()
{
    int port = getPort(m_strUrl);
    if (port < 0) return ST_InvalidURL;

    std::string ip = getIP(m_strUrl);
    if (ip.empty()) return ST_InvalidURL;

    // 连接服务器
    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address." << std::endl;
        return ST_InvalidURL;
    }

#ifdef _WIN32
    unsigned long flag = 0;
    // 设置非阻塞
    flag = 1;
    if (ioctlsocket(m_iSocketFd, FIONBIO, &flag) != 0) return ST_IOOperationErr;

    // 连接服务器
    if (connect(m_iSocketFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // 使用select等待连接
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(m_iSocketFd, &writefds);

        int error = 0;
        int len = sizeof(error);

        if (select(m_iSocketFd + 1, NULL, &writefds, NULL, &tv) > 0 && FD_ISSET(m_iSocketFd, &writefds) && (getsockopt(m_iSocketFd, SOL_SOCKET, SO_ERROR, (char*)&error, (socklen_t*)&len), error == 0)) {
        }
        else {
            return ST_ConnectErr;
        }
    }
    // 设置阻塞
    flag = 0;
    if (ioctlsocket(m_iSocketFd, FIONBIO, &flag) != 0) return ST_IOOperationErr;
#else
    // 设置非阻塞
    int flags = fcntl(m_iSocketFd, F_GETFL, 0);
    if (flags == -1) return ST_IOOperationErr;

    flags |= O_NONBLOCK;
    if (fcntl(m_iSocketFd, F_SETFL, flags) == -1) return ST_IOOperationErr;

    // 连接服务器
    if (connect(m_iSocketFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // 使用select等待连接
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(m_iSocketFd, &writefds);

        int error = 0;
        int len = sizeof(error);

        if (select(m_iSocketFd + 1, NULL, &writefds, NULL, &tv) > 0 && FD_ISSET(m_iSocketFd, &writefds) && (getsockopt(m_iSocketFd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&len), error == 0)) {
        }
        else {
            return ST_ConnectErr;
        }
    }

    // 设置阻塞
    flags = fcntl(m_iSocketFd, F_GETFL, 0);
    if (flags == -1) return ST_IOOperationErr;
    flags &= ~O_NONBLOCK;
    if (fcntl(m_iSocketFd, F_SETFL, flags) == -1) return ST_IOOperationErr;
#endif

    return ST_NoErr;
}

int RtspClient::parseSessionField(const std::string& _msg)
{
    try
    {
        // 修正：支持字母数字混合的Session ID
        std::regex regex("([a-zA-Z0-9]+)(;[\\S ]+)?");
        std::smatch results;

        if (std::regex_match(_msg, results, regex))
        {
            // 实现中，需要判断m_strSession是否为空，如果为空，则设置为sessionId
            m_strSession = results[1].str();
            return ST_NoErr;
        }
        return ST_ParseErr;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return ST_ParseErr;
    }
}

int RtspClient::parsePublicField(const std::string& _msg)
{
    try
    {
        std::regex regex("(\\w+)(,|$)");
        for (std::sregex_iterator it(std::cbegin(_msg), std::cend(_msg), regex), end_it; it != end_it; ++it)
        {
            m_setSupportedCommands.insert((*it)[1].str());
        }
        return ST_NoErr;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return ST_ParseErr;
    }
}

int RtspClient::parseTransportField(const std::string& _msg)
{
    if (m_bStreamOverTCP) return ST_NoErr;  // TCP模式不处理
    try
    {
        //RTP/AVP;unicast;client_port=12156-12157;server_port=64160-64161
        std::regex regex("RTP/AVP(?:/UDP)?;unicast;client_port=(\\d+)-(\\d+);server_port=(\\d+)-(\\d+)(?:;ssrc=([0-9a-fA-F]+))?");
        std::smatch results;

        if (std::regex_search(_msg, results, regex))
        {
            if (m_usClientPortNum == std::stoul(results[1].str()))
            {
                m_usServerPortNum = std::stoul(results[3].str());
                return ST_NoErr;
            }
            else return ST_ParseErr;
        }
        return ST_ParseErr;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return ST_ParseErr;
    }
}

int RtspClient::parse3W_AuthenticateField(const std::string& _msg)
{
    // 目前只支持Digest认证
    m_bNeedAuthentication = true;

    try
    {
        std::regex regex("Digest realm=\"(.+)\", nonce=\"(\\w+)\"");
        std::smatch results;

        if (std::regex_search(_msg, results, regex))
        {
            fCurrentAuthenticator.setRealmAndNonce(results[1].str().c_str(), results[2].str().c_str());
            return ST_NoErr;
        }
        else {
            std::cout << "6" << std::endl;
            return ST_ParseErr;

        }
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return ST_ParseErr;
    }
}

int RtspClient::parseResponseStatusLine(const std::string& _msg)
{
    try
    {
        std::regex regex("(RTSP)?/1.0 (\\d+) ((\\w| )+)\\s{0,2}");  // 为了JRTPLIB的影响，前面的字符串可能被截断
        std::smatch results;

        if (std::regex_match(_msg, results, regex))
        {
            m_strRTSPResponseStatusCode = results[2].str();
            m_strRTSPResponseStatusDescription = results[3].str();
            return ST_NoErr;
        }
        else return ST_ParseErr;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return ST_ParseErr;
    }
}

int RtspClient::handleResponseBytes()
{
    char* responseBuffer = new char[m_strRTSPResponse.size() + 1];
    if (responseBuffer == nullptr)
    {
        return ST_MemoryErr;
    }
    memcpy(responseBuffer, m_strRTSPResponse.data(), m_strRTSPResponse.size());
    //strncpy(responseBuffer, m_strRTSPResponse.c_str(), m_strRTSPResponse.size());
    responseBuffer[m_strRTSPResponse.size()] = '\0';
    char* lineStart;
    char* nextLineStart = responseBuffer;
    bool reachedEndOfHeaders;
    unsigned cseq = 0;
    unsigned long long contentLength = 0;
    int ret = ST_NoErr;

    do
    {
        lineStart = nextLineStart;
        nextLineStart = getLine(lineStart);
    } while (lineStart[0] == '\0' && nextLineStart != NULL);// skip over any blank lines at the start

    // 第一行是response的状态
    if (ST_NoErr != parseResponseStatusLine(lineStart))
    {
        delete[] responseBuffer;
        return ST_ParseErr;
    }

    while (1)
    {
        reachedEndOfHeaders = true;// by default; may get changed below
        lineStart = nextLineStart;  // response的第一行
        if (lineStart == NULL) break;

        nextLineStart = getLine(lineStart);
        if (lineStart[0] == '\0') break; // this is a blank line
        reachedEndOfHeaders = false;

        char const* headerParamsStr;
        if (checkForHeader(lineStart, "CSeq:", 5, headerParamsStr))
        {
            if (SSCANF_SAFE(headerParamsStr, "%u", &cseq) != 1 || cseq != m_uiCSeq) break;
        }
        else if (checkForHeader(lineStart, "Content-Length:", 15, headerParamsStr))
        {
            if (SSCANF_SAFE(headerParamsStr, "%llu", &contentLength) != 1) break;
        }
        else if (checkForHeader(lineStart, "Session:", 8, headerParamsStr))
        {
            if (ST_NoErr != parseSessionField(headerParamsStr)) break;
        }
        else if (checkForHeader(lineStart, "Public:", 7, headerParamsStr))
        {
            if (ST_NoErr != parsePublicField(headerParamsStr)) break;
        }
        else if (checkForHeader(lineStart, "Transport:", 10, headerParamsStr))
        {
            if (ST_NoErr != parseTransportField(headerParamsStr)) break;
        }
        else if (checkForHeader(lineStart, "WWW-Authenticate:", 17, headerParamsStr))
        {
            if (ST_NoErr != parse3W_AuthenticateField(headerParamsStr)) break;
        }
    }

    // 处理响应
    if (!reachedEndOfHeaders)
    {
        delete[] responseBuffer;
        return ST_ParseErr;
    }

    // 处理body
    if (contentLength)
    {
        if (m_eRTSPCommand == DESCRIBE)
        {
            m_strSDP.assign(nextLineStart, contentLength);
        }
    }
    delete[] responseBuffer;
    return ret;
}

bool RtspClient::parseSDPLine_o(const std::string& _sdpLine)
{
    try
    {
        //Check for "o=<username> <session id> <session version> <network type> <address type> <unicast-address>"
        std::regex regex("o=(\\S+) (\\d+) (\\d+) (\\w+) (\\w+) (\\d{1,3}(\\.\\d{1,3}){3})\\s{1,2}");
        std::smatch results;

        if (std::regex_match(_sdpLine, results, regex))
        {
            m_sdp.origion.userName = results[1].str();
            m_sdp.origion.sessionId = results[2].str();
            m_sdp.origion.sessionVersion = results[3].str();
            m_sdp.origion.networkType = results[4].str();
            m_sdp.origion.addressType = results[5].str();
            m_sdp.origion.address = results[6].str();
            return true;
        }
        return false;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return false;
    }
}

bool RtspClient::parseSDPLine_v(const std::string& _sdpLine)
{
    try
    {
        std::regex regex("v=(\\d+)\\s{1,2}");
        std::smatch results;
        if (std::regex_match(_sdpLine, results, regex))
        {
            m_sdp.v = std::stoul(results[1].str());
            return true;
        }
        return false;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return false;
    }
}

bool RtspClient::parseSDPLine_t(const std::string& _sdpLine)
{
    try
    {
        //Check for "t=<starttime> <endtime>"
        std::regex regex("t=(\\d+) (\\d+)\\s{1,2}");
        std::smatch results;
        if (std::regex_match(_sdpLine, results, regex))
        {
            m_sdp.sat.startTime = std::stoull(results[1].str());
            m_sdp.sat.stopTime = std::stoull(results[2].str());
            return true;
        }
        else return false;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return false;
    }
}

bool RtspClient::parseSDPLine_control(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap)
{
    try
    {
        std::regex regex("a=control:[ ]?(\\S+)\\s{1,2}");
        std::smatch results;
        if (std::regex_match(_sdpLine, results, regex))
        {
            _attributeMap["control"] = results[1].str();
            return true;
        }
        return false;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return false;
    }
}

bool RtspClient::parseSDPLine_rtpmap(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap)
{
    try
    {
        // Check for a "a=rtpmap:<fmt> <codec>/<freq>" line:
        std::regex regex("a=rtpmap:[ ]?(\\d+) (\\w+)/(\\d+)(/(\\d+))?\\s{1,2}");

        std::smatch results;
        if (std::regex_match(_sdpLine, results, regex))
        {
            _attributeMap["payloadtype"] = results[1].str();
            _attributeMap["codec"] = results[2].str();
            _attributeMap["clockrate"] = results[3].str();
            results[5].matched ? (_attributeMap["channels"] = results[5].str()) : "";
            return true;
        }
        return false;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return false;
    }
}

bool RtspClient::parseSDPLine_framerate(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap)
{
    try
    {
        // Check for a "a=rtpmap:<fmt> <codec>/<freq>" line:
        std::regex regex("a=framerate:[ ]?(\\d+)\\s{1,2}");
        std::smatch results;
        if (std::regex_match(_sdpLine, results, regex))
        {
            _attributeMap["framerate"] = results[1].str();
            return true;
        }
        return false;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return false;
    }
}

bool RtspClient::parseSDPLine_fmtp(const std::string& _sdpLine, std::map<attributeName, attributeValue>& _attributeMap)
{
    try
    {
        std::regex regex("a=\\s*fmtp:(\\d+)\\s*(.*)");
        std::smatch results;
        if (std::regex_search(_sdpLine, results, regex))
        {
            _attributeMap["payloadtype"] = results[1].str();

            std::regex kv_regex("([a-zA-Z0-9-]+)=([^;]+)");
            auto params_str = results[2].str();
            auto const_begin = std::sregex_iterator(params_str.begin(), params_str.end(), kv_regex);
            auto const_end = std::sregex_iterator();
            for (std::sregex_iterator it = const_begin; it != const_end; ++it)
            {
                std::string key = (*it)[1].str();
                std::string value = (*it)[2].str();

                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);

                _attributeMap[key] = value;
            }
            return true;
        }
        return false;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return false;
    }
}

int RtspClient::parseSDPLine_m(const std::string& _sdpLine, MediaDescription& mediaDescription)
{
    try
    {
        //Check m=<medium_name> <client_portNum>/<num_ports> <proto> <fmt>
        std::regex regex("m=(\\w+) (\\d+) (\\S+) (\\d+)\\s+");
        std::smatch results;
        if (std::regex_match(_sdpLine, results, regex))
        {
            mediaDescription.name = results[1].str();
            mediaDescription.port = results[2].str();
            mediaDescription.protocol = results[3].str();
            mediaDescription.fmt = results[4].str();
            return ST_NoErr;
        }
        else return ST_ParseErr;
    }
    catch (std::regex_error e)
    {
        std::cerr << e.what() << '\t' << e.code() << std::endl;
        return ST_ParseErr;
    }
}

void RtspClient::SetAuthenticatorCallback(STRTSPAuthCallback _callback)
{
    Authenticatorcallback = [this, _callback](const std::string& username, const std::string& password) -> bool {
        const char* user = nullptr;
        const char* pass = nullptr;
        bool result = _callback(m_iUserChannelId, m_pUserPtr, &user, &pass);
        if (result && user && pass) {
            fCurrentAuthenticator.setUsernameAndPassword(user, pass);
            return true;
        }
        return false;
    };
}

void RtspClient::SetAuthorization(const std::string& _username, const std::string& _password)
{
    fCurrentAuthenticator.setUsernameAndPassword(_username.c_str(), _password.c_str());
}

void RtspClient::setAuthenticatorcallback(std::function<bool(const std::string&, const std::string&)> _callback)
{
    Authenticatorcallback = _callback;
}

int RtspClient::parseSDP()
{
    if (m_strSDP.size() == 0) return ST_ParseErr;

    const char* lineStart = m_strSDP.c_str();
    const char* nextLineStart = NULL;
    std::string sdpLine;
    m_sdp.v = 0;
    while (1)
    {
        if (lineStart == NULL) break; // there are no m= lines at all
        if (lineStart[0] == 'm') break;
        if (!parseSDPLine(lineStart, nextLineStart)) return ST_ParseErr;
        if (sdpLine.empty()) {
            sdpLine = std::string(lineStart, 5);
        }
        else {
            sdpLine = std::string(lineStart, nextLineStart - lineStart);
        }

        lineStart = nextLineStart;
        if (parseSDPLine_v(sdpLine)) continue;
        if (parseSDPLine_o(sdpLine)) continue;
        if (parseSDPLine_t(sdpLine)) continue;
        if (parseSDPLine_control(sdpLine, m_sdp.sessionAttributeMap)) continue;
    }

    // 在循环外声明 mediaDescription，避免作用域问题
    struct MediaDescription mediaDescription;
    std::string currentMediaName;

    while (lineStart != NULL)
    {
        if (!parseSDPLine(lineStart, nextLineStart)) return ST_ParseErr;
        if (nextLineStart == NULL)
        {
            sdpLine = std::string(lineStart, strlen(lineStart));
        }
        else
        {
            sdpLine = std::string(lineStart, nextLineStart - lineStart);
        }
        lineStart = nextLineStart;

        if (sdpLine[0] == 'm') {
            if (ST_NoErr != parseSDPLine_m(sdpLine, mediaDescription)) return ST_ParseErr;
            currentMediaName = mediaDescription.name;
            m_sdp.mediaMap[currentMediaName] = mediaDescription;
        }
        else {
            if (currentMediaName.empty()) continue;
            // Check for various special SDP lines that we understand:
            if (parseSDPLine_control(sdpLine, m_sdp.mediaMap[currentMediaName].mediaAttributeMap)) continue;
            if (parseSDPLine_rtpmap(sdpLine, m_sdp.mediaMap[currentMediaName].mediaAttributeMap)) continue;
            if (parseSDPLine_framerate(sdpLine, m_sdp.mediaMap[currentMediaName].mediaAttributeMap)) continue;
            if (parseSDPLine_fmtp(sdpLine, m_sdp.mediaMap[currentMediaName].mediaAttributeMap)) continue;
        }
    }
    for (const auto& medium : m_sdp.mediaMap)
    {
        if (medium.first == "video")
        {
            const auto& attributeMap = medium.second.mediaAttributeMap;

            // 循环遍历所有视频属性
            for (const auto& attribute : attributeMap)
            {
                if (attribute.first == "codec" && attribute.second == "H264")
                {
                    m_stMediaInfo.u32VideoCodec = ST_CODEC_ID_H264;
                }
                if (attribute.first == "framerate")
                {
                    m_stMediaInfo.u32VideoFps = std::stoul(attribute.second);
                }
                else if (attribute.first == "sprop-parameter-sets")
                {
                    const std::string value = attribute.second;
                    size_t separatorIndex = std::string::npos;
                    if ((separatorIndex = value.find(',')) != std::string::npos)
                    {
                        std::string spsEn = value.substr(0, separatorIndex);
                        std::string ppsEn = value.substr(separatorIndex + 1, value.size() - separatorIndex - 1);

                        unsigned int spsDeLen = 0;
                        unsigned char* spsDe = base64Decode(spsEn.c_str(), spsEn.size(), spsDeLen);
                        if (spsDeLen > 0 && spsDeLen <= 255)
                        {
                            memmove(m_stMediaInfo.u8Sps, spsDe, spsDeLen);
                            m_stMediaInfo.u32SpsLength = spsDeLen;
                            bool result = H264_decode_sps((const char*)m_stMediaInfo.u8Sps, spsDeLen, m_iVideoWidth, m_iVideoHeight);
                            if (!result)
                            {
                                return ST_ParseErr;
                            }
                        }
                        delete[] spsDe;

                        unsigned int ppsDeLen = 0;
                        unsigned char* ppsDe = base64Decode(ppsEn.c_str(), ppsEn.size(), ppsDeLen);
                        if (ppsDeLen <= 128)
                        {
                            memmove(m_stMediaInfo.u8Pps, ppsDe, ppsDeLen);
                            m_stMediaInfo.u32PpsLength = ppsDeLen;
                        }
                        delete[] ppsDe;
                    }
                }
            }
        }
        else if (medium.first == "audio")
        {
            const auto& attributeMap = medium.second.mediaAttributeMap;
            // 循环遍历所有音频属性
            for (const auto& attribute : attributeMap)
            {
                if (attribute.first == "codec" && attribute.second == "L16")
                {
                    // 参考live555的默认值
                    //case 10:{temp = "L16"; freq = 44100; nCh = 2; break}
                    //case 11:{temp = "L16"; freq = 44100; nCh = 1; break}
                    m_stMediaInfo.u32AudioCodec = ST_CODEC_ID_PCM_S16BE;
                    m_stMediaInfo.u32AudioBitsPerSample = 16;
                    m_stMediaInfo.u32AudioChannel = 1;
                }
                else if (attribute.first == "clockrate")
                {
                    m_stMediaInfo.u32AudioSamplerate = std::stoul(attribute.second);
                }
            }
        }
    }

    if (m_framecallback)
        m_framecallback(m_iUserChannelId, ST_SDK_MEDIA_INFO_FLAG, nullptr, &m_stMediaInfo);

    return ST_NoErr;
}

bool RtspClient::initialize() {
    m_strUrl = getProperty<std::string>(m_Properties, "url", "");
    if (m_strUrl == "") {
        return false;
    }
    m_bStreamOverTCP = getProperty<bool>(m_Properties, "stream_over_tcp", false);
    m_running = true;
    return true;

}

int RtspClient::OpenStream() {
#ifdef DEBUGINFO
    std::cout << "channelid = " << _channelid << "\t connectType" << (_overTCP ? "TCP" : "UDP") << std::endl;
#endif
    ST_Error ret = ST_NoErr;

    ret = createSocket();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = connectToServer();
    if (ret != ST_NoErr)
    {
        return ret;
    }

#ifdef DEBUGINFO
    std::cout << "Create TCP socket for RTSP succeed!" << std::endl;
#endif

    ret = sendOptionsCommand();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    int retrans = 1;
    do {
        ret = sendDescribeCommand();
        if (m_bNeedAuthentication && (fCurrentAuthenticator.username() == nullptr || fCurrentAuthenticator.username()[0] == '\0'))
        {
            if (Authenticatorcallback) {
                if (!Authenticatorcallback("", "")) {
                    return ST_AuthorizationErr;
                }
            }
            else {
                return ST_AuthorizationErr;
            }
        }

    } while (ret == ST_AuthorizationErr && retrans--);
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = sendSetupCommand();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    ret = sendPlayCommand();
    if (ret != ST_NoErr)
    {
        return ret;
    }

    return ret;
}

int RtspClient::SendTeardownCommand()
{
    m_eRTSPCommand = TEARDOWN;

    std::string msg = createRequest("TEARDOWN", nullptr);

    ST_Error ret = ST_NoErr;

    ret = sendRTSP(msg);
    if (ret != ST_NoErr)
    {
        return ret;
    }
    return ret;
}

int RtspClient::CloseStream() {
    ST_Error ret = ST_NoErr;

    if (m_eRTSPCommand == PLAY)
    {
        while (m_strtpsession)
        {
            STRTPSession* toBeDestroyed = m_strtpsession;
            m_strtpsession = m_strtpsession->m_pNext;

            toBeDestroyed->ClearDestinations();
            toBeDestroyed->Destroy();
            delete toBeDestroyed;
            toBeDestroyed = nullptr;
        }

        ret = SendTeardownCommand();
    }
    closeSocket();

    return ret;
}
void RtspClient::SetCallback(FrameCallback _callback) {
    //m_rtspsourcecallback = _callback;
    m_framecallback = _callback;

    STRTPSession* next = m_strtpsession;
    while (next)
    {
        next->SetCallback(m_framecallback);
        next = next->m_pNext;
    }
}

bool RtspClient::updateProperties(Properties* _properties) {
    if (_properties == nullptr)
    {
        return false;
    }
    m_Properties = *_properties;
    initialize();
    return true;
}

int RtspClient::closeSocket()
{
    if (m_iSocketFd > 0)
    {
        // 关闭socket
#ifdef _WIN32
        closesocket(m_iSocketFd);
        //WSACleanup();
#else
        close(m_iSocketFd);
#endif
        m_iSocketFd = -1;
    }
    return 0;
}

namespace {
    Registrar<RtspClient, IProtocolHandler, const Properties&> registrar("rtsp_client");
}