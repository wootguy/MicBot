void print(string text) { g_Game.AlertMessage( at_console, text); }
void println(string text) { print(text + "\n"); }

dictionary g_player_states;

class PlayerState {
	bool isBot = false; // send bot commands to this player?
	bool ttsEnabled = true;
}

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
	
	g_Scheduler.SetInterval("kill_bots_in_survival", 3.0f, -1);
}

void kill_bots_in_survival() {
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
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    !<message> = Hide your message from the chat.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mpitch <1-200>   = set text-to-speech pitch.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mlang <language> = set text-to-speech language.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mlangs           = list valid languages.\n");			
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop            = Stop all audio.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop speak      = Stop all text-to-speech audio.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop last       = Stop all youtube videos except the one that first started playing.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mstop first      = Stop all youtube videos except the one that last started playing.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mtts             = enable/disable text to speech for your messages.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    .mbot             = register/unregister yourself as a bot with the server.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    You can add a timestamp after a youtube link to play at an offset. For example:\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "    https://www.youtube.com/watch?v=b8HO6hba9ZE 0:27\n");
			return true; // hide from chat relay
		}
		if (args[0] == ".mlangs") {
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] Language codes sent to your console.\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "Valid languages:\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  af = African\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  sq = Albanian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ar = Arabic\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  hy = Armenian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  bn = Bengali\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  bs = Bosnian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  bg = Bulgarian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ct = Catalan\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  hr = Croatian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  cs = Czech\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  da = Danish\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  nl = Dutch\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  au = English (Austrailia)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ca = English (Canada)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  in = English (India)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ir = English (Ireland)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  afs = English (South Africa)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ek = English (United Kingdom)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  eo = Esperanto\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  et = Estonian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  tl = Filipino\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  fi = Finnish\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  fc = French (Canada)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  fr = French (France)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  de = German\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  el = Greek\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  hi = Hindi\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  hu = Hungarian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  is = Icelandic\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  id = Indonesian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  it = Italian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ja = Japanese\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  jw = Javanese\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  kn = Kannada\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  km = Khmer\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ko = Korean\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  la = Latin\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  lv = Latvian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  mk = Macedonian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  cn = Mandarin (China Mainland)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  tw = Mandarin (Taiwan)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ne = Nepali\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  no = Norwegian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  pl = Polish\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  br = Portuguese (Brazil)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  pt = Portuguese (Portugal)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ro = Romanian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ru = Russian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  mx = Spanish (Mexico)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  es = Spanish (Spain)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ma = Spanish (United States)\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  sr = Serbian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  si = Sinhala\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  sk = Slovak\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  su = Sundanese\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  sw = Swahili\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  sv = Swedish\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ta = Tamil\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  te = Telugu\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  th = Thai\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  tr = Turkish\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  uk = Ukrainian\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  ur = Urdu\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  vi = Vietnamese\n");
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  cy = Welsh\n");
			//g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "  my = Myanmar (Burmese)\n");
			return true; // hide from chat relay
		}
		if (args[0] == ".mtts") {
			state.ttsEnabled = !state.ttsEnabled;
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "[MicBot] Your text to speech is " + (state.ttsEnabled ? "enabled" : "disabled") + ".\n");
			return true;
		}
		
		if (args[0] == ".mbot") {
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
		} else if (state.ttsEnabled and chatText.Length() > 0 and !state.isBot) {			
			// message starts repeating due to broken newlines if too long
			if (string(plr.pev.netname).Length() + chatText.Length() >= 118) {
				chatText = chatText.SubString(0, 118 - string(plr.pev.netname).Length());
			}
		
			message_bots(plr, chatText);
		}
		
		if (args[0] == '.mstop') {
			string msg = "[MicBot] " + plr.pev.netname + ": " + args[0] + " " + args[1] + "\n";
			g_PlayerFuncs.ClientPrintAll(HUD_PRINTNOTIFY, msg);
			server_print(plr, msg);
			message_bots(plr, args[0] + " " + args[1]);
			return true; // hide from chat relay
		}
		
		if (args[0] == '.mlang' || args[0] == '.mpitch') {
			string msg = "[MicBot] " + plr.pev.netname + ": " + args[0] + " " + args[1] + "\n";
			message_bots(plr, args[0] + " " + args[1]);
			return true; // hide from chat relay
		}
		
		if (args[0] == '.mlangs') {
			return true;
		}
		
		if (args[0][0] == "!") {
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
	for ( int i = 1; i <= g_Engine.maxClients; i++ ) {
		CBasePlayer@ p = g_PlayerFuncs.FindPlayerByIndex(i);
		
		if (p is null or !p.IsConnected()) {
			continue;
		}
		
		PlayerState@ pstate = getPlayerState(p);
		
		if (pstate.isBot) {
			g_PlayerFuncs.ClientPrint(p, HUD_PRINTCONSOLE, "MicBot\\" + sender.pev.netname + "\\" + text + "\n");
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

void consoleCmd( const CCommand@ args ) {
	CBasePlayer@ plr = g_ConCommandSystem.GetCurrentPlayer();
	doCommand(plr, args, "", true);
}