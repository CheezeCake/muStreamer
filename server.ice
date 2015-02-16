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

		StreamToken setupStreaming(string path, string clientIP, string clientPort);
		void play(StreamToken token);
		void stop(StreamToken token);
	};

	interface IMetaServer
	{
		void add(string endpointStr, Song s);
		void remove(MediaInfo media);
		MediaInfoSeq find(string s);

		StreamToken setupStreaming(MediaInfo media);
		void play(StreamToken token);
		void stop(StreamToken token);
	};
};
