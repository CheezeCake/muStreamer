#pragma once

#include <map>
#include <Ice/Ice.h>
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
		void removeMusicServer(const Player::MusicServerInfo& server);

	private:
		Ice::CommunicatorPtr& ic;
		std::map<std::string, Player::MusicServerInfo> serverList;
};
