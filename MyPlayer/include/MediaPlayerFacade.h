#pragma once
#include<thread>
#include<queue>
#include<mutex>
#include "share.h"
#include "MediaPlayer.h"
#include "internal/IMediaFactory.h"
#include "internal/IMediaPipeline.h"
#include "IDataConsumer.h"
class MediaPlayerFacade:public IPlayer{
public:
	explicit MediaPlayerFacade(std::shared_ptr<IMediaFactory> factory, const Properties& config);
	~MediaPlayerFacade() override;
	bool start() override;
	void stop() override;
	bool isRunning() const override;
private:
	std::shared_ptr<EventDispatcher> m_dispatcher;
	std::shared_ptr<IMediaFactory> m_factory;
	std::unique_ptr<IMediaPipeline> m_pipeline;
	std::vector<std::unique_ptr<IDataConsumer>> m_consumers;
};

