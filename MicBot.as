void print(string text) { g_Game.AlertMessage( at_console, text); }
void println(string text) { print(text + "\n"); }

dictionary g_player_states;

class PlayerState {
	bool isBot = false; // send bot commands to this player?
	bool ttsEnabled = true;
	string lang = "en";
	int pitch = 100;
}

dictionary g_langs = {
	{"af", "African"},
	{"sq", "Albanian"},
	{"ar", "Arabic"},
	{"hy", "Armenian"},
	{"bn", "Bengali"},
	{"bs", "Bosnian"},
	{"bg", "Bulgarian"},
	{"ct", "Catalan"},
	{"hr", "Croatian"},
	{"cs", "Czech"},
	{"da", "Danish"},
	{"nl", "Dutch"},
	{"au", "English (Austrailia)"},
	{"ca", "English (Canada)"},
	{"in", "English (India)"},
	{"ir", "English (Ireland)"},
	{"afs", "English (South Africa)"},
	{"ek", "English (United Kingdom)"},
	{"en", "English (United States)"},
	{"eo", "Esperanto"},
	{"et", "Estonian"},
	{"tl", "Filipino"},
	{"fi", "Finnish"},
	{"fc", "French (Canada)"},
	{"fr", "French (France)"},
	{"de", "German"},
	{"el", "Greek"},
	{"hi", "Hindi"},
	{"hu", "Hungarian"},
	{"is", "Icelandic"},
	{"id", "Indonesian"},
	{"it", "Italian"},
	{"ja", "Japanese"},
	{"jw", "Javanese"},
	{"kn", "Kannada"},
	{"km", "Khmer"},
	{"ko", "Korean"},
	{"la", "Latin"},
	{"lv", "Latvian"},
	{"mk", "Macedonian"},
	{"cn", "Mandarin (China Mainland)"},
	{"tw", "Mandarin (Taiwan)"},
	//{"my","Myanmar (Burmese)"},
	{"ne", "Nepali"},
	{"no", "Norwegian"},
	{"pl", "Polish"},
	{"br", "Portuguese (Brazil)"},
	{"pt", "Portuguese (Portugal)"},
	{"ro", "Romanian"},
	{"ru", "Russian"},
	{"mx", "Spanish (Mexico)"},
	{"es", "Spanish (Spain)"},
	{"ma", "Spanish (United States)"},
	{"sr", "Serbian"},
	{"si", "Sinhala"},
	{"sk", "Slovak"},
	{"su", "Sundanese"},
	{"sw", "Swahili"},
	{"sv", "Swedish"},
	{"ta", "Tamil"},
	{"te", "Telugu"},
	{"th", "Thai"},
	{"tr", "Turkish"},
	{"uk", "Ukrainian"},
	{"ur", "Urdu"},
	{"vi", "Vietnamese"},
	{"cy", "Welsh"}
};

PlayerState@ getPlayerState(CBasePlayer@ plr)
{
	if (plr is null or !plr.IsConnected())
		return null;
		
	string steamId = g_EngineFuncs.GetPlayerAuthId( plr.edict() );
	if (steamId == 'STEAM_ID_LAN' or steamId == 'BOT') {
		steamId = plr.pev.netname;
	}
	
	if ( !g_player_states.exists(steamId) )
	{
		PlayerState state;
		g_player_states[steamId] = state;
	}
	return cast<PlayerState@>( g_player_states[steamId] );
}

void PluginInit()
{	
	g_Module.ScriptInfo.SetAuthor( "w00tguy" );
	g_Module.ScriptInfo.SetContactInfo( "https://github.com/wootguy/" );
	
	g_Hooks.RegisterHook( Hooks::Player::ClientSay, @ClientSay );
	
	g_Scheduler.SetInterval("bot_loop", 3.0f, -1);
}

void bot_loop() {
	if (!g_SurvivalMode.IsActive()) {
		return;
	}
	
	for ( int i = 1; i <= g_Engine.maxClients; i++ ) {
		CBasePlayer@ p = g_PlayerFuncs.FindPlayerByIndex(i);
		
		if (p is null or !p.IsConnected() or !p.IsAlive()) {
			continue;
		}
		
		PlayerState@ pstate = getPlayerState(p);
		
		if (pstate.isBot) {
			g_PlayerFuncs.ClientPrint(p, HUD_PRINTTALK, "[MicBot] Bots aren't allowed to live in survival mode.\n");
			g_EntityFuncs.Remove(p);
		}
	}
}

bool doCommand(CBasePlayer@ plr, const CCommand@ args, string chatText, bool inConsole) {
	bool isAdmin = g_PlayerFuncs.AdminLevel(plr) >= ADMIN_YES;
	PlayerState@ state = getPlayerState(plr);
	
	if (args.ArgC() >= 1)
	{
		if (args[0] == ".micbot" || args[0] == ".mhelp") {
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "MicBot commands sent to your console.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "MicBot reads messages aloud and can play audio from youtube links.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    ~<message>        = Hide your message from the chat.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mpitch <1-200>   = set text-to-speech pitch.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mlang <language> = set text-to-speech language.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mlangs           = list valid languages.\n");			
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop            = Stop all audio.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop speak      = Stop all text-to-speech audio.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop last       = Stop all youtube videos except the one that first started playing.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop first      = Stop all youtube videos except the one that last started playing.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mtts             = enable/disable text to speech for your messages.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .msay             = say something as the bot\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mbot             = register/unregister yourself as a bot with the server.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    You can add a timestamp after a youtube link to play at an offset. For example:\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    https://www.youtube.com/watch?v=b8HO6hba9ZE 0:27\n");
			return true; // hide from chat relay
		}
		else if (args[0] == ".mlangs") {
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] Language codes sent to your console.\n");
			
			array<string>@ langKeys = g_langs.getKeys();
			array<string> lines;
			
			langKeys.sort(function(a,b) { return string(g_langs[a]) < string(g_langs[b]); });
			
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "Valid language codes:\n");
			for (uint i = 0; i < g_langs.size(); i++) {
				g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    " + langKeys[i] + " = " + string(g_langs[langKeys[i]]) + "\n");
			}
			
			return true; // hide from chat relay
		}
		if (args[0] == ".mtts") {
			state.ttsEnabled = !state.ttsEnabled;
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] Your text to speech is " + (state.ttsEnabled ? "enabled" : "disabled") + ".\n");
			return true;
		} else if (args[0] == ".mbot") {
			if (args[1] == '1') {
				state.isBot = true;
				return true;
			}
		
			state.isBot = !state.isBot;
			
			if (state.isBot) {
				g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] You are now a bot and will receive special console messages.\n");
			} else {
				g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] You are no longer a bot.\n");
			}

			return true;
		} 
		else if (args[0] == '.msay') {
			string msg = "[MicBot] " + plr.pev.netname + ": " + chatText + "\n";
			g_PlayerFuncs.ClientPrintAll(HUD_PRINTCONSOLE, msg);
			server_print(plr, msg);
			message_bots(plr, chatText);
			return true; // hide from chat relay
		}
		else if (args[0] == '.mstop') {
			string msg = "[MicBot] " + plr.pev.netname + ": " + args[0] + " " + args[1] + "\n";
			g_PlayerFuncs.ClientPrintAll(HUD_PRINTNOTIFY, msg);
			server_print(plr, msg);
			message_bots(plr, args[0] + " " + args[1]);
			return true; // hide from chat relay
		}
		else if (args[0] == '.mlang') {
			string code = args[1].ToLowercase();
			
			if (g_langs.exists(code)) {
				state.lang = code;
				g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] Language set to " + string(g_langs[code]) + ".\n");
			} else {
				g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] Invalid language code \"" + code + "\". Type \".mlangs\" for a list of valid codes.\n");
			}
			
			return true; // hide from chat relay
		}
		else if (args[0] == '.mpitch') {
			int pitch = atoi(args[1]);
			
			if (pitch < 1) {
				pitch = 1;
			} else if (pitch > 200) {
				pitch = 200;
			}
			
			state.pitch = pitch;
			
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] Pitch set to " + pitch + ".\n");
			
			return true; // hide from chat relay
		}
		else if (state.ttsEnabled and chatText.Length() > 0 and chatText.SubString(0,3).ToLowercase() != "/me") {			
			// message starts repeating due to broken newlines if too long
			if (string(plr.pev.netname).Length() + chatText.Length() >= 110) {
				chatText = chatText.SubString(0, 110 - string(plr.pev.netname).Length());
			}
		
			message_bots(plr, chatText);
		}
		
		if (args[0][0] == "~") {
			string msg = "[MicBot] " + plr.pev.netname + ": " + chatText + "\n";
			g_PlayerFuncs.ClientPrintAll(HUD_PRINTCONSOLE, msg);
			server_print(plr, msg);
			return true; // hide from chat relay
		}
	}
	
	return false;
}

void server_print(CBasePlayer@ plr, string msg) {
	g_EngineFuncs.ServerPrint(msg);
	g_Game.AlertMessage(at_logged, "\"%1<%2><%3><player>\" say \"%4\"\n", plr.pev.netname, string(g_EngineFuncs.GetPlayerUserId(plr.edict())), g_EngineFuncs.GetPlayerAuthId(plr.edict()), msg);
}

void message_bots(CBasePlayer@ sender, string text) {
	PlayerState@ senderState = getPlayerState(sender);
	
	string msg = "MicBot\\" + sender.pev.netname + "\\" + senderState.lang + "\\" + senderState.pitch + "\\" + text;
	if (text >= 128) {
		text = text.SubString(0, 128);
	}
	

	for ( int i = 1; i <= g_Engine.maxClients; i++ ) {
		CBasePlayer@ p = g_PlayerFuncs.FindPlayerByIndex(i);
		
		if (p is null or !p.IsConnected()) {
			continue;
		}
		
		PlayerState@ pstate = getPlayerState(p);
		
		if (pstate.isBot) {
			g_PlayerFuncs.ClientPrint(p, HUD_PRINTCONSOLE, "MicBot\\" + sender.pev.netname + "\\" + senderState.lang + "\\" + senderState.pitch + "\\" + text + "\n");
		}
	}
}

HookReturnCode ClientSay( SayParameters@ pParams ) {
	CBasePlayer@ plr = pParams.GetPlayer();
	const CCommand@ args = pParams.GetArguments();
	
	if (pParams.ShouldHide) {
		return HOOK_CONTINUE;
	}
	
	if (args.ArgC() > 0 && doCommand(plr, args, pParams.GetCommand(), false))
	{
		pParams.ShouldHide = true;
		return HOOK_HANDLED;
	}
	return HOOK_CONTINUE;
}

CClientCommand _g("micbot", "Spectate commands", @consoleCmd );
CClientCommand _g2("mlangs", "Spectate commands", @consoleCmd );
CClientCommand _g3("mtts", "Spectate commands", @consoleCmd );
CClientCommand _g4("mstop", "Spectate commands", @consoleCmd );
CClientCommand _g5("mlang", "Spectate commands", @consoleCmd );
CClientCommand _g6("mpitch", "Spectate commands", @consoleCmd );
CClientCommand _g7("mhelp", "Spectate commands", @consoleCmd );
CClientCommand _g8("mbot", "Spectate commands", @consoleCmd );
CClientCommand _g9("msay", "Spectate commands", @consoleCmd );

void consoleCmd( const CCommand@ args ) {
	string chatText = "";
	
	for (int i = 0; i < args.ArgC(); i++) {
		chatText += args[i] + " ";
	}
	
	CBasePlayer@ plr = g_ConCommandSystem.GetCurrentPlayer();
	doCommand(plr, args, chatText, true);
}