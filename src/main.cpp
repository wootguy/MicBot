#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "steam/steam_api.h"

void DumpMicData()
{
	// read local microphone input
	uint32 nBytesAvailable = 0;
	EVoiceResult res = SteamUser()->GetAvailableVoice(&nBytesAvailable, NULL, 0);

	if (res == k_EVoiceResultOK && nBytesAvailable > 0)
	{
		uint32 nBytesWritten = 0;

		// more than 500 bytes usually causes an overflow
		uint8 buffer[500];

		res = SteamUser()->GetVoice(true, buffer, 500, &nBytesWritten, false, NULL, 0, NULL, 0);

		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		uint32_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		timestamp = timestamp & 0xffff;

		if (res == k_EVoiceResultOK && nBytesWritten > 0)
		{
			std::cout << std::setfill('0') << std::setw(4) << timestamp;
			for (int i = 0; i < nBytesWritten; i++) {
				//std::cout << (unsigned int)buffer[i] << " ";
				std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)buffer[i];
			}
			std::cout << std::endl;
			//std::cout << nBytesWritten << std::endl;
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
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}

	SteamAPI_Shutdown();

	return EXIT_SUCCESS;
}
