#include <iostream>
#include <map>
#include <Ice/Ice.h>
#include "server.h"

// TODO: remove this global variable
Ice::CommunicatorPtr ic;

class MetaServer : public Player::IMetaServer
{
	public:
		MetaServer();

		virtual void add(const std::string& endpointStr, const Player::Song& s, const Ice::Current& c);
		virtual void remove(const Player::MediaInfo& media, const Ice::Current& c);
		virtual Player::MediaInfoSeq find(const std::string& s, const Ice::Current& c);

		virtual Player::StreamToken setupStreaming(const Player::MediaInfo& media, const Ice::Current& c);
		virtual void play(const Player::StreamToken& token, const Ice::Current& c);
		virtual void stop(const Player::StreamToken& token, const Ice::Current& c);

	private:
		/* std::map<std::string, Player::IMusicServerPrx> serverList; */
		std::vector<std::pair<std::string, std::string>> serverList;
};

MetaServer::MetaServer()
{
	serverList.push_back(std::make_pair("onchevps.ddns.net", "10001"));
}

void MetaServer::add(const std::string& endpointStr, const Player::Song& s, const Ice::Current& c)
{
	std::cout << "Adding to db: " << s.artist << " - " << s.title << " - " << s.path << '\n';
	/* std::string endpointStr = "MusicServer:default -h " + it.first + " -p " + it.second; */
	/* Ice::ObjectPrx base = ic->stringToProxy(endpointStr); */
	Ice::ObjectPrx base = ic->stringToProxy("MusicServer:default -h onchevps.ddns.net -p 10001");
	Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);
	if (srv)
		srv->add(s);
}

void MetaServer::remove(const Player::MediaInfo& media, const Ice::Current& c)
{
	std::cout << "Removing: " << media.media.path << '\n';
	/* Ice::ObjectPrx base = ic->stringToProxy(media.endpointStr); */
	Ice::ObjectPrx base = ic->stringToProxy("MusicServer:default -h onchevps.ddns.net -p 10001");
	Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);
	if (srv)
		srv->remove(media.media.path);
}

Player::MediaInfoSeq MetaServer::find(const std::string& s, const Ice::Current& c)
{
	std::cout << "Searching for: " << s << '\n';
	Player::MediaInfoSeq medias;

	for (const auto& it : serverList) {
		std::string endpointStr = "MusicServer:default -h " + it.first + " -p " + it.second;
		Ice::ObjectPrx base = ic->stringToProxy(endpointStr);
		Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);

		if (srv) {
			auto results = srv->find(s);
			for (const auto& r : results) {
				Player::MediaInfo m;
				m.endpointStr = endpointStr;
				m.media = r;
				medias.push_back(m);
			}
		}
	}
	return medias;
}

Player::StreamToken MetaServer::setupStreaming(const Player::MediaInfo& media, const Ice::Current& c)
{
	Player::StreamToken token;
	Ice::ObjectPrx base = ic->stringToProxy(media.endpointStr);
	Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);

	if (srv) {
		Ice::IPConnectionInfo* ipConInfo = dynamic_cast<Ice::IPConnectionInfo*>(c.con->getInfo().get());
		token = srv->setupStreaming(media.media.path, ipConInfo->remoteAddress, std::to_string(ipConInfo->remotePort));
		token.endpointStr = media.endpointStr;
	}
	return token;
}

void MetaServer::play(const Player::StreamToken& token, const Ice::Current& c)
{
	Ice::ObjectPrx base = ic->stringToProxy(token.endpointStr);
	Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);

	if (srv)
		srv->play(token);
}

void MetaServer::stop(const Player::StreamToken& token, const Ice::Current& c)
{
	Ice::ObjectPrx base = ic->stringToProxy(token.endpointStr);
	Player::IMusicServerPrx srv = Player::IMusicServerPrx::checkedCast(base);

	if (srv)
		srv->stop(token);
}

int main(int argc, char **argv)
{
	int status = 0;

	try {
		ic = Ice::initialize(argc, argv);
		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("MetaServerAdapter", "default -p 10000");
		Ice::ObjectPtr object = new MetaServer;
		adapter->add(object, ic->stringToIdentity("MetaServer"));
		adapter->activate();
		ic->waitForShutdown();
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
