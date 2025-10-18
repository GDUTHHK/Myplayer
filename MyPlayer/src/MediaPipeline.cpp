#include "MediaPipeline.h"
#include<iostream>
#include<fstream>
#include "products/AVSynchronizer.h" 

MediaPipelineImpl::MediaPipelineImpl(std::unique_ptr<IProtocolHandler> client,
std::unique_ptr<IVideoDecoder> Vdecoder,
std::unique_ptr<IAudioDecoder> Adecoder,
std::shared_ptr<EventDispatcher> dispatcher,
const Properties& config)
: m_client(std::move(client)),
m_Adecoder(std::move(Adecoder)),
m_Vdecoder(std::move(Vdecoder)),
m_dispatcher(dispatcher),
m_Pconfig(config),
m_synchronizer(std::make_unique<AVSynchronizer>()),
m_display_start_time(std::chrono::high_resolution_clock::now()){
	if (m_client) {
		m_client->SetCallback([this](int channelId, int frameType, ST_FRAME* _frame, void* mediaInfo){
			this->processData(channelId, frameType, _frame, mediaInfo);
			});
	}

	if (m_Vdecoder) {
		m_Vdecoder->setOutputCallback([this](std::shared_ptr<DecodedFrame> frame) ->int {
			if (m_isRunning) {
				return this->handleVideoFrame(frame);
			}
			return 0;
			});
	}

	if (m_Adecoder) {
		m_Adecoder->setOutputCallback([this](std::shared_ptr<DecodedFrame> frame) {
			if (m_isRunning) {
				this->handleAudioFrame(frame);
			}
			});
	}
}

MediaPipelineImpl::~MediaPipelineImpl() {
	if (m_isRunning) {
		stop();
	}
}

void MediaPipelineImpl::setFrameRateLimit(bool enabled, double fps) {
    if (m_synchronizer) {
        m_synchronizer->setFrameRateLimit(enabled, fps);
    }
}

bool MediaPipelineImpl::isFrameRateLimitEnabled() const {
    if (m_synchronizer) {
        return m_synchronizer->isFrameRateLimitEnabled();
    }
    return false;
}

void MediaPipelineImpl::loadConfig(const std::string& configPath) {
	try {
		std::ifstream file(configPath);
		if (file.is_open()) {
			file >> m_Jconfig;
			
			// 解析播放模式
			if (m_Jconfig.contains("playback_config")) {
				auto& playback = m_Jconfig["playback_config"];
				
				if (playback.contains("mode")) {
					std::string mode = playback["mode"];
					if (mode == "audio_only") {
						setPlaybackMode(PlaybackMode::AUDIO_ONLY);
					} else if (mode == "video_only") {
						setPlaybackMode(PlaybackMode::VIDEO_ONLY);
					} else if (mode == "audio_video") {
						setPlaybackMode(PlaybackMode::AUDIO_VIDEO);
					}
				}
				
				// 同步设置
				if (playback.contains("sync_enabled")) {
					m_syncEnabled = playback["sync_enabled"];
				}

				// 帧率限制配置
				if (playback.contains("frame_rate_limit")) {
					auto& frl_config = playback["frame_rate_limit"];
					bool enabled = frl_config.value("enabled", false);
					double fps = frl_config.value("fps", 25.0);
					setFrameRateLimit(enabled, fps);
				}
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Config loading error: " << e.what() << std::endl;
		// 使用默认配置
		m_playbackMode = PlaybackMode::AUDIO_VIDEO;
		m_syncEnabled = true;
		setFrameRateLimit(false, 25.0);
	}
}

void MediaPipelineImpl::setPlaybackMode(PlaybackMode mode) {
	m_playbackMode = mode;
	
	switch (mode) {
		case PlaybackMode::AUDIO_ONLY:
			m_videoActive = false;
			m_audioActive = true;
			break;
		case PlaybackMode::VIDEO_ONLY:
			m_videoActive = true;
			m_audioActive = false;
			break;
		case PlaybackMode::AUDIO_VIDEO:
			m_videoActive = true;
			m_audioActive = true;
			break;
		default:
			break;
	}
}

bool MediaPipelineImpl::start() {
	if (!m_client->initialize()){
		return false;
	}
	m_isRunning = true;
	if (m_client->OpenStream() != ST_NoErr) return false;

	if (m_synchronizer) {
		m_synchronizer->resetVideoTiming();
	}

	if(m_Vdecoder&& m_videoActive) {
		m_Vdecoder->initialize(m_Pconfig);
		m_Vdecoder->start();
	}

	if(m_Adecoder && m_audioActive){
		m_Adecoder->initialize(m_Pconfig);
		m_Adecoder->start();
	}

	return true;
}

void MediaPipelineImpl::stop(){
	if (!m_isRunning)
		return;
	m_isRunning = false;
	m_client->CloseStream();
	if (m_Vdecoder)	m_Vdecoder->release();
	if (m_Adecoder) m_Adecoder->release();
}

bool MediaPipelineImpl::isRunning() const{
	return m_isRunning;
}

void MediaPipelineImpl::processData(int _channelId, int _frameType, ST_FRAME* _frame, void* _frameInfo) {
	if (!m_isRunning)
		return;
	if (_frameType == ST_SDK_MEDIA_INFO_FLAG) {
		ST_MEDIA_INFO* info = (ST_MEDIA_INFO*)_frameInfo;

		if (info->u32VideoCodec != ST_CODEC_ID_NONE) {
			m_hasVideoStream = true;
			if (m_Vdecoder && m_videoActive) {
				m_Vdecoder->configure(info);
			}
		}
		
		if (info->u32AudioCodec != ST_CODEC_ID_NONE) {
			m_hasAudioStream = true;
			if (m_Adecoder && m_audioActive) {
				m_Adecoder->configure(info);
			}
		}

		if (m_playbackMode == PlaybackMode::UNKNOWN) {
			if (m_hasVideoStream && m_hasAudioStream) {
				setPlaybackMode(PlaybackMode::AUDIO_VIDEO);
			} else if (m_hasVideoStream) {
				setPlaybackMode(PlaybackMode::VIDEO_ONLY);
			} else if (m_hasAudioStream) {
				setPlaybackMode(PlaybackMode::AUDIO_ONLY);
			}else{
				setPlaybackMode(PlaybackMode::VIDEO_ONLY);
			}
		}
	}
	else if (_frameType == ST_SDK_VIDEO_FRAME_FLAG) {
		ST_FRAME* frame = (ST_FRAME*)_frame;
		if (m_Vdecoder) {
			m_Vdecoder->frameQueue(frame);
		}
	}
	else if (_frameType == ST_SDK_AUDIO_FRAME_FLAG) {
		auto frame = std::make_unique<ST_FRAME>();
		frame->frameLength = _frame->frameLength;
		frame->frameBuffer = new unsigned char[frame->frameLength];
		if (m_Adecoder) {
			m_Adecoder->frameQueue(std::move(frame));
		}
	}
	else if (_frameType == ST_SDK_VIDEO_PRIVATE_FRAME_FLAG) {
		ST_FRAME* frame = (ST_FRAME*)_frame;
		if (m_Vdecoder) {
			m_Vdecoder->frameQueue(frame);
		}
	}
}

int MediaPipelineImpl::handleVideoFrame(std::shared_ptr<DecodedFrame> frame) {
	if (!frame || !m_videoActive) {
		return 0;
	}
	
	// 检查是否应该跳过该帧
	if (shouldSkipFrame(frame)) {
		return 0;
	}
	
	// 同步处理
	if (m_syncEnabled && m_synchronizer) {
		syncAndOutput(frame);
	} else {
		// 直接输出，不进行同步
		if (m_dispatcher) {
			m_dispatcher->publish("DecodedFrame", frame);
		}
	}
	
	return 1;
}

void MediaPipelineImpl::handleAudioFrame(std::shared_ptr<DecodedFrame> frame) {
	if (!frame || !m_audioActive) {
		return;
	}
	
	// 更新音频时钟
	if (m_syncEnabled && m_synchronizer) {
		double audio_pts = frame->timestamp_us / 1000000.0; // 转换为秒
		m_synchronizer->updateAudioClock(audio_pts);
	}
	
	// 发布音频帧
	if (m_dispatcher) {
		m_dispatcher->publish("DecodedAudioFrame", frame);
	}
}

void MediaPipelineImpl::syncAndOutput(std::shared_ptr<DecodedFrame> frame) {
	if (!m_synchronizer) {
		return;
	}
	
	double frame_pts = frame->timestamp_us / 1000000.0; // 转换为秒
	
	// 如果是仅视频模式或音视频模式
	if (m_playbackMode == PlaybackMode::VIDEO_ONLY || 
		(m_playbackMode == PlaybackMode::AUDIO_VIDEO && m_hasAudioStream)) {
		
		// 检查是否需要丢帧
		if (m_synchronizer->shouldDropVideoFrame(frame_pts)) {
			std::cout << "Dropping video frame for sync, pts=" << frame_pts << std::endl;
			return;
		}
		
		// 计算延迟
		double delay = m_synchronizer->calculateVideoDelay(frame_pts);
		
		// 延迟显示
		if (delay > 0 && delay < 1.0) { // 最大延迟1秒
			std::this_thread::sleep_for(std::chrono::duration<double>(delay));
		}
	}
	
	// 输出帧
	if (m_dispatcher) {
		m_dispatcher->publish("DecodedVideoFrame", frame);
	}
}

bool MediaPipelineImpl::shouldSkipFrame(std::shared_ptr<DecodedFrame> frame) const {
	// 根据播放模式决定是否跳过
	switch (m_playbackMode) {
		case PlaybackMode::AUDIO_ONLY:
			return true; // 音频模式跳过视频帧
		case PlaybackMode::VIDEO_ONLY:
		case PlaybackMode::AUDIO_VIDEO:
			return false;
		default:
			return false;
	}
}