#include "replay"

void print(string text) { g_Game.AlertMessage( at_console, text); }
void println(string text) { print(text + "\n"); }

array<uint32> crc32_table;
float g_packet_delay = 0.02f;


void PluginInit()
{
	g_Module.ScriptInfo.SetAuthor( "w00tguy" );
	g_Module.ScriptInfo.SetContactInfo( "w00tguy123 - forums.svencoop.com" );
	
	crc32_table = crc32_generate_table();
	
	// best guess for delay, but it goes too fast sometimes, or cuts out, or plays static
	replay_voice();
	
	uint16 clock = uint16((g_EngineFuncs.Time()*1000) / 20);
	clockAdjust = clock;
	
	expectedNextPacketTime = g_EngineFuncs.Time();
}

uint16 clockAdjust = 0;
int replayIdx = 0;

class OpusFrame {
	uint16 readSize;
	uint16 sequenceNumber;
	array<uint8> compressedSamples;
}

// https://github.com/s1lentq/revoice/blob/8a547601dbe50c5c3bca8c36febd88f5c0cfb669/revoice/src/SteamP2PCodec.cpp

class SvcVoiceDataPacket {
	uint8 playerIdx = 0;	// player to display the voice icon for
	uint16 payloadLength;	// size of the header below + sound data
	
	// below is some sort of header for speex or the sound engine.
	// The constant values here never or rarely changed during the playback of a song.
	// Changing any of these values causes the stream to be completely silent.
	uint64 steamId = 0x1122334455667788;
	uint8 payloadType = 11;
	uint16 sampleRate = 24000;
	uint8 unknown11 = 6;	// set to 0 for last packet in the stream
	uint16 soundLen;		// equal to payloadLength - 18. Probably means the header data is 18 bytes long.

	// frames of bytes send to opus PLS decoder
	array<OpusFrame> frames; 
}

array<uint32> crc32_generate_table()
{
	array<uint32> table;
	
	uint32 polynomial = 0xEDB88320;
	for (uint32 i = 0; i < 256; i++)
	{
		uint32 c = i;
		for (uint32 j = 0; j < 8; j++)
		{
			if (c & 1 != 0) {
				c = polynomial ^ (c >> 1);
			}
			else {
				c >>= 1;
			}
		}
		table.insertLast(c);
	}
	
	return table;
}

uint32 crc32_update(array<uint32>@ table, uint32 initial, array<uint8>@ buf, uint32 len)
{
	uint32 c = initial ^ 0xFFFFFFFF;
	for (size_t i = 0; i < len; ++i)
	{
		c = table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
	}
	return c ^ 0xFFFFFFFF;
}

float expectedNextPacketTime = 0;
float playbackStartTime = 0;

void replay_voice() {
	if (replayIdx == 0) {
		playbackStartTime = g_EngineFuncs.Time();
	}
	
	array<uint8> packet = replay[replayIdx % replay.size()];
	replayIdx += 1;
	
	// 5dc0 = 24000, 2ee0 = 12000, bb80 = 48000 1f40 = 8000
	if (packet.size() > 10) {
		packet[9] = 0x40;
		packet[10] = 0x1f;
		
		for (int i = 0; i < 2; i++) {
			//packet[i] = 255;
			//packet[i] = 255;
		}
		
		uint32 crc32 = crc32_update(crc32_table, 0, packet, packet.size() - 4);
	
		uint32 crc0 = packet[packet.size() - 1];
		uint32 crc1 = packet[packet.size() - 2];
		uint32 crc2 = packet[packet.size() - 3];
		uint32 crc3 = packet[packet.size() - 4];
		uint32 readCrc = (crc0 << 24) + (crc1 << 16) + (crc2 << 8) + (crc3 << 0);
		
		if (readCrc != crc32) {
			//println("BAD CRC");
		}
		
		packet[packet.size() - 1] = (crc32 >> 24) & 0xff;
		packet[packet.size() - 2] = (crc32 >> 16) & 0xff;
		packet[packet.size() - 3] = (crc32 >> 8) & 0xff;
		packet[packet.size() - 4] = (crc32 >> 0) & 0xff;
	}
	
	// SVC_VOICEDATA
	NetworkMessage m(MSG_BROADCAST, NetworkMessages::NetworkMessageType(53), null);
		m.WriteByte(0); // player index
		m.WriteShort(packet.size()); // compressed audio length
		for (uint i = 0; i < packet.size(); i++) {
			m.WriteByte(packet[i]);
		}
	m.End();
	
	string debug = "";
	
	for (uint i = 0; i < packet.size() and i < 24; i++) {
		//debug += formatInt(packet[i], '0H', 2) + " ";
		debug += formatInt(packet[i], 'l', 3) + " ";
	}
	
	//uint16 test = (packet[13] << 8) + packet[12];
	//println("(" + formatInt(packet.size() - test, 'l', 2) + ") " + debug);
	
	
	
	float serverTime = g_EngineFuncs.Time();
	float errorTime = expectedNextPacketTime - serverTime;
	
	expectedNextPacketTime = playbackStartTime + replayIdx*(g_packet_delay - 0.0001f); // slightly fast to prevent mic getting quiet/choppy
	float nextDelay = (expectedNextPacketTime - serverTime) - g_Engine.frametime;
	
	println("TIME: " + formatFloat(expectedNextPacketTime, "", 6, 4) + ", ERROR: " + formatFloat(errorTime, "", 6, 4) + ", " + debug);
	
	if (nextDelay > 0.5f) {
		nextDelay = 0.5f;
	}
	
	if (nextDelay < 0) {
		println("NOT FAST ENOUGH");
		replayIdx += 1;
		g_Scheduler.SetTimeout("replay_voice", 0.00);
		//replay_voice();
	} else {
		g_Scheduler.SetTimeout("replay_voice", nextDelay);
	}
	
	
}