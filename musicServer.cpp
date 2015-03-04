#include <iostream>
#include <regex>
#include <ctime>
#include <Ice/Ice.h>
#include <vlc/libvlc.h>
#include <vlc/vlc.h>
#include "server.h"

class MusicServer : public Player::IMusicServer
{
	public:
		MusicServer();
		~MusicServer();

		virtual void add(const Player::Song& s, const Ice::Current& c);
		virtual void remove(const std::string& path, const Ice::Current& c);
		virtual Player::SongSeq find(const std::string& s, const Ice::Current& c);

		virtual Player::StreamToken setupStreaming(const std::string& path, const std::string& ip, const std::string& port, const Ice::Current& c);
		virtual void play(const Player::StreamToken& token, const Ice::Current& c);
		virtual void stop(const Player::StreamToken& token, const Ice::Current& c);

	private:
		std::map<std::string, Player::Song> db;
		libvlc_instance_t* vlc;

};


MusicServer::MusicServer()
{
	vlc = libvlc_new(0, nullptr);
	if (!vlc)
		throw std::runtime_error("Could not create libvlc instance");
}

MusicServer::~MusicServer()
{
	libvlc_vlm_release(vlc);
}

void MusicServer::add(const Player::Song& s, const Ice::Current& c)
{
	std::cout << "Adding to db: " << s.artist << " - " << s.title << " - " << s.path << '\n';
	db.emplace(s.path, s);
}

void MusicServer::remove(const std::string& path, const Ice::Current& c)
{
	std::cout << "Removing: " << path << '\n';
	db.erase(path);
}

Player::SongSeq MusicServer::find(const std::string& s, const Ice::Current& c)
{
	std::cout << "Searching for: " << s << '\n';
	Player::SongSeq songs;
	std::string str(s);
	std::transform(str.begin(), str.end(), str.begin(), tolower);

	for (const auto& it : db) {
		std::string artist(it.second.artist);
		std::string title(it.second.title);
		std::transform(artist.begin(), artist.end(), artist.begin(), tolower);
		std::transform(title.begin(), title.end(), title.begin(), tolower);

		if (str.find(artist) != std::string::npos || str.find(title) != std::string::npos)
			songs.push_back(it.second);
	}

	return songs;
}

Player::StreamToken MusicServer::setupStreaming(const std::string& path, const std::string& clientIP, const std::string& clientPort, const Ice::Current& c)
{
	std::cout << "Setup stream " << path << '\n';
	Player::StreamToken token;
	std::string::size_type pos = path.find_last_of("/");
	std::string file(path, (pos == std::string::npos) ? 0 : pos);
	std::string cleanClientIP = std::regex_replace(clientIP, std::regex(":"), "");
	std::string mediaName = cleanClientIP + '_' + clientPort + '_' + std::to_string(time(nullptr)) + file;

	std::string sout = "#transcode{acodec=mp3,ab=128,channels=2,"
		"samplerate=44100}:http{dst=:8090/" + mediaName + ".mp3}";

	if (libvlc_vlm_add_broadcast(vlc, mediaName.c_str(), path.c_str(), sout.c_str(), 0, nullptr, true, false) == 0) {
		token.libvlcMediaName = mediaName;
		token.streamingURL = mediaName + ".mp3";
	}

	return token;
}

void MusicServer::play(const Player::StreamToken& token, const Ice::Current& c)
{
	std::cout << "Play " << token.streamingURL << '\n';
	libvlc_vlm_play_media(vlc, token.libvlcMediaName.c_str());
}

void MusicServer::stop(const Player::StreamToken& token, const Ice::Current& c)
{
	std::cout << "Stop " << token.streamingURL << '\n';
	libvlc_vlm_stop_media(vlc, token.libvlcMediaName.c_str());
}

int main(int argc, char **argv)
{
	int status = 0;
	Ice::CommunicatorPtr ic;

	try {
		ic = Ice::initialize(argc, argv);
		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints("MusicServerAdapter", "default -p 10001");
		Ice::ObjectPtr object = new MusicServer;
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
