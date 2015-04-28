#pragma once

#include <Ice/Ice.h>
#include "server.h"
#include "metaServer.hpp"

class Monitor : public Player::IMonitor
{
	public:
		Monitor(MetaServer* ms);
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
