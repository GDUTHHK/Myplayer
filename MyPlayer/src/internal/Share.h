#pragma once

#ifndef _ST_SHARE_H
#define _ST_SHARE_H

#ifdef _WIN32
#define ST_API  __declspec(dllexport)
#define ST_APICALL  __stdcall
#define WIN32_LEAN_AND_MEAN
#define SockType SOCKET
#else
#define ST_API
#define ST_APICALL 
#define SockType int
#endif

#ifdef DEBUGINFO
#include <iostream>
#include <vector>
#include <atomic>
#endif

#include <chrono>
#include<unordered_map>
#include<any>
#include<iostream>
#include<string>
#include<functional>

// Handle
#define ST_Handle void*

typedef int						ST_I;
typedef unsigned char           ST_U8;
typedef unsigned char           ST_UChar;
typedef unsigned short          ST_U16;
typedef unsigned int            ST_UI;

enum RET_CODE {
	ST_NoErr = 0,
	ST_SendErr = -1,
	ST_RecvErr = -2,
	ST_MemoryErr = -3,
	ST_OutOfNumber = -4,	//Handle数量超出
	ST_InvalidParamter = -5,
	ST_InvalidSocket = -6,
	ST_InvalidURL = -7,
	ST_ConnectErr = -8,
	ST_ConnectClosed = -9,
	ST_ParseErr = -10,
	ST_SessionErr = -11,
	ST_AuthorizationErr = -12,
	ST_ResourceErr = -13,
	ST_IOOperationErr = -14,
	ST_ReadErr = -15,
	ST_InvalidPacket = -16,
	ST_ThreadErr = -17,
	ST_PipeErr	= -18,
	ST_PlayErr	= -19,
};

typedef int ST_Error;


// --- 公共配置和数据结构 ---
enum class ProtocolType{ RTSP, PRIVATE, RTMP, STONKAM};
enum class DecoderType { SOFTWARE, HARDWARE};
using Properties = std::unordered_map<std::string, std::any>;

// 解码后的帧数据结构
struct DecodedFrame {
	int width = 0;
	int height = 0;
	long long timestamp_us = 0;
	std::vector<unsigned char> data;
};

/* 视频帧标志 */
#define ST_SDK_VIDEO_FRAME_FLAG	0x00000001		/* 视频帧标志 */
#define ST_SDK_AUDIO_FRAME_FLAG	0x00000002		/* 音频帧标志 */
#define ST_SDK_VIDEO_PRIVATE_FRAME_FLAG	0x00000003		/* 私有帧标志 */
#define ST_SDK_MEDIA_INFO_FLAG	0x00000020		/* 媒体信息标志*/

/* 视频帧类型 */
#define ST_SDK_VIDEO_FRAME_UNKNOWN	0x00	
#define ST_SDK_VIDEO_FRAME_I		0x01		/* I帧 */
#define ST_SDK_VIDEO_FRAME_P		0x02		/* P帧 */
#define ST_SDK_VIDEO_FRAME_B		0x03		/* B֡ */
#define ST_SDK_VIDEO_FRAME_J		0x04		/* JPEG */

using Properties = std::unordered_map<std::string, std::any>;
/* 连接类型 */
typedef enum __ST_RTP_CONNECT_TYPE
{
	ST_RTP_OVER_TCP = 0x01,		/* RTP Over TCP */
	ST_RTP_OVER_UDP	= 0x00		/* RTP Over UDP */
}ST_RTP_CONNECT_TYPE;

//ffmpeg中AVCodecID
enum STCodecID {
	ST_CODEC_ID_NONE = 0x00,
	ST_CODEC_ID_H264 = 0x1B,
	ST_CODEC_ID_HEVC = 0xAD,
#define ST_CODEC_ID_H265 ST_CODEC_ID_HEVC

	/* various PCM "codecs" */
	ST_CODEC_ID_PCM_S16BE = 0x10001,
	/* audio codecs */
	ST_CODEC_ID_AAC = 0x15002,
};

#pragma pack(push, 1)
typedef struct tag_TcpStreamHeader_S
{
	char        sFlag[4];               /* 包头标志#WFC*/
	char        cType;                  /* 包类型:0-视频包,1-音频包 */
	char        cFlag;                  /* 包标志:0-P帧,1-I帧 */
	uint64_t      u64Pts;                 /* 时间戳(us) */
	uint32_t      u32DataSize;            /* 数据包大小 */
} TCP_STREAM_S;
#pragma pack(pop)

/* 媒体信息 */
typedef struct __ST_MEDIA_INFO_T
{
	__ST_MEDIA_INFO_T()
		: u32VideoCodec(ST_CODEC_ID_NONE), u32VideoFps(0)
		, u32AudioCodec(ST_CODEC_ID_NONE), u32AudioSamplerate(0), u32AudioChannel(0), u32AudioBitsPerSample(0)
		, u32VpsLength(0), u32SpsLength(0), u32PpsLength(0), u32SeiLength(0) {}

	ST_UI u32VideoCodec;				/* video codec */
	ST_UI u32VideoFps;				/* video framerate */

	ST_UI u32AudioCodec;				/* audio codec */
	ST_UI u32AudioSamplerate;		/* audio samplerate */
	ST_UI u32AudioChannel;			/* audio channel number */
	ST_UI u32AudioBitsPerSample;		/* audio bit per sample */

	ST_UI u32VpsLength;				/* video vps length */
	ST_UI u32SpsLength;				/* video sps length */
	ST_UI u32PpsLength;				/* video pps length */
	ST_UI u32SeiLength;				/* video sei length */
	ST_U8 u8Vps[255];				/* video vps data */
	ST_U8 u8Sps[255];				/* video sps data */
	ST_U8 u8Pps[128];				/* video pps data */
	ST_U8 u8Sei[128];				/* video sei data */
}ST_MEDIA_INFO;

typedef struct _ParsedPacket{
	char packetType;
	char frameFlag;
	uint64_t timestamp_us;
	std::vector<unsigned char> bodyData;
	uint32_t bodySize;
	int number;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_frameTime;
	_ParsedPacket() :packetType(0), frameFlag(0), timestamp_us(0), bodySize(0),number(0),m_frameTime(std::chrono::high_resolution_clock::now()) {}
}ParsedPacket;

/* 帧 */
typedef struct ST_FRAME
{
	ST_FRAME() : frameBuffer(nullptr), frameLength(0), extraDataBuffer(nullptr), extraDataLength(0) {}
	ST_UChar* frameBuffer;
	ST_UI frameLength;
	ST_UChar* extraDataBuffer;
	ST_UI extraDataLength;
}STFrame;

/* 帧信息 */
typedef struct _ST_FRAME_INFO
{
	_ST_FRAME_INFO()
		: codec(ST_CODEC_ID_NONE)
		, type(ST_SDK_VIDEO_FRAME_UNKNOWN), fps(0), width(0), height(0)
		, sample_rate(0), channels(0), bits_per_sample(0)
		, timestamp_usec(0), timestamp_sec(0) {}
	unsigned int	codec;				/* 编码器ID */

	unsigned int	type;				/* 帧类型 */
	unsigned char	fps;				/* 帧率 */
	unsigned short	width;				/* 宽度 */
	unsigned short  height;				/* 高度 */

	unsigned int	sample_rate;		/* 采样率 */
	unsigned int	channels;			/* 通道数 */
	unsigned int	bits_per_sample;	/* 位深 */

	unsigned int    timestamp_usec;		/* 时间戳,毫秒 */
	unsigned int	timestamp_sec;		/* 时间戳,秒 */

}ST_FRAME_INFO;

typedef struct NaluHeader_S{
	uint8_t nal_unit_type		: 5;
	uint8_t nal_ref_idc 		: 2;
	uint8_t forbidden_zero_bit	: 1;	
}NaluHeader;

typedef struct tag_TcpRequestPacket_S
{
    char        sCmdFlag[4];            /* 请求命令标志: #STA-启动音视频流,#STO-停止音视频流,#ALV-客户端心跳包 */
    char        cType;                  /* 命令附加类型: (#STA)1-仅启动视频流,2-仅启动音频流,3-同时启动音视频流 */
    char        cFlag;                  /* 命令附加标志: (#ALV)包序号(0-255累加，保证前后两次发的序号不一样) */
    uint64_t      u64Pts;                 /* 时间戳(us) */
} TCP_REQ_S;

/*
	_channelid:		通道ID,OpenStream时传入,Android
	_channelPtr:	通道指针,OpenStream时传入,Android/IOS/Windows
	_frameType:		ST_SDK_VIDEO_FRAME_FLAG/ST_SDK_AUDIO_FRAME_FLAG/ST_SDK_MEDIA_INFO_FLAG/...
	_pBuf:			帧数据,Demo中使用
	_frameInfo:		帧信息
*/
typedef int (ST_APICALL* STRTSPSourceCallBack)(int _channelId, void* _channelPtr, int _frameType, const void* _frame, void* _frameInfo);
using FrameCallback = std::function<void(int, int, ST_FRAME*, void*)>;
enum STRTSPException{
	ST_InvalidPayloadType = -1,
};
/*
*	_channelid:		通道ID,OpenStream时传入,Android
*	_channelPtr:	通道指针,OpenStream时传入,Android/IOS/Windows
*	_excption:		异常类型
*	_msg:			异常信息
*/
typedef int (ST_APICALL* STRTSPExceptionCallback)(int _channelId, void* _channelPtr, int _excption, const char* _msg);

#endif  //_ST_SHARE_H