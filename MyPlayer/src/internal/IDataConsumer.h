#pragma once
#include "Share.h"
class IDataConsumer{
public:
	virtual ~IDataConsumer() = default;
	virtual bool initialize(const Properties& config) = 0;
	virtual bool start() = 0;
	virtual void stop() = 0;
	virtual bool isliving() = 0;
};