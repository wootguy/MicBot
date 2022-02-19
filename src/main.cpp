#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include "steam/steam_api.h"

void DumpMicData()
{
	// read local microphone input
	uint32 nBytesAvailable = 0;
	EVoiceResult res = SteamUser()->GetAvailableVoice(&nBytesAvailable, NULL, 0);

	if (res == k_EVoiceResultOK && nBytesAvailable > 0)
	{
		uint32 nBytesWritten = 0;

		// more than 400 bytes usually causes an overflow
		uint8 buffer[512];

		res = SteamUser()->GetVoice(true, buffer, 512, &nBytesWritten, false, NULL, 0, NULL, 0);

		if (res == k_EVoiceResultOK && nBytesWritten > 0)
		{
			for (int i = 0; i < nBytesWritten; i++) {
				//std::cout << (unsigned int)buffer[i] << " ";
				std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)buffer[i];
			}
			std::cout << std::endl;
		}
	}

	//printf("Dumped %d bytes\n", nBytesAvailable);
}

int main(int argc, const char **argv)
{
	if (!SteamAPI_Init()) {
		printf("Steam must be running in order to use this program.\n");
		return 1;
	}

	SteamUser()->StartVoiceRecording();

	while (true) {
		DumpMicData();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	SteamAPI_Shutdown();

	return EXIT_SUCCESS;
}
