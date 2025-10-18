#pragma once

#include <functional>
#include "Share.h"

class IMediaPipeline {
public:
	virtual ~IMediaPipeline() = default;
	//virtual void setOutputCallback(std::function<void(DecodedFrame*)>) = 0;
	virtual bool start() = 0;
	virtual void stop() = 0;
	virtual bool isRunning() const = 0;
};