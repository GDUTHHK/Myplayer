#include "SoftDecoder.h"
#include "factories/GenericFactory.h"

SoftDecoder::SoftDecoder(const Properties& config) {
    m_pPacket = av_packet_alloc();
    m_pFrame = av_frame_alloc();
}

SoftDecoder::~SoftDecoder() {

}

void SoftDecoder::start() {
    m_running = true;
    m_thread = std::thread(&SoftDecoder::decodeThread, this);
}

bool SoftDecoder::initialize(const Properties& config) {
    return true;
}

bool SoftDecoder::configure(const ST_MEDIA_INFO* mediaInfo){
    if(m_pCodecContext){
        avcodec_close(m_pCodecContext);
        avcodec_free_context(&m_pCodecContext);
        m_pCodecContext = nullptr;
    }
  
    if (mediaInfo) {
        const std::vector<unsigned char> start_code = { 0x00, 0x00, 0x00, 0x01 };
        std::vector<unsigned char> decoder_config_data;
        decoder_config_data.insert(decoder_config_data.end(), start_code.begin(), start_code.end());
        decoder_config_data.insert(decoder_config_data.end(), mediaInfo->u8Sps, mediaInfo->u8Sps + mediaInfo->u32SpsLength);
        decoder_config_data.insert(decoder_config_data.end(), start_code.begin(), start_code.end());
        decoder_config_data.insert(decoder_config_data.end(), mediaInfo->u8Pps, mediaInfo->u8Pps + mediaInfo->u32PpsLength);

        if (mediaInfo->u32VideoCodec == ST_CODEC_ID_H264) {
            m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
        }
        else {
            // m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H265);
        }
        m_pCodecContext = avcodec_alloc_context3(m_pCodec);

        m_pCodecContext->extradata = (uint8_t*)av_malloc(decoder_config_data.size() + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!m_pCodecContext->extradata) {
            std::cout << "m_pCodecContext->extradata have some poblem" << std::endl;
            return false;
        }
        memcpy(m_pCodecContext->extradata, decoder_config_data.data(), decoder_config_data.size());
        m_pCodecContext->extradata_size = decoder_config_data.size();
    }
    else {
        m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
        m_pCodecContext = avcodec_alloc_context3(m_pCodec);
    }

    if (m_pCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
        m_pCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED;

    m_pCodecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_pCodecContext->thread_count = 4;
    m_pCodecContext->thread_type = FF_THREAD_FRAME;
    
    m_pCodecContext->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
    m_pCodecContext->err_recognition = AV_EF_IGNORE_ERR;
    m_pCodecContext->flags2 |= AV_CODEC_FLAG2_SHOW_ALL;


    if (avcodec_open2(m_pCodecContext, m_pCodec, nullptr) < 0) {
        std::cerr << "failed to open codec\n";
        return false;
    }

    if (!avcodec_is_open(m_pCodecContext) || !av_codec_is_decoder(m_pCodecContext->codec))
    {
        std::cout<< "error"<<std::endl;
        return false;
    }
    return true;
}

void SoftDecoder::setOutputCallback(const std::function<void(std::shared_ptr<DecodedFrame>)>& callback) {
    m_outputCallback = callback;
}

bool SoftDecoder::frameQueue(const ST_FRAME* _frame) {
    if (!m_running || !_frame) return false;
    ST_FRAME* frame = new ST_FRAME();
    frame->frameBuffer = new unsigned char[_frame->frameLength];
    frame->frameLength = _frame->frameLength;
    memcpy(frame->frameBuffer, _frame->frameBuffer, _frame->frameLength);
    m_frameQueue.push(frame);
    frame = nullptr;
    return true;
}

void SoftDecoder::decodeThread() {
    while (m_running) {
        ST_FRAME* frame = nullptr;
        if (!m_frameQueue.pop(frame)) {
            break;
        }
        if (frame) {
            m_pPacket->data = frame->frameBuffer;
            m_pPacket->size = frame->frameLength;
    
            int ret = avcodec_send_packet(m_pCodecContext, m_pPacket);
            if(ret<0){
                std::cerr << "avcodec_send_packet error: " << ret << std::endl;
                avcodec_flush_buffers(m_pCodecContext);
                continue;
            }
            while (ret>=0) {
                ret = avcodec_receive_frame(m_pCodecContext, m_pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }else if(ret<0){
                    break;
                }
                if (m_outputCallback) {
                    auto decodedframe = std::make_shared<DecodedFrame>();
                    decodedframe->width = m_pFrame->width;
                    decodedframe->height = m_pFrame->height;

                    int y_size = m_pFrame->width * m_pFrame->height;
                    int uv_size = y_size / 4;

                    decodedframe->data.resize(y_size + uv_size + uv_size);

                    memcpy(decodedframe->data.data(), m_pFrame->data[0], y_size);

                    memcpy(decodedframe->data.data() + y_size, m_pFrame->data[1], uv_size);

                    memcpy(decodedframe->data.data() + y_size + uv_size, m_pFrame->data[2], uv_size);
                    m_outputCallback(decodedframe);
                }
            }
            delete[] frame->frameBuffer;
            delete frame;
        }
    }
}

void SoftDecoder::release(){
    if (m_running) {
        m_frameQueue.shutdown();
        if (m_thread.joinable())
            m_thread.join();
    }
    if (m_pCodecContext) {
        avcodec_close(m_pCodecContext);
        avcodec_free_context(&m_pCodecContext);
        m_pCodecContext = nullptr;
    }
    if (m_pFrame) {
        av_frame_free(&m_pFrame);
        m_pFrame = nullptr;
    }
    if (m_pPacket) {
        av_packet_free(&m_pPacket);
        m_pPacket = nullptr;
    }
    std::cout << "SoftDecoder resources released\n";
}

namespace {
    Registrar<SoftDecoder, IVideoDecoder, const Properties&> registrar("soft_decoder");
}