#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <any>
#include <vector>
#include"Share.h"

class IPlayer {
public:
    virtual ~IPlayer() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
};

// std::unique_ptr<IPlayer> createPlayer(const Properties& props);