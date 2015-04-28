#pragma once

#include <Ice/Ice.h>
#include "server.h"
#include "metaServer.hpp"

class MusicServerMonitor : public Player::IMusicServerMonitor
{
	public:
		MusicServerMonitor(MetaServer* ms);
		void newMusicServer(const std::string& hostname, const std::string& listeningPort,
				const std::string& streamingPort, const Ice::Current& c) override;
		void musicServerDown(const std::string& hostname, const std::string& listeningPort,
				const std::string& streamingPort, const Ice::Current& c) override;

		static Player::MusicServerInfo musicServerEndpointStr(const std::string& hostname,
				const std::string& listeningPort, const std::string& streamingPort,
				const Ice::Current& c);

	private:
		MetaServer* metaServer;
};
