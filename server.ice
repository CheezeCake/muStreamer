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

	sequence<MediaInfo> MediaInfoSeq;
	sequence<Song> SongSeq;
	dictionary<string, string> stringMap;

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
	};

	interface IMetaServer
	{
		MediaInfoSeq find(string s);
		MediaInfoSeq findByArtist(string s);
		MediaInfoSeq findByTitle(string s);
		MediaInfoSeq listSongs();
		stringMap listMusicServers();

		StreamToken setupStreaming(MediaInfo media);
		void play(StreamToken token);
		void stop(StreamToken token);
	};
};
