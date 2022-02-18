#include "replay"

void print(string text) { g_Game.AlertMessage( at_console, text); }
void println(string text) { print(text + "\n"); }

void PluginInit()
{
	g_Module.ScriptInfo.SetAuthor( "w00tguy" );
	g_Module.ScriptInfo.SetContactInfo( "w00tguy123 - forums.svencoop.com" );
	
	// best guess for delay, but it goes too fast sometimes, or cuts out, or plays static
	g_Scheduler.SetInterval("replay_voice", 0.047, -1);
}

int replayIdx = 0;

class SvcVoiceDataPacket {
	uint8 playerIdx = 0;	// player to display the voice icon for
	uint16 payloadLength;	// size of the header below + sound data
	
	// below is some sort of header for speex or the sound engine.
	// The constant values here never or rarely changed during the playback of a song.
	// Changing any of these values causes the stream to be completely silent.
	uint8 unknown0 = 67;
	uint8 unknown1 = 151;
	uint8 unknown2 = 144;
	uint8 unknown3 = 18;
	uint8 unknown4 = 1;
	uint8 unknown5 = 0;
	uint8 unknown6 = 16;
	uint8 unknown7 = 1;
	uint8 unknown8 = 11;
	uint8 unknown9 = 192;
	uint8 unknown10 = 93;
	uint8 unknown11 = 6;	// set to 0 for last packet in the stream
	uint16 soundLen;		// equal to payloadLength - 18. Probably means the header data is 18 bytes long.
	uint16 unknown12;		// generally between 70 and 90
	uint16 time; 			// time code or sequence number of some sort. Incremented by 1-6 each packet

	// soundLen bytes long. speex or miles codec maybe.
	// First byte is always 104, second is usually 151 or around there.
	array<uint8> soundBytes; 
}

void replay_voice() {
	array<uint8> packet = replay[replayIdx % replay.size()];
	
	// SVC_VOICEDATA
	NetworkMessage m(MSG_BROADCAST, NetworkMessages::NetworkMessageType(53), null);
		m.WriteByte(0); // player index
		m.WriteShort(packet.size()); // compressed audio length
		for (uint i = 0; i < packet.size(); i++) {
			m.WriteByte(packet[i]);
		}
	m.End();
	
	string debug = "";
	for (uint i = 0; i < packet.size() and i < 32; i++) {
		//debug += formatInt(packet[i], '0H', 2) + " ";
		debug += formatInt(packet[i], 'l', 3) + " ";
	}
	
	uint16 test = (packet[13] << 8) + packet[12];
	//println("(" + formatInt(packet.size() - test, 'l', 2) + ") " + debug);
	println(debug);
	
	replayIdx += 1;
}