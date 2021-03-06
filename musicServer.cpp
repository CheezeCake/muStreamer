#include <iostream>
#include <cstdio>
#include <ctime>
#include <csignal>
#include <cerrno>
#include <IceStorm/IceStorm.h>
#include "musicServer.hpp"

MusicServer::MusicServer(Ice::CommunicatorPtr& ic, const std::string& hName, const std::string& lPort, const std::string& sPort, Player::IMusicServerMonitorPrx& m) :
	hostname(hName), listeningPort(lPort), streamingPort(sPort), MSmonitor(m)
{
	vlc = libvlc_new(0, nullptr);
	if (!vlc)
		throw std::runtime_error("Could not create libvlc instance");

	Ice::ObjectPrx obj = ic->stringToProxy("IceStorm/TopicManager:tcp -h onche.ovh -p 9999");
	IceStorm::TopicManagerPrx topicManager = IceStorm::TopicManagerPrx::checkedCast(obj);

	IceStorm::TopicPrx SEtopic;
	while (!SEtopic) {
		try {
			SEtopic = topicManager->retrieve("SongEvents");
		}
		catch (const IceStorm::NoSuchTopic&) {
			try {
				SEtopic = topicManager->create("SongEvents");
			}
			catch (const IceStorm::TopicExists&) {
				// Another client created the topic.
			}
		}
	}

	Ice::ObjectPrx pub = SEtopic->getPublisher()->ice_oneway();
	Smonitor = Player::ISongMonitorPrx::uncheckedCast(pub);

	MSmonitor->newMusicServer(hName, lPort, sPort);
}

MusicServer::~MusicServer()
{
	libvlc_vlm_release(vlc);
	MSmonitor->musicServerDown(hostname, listeningPort, streamingPort);
}

void MusicServer::add(const Player::Song& s, const Ice::Current&)
{
	std::cout << "Adding to db: " << s.artist << " - " << s.title << " - " << s.path << '\n';
	if (!db.emplace(s.path, s).second)
		throw Player::Error("Another song has the same path");

	Smonitor->newSong(s);
}

void MusicServer::remove(const std::string& path, const Ice::Current&)
{
	std::cout << "Removing: " << path << '\n';
	if (db.erase(path) == 0)
		throw Player::Error("Cannot remove song. Not present in the database : " + path);

	if (::remove(path.c_str()) == -1)
		perror(std::string("Cannot remove ").append(path).c_str());
}

Player::SongSeq MusicServer::find(const std::string& s, const Ice::Current&)
{
	std::cout << "Searching for: " << s << '\n';
	return find(s, FindBy::Everything);
}

Player::SongSeq MusicServer::findByArtist(const std::string& s, const Ice::Current&)
{
	std::cout << "Searching for artist: " << s << '\n';
	return find(s, FindBy::Artist);
}

Player::SongSeq MusicServer::findByTitle(const std::string& s, const Ice::Current&)
{
	std::cout << "Searching for title: " << s << '\n';
	return find(s, FindBy::Title);
}

Player::SongSeq MusicServer::find(const std::string& s, const FindBy fb)
{
	Player::SongSeq songs;
	std::string str(s);
	std::transform(str.begin(), str.end(), str.begin(), tolower);

	for (const auto& it : db) {
		std::string artist(it.second.artist);
		std::string title(it.second.title);
		std::transform(artist.begin(), artist.end(), artist.begin(), tolower);
		std::transform(title.begin(), title.end(), title.begin(), tolower);

		if ((fb == FindBy::Everything && (artist.find(str) != std::string::npos || title.find(str) != std::string::npos))
				|| (fb == FindBy::Artist && artist.find(str) != std::string::npos)
				|| (fb == FindBy::Title && title.find(str) != std::string::npos))
			songs.push_back(it.second);
	}

	return songs;
}

Player::SongSeq MusicServer::listSongs(const Ice::Current&)
{
	Player::SongSeq songs;
	songs.reserve(db.size());

	for (const auto& it : db)
		songs.push_back(it.second);

	return songs;
}

Player::StreamToken MusicServer::setupStreaming(const std::string& path, const std::string& clientIP, const std::string& clientPort, const Ice::Current&)
{
	std::cout << "Setup stream " << path << '\n';
	Player::StreamToken token;
	std::string::size_type pos = path.find_last_of("/");
	std::string file(path, (pos == std::string::npos) ? 0 : pos);
	std::string cleanClientIP = clientIP.substr(clientIP.rfind(':') + 1);
	std::string mediaName = cleanClientIP + '_' + clientPort + '_' + std::to_string(time(nullptr));

	std::string sout = "#transcode{acodec=mp3,ab=128,channels=2,"
		"samplerate=44100}:http{dst=:"+ streamingPort + "/" + mediaName + ".mp3}";

	if (libvlc_vlm_add_broadcast(vlc, mediaName.c_str(), path.c_str(), sout.c_str(), 0, nullptr, true, false) == 0) {
		token.libvlcMediaName = mediaName;
		token.streamingURL = mediaName + ".mp3";
	}

	return token;
}

void MusicServer::play(const Player::StreamToken& token, const Ice::Current&)
{
	std::cout << "Play " << token.streamingURL << '\n';
	libvlc_vlm_play_media(vlc, token.libvlcMediaName.c_str());
}

void MusicServer::stop(const Player::StreamToken& token, const Ice::Current&)
{
	std::cout << "Stop " << token.streamingURL << '\n';
	libvlc_vlm_stop_media(vlc, token.libvlcMediaName.c_str());
}

void MusicServer::uploadFile(const std::string& path, int offset, const Player::ByteSeq& data, const Ice::Current&)
{
	std::cout << "upload " << path << ", offset " << offset;
	FILE* file = fopen(path.c_str(), "a+");
	if (!file)
		throw Player::Error("Cannot open file : " + path);

	fseek(file, 0, SEEK_END);
	long size = ftell(file);

	std::cout << ", file size " << size << ", data size = " << data.size() << '\n';

	if (offset < size)
		throw new Player::Error("Write offset too small : " + path);
	if (offset > size)
		throw new Player::Error("Write offset too big : " + path);

	fwrite(data.data(), sizeof(Player::ByteSeq::value_type), data.size(), file);
	fclose(file);
}

void setPort(std::string& port, const char* arg)
{
	try {
		std::stoul(arg);
	}
	catch (const std::exception& e) {
		std::cerr << "Invalid port number: " << arg << '\n';
		exit(1);
	}

	port = arg;
}

Player::IMusicServerMonitorPrx MSmonitor;
std::string port("10001");
std::string streamPort("8090");
std::string hostname;

void handler(int)
{
	MSmonitor->musicServerDown(hostname, port, streamPort);
	exit(0);
}

int main(int argc, char **argv)
{
	int status = 0;
	Ice::CommunicatorPtr ic;
	int opt;

	signal(SIGINT, handler);

	while ((opt = getopt(argc, argv, "p:s:h:")) != -1) {
		switch (opt) {
			case 'p':
				setPort(port, optarg);
				break;
			case 's':
				setPort(streamPort, optarg);
				break;
			case 'h':
				hostname = optarg;
				break;
			default:
				std::cerr << "Usage: " << argv[0] << " [-p listeningPort] [-s streamingPort] [-h hostname]\n";
				return 1;
		}
	}

	try {
		ic = Ice::initialize();

		Ice::ObjectPrx obj = ic->stringToProxy("IceStorm/TopicManager:tcp -h onche.ovh -p 9999");
		IceStorm::TopicManagerPrx topicManager = IceStorm::TopicManagerPrx::checkedCast(obj);

		IceStorm::TopicPrx MSEtopic;
		while (!MSEtopic) {
			try {
				MSEtopic = topicManager->retrieve("MusicServerEvents");
			}
			catch (const IceStorm::NoSuchTopic&) {
				try {
					MSEtopic = topicManager->create("MusicServerEvents");
				}
				catch (const IceStorm::TopicExists&) {
					// Another client created the topic.
				}
			}
		}

		Ice::ObjectPrx pub = MSEtopic->getPublisher()->ice_oneway();
		MSmonitor = Player::IMusicServerMonitorPrx::uncheckedCast(pub);

		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("MusicServerAdapter", "default -p " + port);
		Ice::ObjectPtr object = new MusicServer(ic, hostname, port, streamPort, MSmonitor);
		adapter->add(object, ic->stringToIdentity("MusicServer"));
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
