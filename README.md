# MicBot
MicBot uses Google Translate to speak everyone's chat messages. It can also play audio from youtube links sent via chat. You'll need to setup a spare PC or VM for the sole purpose of running this bot. The server you join must also have the MicBot plugin installed (this probably won't be a requirement in the future).

Say `.mhelp` for usage on servers which have the MicBot plugin installed.
```
MicBot reads messages aloud and can play audio from youtube links.
    ~<message> = Hide your message from the chat.
    .mpitch <1-200>   = set text-to-speech pitch.
    .mlang <language> = set text-to-speech language.
    .mlangs           = list valid languages.
    .mstop            = Stop all audio.
    .mstop speak      = Stop all text-to-speech audio.
    .mstop last       = Stop all youtube videos except the one that first started playing.
    .mstop first      = Stop all youtube videos except the one that last started playing.
    .mtts             = enable/disable text to speech for your messages.
    .mbot             = register/unregister yourself as a bot with the server.

    You can add a timestamp after a youtube link to play at an offset. For example:
    https://www.youtube.com/watch?v=b8HO6hba9ZE 0:27
```

## Windows installation:
1. Install Python 3
1. `pip install pafy gtts python-vlc pyglet pynput pydub youtube_dl yt-dlp pynput`
    * If `yt-dlp` fails to install, then try this command:
    `pip install --no-deps -U yt-dlp`
    * You might need to install some .NET framework or visual studio stuff. Any error messages you see should be google-able.
1. Install the appropriate version of VLC (64-bit VLC if you got 64-bit python. 32-bit VLC if 32-bit python.)
1. Install ffmpeg and add the `/bin` folder to your system `PATH` (environment variable).
1. Make "Stereo mix" your default recording device for sven or install something like Virtual Audio Cable to get sven to hear your desktop sounds.

## Linux installation:
1. `sudo apt install xdotool python3-gst-1.0 python3 python3-pip ffmpeg vlc`
1. `pip3 install pafy gtts python-vlc pyglet pydub youtube_dl yt-dlp`
1. Redirect sven to record from your speaker output. I had to do this for a Lubuntu 18.04 x64 VM:
    * sudo apt install pavucontrol
    * pactl load-module module-loopback latency_msec=1
    * Set sound card profile to "Off" in Configuration tab of the volume settings (this will disable speakers but I wanted that anyway)

## Final installation steps
1. Edit `backend_youtube_dl.py` in the pafy python library (default windows path: `Python3x/Lib/site-packages/pafy/`):
    * comment out the lines that have `like_count` and `dislike_count`. As of this writing, the current version of pafy will fail to fetch youtube links because of the removal of likes/dislikes from YouTube.
    * **[Optional]** If you get "Sign in to verify your age" errors for some videos, then also replace `youtube_dl import` with `import yt_dlp as youtube_dl`. This may result in other errors or videos not playing though, so maybe try without doing this first.
* **[Optional]** The bot will speak chat sounds by default. If you don't want that, create a file called `chatsounds.txt` next to the script. Each line should contain a single word which the bot will not speak by itself.
    
## Usage:
1. Add `-condebug` to the launch options of Sven Co-op. Then, start the game.
1. Type in console: `volume 0; mp3volume 0; bind F8 "vgui_closeall;+voicerecord;-voicerecord;+voicerecord"`
1. Join a server which has the MicBot plugin installed.
1. Say `.mbot` to register yourself as a bot.
1. Start the `client.py` script
1. Keep the game in focus and without the menu/console showing. The script will continue pressing F8 to keep the mic enabled across level changes.


