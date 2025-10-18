#include "STRTPSession.h"
#include "SpsParse.h"

STRTPSession::STRTPSession()
	: RTPSession()
	, m_pNext(nullptr)
	, m_rtspsourcecallback(nullptr)
	, m_rtspexceptioncallback(nullptr)
	, m_pUserPtr(nullptr)
	, m_uiErrorCount(0)
	, m_iVideoWidth(0)
	, m_iVideoHeight(0)
{
}

STRTPSession::~STRTPSession()
{
	for (std::map<PayloadType, CustomTemporaryStructure>::iterator it = m_mapRTP.begin(), end_it = m_mapRTP.end(); it != end_it; ++it)
	{
		RTPPacketPtrQueue* queue = &(it->second.queueRTPPacketPtr);
		while (queue->size())
		{
			jrtplib::RTPPacket* packet = queue->front();
			queue->pop();
			DeletePacket(packet);
		}
	}	
}

void STRTPSession::AddPayloadType(uint8_t _payloadType, int _frameType, unsigned int _clockrate)
{
	if(m_mapRTP.find(_payloadType) != m_mapRTP.end())
	{
		m_mapRTP[_payloadType].frameType = _frameType;
		m_mapRTP[_payloadType].clockrate = _clockrate;
	}
	else
	{
		CustomTemporaryStructure temp;
		temp.frameType = _frameType;
		temp.clockrate = _clockrate;
		m_mapRTP.emplace(std::make_pair(_payloadType, temp));
	}

#ifdef DEBUGINFO
	std::cout << "Add PayLoadType:" << _payloadType << std::endl;
#endif
}

void STRTPSession::SetCallbackParams(void* _userPtr, int _channelid)
{
	m_pUserPtr = _userPtr;
	m_iUserChannelId = _channelid;
}

void STRTPSession::SetCallback(FrameCallback _callback)
{
	m_rtspsourcecallback = _callback;
}

void STRTPSession::SetExceptionCallback(STRTSPExceptionCallback _callback)
{
	m_rtspexceptioncallback = _callback;
}

void STRTPSession::PunchHole(const std::string& destIP, unsigned short destPort)
{
	// Get the transmission parameters and cast them to the correct type
	jrtplib::RTPUDPv4TransmissionInfo* transinfo = static_cast<jrtplib::RTPUDPv4TransmissionInfo*>(GetTransmissionInfo());
	if (transinfo == nullptr)
	{
#ifdef DEBUGINFO
		std::cerr << "PunchHole Error: Could not get transmission info." << std::endl;
#endif
		return;
	}

	SockType rtpSocket = transinfo->GetRTPSocket();
#ifdef _WIN32
	if (rtpSocket == INVALID_SOCKET)
#else
	if (rtpSocket < 0)
#endif
	{
#ifdef DEBUGINFO
		std::cerr << "PunchHole Error: Could not get RTP socket." << std::endl;
#endif
		return;
	}

	// Prepare server's destination address
	struct sockaddr_in serv_addr;
	std::memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(destPort);
	if (inet_pton(AF_INET, destIP.c_str(), &serv_addr.sin_addr) <= 0) {
#ifdef DEBUGINFO
		std::cerr << "PunchHole Error: Invalid server address." << std::endl;
#endif
		return;
	}

	// A minimal dummy packet, content doesn't matter
	char dummy_packet[1] = { 0 };

	// Send the packet to establish the NAT mapping
	sendto(rtpSocket, dummy_packet, 1, 0, (const struct sockaddr*)&serv_addr, sizeof(serv_addr));

#ifdef DEBUGINFO
	uint16_t localPort = transinfo->GetRTPPort();
	std::cout << "NAT Traversal: Punch-hole packet sent to " << destIP << ":" << destPort << " from our RTP port " << localPort << std::endl;
#endif
}

void STRTPSession::OnValidatedRTPPacket(jrtplib::RTPSourceData* srcdat, jrtplib::RTPPacket* rtppack, bool isonprobation, bool* ispackethandled)
{
	std::map<PayloadType, CustomTemporaryStructure>::iterator it;
	if ((it = m_mapRTP.find(rtppack->GetPayloadType())) != m_mapRTP.end())
	{
		// 检查是否是有效媒体流
		CustomTemporaryStructure* media = &it->second;

		// 检查时间戳是否一致
		if (media->payloadSize != 0 && media->currentTimestamp != rtppack->GetTimestamp())	//检查时间戳是否一致
		{
#ifdef DEBUGINFO
			std::cout << "(info) Has difference timestamp!" << std::endl;
#endif
			clearQueue(media->queueRTPPacketPtr);
			media->payloadSize = 0;
		}
		if (media->payloadSize == 0) media->currentTimestamp = rtppack->GetTimestamp();

		// 存储RTP包
		media->queueRTPPacketPtr.push(rtppack);
		media->payloadSize += rtppack->GetPayloadLength();
		*ispackethandled = true;

		// 检查是否是关键帧
		if (rtppack->HasMarker())
		{
			ST_FRAME stFrame;
			ST_FRAME_INFO stFrameInfo;
			static int VideoWidth = 0;
			static int VideoHeight = 0;

			// 扩展数据
			if (rtppack->HasExtension()) {
				stFrame.extraDataLength = rtppack->GetExtensionLength();
				stFrame.extraDataBuffer = new unsigned char[stFrame.extraDataLength];
				memcpy(stFrame.extraDataBuffer, rtppack->GetExtensionData(), stFrame.extraDataLength);
			}

			// 检查帧类型
			if (media->frameType == ST_SDK_AUDIO_FRAME_FLAG)
			{
				// 音频帧
				RTPPacketPtrQueue* queue = &(media->queueRTPPacketPtr);
				int index = 0;
				stFrame.frameLength = media->payloadSize;
				stFrame.frameBuffer = new unsigned char[stFrame.frameLength];
				while (queue->size() > 0)
				{
					auto packet = queue->front();
					queue->pop();

					memcpy(stFrame.frameBuffer + index, packet->GetPayloadData(), packet->GetPayloadLength());
					index += packet->GetPayloadLength();

					DeletePacket(packet);
				}
				// 设置音频帧信息
				stFrameInfo.codec = m_stMediaInfo->u32AudioCodec;
				stFrameInfo.sample_rate = m_stMediaInfo->u32AudioSamplerate;
				stFrameInfo.channels = m_stMediaInfo->u32AudioChannel;
				stFrameInfo.bits_per_sample = m_stMediaInfo->u32AudioBitsPerSample;
			}
			else if (media->frameType == ST_SDK_VIDEO_FRAME_FLAG)
			{
				RTPPacketPtrQueue* queue = &(media->queueRTPPacketPtr);
				int index = 0;
				bool isKeyFrame = false;
				stFrame.frameLength = 0;
				stFrame.frameBuffer = new unsigned char[media->payloadSize + 4 * queue->size()];	// 预留空间
				static unsigned char nalu_header[] = { 0, 0, 0, 1 };
				while (queue->size())
				{
					auto packet = queue->front();
					queue->pop();
					auto dataPtr = packet->GetPayloadData();
					auto payloadLen = packet->GetPayloadLength();
					if (payloadLen == 0) {
						DeletePacket(packet);
						continue;
					}
					// 检查是否需要NALU头
					auto flag = dataPtr[0] & 0x1F;
					if (flag == 0x07 //SPS
						|| flag == 0x08	//PPS
						|| flag == 0x05	//I֡
						|| flag == 0x01	//P֡
						)
					{
						memcpy(stFrame.frameBuffer + index, nalu_header, 4);
						stFrame.frameLength += 4;
						index += 4;
						memcpy(stFrame.frameBuffer + index, packet->GetPayloadData(), packet->GetPayloadLength());
						stFrame.frameLength += payloadLen;
						index += payloadLen;
						// 检查是否是关键帧
						if (flag == 0x07 || flag == 0x08 || flag == 0x05) isKeyFrame = true;
						// 检查是否是SPS
						if (flag == 0x07)
						{
							int separateIndex = 0;	// SPS的分割索引，可能不存在
							for (; separateIndex < packet->GetPayloadLength(); ++separateIndex)
							{
								while (separateIndex < packet->GetPayloadLength() && dataPtr[separateIndex] != 0) ++separateIndex;
								if ((separateIndex + 3) >= packet->GetPayloadLength())	// 没有分割0001
								{
									separateIndex = packet->GetPayloadLength();
									break;
								}
								if (dataPtr[separateIndex] == 0 && dataPtr[separateIndex + 1] == 0 && dataPtr[separateIndex + 2] == 0 && dataPtr[separateIndex + 3] == 1) break;	//�ҵ���
							}
							H264_decode_sps((const char*)dataPtr, separateIndex, VideoWidth, VideoHeight);
						}
					}
					else if (flag == 0x1C	// FU-A帧
						//|| flag == 0x1D	// FU-B帧, 如果FU-A帧的DON字段是0，则FU-B帧的DON字段是1
						)
					{
						unsigned numBytesToSkip = 0;
						unsigned char startBit = dataPtr[1] & 0x80;
						unsigned char endBit = dataPtr[1] & 0x40;
						if (startBit)
						{
							// 因为帧头是第一个数据，所以也可能是0001
							memcpy(stFrame.frameBuffer + index, nalu_header, 4);
							stFrame.frameLength += 4;
							index += 4;
							numBytesToSkip = 1;
							// 检查是否是关键帧
							if ((dataPtr[1] & 0x1F) == 0x07 || (dataPtr[1] & 0x1F) == 0x08 || (dataPtr[1] & 0x1F) == 0x05 ) isKeyFrame = true;
							// 检查是否是SPS
							if ((dataPtr[1] & 0x1F) == 0x07)
							{
								int separateIndex = numBytesToSkip;	// SPS的分割索引，可能不存在
								for (; separateIndex < packet->GetPayloadLength(); ++separateIndex)
								{
									while (separateIndex < packet->GetPayloadLength() && dataPtr[separateIndex] != 0) ++separateIndex;
									if ((separateIndex + 3) >= packet->GetPayloadLength())	// 没有分割0001
									{
										separateIndex = packet->GetPayloadLength();
										break;
									}
									if (dataPtr[separateIndex] == 0 && dataPtr[separateIndex + 1] == 0 && dataPtr[separateIndex + 2] == 0 && dataPtr[separateIndex + 3] == 1) break;	// 找到
								}
								H264_decode_sps((const char*)dataPtr + numBytesToSkip, separateIndex - numBytesToSkip, VideoWidth, VideoHeight);
							}
						}
						else
						{
							numBytesToSkip = 2;	// 因为帧头是第一个数据，所以也可能是0001
						}

						if (startBit) {
							// 如果是分片的第一个包，我们需要重建并写入 NALU 头
							stFrame.frameBuffer[index] = (dataPtr[0] & 0xE0) | (dataPtr[1] & 0x1F);
							index++;
							stFrame.frameLength++;
						}		
						// 写入 NALU 的载荷数据
						memcpy(stFrame.frameBuffer + index, packet->GetPayloadData() + numBytesToSkip, packet->GetPayloadLength() - numBytesToSkip);
						stFrame.frameLength += packet->GetPayloadLength() - numBytesToSkip;
						index += packet->GetPayloadLength() - numBytesToSkip;
					}
					// 删除RTP包
					DeletePacket(packet);
				}
				// 设置视频帧信息
				stFrameInfo.codec = m_stMediaInfo->u32VideoCodec;
				stFrameInfo.fps = m_stMediaInfo->u32VideoFps;
				stFrameInfo.width = m_iVideoWidth;
				stFrameInfo.height = m_iVideoHeight;
				stFrameInfo.type = isKeyFrame ? ST_SDK_VIDEO_FRAME_I : ST_SDK_VIDEO_FRAME_P;
				
				if (m_iVideoHeight != VideoHeight || m_iVideoWidth != VideoWidth) {
					m_iVideoHeight = VideoHeight;
					m_iVideoWidth = VideoWidth;
					if (m_rtspsourcecallback != nullptr) {
						m_rtspsourcecallback(m_iUserChannelId, media->frameType, &stFrame, &stFrameInfo);
					}
				}
			}
			// 转换时间戳
			timestampTransform(media->currentTimestamp, media->clockrate, stFrameInfo.timestamp_sec, stFrameInfo.timestamp_usec);
			

			// 发送帧数据
			if (m_rtspsourcecallback != nullptr)
			{
				m_rtspsourcecallback(m_iUserChannelId,media->frameType, &stFrame, &stFrameInfo);
			}
			// 释放内存
			delete[] stFrame.extraDataBuffer;
			delete[] stFrame.frameBuffer;
			media->payloadSize = 0;
		}
	}

	if (*ispackethandled != true)
	{
#ifdef DEBUGINFO
		std::cout << std::endl
			<< "(info) Unknown Payload Type"
			<< std::endl;
#endif
		DeletePacket(rtppack);
		*ispackethandled = true;
		std::string errorMsg = "Has too much undefined payload type";
		if (m_rtspexceptioncallback != nullptr)
		{
			m_rtspexceptioncallback(m_iUserChannelId, m_pUserPtr, ST_InvalidPayloadType, errorMsg.c_str());
		}
	}
}

void STRTPSession::clearQueue(RTPPacketPtrQueue& queue)
{
	while (queue.size())
	{
		auto packet = queue.front();
		queue.pop();
		DeletePacket(packet);
	}
}

void STRTPSession::timestampTransform(unsigned int _timestamp, unsigned int _clockrate, unsigned int& _sec, unsigned int& _usec)
{
	// 时间戳的精度不是精确的，因为RTP时间戳的单位是1/_clockrate 
	// https://blog.csdn.net/u012478275/article/details/99623110
	// https://www.jianshu.com/p/653990b71a40
	// https://blog.csdn.net/jasonhwang/article/details/7316168
	// RTP时间戳的计算公式也可以参考live555源码，MultiFramedRTPSink.cpp中的setTimestamp
	// RTP时间戳转换为NPT需要使用相同的时钟频率，并且需要使用相同的时钟源（NTP）。
	// 目前只使用timestamp/frequency得到单位为秒的时间

	_sec = _timestamp / _clockrate;	// 强制转换只取整数部分
	_usec = ((unsigned long long)_timestamp * 1000000 / _clockrate) % 1000000;	// 使用uint32_t计算微秒
}
