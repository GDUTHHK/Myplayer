#include"products/AudioDecoder.h"
#include "factories/GenericFactory.h"

AudioDecoder::AudioDecoder(const Properties config)
	:m_config(config),
	m_pCodecContext(nullptr),
	m_pCodec(nullptr),
	m_pPacket(nullptr),
	m_pFrame(nullptr),
    m_running(false)

{
	m_pPacket = av_packet_alloc();
	m_pFrame = av_frame_alloc();
}

AudioDecoder::~AudioDecoder() {
	release();
	if (m_pPacket) av_packet_free(&m_pPacket);
	if (m_pFrame) av_frame_free(&m_pFrame);
}

bool AudioDecoder::initialize(const Properties& config) {
	m_config = config;
	return true;
}

void AudioDecoder::start() {
	m_running = true;
	m_thread = std::thread(&AudioDecoder::Loop, this);
}

bool AudioDecoder::configure(const ST_MEDIA_INFO* mediaInfo) {
    if (m_pCodecContext) {
        avcodec_close(m_pCodecContext);
        avcodec_free_context(&m_pCodecContext);
        m_pCodecContext = nullptr;
    }

    if (mediaInfo) {
        // 音频编码选择
        if (mediaInfo->u32AudioCodec == ST_CODEC_ID_AAC) {
            m_pCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
        }
        else if (mediaInfo->u32AudioCodec == ST_CODEC_ID_PCM_S16BE) {
            m_pCodec = avcodec_find_decoder(AV_CODEC_ID_PCM_S16BE);
        }

        if (!m_pCodec) {
            std::cerr << "Audio codec not found!" << std::endl;
            return false;
        }

        m_pCodecContext = avcodec_alloc_context3(m_pCodec);
        m_pCodecContext->sample_rate = mediaInfo->u32AudioSamplerate;
        m_pCodecContext->channels = mediaInfo->u32AudioChannel;
    }
    else {
        m_pCodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
        if (!m_pCodec) {
            std::cerr << "Audio codec not found!" << std::endl;
            return false;
            m_pCodecContext = avcodec_alloc_context3(m_pCodec);
        }
        m_pCodecContext->channels = 2;
        m_pCodecContext->channel_layout = (int)av_get_default_channel_layout(m_pCodecContext->channels);
        m_pCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;      // Ĭ��aac������Ҫplanar��ʽPCM�� �����fdk-aac
        m_pCodecContext->sample_rate = 48000;
        m_pCodecContext->bit_rate = 128*1024;
    }

    if (avcodec_open2(m_pCodecContext, m_pCodec, nullptr) < 0) {
        std::cerr << "Could not open audio codec!" << std::endl;
        return false;
    }
    return true;
}

bool AudioDecoder::frameQueue(std::unique_ptr<ST_FRAME> frame) {
    if (!frame) return false;
    m_frameQueue.push(std::move(frame));
    return true;
}

void AudioDecoder::setOutputCallback(const std::function<void(std::shared_ptr<DecodedFrame>)>& callback) {
    m_outputCallback = callback;
}

void AudioDecoder::Loop() {
    while (m_running) {
        std::unique_ptr<ST_FRAME> frame;
        if (!m_frameQueue.pop(frame)) {
            break;
        }

        if (frame && m_pCodecContext) {
            m_pPacket->data = frame->frameBuffer;
            m_pPacket->size = frame->frameLength;

            int ret = avcodec_send_packet(m_pCodecContext, m_pPacket);
            if (ret < 0) {
                std::cerr << "avcodec_send_packet error: " << ret << std::endl;
                delete[] frame->frameBuffer;
                continue;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(m_pCodecContext, m_pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if (ret < 0) {
                    break;
                }

                if (m_outputCallback) {
                    auto decodedFrame = std::make_shared<DecodedFrame>();
                    decodedFrame->width = m_pFrame->sample_rate;
                    decodedFrame->height = m_pFrame->channels;

                    int samples = m_pFrame->nb_samples;
                    int bytes_per_sample = av_get_bytes_per_sample(m_pCodecContext->sample_fmt);
                    int data_size = samples * m_pFrame->channels * bytes_per_sample;

                    decodedFrame->data.resize(data_size);

                    if (av_sample_fmt_is_planar(m_pCodecContext->sample_fmt)) {
                        for (int i = 0; i < samples; i++) {
                            for (int ch = 0; ch < m_pFrame->channels; ch++) {
                                memcpy(decodedFrame->data.data() + (i * m_pFrame->channels + ch) * bytes_per_sample,
                                    m_pFrame->data[ch] + i * bytes_per_sample, bytes_per_sample);
                            }
                        }
                    }
                    else {
                        memcpy(decodedFrame->data.data(), m_pFrame->data[0], data_size);
                    }
                    m_outputCallback(decodedFrame);
                }
            }

            delete[] frame->frameBuffer;
        }
    }
}

void AudioDecoder::release() {
    if (m_running) {
        m_running = false;
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    if (m_pCodecContext) {
        avcodec_close(m_pCodecContext);
        avcodec_free_context(&m_pCodecContext);
        m_pCodecContext = nullptr;
    }
}

// ע�ᵽ����
namespace {
    using AudioDecoderFactory = GenericFactory<IAudioDecoder, const Properties&>;
    Registrar<AudioDecoder, IAudioDecoder, const Properties&> audioDecoder("SoftAudioDecoder");
}