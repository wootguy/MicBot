const string VOICE_FILE = "scripts/plugins/MicBot/voice.dat";
array<array<uint8>> g_voice_packets;
uint last_file_size = 0;

void print(string text) { g_Game.AlertMessage( at_console, text); }
void println(string text) { print(text + "\n"); }

void PluginInit()
{
	g_Module.ScriptInfo.SetAuthor( "w00tguy" );
	g_Module.ScriptInfo.SetContactInfo( "w00tguy123 - forums.svencoop.com" );
	
	//g_Scheduler.SetInterval("load_samples", 0.05, -1);
	load_samples();
}

void load_samples() {
	File@ file = g_FileSystem.OpenFile(VOICE_FILE, OpenFile::READ);

	if (file !is null && file.IsOpen()) {
		if (file.GetSize() == last_file_size) {
			g_Scheduler.SetTimeout("load_samples", 0.02f);
			return; // file hasn't been updated yet
		}
	
		last_file_size = file.GetSize();
	
		g_last_packet_send = g_EngineFuncs.Time();
		load_packets_from_file(file, false);
	} else {
		g_Log.PrintF("[FakeMic] voice file not found: " + VOICE_FILE + "\n");
	}
}

float g_last_packet_send = 0;

// convert lowercase hex letter to integer
array<uint8> char_to_nibble = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 10, 11, 12, 13, 14, 15
};

void load_packets_from_file(File@ file, bool fastSend) {
	if (file.EOFReached()) {
		file.Close();
		load_samples();
		return;
	}
	
	string line;
	file.ReadLine(line);
	
	if (line.IsEmpty()) {
		file.Close();
		load_samples();
		return;
	}
	
	uint len = line.Length()/2;

	NetworkMessage m(MSG_BROADCAST, NetworkMessages::NetworkMessageType(53), null);
		m.WriteByte(200); // player index
		m.WriteShort(len); // compressed audio length
		for (uint i = 0; i < len; i++) {
			uint n1 = char_to_nibble[ uint(line[i*2]) ];
			uint n2 = char_to_nibble[ uint(line[i*2 + 1]) ];
			m.WriteByte((n1 << 4) + n2);
		}
	m.End();
	
	bool lowServerFramerate = fastSend or g_EngineFuncs.Time() - g_last_packet_send > 0.04f; // 50 fps
	g_last_packet_send = g_EngineFuncs.Time();
	
	println("Write " + len + " samples" + (lowServerFramerate ? "!" : ""));
	
	if (lowServerFramerate) {
		// Send the rest of the packets now so the stream doesn't cut out.
		// Sending too fast doesn't affect the stream quality, but too slow causes static.
		// Steam voice packets load in at about 20-30 per second.
		load_packets_from_file(file, true);
	} else {
		// Try not to impact frametimes too much.
		// The game has an annoying stutter when packets are sent all at once.
		g_Scheduler.SetTimeout("load_packets_from_file", 0.0f, @file, false);
	}
}