#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "webm/callback.h"
#include "webm/file_reader.h"
#include "webm/status.h"
#include "webm/webm_parser.h"

using namespace webm;  // NOLINT

template <typename T>
std::ostream& PrintUnknownEnumValue(std::ostream& os, T value) {
    return os << std::to_string(static_cast<std::uint64_t>(value)) << " (?)";
}

// Overloads for operator<< for pretty printing enums.
std::ostream& operator<<(std::ostream& os, Id id) {
    switch (id) {
    case Id::kEbml:
        return os << "EBML";
    case Id::kEbmlVersion:
        return os << "EBMLVersion";
    case Id::kEbmlReadVersion:
        return os << "EBMLReadVersion";
    case Id::kEbmlMaxIdLength:
        return os << "EBMLMaxIDLength";
    case Id::kEbmlMaxSizeLength:
        return os << "EBMLMaxSizeLength";
    case Id::kDocType:
        return os << "DocType";
    case Id::kDocTypeVersion:
        return os << "DocTypeVersion";
    case Id::kDocTypeReadVersion:
        return os << "DocTypeReadVersion";
    case Id::kVoid:
        return os << "Void";
    case Id::kSegment:
        return os << "Segment";
    case Id::kSeekHead:
        return os << "SeekHead";
    case Id::kSeek:
        return os << "Seek";
    case Id::kSeekId:
        return os << "SeekID";
    case Id::kSeekPosition:
        return os << "SeekPosition";
    case Id::kInfo:
        return os << "Info";
    case Id::kTimecodeScale:
        return os << "TimecodeScale";
    case Id::kDuration:
        return os << "Duration";
    case Id::kDateUtc:
        return os << "DateUTC";
    case Id::kTitle:
        return os << "Title";
    case Id::kMuxingApp:
        return os << "MuxingApp";
    case Id::kWritingApp:
        return os << "WritingApp";
    case Id::kCluster:
        return os << "Cluster";
    case Id::kTimecode:
        return os << "Timecode";
    case Id::kPrevSize:
        return os << "PrevSize";
    case Id::kSimpleBlock:
        return os << "SimpleBlock";
    case Id::kBlockGroup:
        return os << "BlockGroup";
    case Id::kBlock:
        return os << "Block";
    case Id::kBlockVirtual:
        return os << "BlockVirtual";
    case Id::kBlockAdditions:
        return os << "BlockAdditions";
    case Id::kBlockMore:
        return os << "BlockMore";
    case Id::kBlockAddId:
        return os << "BlockAddID";
    case Id::kBlockAdditional:
        return os << "BlockAdditional";
    case Id::kBlockDuration:
        return os << "BlockDuration";
    case Id::kReferenceBlock:
        return os << "ReferenceBlock";
    case Id::kDiscardPadding:
        return os << "DiscardPadding";
    case Id::kSlices:
        return os << "Slices";
    case Id::kTimeSlice:
        return os << "TimeSlice";
    case Id::kLaceNumber:
        return os << "LaceNumber";
    case Id::kTracks:
        return os << "Tracks";
    case Id::kTrackEntry:
        return os << "TrackEntry";
    case Id::kTrackNumber:
        return os << "TrackNumber";
    case Id::kTrackUid:
        return os << "TrackUID";
    case Id::kTrackType:
        return os << "TrackType";
    case Id::kFlagEnabled:
        return os << "FlagEnabled";
    case Id::kFlagDefault:
        return os << "FlagDefault";
    case Id::kFlagForced:
        return os << "FlagForced";
    case Id::kFlagLacing:
        return os << "FlagLacing";
    case Id::kDefaultDuration:
        return os << "DefaultDuration";
    case Id::kName:
        return os << "Name";
    case Id::kLanguage:
        return os << "Language";
    case Id::kCodecId:
        return os << "CodecID";
    case Id::kCodecPrivate:
        return os << "CodecPrivate";
    case Id::kCodecName:
        return os << "CodecName";
    case Id::kCodecDelay:
        return os << "CodecDelay";
    case Id::kSeekPreRoll:
        return os << "SeekPreRoll";
    case Id::kVideo:
        return os << "Video";
    case Id::kFlagInterlaced:
        return os << "FlagInterlaced";
    case Id::kStereoMode:
        return os << "StereoMode";
    case Id::kAlphaMode:
        return os << "AlphaMode";
    case Id::kPixelWidth:
        return os << "PixelWidth";
    case Id::kPixelHeight:
        return os << "PixelHeight";
    case Id::kPixelCropBottom:
        return os << "PixelCropBottom";
    case Id::kPixelCropTop:
        return os << "PixelCropTop";
    case Id::kPixelCropLeft:
        return os << "PixelCropLeft";
    case Id::kPixelCropRight:
        return os << "PixelCropRight";
    case Id::kDisplayWidth:
        return os << "DisplayWidth";
    case Id::kDisplayHeight:
        return os << "DisplayHeight";
    case Id::kDisplayUnit:
        return os << "DisplayUnit";
    case Id::kAspectRatioType:
        return os << "AspectRatioType";
    case Id::kFrameRate:
        return os << "FrameRate";
    case Id::kColour:
        return os << "Colour";
    case Id::kMatrixCoefficients:
        return os << "MatrixCoefficients";
    case Id::kBitsPerChannel:
        return os << "BitsPerChannel";
    case Id::kChromaSubsamplingHorz:
        return os << "ChromaSubsamplingHorz";
    case Id::kChromaSubsamplingVert:
        return os << "ChromaSubsamplingVert";
    case Id::kCbSubsamplingHorz:
        return os << "CbSubsamplingHorz";
    case Id::kCbSubsamplingVert:
        return os << "CbSubsamplingVert";
    case Id::kChromaSitingHorz:
        return os << "ChromaSitingHorz";
    case Id::kChromaSitingVert:
        return os << "ChromaSitingVert";
    case Id::kRange:
        return os << "Range";
    case Id::kTransferCharacteristics:
        return os << "TransferCharacteristics";
    case Id::kPrimaries:
        return os << "Primaries";
    case Id::kMaxCll:
        return os << "MaxCLL";
    case Id::kMaxFall:
        return os << "MaxFALL";
    case Id::kMasteringMetadata:
        return os << "MasteringMetadata";
    case Id::kPrimaryRChromaticityX:
        return os << "PrimaryRChromaticityX";
    case Id::kPrimaryRChromaticityY:
        return os << "PrimaryRChromaticityY";
    case Id::kPrimaryGChromaticityX:
        return os << "PrimaryGChromaticityX";
    case Id::kPrimaryGChromaticityY:
        return os << "PrimaryGChromaticityY";
    case Id::kPrimaryBChromaticityX:
        return os << "PrimaryBChromaticityX";
    case Id::kPrimaryBChromaticityY:
        return os << "PrimaryBChromaticityY";
    case Id::kWhitePointChromaticityX:
        return os << "WhitePointChromaticityX";
    case Id::kWhitePointChromaticityY:
        return os << "WhitePointChromaticityY";
    case Id::kLuminanceMax:
        return os << "LuminanceMax";
    case Id::kLuminanceMin:
        return os << "LuminanceMin";
    case Id::kProjection:
        return os << "Projection";
    case Id::kProjectionType:
        return os << "kProjectionType";
    case Id::kProjectionPrivate:
        return os << "kProjectionPrivate";
    case Id::kProjectionPoseYaw:
        return os << "kProjectionPoseYaw";
    case Id::kProjectionPosePitch:
        return os << "kProjectionPosePitch";
    case Id::kProjectionPoseRoll:
        return os << "ProjectionPoseRoll";
    case Id::kAudio:
        return os << "Audio";
    case Id::kSamplingFrequency:
        return os << "SamplingFrequency";
    case Id::kOutputSamplingFrequency:
        return os << "OutputSamplingFrequency";
    case Id::kChannels:
        return os << "Channels";
    case Id::kBitDepth:
        return os << "BitDepth";
    case Id::kContentEncodings:
        return os << "ContentEncodings";
    case Id::kContentEncoding:
        return os << "ContentEncoding";
    case Id::kContentEncodingOrder:
        return os << "ContentEncodingOrder";
    case Id::kContentEncodingScope:
        return os << "ContentEncodingScope";
    case Id::kContentEncodingType:
        return os << "ContentEncodingType";
    case Id::kContentEncryption:
        return os << "ContentEncryption";
    case Id::kContentEncAlgo:
        return os << "ContentEncAlgo";
    case Id::kContentEncKeyId:
        return os << "ContentEncKeyID";
    case Id::kContentEncAesSettings:
        return os << "ContentEncAESSettings";
    case Id::kAesSettingsCipherMode:
        return os << "AESSettingsCipherMode";
    case Id::kCues:
        return os << "Cues";
    case Id::kCuePoint:
        return os << "CuePoint";
    case Id::kCueTime:
        return os << "CueTime";
    case Id::kCueTrackPositions:
        return os << "CueTrackPositions";
    case Id::kCueTrack:
        return os << "CueTrack";
    case Id::kCueClusterPosition:
        return os << "CueClusterPosition";
    case Id::kCueRelativePosition:
        return os << "CueRelativePosition";
    case Id::kCueDuration:
        return os << "CueDuration";
    case Id::kCueBlockNumber:
        return os << "CueBlockNumber";
    case Id::kChapters:
        return os << "Chapters";
    case Id::kEditionEntry:
        return os << "EditionEntry";
    case Id::kChapterAtom:
        return os << "ChapterAtom";
    case Id::kChapterUid:
        return os << "ChapterUID";
    case Id::kChapterStringUid:
        return os << "ChapterStringUID";
    case Id::kChapterTimeStart:
        return os << "ChapterTimeStart";
    case Id::kChapterTimeEnd:
        return os << "ChapterTimeEnd";
    case Id::kChapterDisplay:
        return os << "ChapterDisplay";
    case Id::kChapString:
        return os << "ChapString";
    case Id::kChapLanguage:
        return os << "ChapLanguage";
    case Id::kChapCountry:
        return os << "ChapCountry";
    case Id::kTags:
        return os << "Tags";
    case Id::kTag:
        return os << "Tag";
    case Id::kTargets:
        return os << "Targets";
    case Id::kTargetTypeValue:
        return os << "TargetTypeValue";
    case Id::kTargetType:
        return os << "TargetType";
    case Id::kTagTrackUid:
        return os << "TagTrackUID";
    case Id::kSimpleTag:
        return os << "SimpleTag";
    case Id::kTagName:
        return os << "TagName";
    case Id::kTagLanguage:
        return os << "TagLanguage";
    case Id::kTagDefault:
        return os << "TagDefault";
    case Id::kTagString:
        return os << "TagString";
    case Id::kTagBinary:
        return os << "TagBinary";
    default:
        return PrintUnknownEnumValue(os, id);
    }
}

std::ostream& operator<<(std::ostream& os, Lacing value) {
    switch (value) {
    case Lacing::kNone:
        return os << "0 (none)";
    case Lacing::kXiph:
        return os << "2 (Xiph)";
    case Lacing::kFixed:
        return os << "4 (fixed)";
    case Lacing::kEbml:
        return os << "6 (EBML)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, MatrixCoefficients value) {
    switch (value) {
    case MatrixCoefficients::kRgb:
        return os << "0 (identity, RGB/XYZ)";
    case MatrixCoefficients::kBt709:
        return os << "1 (Rec. ITU-R BT.709-5)";
    case MatrixCoefficients::kUnspecified:
        return os << "2 (unspecified)";
    case MatrixCoefficients::kFcc:
        return os << "4 (US FCC)";
    case MatrixCoefficients::kBt470Bg:
        return os << "5 (Rec. ITU-R BT.470-6 System B, G)";
    case MatrixCoefficients::kSmpte170M:
        return os << "6 (SMPTE 170M)";
    case MatrixCoefficients::kSmpte240M:
        return os << "7 (SMPTE 240M)";
    case MatrixCoefficients::kYCgCo:
        return os << "8 (YCgCo)";
    case MatrixCoefficients::kBt2020NonconstantLuminance:
        return os << "9 (Rec. ITU-R BT.2020, non-constant luma)";
    case MatrixCoefficients::kBt2020ConstantLuminance:
        return os << "10 (Rec. ITU-R BT.2020 , constant luma)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, Range value) {
    switch (value) {
    case Range::kUnspecified:
        return os << "0 (unspecified)";
    case Range::kBroadcast:
        return os << "1 (broadcast)";
    case Range::kFull:
        return os << "2 (full)";
    case Range::kDerived:
        return os << "3 (defined by MatrixCoefficients/TransferCharacteristics)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, TransferCharacteristics value) {
    switch (value) {
    case TransferCharacteristics::kBt709:
        return os << "1 (Rec. ITU-R BT.709-6)";
    case TransferCharacteristics::kUnspecified:
        return os << "2 (unspecified)";
    case TransferCharacteristics::kGamma22curve:
        return os << "4 (gamma 2.2, Rec. ITU‑R BT.470‑6 System M)";
    case TransferCharacteristics::kGamma28curve:
        return os << "5 (gamma 2.8, Rec. ITU‑R BT.470-6 System B, G)";
    case TransferCharacteristics::kSmpte170M:
        return os << "6 (SMPTE 170M)";
    case TransferCharacteristics::kSmpte240M:
        return os << "7 (SMPTE 240M)";
    case TransferCharacteristics::kLinear:
        return os << "8 (linear)";
    case TransferCharacteristics::kLog:
        return os << "9 (log, 100:1 range)";
    case TransferCharacteristics::kLogSqrt:
        return os << "10 (log, 316.2:1 range)";
    case TransferCharacteristics::kIec6196624:
        return os << "11 (IEC 61966-2-4)";
    case TransferCharacteristics::kBt1361ExtendedColourGamut:
        return os << "12 (Rec. ITU-R BT.1361, extended colour gamut)";
    case TransferCharacteristics::kIec6196621:
        return os << "13 (IEC 61966-2-1, sRGB or sYCC)";
    case TransferCharacteristics::k10BitBt2020:
        return os << "14 (Rec. ITU-R BT.2020-2, 10-bit)";
    case TransferCharacteristics::k12BitBt2020:
        return os << "15 (Rec. ITU-R BT.2020-2, 12-bit)";
    case TransferCharacteristics::kSmpteSt2084:
        return os << "16 (SMPTE ST 2084)";
    case TransferCharacteristics::kSmpteSt4281:
        return os << "17 (SMPTE ST 428-1)";
    case TransferCharacteristics::kAribStdB67Hlg:
        return os << "18 (ARIB STD-B67/Rec. ITU-R BT.[HDR-TV] HLG)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, Primaries value) {
    switch (value) {
    case Primaries::kBt709:
        return os << "1 (Rec. ITU‑R BT.709-6)";
    case Primaries::kUnspecified:
        return os << "2 (unspecified)";
    case Primaries::kBt470M:
        return os << "4 (Rec. ITU‑R BT.470‑6 System M)";
    case Primaries::kBt470Bg:
        return os << "5 (Rec. ITU‑R BT.470‑6 System B, G)";
    case Primaries::kSmpte170M:
        return os << "6 (SMPTE 170M)";
    case Primaries::kSmpte240M:
        return os << "7 (SMPTE 240M)";
    case Primaries::kFilm:
        return os << "8 (generic film)";
    case Primaries::kBt2020:
        return os << "9 (Rec. ITU-R BT.2020-2)";
    case Primaries::kSmpteSt4281:
        return os << "10 (SMPTE ST 428-1)";
    case Primaries::kJedecP22Phosphors:
        return os << "22 (EBU Tech. 3213-E/JEDEC P22 phosphors)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, ProjectionType value) {
    switch (value) {
    case ProjectionType::kRectangular:
        return os << "0 (rectangular)";
    case ProjectionType::kEquirectangular:
        return os << "1 (equirectangular)";
    case ProjectionType::kCubeMap:
        return os << "2 (cube map)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, FlagInterlaced value) {
    switch (value) {
    case FlagInterlaced::kUnspecified:
        return os << "0 (unspecified)";
    case FlagInterlaced::kInterlaced:
        return os << "1 (interlaced)";
    case FlagInterlaced::kProgressive:
        return os << "2 (progressive)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, StereoMode value) {
    switch (value) {
    case StereoMode::kMono:
        return os << "0 (mono)";
    case StereoMode::kSideBySideLeftFirst:
        return os << "1 (side-by-side, left eye first)";
    case StereoMode::kTopBottomRightFirst:
        return os << "2 (top-bottom, right eye first)";
    case StereoMode::kTopBottomLeftFirst:
        return os << "3 (top-bottom, left eye first)";
    case StereoMode::kCheckboardRightFirst:
        return os << "4 (checkboard, right eye first)";
    case StereoMode::kCheckboardLeftFirst:
        return os << "5 (checkboard, left eye first)";
    case StereoMode::kRowInterleavedRightFirst:
        return os << "6 (row interleaved, right eye first)";
    case StereoMode::kRowInterleavedLeftFirst:
        return os << "7 (row interleaved, left eye first)";
    case StereoMode::kColumnInterleavedRightFirst:
        return os << "8 (column interleaved, right eye first)";
    case StereoMode::kColumnInterleavedLeftFirst:
        return os << "9 (column interleaved, left eye first)";
    case StereoMode::kAnaglyphCyanRed:
        return os << "10 (anaglyph, cyan/red)";
    case StereoMode::kSideBySideRightFirst:
        return os << "11 (side-by-side, right eye first)";
    case StereoMode::kAnaglyphGreenMagenta:
        return os << "12 (anaglyph, green/magenta)";
    case StereoMode::kBlockLacedLeftFirst:
        return os << "13 (block laced, left eye first)";
    case StereoMode::kBlockLacedRightFirst:
        return os << "14 (block laced, right eye first)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, DisplayUnit value) {
    switch (value) {
    case DisplayUnit::kPixels:
        return os << "0 (pixels)";
    case DisplayUnit::kCentimeters:
        return os << "1 (centimeters)";
    case DisplayUnit::kInches:
        return os << "2 (inches)";
    case DisplayUnit::kDisplayAspectRatio:
        return os << "3 (display aspect ratio)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, AspectRatioType value) {
    switch (value) {
    case AspectRatioType::kFreeResizing:
        return os << "0 (free resizing)";
    case AspectRatioType::kKeep:
        return os << "1 (keep aspect ratio)";
    case AspectRatioType::kFixed:
        return os << "2 (fixed)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, AesSettingsCipherMode value) {
    switch (value) {
    case AesSettingsCipherMode::kCtr:
        return os << "1 (CTR)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, ContentEncAlgo value) {
    switch (value) {
    case ContentEncAlgo::kOnlySigned:
        return os << "0 (only signed, not encrypted)";
    case ContentEncAlgo::kDes:
        return os << "1 (DES)";
    case ContentEncAlgo::k3Des:
        return os << "2 (3DES)";
    case ContentEncAlgo::kTwofish:
        return os << "3 (Twofish)";
    case ContentEncAlgo::kBlowfish:
        return os << "4 (Blowfish)";
    case ContentEncAlgo::kAes:
        return os << "5 (AES)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, ContentEncodingType value) {
    switch (value) {
    case ContentEncodingType::kCompression:
        return os << "0 (compression)";
    case ContentEncodingType::kEncryption:
        return os << "1 (encryption)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

std::ostream& operator<<(std::ostream& os, TrackType value) {
    switch (value) {
    case TrackType::kVideo:
        return os << "1 (video)";
    case TrackType::kAudio:
        return os << "2 (audio)";
    case TrackType::kComplex:
        return os << "3 (complex)";
    case TrackType::kLogo:
        return os << "16 (logo)";
    case TrackType::kSubtitle:
        return os << "17 (subtitle)";
    case TrackType::kButtons:
        return os << "18 (buttons)";
    case TrackType::kControl:
        return os << "32 (control)";
    default:
        return PrintUnknownEnumValue(os, value);
    }
}

// For binary elements, just print out its size.
std::ostream& operator<<(std::ostream& os,
    const std::vector<std::uint8_t>& value) {
    return os << '<' << value.size() << " bytes>";
}