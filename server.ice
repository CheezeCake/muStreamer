module Player
{
	exception Error
	{
		string what;
	};

	struct Song
	{
		string artist;
		string title;
		string path;
	};

	struct MediaInfo
	{
		string endpointStr;
		Song media;
	};

	struct MusicServerInfo
	{
		string endpointStr;
		string hostname;
		short listeningPort;
		short streamingPort;
	};

	sequence<byte> ByteSeq;
	sequence<MediaInfo> MediaInfoSeq;
	sequence<Song> SongSeq;
	sequence<MusicServerInfo> MusicServerInfoSeq;

	struct StreamToken
	{
		string endpointStr;
		string libvlcMediaName;
		string streamingURL;
	};

	interface IMusicServer
	{
		void add(Song s) throws Error;
		void remove(string path) throws Error;
		SongSeq find(string s);
		SongSeq findByArtist(string s);
		SongSeq findByTitle(string s);
		SongSeq listSongs();

		StreamToken setupStreaming(string path, string clientIP, string clientPort);
		void play(StreamToken token);
		void stop(StreamToken token);

		void uploadFile(string path, int offset, ByteSeq data) throws Error;
	};

	interface IMetaServer
	{
		MediaInfoSeq find(string s);
		MediaInfoSeq findByArtist(string s);
		MediaInfoSeq findByTitle(string s);
		MediaInfoSeq listSongs();
		MusicServerInfoSeq listMusicServers();

		StreamToken setupStreaming(MediaInfo media);
	};

	interface IMonitor
	{
		void newMusicServer(string hostname, string listeningPort, string streamingPort);
	};
};
