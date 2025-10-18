#include "AVSynchronizer.h"
#include <iostream>
#include <algorithm>

AVSynchronizer::AVSynchronizer() {
    m_startTime = std::chrono::high_resolution_clock::now();
}

void AVSynchronizer::AudioClock::update_audio_clock(double pts, double clock_time) {
    audio_pts.store(pts);
    audio_clock_time.store(clock_time);
}

//音频走过了多少秒
double AVSynchronizer::AudioClock::get_audio_clock() const {
    double current_time = AVSynchronizer::getCurrentTime();
    double stored_pts = audio_pts.load();
    double stored_clock_time = audio_clock_time.load();
    
    // 根据时间差计算当前音频时钟
    return stored_pts + (current_time - stored_clock_time);
}

//如果视频比音频慢了1秒以上或者需要强制等待，则需要等待视频
void AVSynchronizer::AudioClock::audio_wait_video(double video_pts, bool force_wait) {
    if (force_wait || (video_pts - audio_pts.load()) < -1.0) {
        should_wait.store(true);
        std::cout << "Audio waiting for video, video_pts=" << video_pts 
                  << ", audio_pts=" << audio_pts.load() << std::endl;
    } else {
        should_wait.store(false);
    }
}


void AVSynchronizer::VideoClock::video_drop_frame(double ref_clock, bool force_drop) {
    if (force_drop) {
        should_drop.store(true);
        drop_frame_count.fetch_add(1);
        std::cout << "Force dropping video frame, ref_clock=" << ref_clock 
                  << ", dropped_count=" << drop_frame_count.load() << std::endl;
    }
}

bool AVSynchronizer::VideoClock::should_drop_frame() const {
    return should_drop.load();
}

void AVSynchronizer::VideoClock::reset_drop_state() {
    should_drop.store(false);
}

void AVSynchronizer::updateAudioClock(double audio_pts) {
    double current_time = getCurrentTime();
    m_audioClock.update_audio_clock(audio_pts, current_time);
}

double AVSynchronizer::getAudioClock() const {
    return m_audioClock.get_audio_clock();
}

// double AVSynchronizer::calculateVideoDelay(double current_pts) {
//     // 获取上一帧需要显示的时长delay
//     double last_pts = m_videoClock.frame_last_pts.load();
//     double delay = current_pts - last_pts;  
//     //如果是0或者大于1秒，则使用上一帧的delay
//     if (delay <= 0 || delay >= 1.0) {
//         delay = m_videoClock.frame_last_delay.load();
//     }  
//     // 根据视频PTS和参考时钟调整delay
//     double ref_clock = getAudioClock();
//     double diff = current_pts - ref_clock; // diff < 0: video slow, diff > 0: video fast  
//     // 一帧视频时间或10ms，10ms音视频差距无法察觉
//     double sync_threshold = std::max(MIN_SYNC_THRESHOLD, 
//                                    std::min(MAX_SYNC_THRESHOLD, delay));
//     m_audioClock.audio_wait_video(current_pts, false);
//     m_videoClock.video_drop_frame(ref_clock, false);
//     if (!std::isnan(diff) && std::abs(diff) < NOSYNC_THRESHOLD) { // 不同步
//         if (diff <= -sync_threshold) { // 视频比音频慢，加快
//             delay = std::max(0.0, delay + diff);
//             static int last_delay_zero_counts = 0;      
//             if (m_videoClock.frame_last_delay.load() <= 0) {
//                 last_delay_zero_counts++;
//             } else {
//                 last_delay_zero_counts = 0;
//             }       
//             //差了1秒以上，且上一帧的delay为0，则需要丢帧
//             if (diff < -1.0 && last_delay_zero_counts >= 10) {
//                 std::cout << "Video codec too slow, adjusting video&audio" << std::endl;
//                 m_audioClock.audio_wait_video(current_pts, true); // 差距较大，需要反馈音频等待视频
//                 m_videoClock.video_drop_frame(ref_clock, true);   // 差距较大，需要视频丢帧追上
//             }
//         }
//         // 视频比音频快，减慢
//         else if (diff >= sync_threshold && delay > SYNC_FRAMEDUP_THRESHOLD) {
//             delay = delay + diff; // 音视频差距较大，且一帧的超过帧最长时间，一步到位
//         } else if (diff >= sync_threshold) {
//             delay = 2 * delay; // 音视频差距较小，加倍延迟，逐渐缩小
//         }
//     }  
//     m_videoClock.frame_last_delay.store(delay);
//     m_videoClock.frame_last_pts.store(current_pts); 
//     return delay;
// }

double AVSynchronizer::calculateVideoDelay(double current_pts) {
    // 根据开关选择不同的计算模式
    if (m_frame_rate_limit_enabled.load()) {
        // 模式1：固定帧率限制模式
        return calculateFixedFrameRateDelay(current_pts);
    } else {
        // 模式2：差额同步模式
        return calculateDifferentialDelay(current_pts);
    }
}

double AVSynchronizer::calculateFixedFrameRateDelay(double current_pts) {
    // 初始化基准时间
    if (m_first_video_pts.load() < 0) {
        m_first_video_pts.store(current_pts);
        m_playback_start_time = std::chrono::high_resolution_clock::now();
    }
    
    // 计算固定帧率的显示时间
    int frame_index = m_frame_count.fetch_add(1);
    double frame_interval = 1.0 / m_target_fps.load();
    auto target_time = m_playback_start_time + 
                      std::chrono::duration<double>(frame_index * frame_interval);
    
    // 获取音视频同步差异
    double audio_clock = getAudioClock();
    double diff = current_pts - audio_clock;
    
    // 根据同步情况微调显示时间
    if (!std::isnan(diff) && std::abs(diff) < NOSYNC_THRESHOLD) {
        if (diff > 0.02) {  // 视频超前20ms以上，延迟显示
            target_time += std::chrono::duration<double>(std::min(diff * 0.5, 0.05));
        } else if (diff < -0.02) {  // 视频滞后20ms以上，提前显示
            target_time += std::chrono::duration<double>(std::max(diff * 0.5, -0.05));
        }
    }
    
    // 计算等待时间
    auto current_time = std::chrono::high_resolution_clock::now();
    auto wait_duration = target_time - current_time;
    double wait_time = std::chrono::duration<double>(wait_duration).count();
    
    // 更新状态
    m_videoClock.frame_last_pts.store(current_pts);
    m_videoClock.frame_last_delay.store(std::max(0.0, wait_time));
    
    return std::max(0.0, wait_time);
}

double AVSynchronizer::calculateDifferentialDelay(double current_pts) {
    // 获取音视频同步差异
    double audio_clock = getAudioClock();
    double diff = current_pts - audio_clock; // diff < 0: video slow, diff > 0: video fast
    
    // 计算基础延迟（帧间隔）
    double last_pts = m_videoClock.frame_last_pts.load();
    double base_delay = current_pts - last_pts;
    
    if (base_delay <= 0 || base_delay >= 1.0) {
        base_delay = m_videoClock.frame_last_delay.load();
    }
    
    double final_delay = 0.0;
    
    if (!std::isnan(diff) && std::abs(diff) < NOSYNC_THRESHOLD) {
        // 计算同步阈值
        double sync_threshold = std::max(MIN_SYNC_THRESHOLD, 
                                       std::min(MAX_SYNC_THRESHOLD, base_delay));
        
        if (diff <= -sync_threshold) {
            // 视频滞后:立即显示
            final_delay = 0.0;  // 立即显示
        } else if (diff >= sync_threshold) {
            // 视频超前:等待差额时间
            final_delay = std::min(diff, 0.2);  // 最多等待200ms
        } else {
            // 同步:立即显示
            final_delay = 0.0;
        }
        // 处理严重不同步情况
        static int consecutive_zero_delays = 0;
        if (final_delay <= 0) {
            consecutive_zero_delays++;
        } else {
            consecutive_zero_delays = 0;
        }
        
        if (diff < -1.0 && consecutive_zero_delays >= 10) {
            m_audioClock.audio_wait_video(current_pts, true);
            m_videoClock.video_drop_frame(audio_clock, true);
        }
    } else {

        final_delay = 0.0;
    }
    
    // 更新状态
    m_videoClock.frame_last_delay.store(final_delay);
    m_videoClock.frame_last_pts.store(current_pts);
    
    return final_delay;
}

bool AVSynchronizer::shouldDropVideoFrame(double video_pts) {
    bool should_drop = m_videoClock.should_drop_frame();
    if (should_drop) {
        m_videoClock.reset_drop_state();
    }
    return should_drop;
}

void AVSynchronizer::resetVideoTiming() {
    m_videoClock.frame_timer.store(0.0);
    m_videoClock.frame_last_pts.store(0.0);
    m_videoClock.frame_last_delay.store(0.0);
}

double AVSynchronizer::getCurrentTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

void AVSynchronizer::setFrameRateLimit(bool enabled, double fps) {
    m_frame_rate_limit_enabled.store(enabled);
    m_target_fps.store(fps);
    
    if (enabled) {
        // 重置计时器
        m_playback_start_time = std::chrono::high_resolution_clock::now();
        m_frame_count.store(0);
        m_first_video_pts.store(-1.0);
        std::cout << "Frame rate limit enabled: " << fps << " FPS" << std::endl;
    } else {
        std::cout << "Frame rate limit disabled" << std::endl;
    }
}

bool AVSynchronizer::isFrameRateLimitEnabled() const {
    return m_frame_rate_limit_enabled.load();
}
