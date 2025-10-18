// #include "MediaPlayer.h"
// #include "IMediaFactory.h"
// #include "MediaPlayerFacade.h"
// #include <stdexcept>
// #include <iostream>

// std::unique_ptr<IPlayer> createPlayer(const Properties& config) {
// 	try {
// 		if (config.find("protocol_type") == config.end() ||
// 			config.find("decoder_type") == config.end() ||
// 			config.find("url") == config.end()) {
// 			std::cerr << "create Player have failure " << std::endl;
// 		}
// 		return std::make_unique<MediaPlayerFacade>(config);
// 	}
// 	catch (const std::exception& e) {
// 		std::cerr << "create Player have failure " <<e.what()<<std::endl;
// 		return nullptr;
// 	}
// }