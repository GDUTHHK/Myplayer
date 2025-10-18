#include "PrivateRtsp.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include <string>
#include <regex>
#include <fstream>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#endif



#define DEFAULT_PORT    554
#define DEFAULT_RECV_RTSP_BUFFER_SIZE   1024


#if defined(__WIN32__) || defined(_WIN32) || defined(_QNX4)
#define _strncasecmp _strnicmp
#define snprintf _snprintf
#else
#define _strncasecmp strncasecmp
#endif

PrivateRtsp::PrivateRtsp(const Properties pProperties)
    : m_Properties(pProperties)
    , m_iSocketFd(-1)
    , m_running(false)
    , m_bStreamOverTCP(false)
    , m_framecallback(nullptr)
{
}

PrivateRtsp::~PrivateRtsp()
{
    CloseStream();
}

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

static std::string getIP(const std::string& _url)
{
    std::regex ip_regex("stonkam://(\\d{1,3}(\\.\\d{1,3}){3})(:\\d+)?\\S*");
    std::smatch results;
    try
    {
        if (std::regex_match(_url, results, ip_regex))
        {
            return results[1].str();
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
    std::regex port_regex("stonkam://(\\d{1,3}(\\.\\d{1,3}){3})(:(\\d+))?\\S*");
    std::smatch results;

    try
    {
        if (std::regex_match(_url, results, port_regex))
        {
            if (results[4].matched)
            {
                return std::stoi(results[4].str());
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

bool PrivateRtsp::initialize()
{
    m_strUrl = getProperty<std::string>(m_Properties,"url", "");
    if (m_strUrl == "") {
        return false;
    }
    m_bStreamOverTCP = getProperty<bool>(m_Properties,"stream_over_tcp", false);
    m_running = true;
    return true;
}

bool PrivateRtsp::updateProperties(Properties* _properties)
{
    if(_properties == nullptr)
    {
        return false;
    }
    m_Properties = *_properties;
    initialize();
    return true;
}

void PrivateRtsp::SetCallback(FrameCallback _callback)
{
    m_framecallback = _callback;
}

int PrivateRtsp::sendCommand(const void *command,const int lenth){
    if (m_iSocketFd < 0)
    return ST_InvalidSocket;
    static int number =0;
    TCP_REQ_S reqPacket;
    memcpy(reqPacket.sCmdFlag,command, lenth);
    reqPacket.cType = 1;
    reqPacket.cFlag = number++;
    if(number == 255) number = 0;
    auto now  = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    reqPacket.u64Pts = static_cast<uint64_t>(timestamp);

#ifdef DEBUGINFO
    std::cout << "SEND START MESSAGE: ";
    std::cout.write(reqPacket.sCmdFlag, 4);
    std::cout<< ", Type: " << (int)reqPacket.cType 
              << ", Flag: " << (int)reqPacket.cFlag 
              << ", Timestamp: " << reqPacket.u64Pts << std::endl;
#endif
    const char* msg = reinterpret_cast<const char *>(&reqPacket);
    size_t size = sizeof(TCP_REQ_S);
    int sendResult = 0;
    int index = 0;
    int err = ST_NoErr;
    while (size > 0)
    {
        sendResult = send(m_iSocketFd, msg + index, size, 0);
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

int PrivateRtsp::OpenStream() {
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
    ret = sendCommand("#STA",4);
    if(ret != ST_NoErr){
        return ret;
    }
    m_thread = std::thread(&PrivateRtsp::parserLoop, this);

    if(m_framecallback != nullptr){
        m_framecallback(-1,ST_SDK_MEDIA_INFO_FLAG,nullptr,nullptr); 
    }
    return ST_NoErr;
}

//int PrivateRtsp::getIdentity(){
//    return m_iIdentity;
//}

static uint64_t get_timestamp_us_on_windows() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL;
    return (uli.QuadPart - EPOCH_DIFFERENCE) / 10;
}

void PrivateRtsp::parserLoop() {
    TCP_STREAM_S dataHead;
    memset(&dataHead, 0, sizeof(TCP_STREAM_S));
    while (m_running) {
        int headLen = sizeof(TCP_STREAM_S);
        memset(&dataHead, 0, headLen);
        int headSize = receiveData(m_iSocketFd, (char*)&dataHead, headLen, 1000);
        uint64_t now = get_timestamp_us_on_windows();
        
        int64_t  difference_in_us = static_cast<int64_t>(now) - static_cast<int64_t>(dataHead.u64Pts);

        if (headSize == headLen && strncmp(dataHead.sFlag,"#WFC",4)==0 ) {
            char* data = new char[dataHead.u32DataSize];
            int dataBody = recv(m_iSocketFd, data, dataHead.u32DataSize, MSG_WAITALL);
            if (dataBody >= 0 && dataBody == dataHead.u32DataSize ) {
                ST_FRAME frame;
                frame.frameBuffer = (ST_UChar *)data;
                frame.frameLength = dataBody;

                if (m_framecallback != nullptr) {
                    m_framecallback(-1, ST_SDK_VIDEO_PRIVATE_FRAME_FLAG, &frame, nullptr);
                }
            }
            else {
                std::cout << "dataBody error:" << dataBody 
                           <<"  need dataHead.u32DataSize:"<< dataHead.u32DataSize<< std::endl;
            }
            delete[]data;
        }
        else {
            std::cout << "sFlag error:" << dataHead.sFlag
                        <<" Head Size:"<< headSize <<std::endl;
        }
    }
}

int PrivateRtsp::pollReadFd(const int& iFd, const unsigned int& uiTimeOut)
 {
     fd_set read_set;
     FD_ZERO(&read_set);
     FD_SET(iFd, &read_set);
     struct timeval    tv;
     tv.tv_sec = uiTimeOut / 1000;
     tv.tv_usec = 1000 * (uiTimeOut % 1000);

     int iMaxFd = iFd;
     int iRes = select(iMaxFd + 1, &read_set, NULL, NULL, &tv);

     return iRes;
 }

 int PrivateRtsp::receiveData(const int& iFd, char* buf, const int& iBufLen, const unsigned int& uiTimeOut)
 {
        if (iBufLen <= 0)
            return 0;
        int iResult, iErrno;
        char* orig = buf;
        iResult = pollReadFd(iFd, uiTimeOut);
        if (iResult < 0)
        {
            iErrno = errno;
            std::cout << "receiveData error" << std::endl;
            return -1;
        }
        else if (iResult == 0)
        {
            return 0;
        }
        iResult = recv(iFd, (char*)buf, iBufLen, 0);
        if (iResult <= 0)//if using poll and fd is unblock the return value (<0 and ==0) is the same
        {
            iErrno = errno;
            if (iErrno == EAGAIN || iErrno == EINTR) //same in iResult <= 0 and iResult == 0 if fd is unblock
            {
                return 0;
            }
            else
            {
                std::cout << "receiveData error" << std::endl;
                return -1;
            }
        }
        buf += iResult;
        return (int)(buf - orig);
 }

 int PrivateRtsp::CloseStream()
{
    ST_Error ret = ST_NoErr;
    if(m_running){
        m_running = false;
    }   

    if(m_thread.joinable()){
        m_thread.join();
    }

    if (m_iSocketFd > 0) {
        sendCommand("#STO", 4);
        closeSocket();
    }
    return ret;
}

int PrivateRtsp::createSocket()
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
    struct timeval timeout = {5,0};
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#endif
    
    m_iSocketFd = sock;
    return ST_NoErr;
}

int PrivateRtsp::connectToServer()
{
    int port = getPort(m_strUrl);
    if (port < 0) return ST_InvalidURL;

    std::string ip = getIP(m_strUrl);
    if(ip.empty()) return ST_InvalidURL;

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
    //flag = 1;
    //if (ioctlsocket(m_iSocketFd, FIONBIO, &flag) != 0) return ST_IOOperationErr;

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
    //// 设置阻塞
    // flag = 0;
    // if (ioctlsocket(m_iSocketFd, FIONBIO, &flag) != 0) return ST_IOOperationErr;
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

int PrivateRtsp::closeSocket()
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