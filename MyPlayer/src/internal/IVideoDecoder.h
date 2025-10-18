#pragma once
#include "share.h"

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;
    virtual void start() = 0;
    virtual bool initialize(const Properties& config) = 0;
    virtual bool configure(const ST_MEDIA_INFO* info) = 0;
    virtual bool frameQueue(const ST_FRAME*) = 0;
    virtual void setOutputCallback(const std::function<void(std::shared_ptr<DecodedFrame>)>& callback) = 0;
    virtual void release() = 0;
};

