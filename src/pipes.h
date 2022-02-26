#pragma once
#include <string>
#include <mutex>
#include "ThreadInputBuffer.h"

// create an input pipe and return its os-specific name
std::string createInputPipe(std::string id);

// read data from pipe into a buffer which the main thread can read from safely
void readPipe(std::string pipeName, ThreadInputBuffer* inputBuffer);