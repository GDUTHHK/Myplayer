#pragma once
#include "share.h"

class IAudioDecoder {
public:
    virtual ~IAudioDecoder() = default;
    virtual void start() = 0;
    virtual bool initialize(const Properties& config) = 0;
    virtual bool configure(const ST_MEDIA_INFO* info) = 0;
    virtual bool frameQueue(std::unique_ptr<ST_FRAME> ST_FRAME) = 0;
    virtual void setOutputCallback(const std::function<void(std::shared_ptr<DecodedFrame>)>& callback) = 0;
    virtual void release() = 0;
};

 