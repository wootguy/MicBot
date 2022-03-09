### The [Radio](https://github.com/wootguy/Radio) plugin replaces this. MicBot could still be useful if you want to have a community mic spammer on servers that won't install the Radio plugin. Well, that's what I would say if I ever got this updated to not require the MicBot server plugin. Anyway, the last working version using a real client is here: https://github.com/wootguy/MicBot/tree/e4e3dcc178f0618baec51e33468a2f2c61ec09ec

# MicBot
MicBot uses Google Translate to speak everyone's chat messages. It can also play audio from youtube links sent via chat.

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
``
