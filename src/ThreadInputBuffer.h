#pragma once
#include "ThreadSafeInt.h"
#include <thread>

enum BUFFER_STATUS {
	PIB_FULL,  // input buffer is full and ready to be read
	PIB_WRITE, // input buffer can be or is currently being written to by a pipe thread.
	PIB_READ,  // input buffer is currently being read from the main thread. No writing allowed.
	PIB_FLUSH, // input buffer can't be written to anymore. Reader should take whatever is left.
};

class ThreadInputBuffer {
public:
	ThreadInputBuffer(size_t bufferSize);

	// call from the main thread to read data from a pipe
	// returns 0 on success, 1 for failure
	int read(char* outputBuffer, size_t readSize);

	// call from the pipe thread to add data to the buffer
	// returns bytes that were actually written
	size_t write(char* inputBuffer, size_t inputSize);

	// finish a write before the buffer is full
	void flush();

	void startPipeInputThread(std::string pipeName);

	void startMp3InputThread(std::string fileName, int sampleRate);

private:
	std::string resourceName; // pipe or file name
	std::thread inputThread;

	ThreadSafeInt status; // used to prevent multiple threads reading/writing to the writeBuffer

	char* writeBuffer; // data written to by pipe thread
	char* readBuffer;  // data which can be read while threadData is written
	size_t bufferSize;
	size_t writePos;
	size_t readPos;
};