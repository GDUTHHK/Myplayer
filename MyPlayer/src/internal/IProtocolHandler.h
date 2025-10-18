#pragma once
#include <string>
#include "share.h"

class IProtocolHandler {
public:
    virtual ~IProtocolHandler() = default;
    virtual bool initialize() = 0;
    virtual int OpenStream() = 0;
    virtual int CloseStream() = 0;
    //virtual int getIdentity() = 0;
    //virtual void SetCallback(STRTSPSourceCallBack cb) = 0;
    virtual void SetCallback(FrameCallback cb) = 0;
    virtual bool updateProperties(Properties* _properties) = 0;
};

