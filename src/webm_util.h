#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "webm/callback.h"
#include "webm/file_reader.h"
#include "webm/status.h"
#include "webm/webm_parser.h"

template <typename T>
std::ostream& PrintUnknownEnumValue(std::ostream& os, T value);

// Overloads for operator<< for pretty printing enums.
std::ostream& operator<<(std::ostream& os, webm::Id id);

std::ostream& operator<<(std::ostream& os, webm::Lacing value);

std::ostream& operator<<(std::ostream& os, webm::MatrixCoefficients value);

std::ostream& operator<<(std::ostream& os, webm::Range value);

std::ostream& operator<<(std::ostream& os, webm::TransferCharacteristics value);

std::ostream& operator<<(std::ostream& os, webm::Primaries value);

std::ostream& operator<<(std::ostream& os, webm::ProjectionType value);

std::ostream& operator<<(std::ostream& os, webm::FlagInterlaced value);

std::ostream& operator<<(std::ostream& os, webm::StereoMode value);

std::ostream& operator<<(std::ostream& os, webm::DisplayUnit value);

std::ostream& operator<<(std::ostream& os, webm::AspectRatioType value);

std::ostream& operator<<(std::ostream& os, webm::AesSettingsCipherMode value);

std::ostream& operator<<(std::ostream& os, webm::ContentEncAlgo value);

std::ostream& operator<<(std::ostream& os, webm::ContentEncodingType value);

std::ostream& operator<<(std::ostream& os, webm::TrackType value);

// For binary elements, just print out its size.
std::ostream& operator<<(std::ostream& os,  const std::vector<std::uint8_t>& value);