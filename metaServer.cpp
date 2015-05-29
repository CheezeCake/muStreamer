#include <iostream>
#include <IceStorm/IceStorm.h>
#include "metaServer.hpp"
#include "musicServerMonitor.hpp"

MetaServer::MetaServer(Ice::CommunicatorPtr& iceCom) : ic(iceCom)
{}

void MetaServer::addMusicServer(const Player::MusicServerInfo& server)
{
	if (serverList.insert(std::make_pair(server.endpointStr, server)).second)
		std::cout << "Adding server : " << server.endpointStr << '\n';
	else
		std::cout << server.endpointStr << " already in serverList\n";
}

void MetaServer::removeMusicServer(const Player::MusicServerInfo& server)
{
	if (serverList.erase(server.endpointStr) == 1)
		std::cout << server.endpointStr << " removed from serverList\n";
	else
		std::cerr << "Cannot remove " << server.endpointStr << " : not in serverList\n";
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
			std::cerr << "Usage: " << argv[0] << " [-p listeningPort]\n";
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
		adapter = ic->createObjectAdapterWithEndpoints("MusicServerMonitorAdapter", "tcp");
		Player::IMusicServerMonitorPtr monitor = new MusicServerMonitor(srv);
		Ice::ObjectPrx proxy = adapter->addWithUUID(monitor)->ice_oneway();
		adapter->activate();

		IceStorm::TopicPrx topic;
		topic = topicManager->retrieve("MusicServerEvents");
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
