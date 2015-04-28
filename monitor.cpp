#include <iostream>
#include "monitor.hpp"

Monitor::Monitor(MetaServer* ms) : metaServer(ms)
{}

Player::MusicServerInfo Monitor::musicServerEndpointStr(const std::string& hostname, const std::string& listeningPort, const std::string& streamingPort, const Ice::Current& c)
{
	std::string hostnameIP(hostname);

	if (hostname.empty()) {
		Ice::IPConnectionInfo* ipConInfo = dynamic_cast<Ice::IPConnectionInfo*>(c.con->getInfo().get());
		hostnameIP = ipConInfo->remoteAddress.substr(ipConInfo->remoteAddress.rfind(':') + 1);
	}

	std::string endpointStr("MusicServer:default -h " + hostnameIP + " -p " + listeningPort);
	Player::MusicServerInfo musicSrv = { endpointStr, hostnameIP,
		static_cast<short>(std::stoul(listeningPort)),
		static_cast<short>(std::stoul(streamingPort)) };

	return musicSrv;
}

void Monitor::newMusicServer(const std::string& hostname, const std::string& listeningPort, const std::string& streamingPort, const Ice::Current& c)
{
	try {
		metaServer->addMusicServer(musicServerEndpointStr(hostname, listeningPort, streamingPort, c));
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Error parsing port numbers of new music server\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Error parsing port numbers of new music server\n";
	}
}

void Monitor::musicServerDown(const std::string& hostname, const std::string& listeningPort, const std::string& streamingPort, const Ice::Current& c)
{
	try {
		metaServer->removeMusicServer(musicServerEndpointStr(hostname, listeningPort, streamingPort, c));
	}
	catch (const std::invalid_argument&) {
		std::cerr << "Error parsing port numbers of new music server\n";
	}
	catch (const std::out_of_range&) {
		std::cerr << "Error parsing port numbers of new music server\n";
	}
}

