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
```

I'm rewriting this so it can run 24/7 on a cheap VPS. No setup instructions yet.
