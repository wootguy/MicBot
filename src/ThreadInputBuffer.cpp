#include "ThreadInputBuffer.h"
#include "pipes.h"
#include "stream_mp3.h"
#include <thread>

using namespace std;

ThreadInputBuffer::ThreadInputBuffer(size_t bufferSize)
{	
	this->bufferSize = bufferSize;
	this->writePos = 0;
	this->readPos = bufferSize; // no data left in buffer initially
	this->status.setValue(TIB_WRITE);
	writeBuffer = new char[bufferSize];
	readBuffer = new char[bufferSize];
}

ThreadInputBuffer::~ThreadInputBuffer()
{
	inputThread.join();
	delete[] writeBuffer;
	delete[] readBuffer;
}

int ThreadInputBuffer::read(char* outputBuffer, size_t readSize)
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

	int curStatus = status.getValue();
	bool isFlushing = curStatus == TIB_FLUSH;

	if (curStatus != TIB_FULL && !isFlushing) {
		// need to grab input from write buffer, but can't because it's currently being written
		if (curStatus == TIB_FLUSHED) {
			size_t cantRead = readSize - canRead;
			memcpy(outputBuffer, readBuffer + readPos, canRead);
			memset(outputBuffer + canRead, 0, cantRead);
			status.setValue(TIB_FINISHED);
			return 0;
		}
		return -2;
	}

	memcpy(outputBuffer, readBuffer + readPos, canRead); // read what's left of the read buffer

	status.setValue(TIB_READ);
	memcpy(readBuffer, writeBuffer, bufferSize); // refill read buffer
	//fprintf(stderr, "Refilled read buffer\n");
	status.setValue(isFlushing ? TIB_FLUSHED : TIB_WRITE);
	if (isFlushing)
		fprintf(stderr, "Flushed output\n");

	// read what's left of readSize
	size_t readLeft = readSize - canRead;
	memcpy(outputBuffer + canRead, readBuffer, readLeft);
	readPos = readLeft;

	return 0;
}

size_t ThreadInputBuffer::write(char* inputBuffer, size_t inputSize)
{
	if (status.getValue() != TIB_WRITE) {
		return 0;
	}

	size_t canWrite = ::min(bufferSize - writePos, inputSize);
	memcpy(writeBuffer + writePos, inputBuffer, canWrite);
	writePos += canWrite;

	if (writePos >= bufferSize) {
		//fprintf(stderr, "Filled write buffer\n");
		status.setValue(TIB_FULL);
		writePos = 0;
	}

	return canWrite;
}

void ThreadInputBuffer::flush()
{
	if (status.getValue() == TIB_WRITE) {
		memset(writeBuffer + writePos, 0, bufferSize - writePos);
		status.setValue(TIB_FLUSH);
	}
}

void ThreadInputBuffer::startPipeInputThread(std::string pipeName)
{
	this->resourceName = pipeName;
	this->inputThread = thread(readPipe, pipeName, this);
}

void ThreadInputBuffer::startMp3InputThread(std::string fileName, int sampleRate)
{
	this->resourceName = fileName;
	this->inputThread = thread(streamMp3, fileName, this, sampleRate);
}

bool ThreadInputBuffer::isFinished()
{
	return status.getValue() == TIB_FINISHED;
}
