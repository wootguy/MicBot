class VoicePacket {
	float time;
	array<uint8> data;
}

array<VoicePacket> g_packet_stream;
float g_clock_adjust = 0; // adjustment made to sync the server clock with the packet times

const string VOICE_FILE = "scripts/plugins/store/_fromvoice.txt";
array<array<uint8>> g_voice_packets;
uint last_file_size = 0;
uint ideal_buffer_size = 12; // amount of packets to delay playback of. Higher = more latency + bad connection tolerance
float sample_load_start = 0;
int g_voice_ent_idx = 0;
float file_check_interval = 0.02f;

// longer than this and python might overwrite what is currently being read
// keep this in sync with server.py
const float MAX_SAMPLE_LOAD_TIME = 0.1f; 

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

HookReturnCode ClientJoin(CBasePlayer@ plr) 
{
	g_voice_ent_idx = getEmptyPlayerSlotIdx();
	return HOOK_CONTINUE;
}

int getEmptyPlayerSlotIdx() {
	for ( int i = 1; i <= g_Engine.maxClients; i++ ) {
		CBasePlayer@ p = g_PlayerFuncs.FindPlayerByIndex(i);
		
		if (p is null or !p.IsConnected()) {
			return i-1;
		}
	}
	
	return 0;
}

void load_samples() {
	File@ file = g_FileSystem.OpenFile(VOICE_FILE, OpenFile::READ);

	if (file !is null && file.IsOpen()) {
		if (file.GetSize() == last_file_size) {
			g_Scheduler.SetTimeout("load_samples", file_check_interval);
			return; // file hasn't been updated yet
		}
	
		sample_load_start = g_EngineFuncs.Time();
		last_file_size = file.GetSize();
		load_packets_from_file(file, false);
	} else {
		g_Log.PrintF("[FakeMic] voice file not found: " + VOICE_FILE + "\n");
	}
}

void handle_micbot_message(string msg) {
	bool chatNotNotify = msg[0] != '~';
	if (!chatNotNotify) {
		msg = msg.SubString(1);
	}
	send_notification("[MicBot] " + msg + "\n", chatNotNotify);
}

void finish_sample_load(File@ file) {
	float loadTime = (g_EngineFuncs.Time() - sample_load_start) + file_check_interval + g_Engine.frametime;
	
	if (loadTime > MAX_SAMPLE_LOAD_TIME) {
		g_PlayerFuncs.ClientPrintAll(HUD_PRINTCONSOLE, "[MicBot] Server can't load samples fast enough (" + loadTime + " / " + MAX_SAMPLE_LOAD_TIME + ")\n");
	}
	
	//println("Loaded samples from file in " + loadTime + " seconds");
	
	file.Close();
	load_samples();
}

void load_packets_from_file(File@ file, bool fastSend) {
	if (file.EOFReached()) {
		finish_sample_load(file);
		return;
	}
	
	string line;
	file.ReadLine(line);
	
	if (line.IsEmpty()) {
		finish_sample_load(file);
		return;
	}
	
	if (line[0] == 'm') {
		// server message, not a voice packet
		handle_micbot_message(line.SubString(1));
	} else {
		uint16 timestamp = (char_to_nibble[ uint(line[0]) ] << 12) + (char_to_nibble[ uint(line[1]) ] << 8) + 
					   (char_to_nibble[ uint(line[2]) ] << 4) + char_to_nibble[ uint(line[3]) ];
		uint len = (line.Length()-4)/2;

		VoicePacket packet;
		packet.time = timestamp / 1000.0f;
		
		for (uint i = 4; i < line.Length()-1; i += 2) {
			uint n1 = char_to_nibble[ uint(line[i]) ];
			uint n2 = char_to_nibble[ uint(line[i + 1]) ];
			packet.data.insertLast((n1 << 4) + n2);
		}
		
		g_packet_stream.insertLast(packet);

		if (line.Length() % 2 == 1) {
			println("ODD LENGTH");
		}
	}
	
	float loadTime = (g_EngineFuncs.Time() - sample_load_start) + file_check_interval + g_Engine.frametime;
	
	if (g_Engine.frametime > 0.03f || loadTime > MAX_SAMPLE_LOAD_TIME*0.5f) {
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

void play_samples(bool buffering) {
	if (g_packet_stream.size() < 2 or (buffering and g_packet_stream.size() < ideal_buffer_size)) {
		if (!buffering) {
			send_debug_message("[MicBot] Buffering voice packets...\n");
		}
		
		send_debug_message("Buffering voice packets " + g_packet_stream.size() + " / " + ideal_buffer_size + "\n");
		g_Scheduler.SetTimeout("play_samples", 0.1f, true);
		return;
	}
	
	VoicePacket packet = g_packet_stream[0];
	g_packet_stream.removeAt(0);
	
	int speaker = getEmptyPlayerSlotIdx();
	
	if (packet.data.size() == 0) {
		g_Scheduler.SetTimeout("play_samples", 0.0f, false);
		return;
	}
	
	bool silentPacket = packet.data.size() <= 4 and packet.data[0] == 0xff;
	
	if (!silentPacket) {
		for ( int i = 1; i <= g_Engine.maxClients; i++ )
		{
			CBasePlayer@ plr = g_PlayerFuncs.FindPlayerByIndex(i);
			
			if (plr is null or !plr.IsConnected()) {
				continue;
			}
			
			PlayerState@ state = getPlayerState(plr);
			if (state.isListening) {
				NetworkMessage m(MSG_ONE_UNRELIABLE, NetworkMessages::NetworkMessageType(53), plr.edict());
					m.WriteByte(g_voice_ent_idx); // entity which is "speaking"
					m.WriteShort(packet.data.size()); // compressed audio length
					for (uint k = 0; k < packet.data.size(); k++) {
						m.WriteByte(packet.data[k]);
					}
				m.End();
			}
		}
	}
	
	VoicePacket nextPacket = g_packet_stream[0];
	
	while (nextPacket.data.size() == 0) {
		// lost packet
		g_packet_stream.removeAt(0);
		println("Lost packet");
		
		if (g_packet_stream.size() < 2) {
			play_samples(true); // time to buffer more packets
			return; 
		}
		
		nextPacket = g_packet_stream[0];
	}
	
	// try to keep buffer near ideal size
	string logSpecial = silentPacket ? "silence " : "";
	if (g_packet_stream.size() > ideal_buffer_size*2) {
		g_clock_adjust += 0.05f;
		logSpecial = "Speedup x5";
	} else if (g_packet_stream.size() > ideal_buffer_size) {
		g_clock_adjust += 0.01f;
		logSpecial = "Speedup";
	} else if (g_packet_stream.size() < 4) {
		g_clock_adjust -= 0.01f;
		logSpecial = "Slowdown";
	}
	
	float serverTime = g_EngineFuncs.Time() + g_clock_adjust; 
	float clockDiff = serverTime - packet.time;				  // time difference between server clock and packet clock
	float delayNeeded = nextPacket.time - serverTime; // time the next packet should be sent
	float delayNext = delayNeeded - g_Engine.frametime; // how much to delay with the scheduler, accounting for server fps
	
	send_debug_message("Sync: " + formatFloat(packet.time, "", 6, 3)
			+ " " + formatFloat(serverTime, "", 6, 3)
			+ " " + formatFloat(clockDiff, "+", 3, 3)
			+ "   Delay: " + formatFloat(delayNeeded, "", 6, 4)
			+ " " + formatFloat(g_Engine.frametime, "", 6, 4)
			+ " " + formatFloat(delayNext, "+", 6, 4)
			+ "   Sz: " + formatInt(packet.data.size(), "", 3)
			+ "   Buff: " + formatInt(g_packet_stream.size(), "", 2) + " / " + ideal_buffer_size +
			"  " + logSpecial + "\n");
	
	if (abs(clockDiff) > 0.5f or delayNext > 0.5f) {
		println("Syncing server time with mic packet time by " + -clockDiff);
		g_clock_adjust = packet.time - g_EngineFuncs.Time();
		if (delayNext > 1.0f)
			delayNext = 0;
	}
	
	if (delayNext < 0) {
		play_samples(false);
	} else {
		g_Scheduler.SetTimeout("play_samples", delayNext, false);
	}	
}