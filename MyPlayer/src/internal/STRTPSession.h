#pragma once
#include <list>
#include <queue>
#include <map>

#include "jrtplib3/rtpsession.h"
#include "jrtplib3/rtpsession.h"
#include "jrtplib3/rtppacket.h"
#include "jrtplib3/rtpsourcedata.h"
#include "jrtplib3/rtpsessionparams.h"
#include "jrtplib3/rtptcpaddress.h"
#include "jrtplib3/rtpudpv4transmitter.h"

#include "internal/Share.h"

typedef uint8_t PayloadType;
typedef  std::queue<jrtplib::RTPPacket*, std::list<jrtplib::RTPPacket*>> RTPPacketPtrQueue;
typedef struct CustomTemporaryStructure {
	int frameType;
	unsigned int clockrate;
	RTPPacketPtrQueue queueRTPPacketPtr;
	unsigned long long payloadSize = 0;
	unsigned int currentTimestamp; 	// 当前时间戳
} CTS;

class STRTPSession : public jrtplib::RTPSession
{
public:
	STRTPSession();
	~STRTPSession();
	void AddPayloadType(uint8_t _payloadType, int _frameType, unsigned int _clockrate);
	void SetCallbackParams(void* _userPtr, int _channelid);
	//void SetCallback(STRTSPSourceCallBack _callback);
	void SetCallback(FrameCallback _callback);

	void SetExceptionCallback(STRTSPExceptionCallback _callback);
	void PunchHole(const std::string& _destIP, unsigned short _destPort);
protected:
	void OnValidatedRTPPacket(jrtplib::RTPSourceData* srcdat, jrtplib::RTPPacket* rtppack, bool isonprobation, bool* ispackethandled) override;
private:
	void clearQueue(RTPPacketPtrQueue& queue);
	void timestampTransform(unsigned int _timestamp, unsigned int _clockrate, unsigned int& _sec, unsigned int& _usec);

public:
	const ST_MEDIA_INFO* m_stMediaInfo;
	int m_iVideoWidth;
	int m_iVideoHeight;
	STRTPSession* m_pNext;
private:
	std::map<PayloadType, CustomTemporaryStructure> m_mapRTP;
	FrameCallback m_rtspsourcecallback;
	//STRTSPSourceCallBack m_rtspsourcecallback;
	STRTSPExceptionCallback m_rtspexceptioncallback;
	void* m_pUserPtr;
	int m_iUserChannelId;
	unsigned int m_uiErrorCount;
};

