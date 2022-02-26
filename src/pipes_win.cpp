#include <pipes.h>
#include <Windows.h>
#include <vector>
#include <map>
#include <thread>
#include <chrono>

using namespace std;

map<string, HANDLE> g_pipes;

std::string createInputPipe(std::string id) {
    HANDLE hPipe;
    string pipeName = "\\\\.\\pipe\\" + id;

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

void readPipe(string pipeName, ThreadInputBuffer* inputBuffer) {
    map<string, HANDLE>::const_iterator iter = g_pipes.find(pipeName);

    if (iter == g_pipes.end()) {
        printf("Unknown pipe %s\n", pipeName.c_str());
    }

    HANDLE hPipe = iter->second;

    char buffer[1024];

    while (1) {
        printf("Try connect pipe\n");
        if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
        {
            while (1) {
                DWORD bytesRead;

                if (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL) != FALSE)
                {
                    size_t bytesLeftToWrite = bytesRead;
                    size_t writePos = 0;

                    while (bytesLeftToWrite) {
                        size_t written = inputBuffer->write(buffer + writePos, bytesLeftToWrite);
                        bytesLeftToWrite -= written;
                        writePos += written;

                        if (written) {
                            //printf("Wrote %llu to input buffer\n", written);
                        }
                        else {
                            //printf("input buffer not ready for writing yet\n");
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                    }
                }
                else {
                    printf("No data in pipe\n");
                    DisconnectNamedPipe(hPipe);
                    break;
                }
            }
        }
        else {
            printf("Can't connect to pipe\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}