# pip install pafy gtts python-vlc pyglet pywin32 psutil pydub
# pip install --no-deps -U yt-dlp
# edit backend_youtube_dl.py in pafy library:
#    replace youtube_dl import with this:
#        import yt_dlp as youtube_dl
#    Remove like/dislike stuff from info
# Windows notes:
#    Need to install 64 bit version of VLC if 64 bit version of python, or 32 bit if 32 bit python
#    Might need to edit script to point to VLC folder to load DLLs
#    Some .net or visual studio stuff might be needed to install pip stuff

import time, os, sys
from threading import Thread
import pafy
import vlc
from gtts import gTTS
import pyglet
from pyglet.media import Player
import win32api, win32gui, win32con, ctypes, win32process, psutil
from pydub import AudioSegment, effects

import ctypes, time
# Bunch of stuff so that the script can send keystrokes to game #

SendInput = ctypes.windll.user32.SendInput

# C struct redefinitions 
PUL = ctypes.POINTER(ctypes.c_ulong)
class KeyBdInput(ctypes.Structure):
    _fields_ = [("wVk", ctypes.c_ushort),
                ("wScan", ctypes.c_ushort),
                ("dwFlags", ctypes.c_ulong),
                ("time", ctypes.c_ulong),
                ("dwExtraInfo", PUL)]

class HardwareInput(ctypes.Structure):
    _fields_ = [("uMsg", ctypes.c_ulong),
                ("wParamL", ctypes.c_short),
                ("wParamH", ctypes.c_ushort)]

class MouseInput(ctypes.Structure):
    _fields_ = [("dx", ctypes.c_long),
                ("dy", ctypes.c_long),
                ("mouseData", ctypes.c_ulong),
                ("dwFlags", ctypes.c_ulong),
                ("time",ctypes.c_ulong),
                ("dwExtraInfo", PUL)]

class Input_I(ctypes.Union):
    _fields_ = [("ki", KeyBdInput),
                 ("mi", MouseInput),
                 ("hi", HardwareInput)]

class Input(ctypes.Structure):
    _fields_ = [("type", ctypes.c_ulong),
                ("ii", Input_I)]

# Actuals Functions
def PressKey(hexKeyCode):
    extra = ctypes.c_ulong(0)
    ii_ = Input_I()
    ii_.ki = KeyBdInput( 0, hexKeyCode, 0x0008, 0, ctypes.pointer(extra) )
    x = Input( ctypes.c_ulong(1), ii_ )
    ctypes.windll.user32.SendInput(1, ctypes.pointer(x), ctypes.sizeof(x))

def ReleaseKey(hexKeyCode):
    extra = ctypes.c_ulong(0)
    ii_ = Input_I()
    ii_.ki = KeyBdInput( 0, hexKeyCode, 0x0008 | 0x0002, 0, ctypes.pointer(extra) )
    x = Input( ctypes.c_ulong(1), ii_ )
    ctypes.windll.user32.SendInput(1, ctypes.pointer(x), ctypes.sizeof(x))

# http://www.flint.jp/misc/?q=dik&lang=en
def PressButton(code):
	global sven_hwnd

	win32gui.SetForegroundWindow(sven_hwnd)
	PressKey(code)
	time.sleep(.05)
	ReleaseKey(code)

def follow(thefile):
    '''generator function that yields new lines in a file
    '''
    # seek the end of the file
    thefile.seek(0, os.SEEK_END)
    
    # start infinite loop
    while True:
        # read last line of file
        line = thefile.readline()        # sleep if file hasn't been updated
        if not line:
            time.sleep(0.1)
            continue

        yield line

players = []
sound_threads = []
g_chatsounds = []
tts_id = 0
g_accents = {}
g_pitches = {}
g_players = {}

def load_all_chatsounds():
	file1 = open('chatsounds.txt', 'r')
	for line in file1.readlines():
		g_chatsounds.append(line.split()[0])

def playsound_async(speaker, sound):	
	if speaker in g_players:
		g_players[speaker].pause()
		g_players[speaker].delete()
	
	player = g_players[speaker] = Player()
	
	source = pyglet.media.load(sound, streaming=False)
	player.pitch = (g_pitches[speaker] / 100.0) if speaker in g_pitches else 1
	player.queue(source)
	player.play()
	sound_threads.append(player)
	
def playtube_async(url, offset):
	# https://youtu.be/GXv1hDICJK0 (age restricted)
	try:
		global players
		print("Fetch best audio " + url)
		video = pafy.new(url)
		best = video.getbestaudio()
		playurl = best.url
		
		print("Create vlc instance")
		Instance = vlc.Instance()
		player = Instance.media_player_new()
		media = Instance.media_new(playurl)
		media.get_mrl()
		media.add_option('start-time=%d' % offset)
		player.set_media(media)
		player.play()
		
		players.append(player)
		print("Play offset %d: " % offset + video.title)
	except Exception as e:
		print(e)
		
		t = Thread(target = play_tts, args =('', str(e), ))
		t.daemon = True
		t.start()

def play_tts(speaker, text):	  
	global tts_id
	
	# Language in which you want to convert
	language = 'en'
	tld = 'com'
	
	if speaker in g_accents:
		language = g_accents[speaker]['code']
		tld = g_accents[speaker]['tld']
	  
	# Passing the text and language to the engine, 
	# here we have marked slow=False. Which tells 
	# the module that the converted audio should 
	# have a high speed
	print("Translating %d" % tts_id)
	myobj = gTTS(text=text, tld=tld, lang=language, slow=False)
	  
	# Saving the converted audio in a mp3 file named
	# welcome
	fname = 'tts/tts%d' % tts_id + '.mp3'
	myobj.save(fname)
	
	totalCaps = sum(1 for c in text if c.isupper())
	totalLower = sum(1 for c in text if c.islower())
	totalExclaim = sum(1 for c in text if c == '!')
	
	for x in range(0, 10):
		try:
			print("Normalize %d (attempt %d)" % (tts_id, x))
			output = 'tts/tts%d.wav' % tts_id
			
			rawsound = AudioSegment.from_file(fname, "mp3")  
			normalizedsound = effects.normalize(rawsound)  
			
			if totalCaps > totalLower:
				normalizedsound = normalizedsound + 1000
			
			normalizedsound.export(output, format="wav")
			
			break
		except Exception as e:
			print(e)
			time.sleep(0.1)
	 
	print("Play %d" % tts_id)
	# Playing the converted file
	playsound_async(speaker, output)
	
	tts_id += 1
	
	for x in range(0, 10):
		try:
			print("Delete attempt %d" % x)
			if os.path.exists(fname):
				os.remove(fname)
			if os.path.exists(output):
				os.remove(output)
			break
		except Exception as e:
			print(e)
			time.sleep(0.1)
	

sven_root = '../../../..'
csound_path = os.path.join(sven_root, 'svencoop_downloads/sound/twlz')

def sanitize_name(name):
	return name.replace("0", "o")
	
def find_console_log():
	global sven_root
	
	logs = []
	
	for file in os.listdir(sven_root):
		if file.startswith('console-') and file.endswith('.log'):
			logs.append(file)
			
	logs.sort()
	
	if len(logs) == 0:
		print("No console log found. Add -condebug to Sven Co-op's launch options and start the game.")
		print("If you did that, then make sure this script is in this folder: svencoop_addon/plugins/scripts/MicBot/")
		sys.exit()
	
	return os.path.join(sven_root, logs[len(logs)-1])

filename = find_console_log()
load_all_chatsounds()
print("Following log file: " + filename)

tts_enabled = True

sven_hwnd = -1

def winEnumHandler( hwnd, ctx ):
	global sven_hwnd
	
	if win32gui.IsWindowVisible( hwnd ):
		text = window_process_name( hwnd )
		if not text:
			return
		if "svencoop.exe" == text:
			sven_hwnd = hwnd
			
def window_process_name(hwnd):
	try:
		pid = win32process.GetWindowThreadProcessId(hwnd)
		return(psutil.Process(pid[-1]).name())
	except Exception as e:
		print(e)
		return ""

win32gui.EnumWindows( winEnumHandler, None )
if sven_hwnd == -1:
	print("Failed to find Sven Co-op window. Is the game running?")
	sys.exit()

def type_sven(text):
	for char in text:
		win32api.PostMessage(sven_hwnd, win32con.WM_CHAR, ord(char), 0)
		
	old_hwnd = win32gui.GetForegroundWindow()
	win32gui.SetForegroundWindow(sven_hwnd)
	PressButton(0x1C) # enter key
	win32gui.SetForegroundWindow(old_hwnd)


# tell the server we're alive. It will send a new 
def heartbeat_thread():
	while True:
		old_hwnd = win32gui.GetForegroundWindow()
		win32gui.SetForegroundWindow(sven_hwnd)
		PressButton(0x3E)
		win32gui.SetForegroundWindow(old_hwnd)
	
		time.sleep(1)
	

t = Thread(target = heartbeat_thread, args =( ))
t.daemon = True
t.start()

logfile = open(filename, encoding='utf8', errors='ignore')
loglines = follow(logfile)   
 # iterate over the generator
for line in loglines:
	print(line)
	
	if line.startswith('MicBot\\'):
		line = line[len('MicBot\\'):]
		name = sanitize_name(line[:line.find("\\")])
		line = line[line.find("\\")+1:]
		print(name + ": " + line.strip())
		
		if line.startswith('https://www.youtube.com') or line.startswith('!https://www.youtube.com') or line.startswith('https://youtu.be') or line.startswith('!https://youtu.be'):
			if line.startswith("!"):
				line = line[1:]
				
			args = line.split()
			offset = 0
			try:
				if len(args) >= 2:
					timecode = args[1]
					if ':' in timecode:
						minutes = timecode[:timecode.find(':')]
						seconds = timecode[timecode.find(':')+1:]
						offset = int(minutes)*60 + int(seconds)
					else:
						offset = int(timecode)
			except Exception as e:
				print(e)
		
			t = Thread(target = playtube_async, args =(args[0], offset, ))
			t.daemon = True
			t.start()
			continue
			
		if line.startswith('.mstop'):
			args = line.split()
			arg = args[1] if len(args) > 1 else ""
		
			if arg == "":
				for player in players:
					player.stop()
				players = []
			elif arg == "last":
				for player in players[1:]:
					player.stop()
				players = players[:1]
			elif arg == "first":
				for player in players[:-1]:
					player.stop()
				players = players[-1:]
				
			if arg == "" or arg == 'speak':
				for player in sound_threads:
					player.pause()
					player.delete()
				sound_threads = []
			
		
			t = Thread(target = play_tts, args =(name, 'stop ' + arg, ))
			t.daemon = True
			t.start()
			continue
		
		'''
		if line.startswith('!'):
			cmd = line[1:line.find(' ')]
			
			check_path = os.path.join(csound_path, cmd + '.wav')
			if os.path.exists(check_path):
				playsound_async(name, check_path)
				continue
		'''
		
		if tts_enabled:			
			if line.strip().lower() in g_chatsounds:
				continue
			
			if line.startswith('.mpitch') and len(line.split()) > 1:
				g_pitches[name] = int(line.split()[1])
				if name in g_players:
					g_players[name].pitch = g_pitches[name] / 100.0
				continue
			
			if line.startswith('.mlang '):
				lang = line[6:].strip().lower()
				
				valid_langs = {
					'af': {'tld': 'com', 'code': 'af', 'name': 'African'},
					'afs': {'tld': 'co.za', 'code': 'en', 'name': 'South African'},
					'ar': {'tld': 'com', 'code': 'ar', 'name': 'arabic'},
					'au': {'tld': 'com.au', 'code': 'en', 'name': 'Austrailian'},
					'bg': {'tld': 'com', 'code': 'bg', 'name': 'bulgarian'},
					'bn': {'tld': 'com', 'code': 'bn', 'name': 'Bengali'},
					'br': {'tld': 'com.br', 'code': 'pt', 'name': 'brazilian'},
					'bs': {'tld': 'com', 'code': 'bs', 'name': 'Bosnian'},
					'ca': {'tld': 'ca', 'code': 'en', 'name': 'Canadian'},
					'cn': {'tld': 'com', 'code': 'zh-CN', 'name': 'chinese'},
					'cs': {'tld': 'com', 'code': 'cs', 'name': 'Czech'},
					'ct': {'tld': 'com', 'code': 'ca', 'name': 'Catalan'},
					'cy': {'tld': 'com', 'code': 'cy', 'name': 'Welsh'},
					'da': {'tld': 'com', 'code': 'da', 'name': 'Danish'},
					'de': {'tld': 'com', 'code': 'de', 'name': 'German'},
					'el': {'tld': 'com', 'code': 'el', 'name': 'Greek'},
					'en': {'tld': 'com', 'code': 'en', 'name': 'american'},
					'eo': {'tld': 'com', 'code': 'eo', 'name': 'Esperanto'},
					'es': {'tld': 'es', 'code': 'es', 'name': 'spanish'},
					'et': {'tld': 'com', 'code': 'et', 'name': 'Estonian'},
					'fc': {'tld': 'ca', 'code': 'fr', 'name': 'french canadian'},
					'fi': {'tld': 'com', 'code': 'fi', 'name': 'Finnish'},
					'fr': {'tld': 'fr', 'code': 'fr', 'name': 'french'},
					'gu': {'tld': 'com', 'code': 'gu', 'name': 'Gujarati'},
					'hi': {'tld': 'com', 'code': 'hi', 'name': 'Hindi'},
					'hr': {'tld': 'com', 'code': 'hr', 'name': 'Croatian'},
					'hu': {'tld': 'com', 'code': 'hu', 'name': 'Hungarian'},
					'hy': {'tld': 'com', 'code': 'hy', 'name': 'Armenian'},
					'is': {'tld': 'com', 'code': 'is', 'name': 'Icelandic'},
					'id': {'tld': 'com', 'code': 'id', 'name': 'Indonesian'},
					'in': {'tld': 'co.in', 'code': 'en', 'name': 'Indian'},
					'ir': {'tld': 'ie', 'code': 'en', 'name': 'Irish'},
					'it': {'tld': 'com', 'code': 'it', 'name': 'Italian'},
					'ja': {'tld': 'com', 'code': 'ja', 'name': 'Japanese'},
					'jw': {'tld': 'com', 'code': 'jw', 'name': 'Javanese'},
					'km': {'tld': 'com', 'code': 'km', 'name': 'Khmer'},
					'kn': {'tld': 'com', 'code': 'kn', 'name': 'Kannada'},
					'ko': {'tld': 'com', 'code': 'ko', 'name': 'Korean'},
					'la': {'tld': 'com', 'code': 'la', 'name': 'Latin'},
					'lv': {'tld': 'com', 'code': 'lv', 'name': 'Latvian'},
					'ma': {'tld': 'com', 'code': 'es', 'name': 'Mexican American'},
					'mk': {'tld': 'com', 'code': 'mk', 'name': 'Macedonian'},
					'mr': {'tld': 'com', 'code': 'mr', 'name': 'Marathi'},
					'mx': {'tld': 'com.mx', 'code': 'es', 'name': 'Mexican'},
					'my': {'tld': 'com', 'code': 'my', 'name': 'Myanmar (Burmese)'},
					'ne': {'tld': 'com', 'code': 'ne', 'name': 'Nepali'},
					'nl': {'tld': 'com', 'code': 'nl', 'name': 'Dutch'},
					'no': {'tld': 'com', 'code': 'no', 'name': 'Norwegian'},
					'pl': {'tld': 'com', 'code': 'pl', 'name': 'Polish'},
					'pt': {'tld': 'pt', 'code': 'pt', 'name': 'portuguese'},
					'ro': {'tld': 'com', 'code': 'ro', 'name': 'Romanian'},
					'ru': {'tld': 'com', 'code': 'ru', 'name': 'Russian'},
					'si': {'tld': 'com', 'code': 'si', 'name': 'Sinhala'},
					'sk': {'tld': 'com', 'code': 'sk', 'name': 'Slovak'},
					'sq': {'tld': 'com', 'code': 'sq', 'name': 'Albanian'},
					'sr': {'tld': 'com', 'code': 'sr', 'name': 'Serbian'},
					'su': {'tld': 'com', 'code': 'su', 'name': 'Sundanese'},
					'sv': {'tld': 'com', 'code': 'sv', 'name': 'Swedish'},
					'sw': {'tld': 'com', 'code': 'sw', 'name': 'Swahili'},
					'ta': {'tld': 'com', 'code': 'ta', 'name': 'Tamil'},
					'te': {'tld': 'com', 'code': 'te', 'name': 'Telugu'},
					'th': {'tld': 'com', 'code': 'th', 'name': 'Thai'},
					'tl': {'tld': 'com', 'code': 'tl', 'name': 'Filipino'},
					'tr': {'tld': 'com', 'code': 'tr', 'name': 'Turkish'},
					'tw': {'tld': 'com', 'code': 'zh-TW', 'name': 'taiwanese'},
					'ek': {'tld': 'co.uk', 'code': 'en', 'name': 'british'},
					'uk': {'tld': 'com', 'code': 'uk', 'name': 'Ukrainian'},
					'ur': {'tld': 'com', 'code': 'ur', 'name': 'Urdu'},
					'vi': {'tld': 'com', 'code': 'vi', 'name': 'Vietnamese'}
				}
				
				if lang in valid_langs:
					g_accents[name] = valid_langs[lang]
					msg = valid_langs[lang]['name'] + " accent, activated"
					t = Thread(target = play_tts, args =(name, msg, ))
					t.daemon = True
					t.start()
				else:
					msg = "invalid accent " + lang
					t = Thread(target = play_tts, args =(name, msg, ))
					t.daemon = True
					t.start()
				
				continue
			
			if line.startswith('!'):
				line = line[1:]
			
			t = Thread(target = play_tts, args =(name, line, ))
			t.start()
		
	
	
