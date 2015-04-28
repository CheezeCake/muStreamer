#pragma once

#include <map>
#include <Ice/Ice.h>
#include <vlc/libvlc.h>
#include <vlc/vlc.h>
#include "server.h"

class MusicServer : public Player::IMusicServer
{
	public:
		enum FindBy { Artist, Title, Everything };

		MusicServer(Ice::CommunicatorPtr& ic, const std::string& hostname,
				const std::string& lPort, const std::string& sPort,
				Player::IMusicServerMonitorPrx& m);
		~MusicServer();

		virtual void add(const Player::Song& s, const Ice::Current& c) override;
		virtual void remove(const std::string& path, const Ice::Current& c) override;
		virtual Player::SongSeq find(const std::string& s, const Ice::Current& c) override;
		Player::SongSeq find(const std::string& s, const FindBy fb);
		virtual Player::SongSeq findByArtist(const std::string& s, const Ice::Current& c) override;
		virtual Player::SongSeq findByTitle(const std::string& s, const Ice::Current& c) override;
		virtual Player::SongSeq listSongs(const Ice::Current& c) override;

		virtual Player::StreamToken setupStreaming(const std::string& path, const std::string& ip, const std::string& port, const Ice::Current& c) override;
		virtual void play(const Player::StreamToken& token, const Ice::Current& c) override;
		virtual void stop(const Player::StreamToken& token, const Ice::Current& c) override;

		virtual void uploadFile(const std::string& path, int offset, const Player::ByteSeq& data, const Ice::Current& c) override;

	private:
		std::map<std::string, Player::Song> db;
		libvlc_instance_t* vlc;
		Player::IMusicServerMonitorPrx& MSmonitor;
		Player::ISongMonitorPrx Smonitor;
		const std::string hostname;
		const std::string listeningPort;
		const std::string streamingPort;
};
