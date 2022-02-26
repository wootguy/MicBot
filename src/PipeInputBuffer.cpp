#include "PipeInputBuffer.h"
#include "pipes.h"
#include <thread>

using namespace std;

PipeInputBuffer::PipeInputBuffer(string pipeName, size_t bufferSize)
{	
	this->bufferSize = bufferSize;
	this->writePos = 0;
	this->readPos = bufferSize; // no data left in buffer initially
	this->status.setValue(PIB_WRITE);
	writeBuffer = new char[bufferSize];
	readBuffer = new char[bufferSize];

	this->pipeName = createInputPipe(pipeName);
	this->inputThread = thread(readPipe, this->pipeName, this);
}

int PipeInputBuffer::read(char* outputBuffer, size_t readSize)
{
	size_t canRead = ::min(bufferSize - readPos, readSize);

	if (readSize > bufferSize) {
		return -1; // too large of a read for configured buffer size
	}
	
	if (canRead >= readSize) {
		memcpy(outputBuffer, readBuffer + readPos, canRead);
		readPos += canRead;
		return 0;
	}

	if (status.getValue() != PIB_FULL) {
		// need to grab input from write buffer, but can't because it's currently being written
		return -2;
	}

	memcpy(outputBuffer, readBuffer + readPos, canRead); // read what's left of the read buffer

	status.setValue(PIB_READ);
	memcpy(readBuffer, writeBuffer, bufferSize); // refill read buffer
	printf("Refilled read buffer\n");
	status.setValue(PIB_WRITE);

	// read what's left of readSize
	size_t readLeft = readSize - canRead;
	memcpy(outputBuffer + canRead, readBuffer, readLeft);
	readPos = readLeft;

	return 0;
}

size_t PipeInputBuffer::write(char* inputBuffer, size_t inputSize)
{
	if (status.getValue() != PIB_WRITE) {
		return 0;
	}

	size_t canWrite = ::min(bufferSize - writePos, inputSize);
	memcpy(writeBuffer + writePos, inputBuffer, canWrite);
	writePos += canWrite;

	if (writePos >= bufferSize) {
		printf("Filled write buffer\n");
		status.setValue(PIB_FULL);
		writePos = 0;
	}

	return canWrite;
}
