#include"internal/IMediaPipeline.h"
#include"internal/IProtocolHandler.h"
#include "internal/IVideoDecoder.h"
#include "internal/IAudioDecoder.h"
#include "internal/EventDispatcher.h"
#include "nlohmann/json.hpp"
#include "internal/Share.h"

enum class PlaybackMode {
    UNKNOWN,        // 未知模式
    AUDIO_ONLY,     // 纯音频
    VIDEO_ONLY,     // 纯视频
    AUDIO_VIDEO     // 音视频
};

class MediaPipelineImpl :public IMediaPipeline {
public:
	MediaPipelineImpl(std::unique_ptr<IProtocolHandler> client,
						std::unique_ptr<IVideoDecoder> Vdecoder,
						std::unique_ptr<IAudioDecoder> Adecoder,
						std::shared_ptr<EventDispatcher> dispatcher,
						const Properties& config);
	~MediaPipelineImpl()override;

	bool start()override;
	void stop()override;
	bool isRunning() const override;

	PlaybackMode getPlaybackMode() const { return m_playbackMode.load(); }
	// 配置相关
	void loadConfig(const std::string& configPath);
	void setPlaybackMode(PlaybackMode mode);

	//帧率限制控制
	void setFrameRateLimit(bool enabled, double fps = 25.0);
    bool isFrameRateLimitEnabled() const;
private:
	void processData(int _channelId, int _frameType, ST_FRAME* _frame, void* _frameInfo);

	// 解码后处理方法
	int handleVideoFrame(std::shared_ptr<DecodedFrame> frame);
	void handleAudioFrame(std::shared_ptr<DecodedFrame> frame);
	
	// 同步相关
	void syncAndOutput(std::shared_ptr<DecodedFrame> frame);
	bool shouldSkipFrame(std::shared_ptr<DecodedFrame> frame) const;
private:
	Properties m_Pconfig;
	std::unique_ptr<IProtocolHandler> m_client;
	std::unique_ptr<IVideoDecoder> m_Vdecoder;
	std::unique_ptr<IAudioDecoder> m_Adecoder;
	std::shared_ptr<EventDispatcher> m_dispatcher;
	std::function<void(DecodedFrame*)> m_outputCallback;
	std::atomic<bool> m_isRunning{ false };

	// 播放模式管理
	std::atomic<PlaybackMode> m_playbackMode{ PlaybackMode::UNKNOWN };
	std::atomic<bool> m_hasVideoStream{ false };
	std::atomic<bool> m_hasAudioStream{ false };
	std::atomic<bool> m_videoActive{ false };
	std::atomic<bool> m_audioActive{ false };

	// 同步管理
	std::unique_ptr<AVSynchronizer> m_synchronizer;
	std::atomic<bool> m_syncEnabled{ true };

	// 25Hz显示控制 - 新增
	std::chrono::high_resolution_clock::time_point m_display_start_time;
	std::atomic<int> m_display_frame_count{0};
	static constexpr double DISPLAY_FPS = 25.0;
	static constexpr double DISPLAY_INTERVAL = 1.0 / DISPLAY_FPS; // 0.04秒

	// 配置参数
	nlohmann::json m_Jconfig;
};