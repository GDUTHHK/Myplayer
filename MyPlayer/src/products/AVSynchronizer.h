#pragma once
#include <memory>
#include <atomic>
#include <chrono>
#include <cmath>
#include "Share.h"

// 同步相关常量
#define MIN_SYNC_THRESHOLD     0.04    // 最小同步阈值40ms
#define MAX_SYNC_THRESHOLD     0.1     // 最大同步阈值100ms  
#define NOSYNC_THRESHOLD       10.0    // 不同步阈值10秒
#define SYNC_FRAMEDUP_THRESHOLD 0.1    // 帧重复阈值100ms
#define MIN_REFRSH_S           0.01    // 最小刷新间隔10ms

class AVSynchronizer {
public:
    struct AudioClock {
        std::atomic<double> audio_pts{0.0};
        std::atomic<double> audio_clock_time{0.0};
        std::atomic<int> sample_rate{48000};
        std::atomic<int> channels{2};
        std::atomic<bool> should_wait{false};
        
        void update_audio_clock(double pts, double clock_time);
        double get_audio_clock() const;
        void audio_wait_video(double video_pts, bool force_wait);
    };
    
    struct VideoClock {
        std::atomic<double> frame_last_pts{0.0};
        std::atomic<double> frame_last_delay{0.0};
        std::atomic<double> frame_timer{0.0};
        std::atomic<bool> should_drop{false};
        std::atomic<int> drop_frame_count{0};
        
        void video_drop_frame(double ref_clock, bool force_drop);
        bool should_drop_frame() const;
        void reset_drop_state();
    };

public:
    AVSynchronizer();
    ~AVSynchronizer() = default;
    
    // 音频时钟相关
    void updateAudioClock(double audio_pts);
    double getAudioClock() const;
    
    // 帧率限制开关控制
    void setFrameRateLimit(bool enabled, double fps = 25.0);
    bool isFrameRateLimitEnabled() const;

    // 视频同步相关  
    //返回绝对显示时间或差额等待时间
    double calculateVideoDelay(double current_pts);
    bool shouldDropVideoFrame(double video_pts);
    void resetVideoTiming();
    
    // 获取当前时间（秒）
    static double getCurrentTime();
    
private:
    AudioClock m_audioClock;
    VideoClock m_videoClock;
    
    std::chrono::high_resolution_clock::time_point m_startTime;

    // 新增：帧率限制相关
    std::atomic<bool> m_frame_rate_limit_enabled{false};
    std::atomic<double> m_target_fps{25.0};
    std::chrono::high_resolution_clock::time_point m_playback_start_time;
    std::atomic<double> m_first_video_pts{-1.0};
    std::atomic<int> m_frame_count{0};
};