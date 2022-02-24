#include <pipes.h>
#include <Windows.h>
#include <vector>
#include <map>

using namespace std;

map<string, HANDLE> g_pipes;

// ffmpeg -i file.mp3 -y -f wav - >\\.\pipe\Pipe

std::string createInputPipe() {
    HANDLE hPipe;
    string pipeName = "\\\\.\\pipe\\Pipe";

    hPipe = CreateNamedPipe(TEXT(pipeName.c_str()),
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
        1,
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("Failed to create pipe\n");
        return "";
    }

    g_pipes.insert(make_pair(pipeName, hPipe));
    return pipeName;
}


void readPipe(string pipeName) {
    map<string, HANDLE>::const_iterator iter = g_pipes.find(pipeName);

    if (iter == g_pipes.end()) {
        printf("Unknown pipe %s\n", pipeName.c_str());
    }

    HANDLE hPipe = iter->second;

    char buffer[1024];
    DWORD dwRead;

    printf("Try connect pipe\n");
    while (1) {
        if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
        {
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
            {
                /* add terminating zero */
                buffer[dwRead] = '\0';

                /* do something with data in buffer */
                printf("%s", buffer);
            }
            else {
                printf("No data in pipe\n");
            }

            DisconnectNamedPipe(hPipe);
        }
        else {
            printf("Can't connect to pipe\n");
        }
    }
}