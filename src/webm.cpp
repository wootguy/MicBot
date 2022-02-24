// Copyright (c) 2016 The WebM project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>

#include "webm/callback.h"
#include "webm/file_reader.h"
#include "webm/status.h"
#include "webm/webm_parser.h"
#include "webm_util.h"
#include "opus/opus.h"

#include "opus_util.h"

// We use pretty much everything in the webm namespace. Just pull
// it all in.
using namespace webm;
using namespace std;

class DemoCallback : public Callback {
public:
    int indent = 0;
    int spaces_per_indent = 2;

    TrackEntry audioTrack;
    OpusDecoder* decoder;
    ofstream outFile;
    vector<int16_t> allSamples;
    bool decodeNextFrame = false;

    void PrintElementMetadata(const std::string& name, const ElementMetadata& metadata) {
        // Since we aren't doing any seeking in this demo, we don't have to worry
        // about kUnknownHeaderSize or kUnknownElementPosition when adding the
        // position and sizes.
        const std::uint64_t header_start = metadata.position;
        const std::uint64_t header_end = header_start + metadata.header_size;
        const std::uint64_t body_start = header_end;
        std::cout << std::string(indent * spaces_per_indent, ' ') << name;
        // The ContentEncAESSettings element has the longest name (out of all other
        // master elements) at 21 characters. It's also the deepest master element
        // at a level of 6. Insert enough whitespace so there's room for it.
        std::cout << std::string(21 + 6 * spaces_per_indent -
            indent * spaces_per_indent - name.size(),
            ' ')
            << "    header: [" << header_start << ", " << header_end
            << ")  body: [" << body_start << ", ";
        if (metadata.size != kUnknownElementSize) {
            const std::uint64_t body_end = body_start + metadata.size;
            std::cout << body_end;
        }
        else {
            std::cout << '?';
        }
        std::cout << ")\n";
    }

    template <typename T>
    void PrintMandatoryElement(const std::string& name,  const Element<T>& element) {
        std::cout << std::string(indent * spaces_per_indent, ' ') << name;
        if (!element.is_present()) {
            std::cout << " (implicit)";
        }
        std::cout << ": " << element.value() << '\n';
    }

    template <typename T>
    void PrintMandatoryElement(const std::string& name, const std::vector<Element<T>>& elements) {
        for (const Element<T>& element : elements) {
            PrintMandatoryElement(name, element);
        }
    }

    template <typename T>
    void PrintOptionalElement(const std::string& name, const Element<T>& element) {
        if (element.is_present()) {
            std::cout << std::string(indent * spaces_per_indent, ' ') << name << ": "
                << element.value() << '\n';
        }
    }

    template <typename T>
    void PrintOptionalElement(const std::string& name,  const std::vector<Element<T>>& elements) {
        for (const Element<T>& element : elements) {
            PrintOptionalElement(name, element);
        }
    }

    void PrintMasterElement(const BlockAdditions& block_additions) {
        PrintMasterElement("BlockMore", block_additions.block_mores);
    }

    void PrintMasterElement(const BlockMore& block_more) {
        PrintMandatoryElement("BlockAddID", block_more.id);
        PrintMandatoryElement("BlockAdditional", block_more.data);
    }

    void PrintMasterElement(const Slices& slices) {
        PrintMasterElement("TimeSlice", slices.slices);
    }

    void PrintMasterElement(const TimeSlice& time_slice) {
        PrintOptionalElement("LaceNumber", time_slice.lace_number);
    }

    void PrintMasterElement(const Video& video) {
        PrintMandatoryElement("FlagInterlaced", video.interlaced);
        PrintOptionalElement("StereoMode", video.stereo_mode);
        PrintOptionalElement("AlphaMode", video.alpha_mode);
        PrintMandatoryElement("PixelWidth", video.pixel_width);
        PrintMandatoryElement("PixelHeight", video.pixel_height);
        PrintOptionalElement("PixelCropBottom", video.pixel_crop_bottom);
        PrintOptionalElement("PixelCropTop", video.pixel_crop_top);
        PrintOptionalElement("PixelCropLeft", video.pixel_crop_left);
        PrintOptionalElement("PixelCropRight", video.pixel_crop_right);
        PrintOptionalElement("DisplayWidth", video.display_width);
        PrintOptionalElement("DisplayHeight", video.display_height);
        PrintOptionalElement("DisplayUnit", video.display_unit);
        PrintOptionalElement("AspectRatioType", video.aspect_ratio_type);
        PrintOptionalElement("FrameRate", video.frame_rate);
        PrintMasterElement("Colour", video.colour);
        PrintMasterElement("Projection", video.projection);
    }

    void PrintMasterElement(const Colour& colour) {
        PrintOptionalElement("MatrixCoefficients", colour.matrix_coefficients);
        PrintOptionalElement("BitsPerChannel", colour.bits_per_channel);
        PrintOptionalElement("ChromaSubsamplingHorz", colour.chroma_subsampling_x);
        PrintOptionalElement("ChromaSubsamplingVert", colour.chroma_subsampling_y);
        PrintOptionalElement("CbSubsamplingHorz", colour.cb_subsampling_x);
        PrintOptionalElement("CbSubsamplingVert", colour.cb_subsampling_y);
        PrintOptionalElement("ChromaSitingHorz", colour.chroma_siting_x);
        PrintOptionalElement("ChromaSitingVert", colour.chroma_siting_y);
        PrintOptionalElement("Range", colour.range);
        PrintOptionalElement("TransferCharacteristics",
            colour.transfer_characteristics);
        PrintOptionalElement("Primaries", colour.primaries);
        PrintOptionalElement("MaxCLL", colour.max_cll);
        PrintOptionalElement("MaxFALL", colour.max_fall);
        PrintMasterElement("MasteringMetadata", colour.mastering_metadata);
    }

    void PrintMasterElement(const MasteringMetadata& mastering_metadata) {
        PrintOptionalElement("PrimaryRChromaticityX",
            mastering_metadata.primary_r_chromaticity_x);
        PrintOptionalElement("PrimaryRChromaticityY",
            mastering_metadata.primary_r_chromaticity_y);
        PrintOptionalElement("PrimaryGChromaticityX",
            mastering_metadata.primary_g_chromaticity_x);
        PrintOptionalElement("PrimaryGChromaticityY",
            mastering_metadata.primary_g_chromaticity_y);
        PrintOptionalElement("PrimaryBChromaticityX",
            mastering_metadata.primary_b_chromaticity_x);
        PrintOptionalElement("PrimaryBChromaticityY",
            mastering_metadata.primary_b_chromaticity_y);
        PrintOptionalElement("WhitePointChromaticityX",
            mastering_metadata.white_point_chromaticity_x);
        PrintOptionalElement("WhitePointChromaticityY",
            mastering_metadata.white_point_chromaticity_y);
        PrintOptionalElement("LuminanceMax", mastering_metadata.luminance_max);
        PrintOptionalElement("LuminanceMin", mastering_metadata.luminance_min);
    }

    void PrintMasterElement(const Projection& projection) {
        PrintMandatoryElement("ProjectionType", projection.type);
        PrintOptionalElement("ProjectionPrivate", projection.projection_private);
        PrintMandatoryElement("ProjectionPoseYaw", projection.pose_yaw);
        PrintMandatoryElement("ProjectionPosePitch", projection.pose_pitch);
        PrintMandatoryElement("ProjectionPoseRoll", projection.pose_roll);
    }

    void PrintMasterElement(const Audio& audio) {
        PrintMandatoryElement("SamplingFrequency", audio.sampling_frequency);
        PrintOptionalElement("OutputSamplingFrequency", audio.output_frequency);
        PrintMandatoryElement("Channels", audio.channels);
        PrintOptionalElement("BitDepth", audio.bit_depth);
    }

    void PrintMasterElement(const ContentEncodings& content_encodings) {
        PrintMasterElement("ContentEncoding", content_encodings.encodings);
    }

    void PrintMasterElement(const ContentEncoding& content_encoding) {
        PrintMandatoryElement("ContentEncodingOrder", content_encoding.order);
        PrintMandatoryElement("ContentEncodingScope", content_encoding.scope);
        PrintMandatoryElement("ContentEncodingType", content_encoding.type);
        PrintMasterElement("ContentEncryption", content_encoding.encryption);
    }

    void PrintMasterElement(const ContentEncryption& content_encryption) {
        PrintOptionalElement("ContentEncAlgo", content_encryption.algorithm);
        PrintOptionalElement("ContentEncKeyID", content_encryption.key_id);
        PrintMasterElement("ContentEncAESSettings",
            content_encryption.aes_settings);
    }

    void PrintMasterElement(const ContentEncAesSettings& content_enc_aes_settings) {
        PrintMandatoryElement("AESSettingsCipherMode",
            content_enc_aes_settings.aes_settings_cipher_mode);
    }

    void PrintMasterElement(const CueTrackPositions& cue_track_positions) {
        PrintMandatoryElement("CueTrack", cue_track_positions.track);
        PrintMandatoryElement("CueClusterPosition",
            cue_track_positions.cluster_position);
        PrintOptionalElement("CueRelativePosition",
            cue_track_positions.relative_position);
        PrintOptionalElement("CueDuration", cue_track_positions.duration);
        PrintOptionalElement("CueBlockNumber", cue_track_positions.block_number);
    }

    void PrintMasterElement(const ChapterAtom& chapter_atom) {
        PrintMandatoryElement("ChapterUID", chapter_atom.uid);
        PrintOptionalElement("ChapterStringUID", chapter_atom.string_uid);
        PrintMandatoryElement("ChapterTimeStart", chapter_atom.time_start);
        PrintOptionalElement("ChapterTimeEnd", chapter_atom.time_end);
        PrintMasterElement("ChapterDisplay", chapter_atom.displays);
        PrintMasterElement("ChapterAtom", chapter_atom.atoms);
    }

    void PrintMasterElement(const ChapterDisplay& chapter_display) {
        PrintMandatoryElement("ChapString", chapter_display.string);
        PrintMandatoryElement("ChapLanguage", chapter_display.languages);
        PrintOptionalElement("ChapCountry", chapter_display.countries);
    }

    void PrintMasterElement(const Targets& targets) {
        PrintOptionalElement("TargetTypeValue", targets.type_value);
        PrintOptionalElement("TargetType", targets.type);
        PrintMandatoryElement("TagTrackUID", targets.track_uids);
    }

    void PrintMasterElement(const SimpleTag& simple_tag) {
        PrintMandatoryElement("TagName", simple_tag.name);
        PrintMandatoryElement("TagLanguage", simple_tag.language);
        PrintMandatoryElement("TagDefault", simple_tag.is_default);
        PrintOptionalElement("TagString", simple_tag.string);
        PrintOptionalElement("TagBinary", simple_tag.binary);
        PrintMasterElement("SimpleTag", simple_tag.tags);
    }

    // When printing a master element that's wrapped in Element<>, peel off the
    // Element<> wrapper and print the underlying master element if it's present.
    template <typename T>
    void PrintMasterElement(const std::string& name, const Element<T>& element) {
        if (element.is_present()) {
            std::cout << std::string(indent * spaces_per_indent, ' ') << name << "\n";
            ++indent;
            PrintMasterElement(element.value());
            --indent;
        }
    }

    template <typename T>
    void PrintMasterElement(const std::string& name, const std::vector<Element<T>>& elements) {
        for (const Element<T>& element : elements) {
            PrintMasterElement(name, element);
        }
    }

    template <typename T>
    void PrintValue(const std::string& name, const T& value) {
        std::cout << std::string(indent * spaces_per_indent, ' ') << name << ": "
            << value << '\n';
    }

    Status OnElementBegin(const ElementMetadata& metadata, Action* action) override {
        // Print out metadata for some level 1 elements that don't have explicit
        // callbacks.
        switch (metadata.id) {
        case Id::kSeekHead:
            indent = 1;
            PrintElementMetadata("SeekHead", metadata);
            break;
        case Id::kTracks:
            indent = 1;
            PrintElementMetadata("Tracks", metadata);
            break;
        case Id::kCues:
            indent = 1;
            PrintElementMetadata("Cues", metadata);
            break;
        case Id::kChapters:
            indent = 1;
            PrintElementMetadata("Chapters", metadata);
            break;
        case Id::kTags:
            indent = 1;
            PrintElementMetadata("Tags", metadata);
            break;
        default:
            break;
        }

        *action = Action::kRead;
        return Status(Status::kOkCompleted);
    }

    Status OnUnknownElement(const ElementMetadata& metadata, Reader* reader, std::uint64_t* bytes_remaining) override {
        // Output unknown elements without any indentation because we aren't
        // tracking which element contains them.
        int original_indent = indent;
        indent = 0;
        PrintElementMetadata("UNKNOWN_ELEMENT!", metadata);
        indent = original_indent;
        // The base class's implementation will just skip the element via
        // Reader::Skip().
        return Callback::OnUnknownElement(metadata, reader, bytes_remaining);
    }

    Status OnEbml(const ElementMetadata& metadata, const Ebml& ebml) override {
        indent = 0;
        PrintElementMetadata("EBML", metadata);
        indent = 1;
        PrintMandatoryElement("EBMLVersion", ebml.ebml_version);
        PrintMandatoryElement("EBMLReadVersion", ebml.ebml_read_version);
        PrintMandatoryElement("EBMLMaxIDLength", ebml.ebml_max_id_length);
        PrintMandatoryElement("EBMLMaxSizeLength", ebml.ebml_max_size_length);
        PrintMandatoryElement("DocType", ebml.doc_type);
        PrintMandatoryElement("DocTypeVersion", ebml.doc_type_version);
        PrintMandatoryElement("DocTypeReadVersion", ebml.doc_type_read_version);
        return Status(Status::kOkCompleted);
    }

    Status OnVoid(const ElementMetadata& metadata, Reader* reader, std::uint64_t* bytes_remaining) override {
        // Output Void elements without any indentation because we aren't tracking
        // which element contains them.
        int original_indent = indent;
        indent = 0;
        PrintElementMetadata("Void", metadata);
        indent = original_indent;
        // The base class's implementation will just skip the element via
        // Reader::Skip().
        return Callback::OnVoid(metadata, reader, bytes_remaining);
    }

    Status OnSegmentBegin(const ElementMetadata& metadata, Action* action) override {
        indent = 0;
        PrintElementMetadata("Segment", metadata);
        indent = 1;
        *action = Action::kRead;
        return Status(Status::kOkCompleted);
    }

    Status OnSeek(const ElementMetadata& metadata, const Seek& seek) override {
        indent = 2;
        PrintElementMetadata("Seek", metadata);
        indent = 3;
        PrintMandatoryElement("SeekID", seek.id);
        PrintMandatoryElement("SeekPosition", seek.position);
        return Status(Status::kOkCompleted);
    }

    Status OnInfo(const ElementMetadata& metadata, const Info& info) override {
        indent = 1;
        PrintElementMetadata("Info", metadata);
        indent = 2;
        PrintMandatoryElement("TimecodeScale", info.timecode_scale);
        PrintOptionalElement("Duration", info.duration);
        PrintOptionalElement("DateUTC", info.date_utc);
        PrintOptionalElement("Title", info.title);
        PrintOptionalElement("MuxingApp", info.muxing_app);
        PrintOptionalElement("WritingApp", info.writing_app);
        return Status(Status::kOkCompleted);
    }

    Status OnClusterBegin(const ElementMetadata& metadata, const Cluster& cluster, Action* action) override {
        indent = 1;
        PrintElementMetadata("Cluster", metadata);
        // A properly muxed file will have Timecode and PrevSize first before any
        // SimpleBlock or BlockGroups. The parser takes advantage of this and delays
        // calling OnClusterBegin() until it hits the first SimpleBlock or
        // BlockGroup child (or the Cluster ends if it's empty). It's possible for
        // an improperly muxed file to have Timecode or PrevSize after the first
        // block, in which case they'll be absent here and may be accessed in
        // OnClusterEnd() when the Cluster and all its children have been fully
        // parsed. In this demo we assume the file has been properly muxed.
        indent = 2;
        PrintMandatoryElement("Timecode", cluster.timecode);
        PrintOptionalElement("PrevSize", cluster.previous_size);
        *action = Action::kRead;
        return Status(Status::kOkCompleted);
    }

    Status OnSimpleBlockBegin(const ElementMetadata& metadata, const SimpleBlock& simple_block, Action* action) override {
        /*
        indent = 2;
        PrintElementMetadata("SimpleBlock", metadata);
        indent = 3;
        PrintValue("track number", simple_block.track_number);
        PrintValue("frames", simple_block.num_frames);
        PrintValue("timecode", simple_block.timecode);
        PrintValue("lacing", simple_block.lacing);
        std::string flags = (simple_block.is_visible) ? "visible" : "invisible";
        if (simple_block.is_key_frame)
            flags += ", key frame";
        if (simple_block.is_discardable)
            flags += ", discardable";
        PrintValue("flags", flags);
        */
        decodeNextFrame = simple_block.track_number == audioTrack.track_number.value();

        if (simple_block.num_frames > 1) {
            printf("ZOMG WHAT\n");
        }

        *action = Action::kRead;
        return Status(Status::kOkCompleted);
    }

    Status OnSimpleBlockEnd(const ElementMetadata& metadata , const SimpleBlock& simple_block ) override {
        return Status(Status::kOkCompleted);
    }

    Status OnBlockGroupBegin(const ElementMetadata& metadata, Action* action) override {
        indent = 2;
        PrintElementMetadata("BlockGroup", metadata);
        *action = Action::kRead;
        return Status(Status::kOkCompleted);
    }

    Status OnBlockBegin(const ElementMetadata& metadata, const Block& block, Action* action) override {
        indent = 3;
        PrintElementMetadata("Block", metadata);
        indent = 4;
        PrintValue("track number", block.track_number);
        PrintValue("frames", block.num_frames);
        PrintValue("timecode", block.timecode);
        PrintValue("lacing", block.lacing);
        PrintValue("flags", (block.is_visible) ? "visible" : "invisible");
        *action = Action::kRead;
        return Status(Status::kOkCompleted);
    }

    Status OnBlockEnd(const ElementMetadata& /* metadata */, const Block& /* block */) override {
        return Status(Status::kOkCompleted);
    }

    Status OnBlockGroupEnd(const ElementMetadata& /* metadata */, const BlockGroup& block_group) override {
        if (block_group.virtual_block.is_present()) {
            std::cout << std::string(indent * spaces_per_indent, ' ')
                << "BlockVirtual\n";
            indent = 4;
            PrintValue("track number",
                block_group.virtual_block.value().track_number);
            PrintValue("timecode", block_group.virtual_block.value().timecode);
        }
        indent = 3;
        PrintMasterElement("BlockAdditions", block_group.additions);
        PrintOptionalElement("BlockDuration", block_group.duration);
        PrintOptionalElement("ReferenceBlock", block_group.references);
        PrintOptionalElement("DiscardPadding", block_group.discard_padding);
        PrintMasterElement("Slices", block_group.slices);
        // BlockGroup::block has been set, but we've already printed it in
        // OnBlockBegin().
        return Status(Status::kOkCompleted);
    }

    Status OnFrame(const FrameMetadata& metadata, Reader* reader, std::uint64_t* bytes_remaining) override {
        //PrintValue("frame byte range",
        //    '[' + std::to_string(metadata.position) + ", " +
        //    std::to_string(metadata.position + metadata.size) + ')');
        // The base class's implementation will just skip the frame via
        // Reader::Skip().

        
        if (decodeNextFrame) {
            static uint8_t inbuffer[8192];
            static int16_t outbuffer[8192];
            static uint64_t actualRead;

            reader->Read(metadata.size, inbuffer, &actualRead);
            *bytes_remaining = metadata.size - actualRead;

            if (actualRead < metadata.size) {
                printf("NOT ENOUGH READ\n");
            }

            int nBytes = opus_decode(decoder, (uint8_t*)inbuffer, actualRead, outbuffer, 8192, 0);

            if (nBytes < 0) {
                printf("Failed to decode %d\n", nBytes);
            }

            for (int i = 0; i < nBytes * audioTrack.audio.value().channels.value(); i++) {
                allSamples.push_back(outbuffer[i]);
            }

            if (allSamples.size() % (32768) == 0) {
                WriteOutputWav();
            }
        }
        else {
            printf("SKIPPED FRAME\n");
        }

        return Callback::OnFrame(metadata, reader, bytes_remaining);
    }

    Status OnClusterEnd(const ElementMetadata& /* metadata */, const Cluster& /* cluster */) override {
        // The Cluster and all its children have been fully parsed at this point. If
        // the file wasn't properly muxed and Timecode or PrevSize were missing in
        // OnClusterBegin(), they'll be set here (if the Cluster contained them). In
        // this demo we already handled them, though.
        return Status(Status::kOkCompleted);
    }

    Status OnTrackEntry(const ElementMetadata& metadata, const TrackEntry& track_entry) override {
        indent = 2;
        PrintElementMetadata("TrackEntry", metadata);
        indent = 3;
        PrintMandatoryElement("TrackNumber", track_entry.track_number);
        PrintMandatoryElement("TrackUID", track_entry.track_uid);
        PrintMandatoryElement("TrackType", track_entry.track_type);
        PrintMandatoryElement("FlagEnabled", track_entry.is_enabled);
        PrintMandatoryElement("FlagDefault", track_entry.is_default);
        PrintMandatoryElement("FlagForced", track_entry.is_forced);
        PrintMandatoryElement("FlagLacing", track_entry.uses_lacing);
        PrintOptionalElement("DefaultDuration", track_entry.default_duration);
        PrintOptionalElement("Name", track_entry.name);
        PrintOptionalElement("Language", track_entry.language);
        PrintMandatoryElement("CodecID", track_entry.codec_id);
        PrintOptionalElement("CodecPrivate", track_entry.codec_private);
        PrintOptionalElement("CodecName", track_entry.codec_name);
        PrintOptionalElement("CodecDelay", track_entry.codec_delay);
        PrintMandatoryElement("SeekPreRoll", track_entry.seek_pre_roll);
        PrintMasterElement("Video", track_entry.video);
        PrintMasterElement("Audio", track_entry.audio);
        PrintMasterElement("ContentEncodings", track_entry.content_encodings);

        if (track_entry.track_type.value() == TrackType::kAudio) {
            if (audioTrack.track_type.is_present()) {
                printf("UH OH MULTIPLE AUDIO TRACKS\n");
            }
            audioTrack = track_entry;

            int* err = NULL;
            Audio audio = audioTrack.audio.value();
            int32_t rate = audio.sampling_frequency.value();
            int channels = audio.channels.value();
            decoder = opus_decoder_create(rate, channels, err);

            if (err != OPUS_OK) {
                printf("Failed to create decoder\n");
            }

            if (audio.bit_depth.value() != 16) {
                printf("ZOMG NOT 16bit\n");
            }
        }

        return Status(Status::kOkCompleted);
    }

    Status OnCuePoint(const ElementMetadata& metadata, const CuePoint& cue_point) override {
        return Status(Status::kOkCompleted);

        indent = 2;
        PrintElementMetadata("CuePoint", metadata);
        indent = 3;
        PrintMandatoryElement("CueTime", cue_point.time);
        PrintMasterElement("CueTrackPositions", cue_point.cue_track_positions);
        return Status(Status::kOkCompleted);
    }

    Status OnEditionEntry(const ElementMetadata& metadata, const EditionEntry& edition_entry) override {
        indent = 2;
        PrintElementMetadata("EditionEntry", metadata);
        indent = 3;
        PrintMasterElement("ChapterAtom", edition_entry.atoms);
        return Status(Status::kOkCompleted);
    }

    Status OnTag(const ElementMetadata& metadata, const Tag& tag) override {
        indent = 2;
        PrintElementMetadata("Tag", metadata);
        indent = 3;
        PrintMasterElement("Targets", tag.targets);
        PrintMasterElement("SimpleTag", tag.tags);
        return Status(Status::kOkCompleted);
    }

    Status OnSegmentEnd(const ElementMetadata& /* metadata */) override {
        WriteOutputWav();
        return Status(Status::kOkCompleted);
    }

    void WriteOutputWav() {
        wav_hdr header;
        header.ChunkSize = allSamples.size() * 2 + sizeof(wav_hdr) - 8;
        header.Subchunk2Size = allSamples.size() * 2 + sizeof(wav_hdr) - 44;

        string fname = "webm.wav";
        ofstream out(fname, ios::binary);
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));

        for (int z = 0; z < allSamples.size(); z++) {
            out.write(reinterpret_cast<char*>(&allSamples[z]), sizeof(int16_t));
        }

        out.close();
        printf("Wrote file!\n");
    }
};

int webm_test() {
    FILE* file = std::fopen("test.webm", "rb");

    if (!file) {
        std::cerr << "File cannot be opened\n";
        return 1;
    }

    FileReader reader(file);
    DemoCallback callback;
    WebmParser parser;
    Status status = parser.Feed(&callback, &reader);
    if (!status.completed_ok()) {
        std::cerr << "Parsing error; status code: " << status.code << '\n';
        return 1;
    }

    return 0;
}
