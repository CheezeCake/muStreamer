#include <iostream>
#include <map>
#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include "server.h"

class MetaServer : public Player::IMetaServer
{
	public:
		enum FindBy { Artist, Title, Everything };

		MetaServer(Ice::CommunicatorPtr& iceCom);

		virtual Player::MediaInfoSeq find(const std::string& s, const Ice::Current& c) override;
		Player::MediaInfoSeq find(const std::string& s, const MetaServer::FindBy sb);
		virtual Player::MediaInfoSeq findByArtist(const std::string& s, const Ice::Current& c) override;
		virtual Player::MediaInfoSeq findByTitle(const std::string& s, const Ice::Current& c) override;
		virtual Player::MediaInfoSeq listSongs(const Ice::Current& c) override;
		virtual Player::MusicServerInfoSeq listMusicServers(const Ice::Current& c) override;

		virtual Player::StreamToken setupStreaming(const Player::MediaInfo& media, const Ice::Current& c) override;

		void addMusicServer(const Player::MusicServerInfo& server);

	private:
		Ice::CommunicatorPtr& ic;
		std::map<std::string, Player::MusicServerInfo> serverList;
};

class Monitor : public Player::IMonitor
{
	public:
		Monitor(MetaServer* ms);
		void newMusicServer(const std::string& hostname, const std::string& listeningPort,
				const std::string& streamingPort, const Ice::Current& c) override;

	private:
		MetaServer* metaServer;
};

Monitor::Monitor(MetaServer* ms) : metaServer(ms)
{}

void Monitor::newMusicServer(const std::string& hostname, const std::string& listeningPort, const std::string& streamingPort, const Ice::Current& c)
{
	std::string hostnameIp;
	if (hostname.empty()) {
		Ice::IPConnectionInfo* ipConInfo = dynamic_cast<Ice::IPConnectionInfo*>(c.con->getInfo().get());
		hostnameIp = ipConInfo->remoteAddress.substr(ipConInfo->remoteAddress.rfind(':') + 1);
	}
	else {
		hostnameIp = hostname;
	}

	std::string endpointStr("MusicServer:default -h " + hostnameIp + " -p " + listeningPort);
	try {
		Player::MusicServerInfo musicSrv = { endpointStr, hostnameIp,
			static_cast<short>(std::stoul(listeningPort)),
			static_cast<short>(std::stoul(streamingPort)) };

		metaServer->addMusicServer(musicSrv);
	}
	catch (const std::exception&) {
		std::cerr << "Error parsing port numbers of new music server\n";
	}
}

MetaServer::MetaServer(Ice::CommunicatorPtr& iceCom) : ic(iceCom)
{}

void MetaServer::addMusicServer(const Player::MusicServerInfo& server)
{
	if (serverList.insert(std::make_pair(server.endpointStr, server)).second)
		std::cout << "Adding server : " << server.endpointStr << '\n';
	else
		std::cout << server.endpointStr << " already in serverList\n";
}

Player::MediaInfoSeq MetaServer::find(const std::string& s, const Ice::Current&)
{
	std::cout << "Searching for: " << s << '\n';
	return find(s, FindBy::Everything);
}

Player::MediaInfoSeq MetaServer::findByArtist(const std::string& s, const Ice::Current&)
{
	std::cout << "Searching for artist: " << s << '\n';
	return find(s, FindBy::Artist);
}

Player::MediaInfoSeq MetaServer::findByTitle(const std::string& s, const Ice::Current&)
{
	std::cout << "Searching for title: " << s << '\n';
	return find(s, FindBy::Title);
}

Player::MediaInfoSeq MetaServer::find(const std::string& s, const FindBy fb)
{
	Player::MediaInfoSeq medias;

	for (const auto& it : serverList) {
		Ice::ObjectPrx base = ic->stringToProxy(it.first);
		Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);

		if (srv) {
			Player::SongSeq results;

			if (fb == FindBy::Artist)
				results = srv->findByArtist(s);
			else if (fb == FindBy::Title)
				results = srv->findByTitle(s);
			else
				results = srv->find(s);

			for (const auto& r : results) {
				Player::MediaInfo m;
				m.endpointStr = it.first;
				m.media = r;
				medias.push_back(m);
			}
		}
	}

	return medias;
}

Player::MediaInfoSeq MetaServer::listSongs(const Ice::Current&)
{
	Player::MediaInfoSeq medias;

	for (const auto& it : serverList) {
		Ice::ObjectPrx base = ic->stringToProxy(it.first);
		Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);

		if (srv) {
			Player::SongSeq results = srv->listSongs();

			for (const auto& r : results) {
				Player::MediaInfo m;
				m.endpointStr = it.first;
				m.media = r;
				medias.push_back(m);
			}
		}
	}

	return medias;
}

Player::MusicServerInfoSeq MetaServer::listMusicServers(const Ice::Current&)
{
	Player::MusicServerInfoSeq musicServers;
	musicServers.reserve(serverList.size());

	for (const auto& it : serverList)
		musicServers.push_back(it.second);

	return musicServers;
}

Player::StreamToken MetaServer::setupStreaming(const Player::MediaInfo& media, const Ice::Current& c)
{
	std::cout << "Setup stream on " << media.endpointStr << ", " << media.media.path << '\n';
	Player::StreamToken token;
	Ice::ObjectPrx base = ic->stringToProxy(media.endpointStr);
	Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);

	if (srv) {
		Ice::IPConnectionInfo* ipConInfo = dynamic_cast<Ice::IPConnectionInfo*>(c.con->getInfo().get());
		token = srv->setupStreaming(media.media.path, ipConInfo->remoteAddress, std::to_string(ipConInfo->remotePort));
		token.endpointStr = media.endpointStr;
		try {
			const Player::MusicServerInfo musicSrv = serverList.at(media.endpointStr);
			const std::string urlPort = musicSrv.hostname + ':' + std::to_string(musicSrv.streamingPort);
			token.streamingURL = "http://" + urlPort + '/' + token.streamingURL; // http://hostname:streamingPort
		}
		catch (const std::exception& e) {
			return Player::StreamToken();
		}
	}

	return token;
}

int main(int argc, char **argv)
{
	Ice::CommunicatorPtr ic;
	int status = 0;
	std::string port("10000");
	int opt;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		if (opt == 'p') {
			try {
				std::stoul(optarg);
			}
			catch (const std::exception& e) {
				std::cerr << "Invalid port number: " << optarg << '\n';
				return 1;
			}

			port = optarg;
		}
		else {
			std::cerr << "Usage: " << argv[0] << " [-p listeningPort] [-s streamingPort]\n";
			return 1;
		}
	}

	try {
		ic = Ice::initialize();
		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("MetaServerAdapter", "default -p " + port);
		MetaServer* srv = new MetaServer(ic);
		Ice::ObjectPtr object = srv;
		adapter->add(object, ic->stringToIdentity("MetaServer"));
		adapter->activate();

		Ice::ObjectPrx obj = ic->stringToProxy("IceStorm/TopicManager:tcp -h onche.ovh -p 9999");
		IceStorm::TopicManagerPrx topicManager = IceStorm::TopicManagerPrx::checkedCast(obj);
		adapter = ic->createObjectAdapterWithEndpoints("MonitorAdapter", "tcp");
		Player::IMonitorPtr monitor = new Monitor(srv);
		Ice::ObjectPrx proxy = adapter->addWithUUID(monitor)->ice_oneway();
		adapter->activate();

		IceStorm::TopicPrx topic;
		topic = topicManager->retrieve("NewMusicServer");
		IceStorm::QoS qos;
		qos["reliability"] = "ordered";
		topic->subscribeAndGetPublisher(qos, proxy->ice_twoway());

		ic->waitForShutdown();

		topic->unsubscribe(proxy);
	}
	catch (const Ice::Exception& e) {
		std::cerr << e << std::endl;
		status = 1;
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		status = 1;
	}
	catch (...) {
		status = 1;
	}

	if (ic) {
		try {
			ic->destroy();
		}
		catch (const Ice::Exception& e) {
			std::cerr << e << std::endl;
			status = 1;
		}
	}

	return status;
}
