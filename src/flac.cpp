/**
 * Copyright (c) 2022 - 2022, Daniel Fedai Larsen.
 *
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "wav.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t byte_t;

// Convert N whole bytes to an uint32_t
// Input byte stream must be big-endian (most significant byte first)
// Returns uint32_t value
uint32_t unpack_uint32_big_endian(byte_t* bytes, uint8_t byte_count)
{
    // Cannot put more than 4 bytes into a uint32_t
    assert(byte_count <= sizeof(uint32_t));

    uint32_t value = 0;
    for(uint8_t i = 0; i < byte_count; i++)
    {
        // 1) Make room for 1 byte
        value = value << 8;
        // 2) OR with next byte
        value |= (uint32_t)(*bytes);
        // 3) Read 1 byte
        bytes += 1;
    }

    return value;
}

// Convert N whole bytes to an uint64_t
// Input byte stream must be big-endian (most significant byte first)
// Returns uint64_t value
uint64_t unpack_uint64_big_endian(byte_t* bytes, uint8_t byte_count)
{
    // Cannot put more than 8 bytes into a uint64_t
    assert(byte_count <= sizeof(uint64_t));

    uint64_t value = 0;
    for(uint8_t i = 0; i < byte_count; i++)
    {
        // 1) Make room for 1 more byte
        value = value << 8;
        // 2) OR with next byte
        value |= (uint64_t)(*bytes);
        // 3) Read 1 byte
        bytes += 1;
    }

    return value;
}

// Convert an UTF-8 byte stream to an uint32_t
// https://en.wikipedia.org/wiki/UTF-8#Encoding
// Returns uint32_t value
uint64_t unpack_utf8_to_uint32(byte_t* bytes, uint32_t* value)
{
    uint8_t byte_count = 0;
    if ((*bytes & 0b10000000) == 0) // First bit isn't set
    {
        byte_count = 1;
    }
    else if (((*bytes & 0b11000000) == 0b11000000) && // First 2 bits are set
             ((*bytes & 0b00100000) != 0b00100000)) // Third bit isn't set
    {
        byte_count = 2;
    }
    else if (((*bytes & 0b11100000) == 0b11100000) && // First 3 bits are set
             ((*bytes & 0b00010000) != 0b00010000)) // Fourth bit isn't set
    {
        byte_count = 3;
    }
    else if (((*bytes & 0b11110000) == 0b11110000) && // First 4 bits are set
             ((*bytes & 0b00001000) != 0b00001000)) // Fifth bit isn't set
    {
        byte_count = 4;
    }
    else
    {
        printf("%s:%i Invalid length of UTF-8 stream for unpacking into uint32_t\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    uint32_t tmp_value = 0;
    for (uint8_t i = 0; i < byte_count; i++)
    {
        byte_t* byte = bytes + i;

        // Make room for 6 bits
        tmp_value = tmp_value << 6;
        // Only use last 6 bits
        tmp_value |= ((uint64_t)(*byte)) & 0x3F;
    }

    *value = tmp_value;
    return (uint64_t)byte_count;
}

// Convert an UTF-8 byte stream to an uint64_t
// https://en.wikipedia.org/wiki/UTF-8#Encoding
// Returns uint64_t value
uint64_t unpack_utf8_to_uint64(byte_t* bytes, uint64_t* value)
{
    uint8_t byte_count = 0;
    if ((*bytes & 0b10000000) == 0) // First bit isn't set
    {
        byte_count = 1;
    }
    else if (((*bytes & 0b11000000) == 0b11000000) && // First 2 bits are set
             ((*bytes & 0b00100000) != 0b00100000)) // Third bit isn't set
    {
        byte_count = 2;
    }
    else if (((*bytes & 0b11100000) == 0b11100000) && // First 3 bits are set
             ((*bytes & 0b00010000) != 0b00010000)) // Fourth bit isn't set
    {
        byte_count = 3;
    }
    else if (((*bytes & 0b11110000) == 0b11110000) && // First 4 bits are set
             ((*bytes & 0b00001000) != 0b00001000)) // Fifth bit isn't set
    {
        byte_count = 4;
    }
    else if (((*bytes & 0b11111000) == 0b11111000) && // First 5 bits are set
             ((*bytes & 0b00000100) != 0b00000100)) // Sixth bit isn't set
    {
        byte_count = 5;
    }
    else if (((*bytes & 0b11111100) == 0b11111100) && // First 6 bits are set
             ((*bytes & 0b00000010) != 0b00000010)) // Seventh bit isn't set
    {
        byte_count = 6;
    }
    else if (((*bytes & 0b11111110) == 0b11111110) && // First 7 bits are set
             ((*bytes & 0b00000001) != 0b00000001)) // Eigth bit isn't set
    {
        byte_count = 7;
    }
    else
    {
        printf("%s:%i Invalid length of UTF-8 stream for unpacking into uint64_t\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    uint64_t tmp_value = 0;
    for (uint8_t i = 0; i < byte_count; i++)
    {
        byte_t* byte = bytes + i;

        // Make room for 6 bits
        tmp_value = tmp_value << 6;
        // Only use last 6 bits
        tmp_value |= ((uint64_t)(*byte)) & 0x3F;
    }

    *value = tmp_value;
    return (uint64_t)byte_count;
}

// Convert N bits to an uint32_t
// Returns bytes read
uint64_t unpack_bits_to_uint32(byte_t* bytes, uint32_t bit_count, uint8_t bit_current, uint8_t* bit_end, uint32_t* value)
{
    // Cannot put more than 32 bits into an uint32_t
    assert(bit_count <= (sizeof(uint32_t) * 8));

    byte_t* bytes_start = bytes;
    uint32_t tmp_value = 0;
    
    // The starting bit is within a byte
    if (bit_current > 0)
    {
        uint32_t remaining_bits_in_byte = (8 - bit_current);
        
        // Determine how many bits to read in byte
        uint32_t bits_to_read = 0;
        if (bit_count < remaining_bits_in_byte) // Reading fewer bits than there are left in byte
        {
            bits_to_read = bit_count;
        }
        else // Reading more bits than there are left in byte
        {
            bits_to_read = remaining_bits_in_byte;
        }
        
        // Mask out bits before the current bit
        uint32_t mask = 0xFF >> bit_current;
        // Potentially mask out bits at end of byte that aren't to be read
        for (uint32_t i = bit_current + bits_to_read; i < 8; i++)
        {
            mask &= 0xFF << (8 - i);
        }
        
        // 1) Extract bits
        uint32_t bits = (uint32_t)(*bytes & mask);
        // 2) Move bits to start
        bits = bits >> (remaining_bits_in_byte - bits_to_read);
        // 3) OR with result
        tmp_value |= bits;
        // 4) Read N bits
        bit_count -= bits_to_read;
        
        // Check if later readings will start from next btye
        if (bits_to_read == remaining_bits_in_byte) // Read rest of byte
        {
            bytes += 1;
            bit_current = 0;
        }
        else
        {
            bit_current += bits_to_read;
        }
    }

    // If there are more bits to read, try reading whole bytes
    if (bit_count >= 8)
    {
        uint32_t bytes_to_read = bit_count / 8;
        
        // If it's possible to read whole bytes
        if (bytes_to_read > 0)
        {
            uint32_t bits_to_read = (bytes_to_read * 8);
            
            // 1) Make room for bytes
            tmp_value = tmp_value << bits_to_read;
            // 2) Read whole bytes
            tmp_value |= unpack_uint32_big_endian(bytes, bytes_to_read);
            // 3) Read N bits (M bytes)
            bit_count -= bits_to_read;
            bytes += bytes_to_read;
        }
    }

    // Some remaining bits to read in next byte
    if (bit_count > 0)
    {
        bit_current = bit_count;
        // 1) Make room for bits
        tmp_value = tmp_value << bit_count;
        // 2) Mask out later bits
        uint32_t mask = 0xFF << (8 - bit_count);
        // 3) Extrac bits
        uint32_t bits = (uint32_t)(*bytes & mask);
        // 4) Move to start
        bits = bits >> (8 - bit_count);
        // OR with result
        tmp_value |= bits;
    }

    // Is not set to 0 by the last reading, so it will contain the position
    *bit_end = bit_current;
    *value = tmp_value;
    return (bytes - bytes_start);
}

// Convert N bits to an int32_t
uint64_t unpack_bits_to_int32(byte_t* bytes, uint32_t bit_count, uint8_t bit_current, uint8_t* bit_end, int32_t* value)
{
    // Cannot put more than 32 bits into an int32_t
    assert(bit_count <= (sizeof(int32_t) * 8));
    
    uint32_t tmp_value;
    uint64_t bytes_read = unpack_bits_to_uint32(bytes, bit_count, bit_current, &bit_current, &tmp_value);
    /* sign-extend *val assuming it is currently bits wide. */
    /* From: https://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend */
    uint32_t mask = 0x01 << (bit_count - 1); // Mark sign bit
    *value = (tmp_value ^ mask) - mask;
    return bytes_read;
}

// https://michaeldipperstein.github.io/rice.html
// Convert RICE bit stream to an int32_t
// k = bit length of r
uint64_t unpack_rice_to_int32(byte_t* bytes, uint8_t bit_current, uint8_t* bit_end, uint32_t k, int32_t* rice_value)
{
    byte_t* bytes_start = bytes;
    
    // Read unary value
    uint32_t q = 0;
    while (1)
    {
        // 1) Mask out all but current bit
        uint8_t mask = 0x1 << (7 - bit_current);
        // 2) Extract bit
        uint8_t bit = (*bytes & mask);
        // 3) Read 1 bit
        bit_current += 1;
        if (bit_current == 8)
        {
            bit_current = 0;
            bytes += 1;
        }
        // 4) Check bit is set
        if (bit != 0)
        {
            break;
        }
        // 5) Count bits
        q += 1;
    }
    
    // Read k bits as r
    uint32_t r = 0;
    if (k != 0)
    {
        bytes += unpack_bits_to_uint32(bytes, k, bit_current, &bit_current, &r);
    }
    
    // Construct RICE value using k, q and r = (Q * 2^K) + R
    // (I don't know how this works, but it works)
    // https://github.com/xiph/flac/blob/27c615706cedd252a206dd77e3910dfa395dcc49/src/libFLAC/bitreader.c#L741
    uint32_t val = (q << k) | r;
    if (val & 1)
    {
        val = -((int)(val >> 1)) - 1;
    }
    else
    {
        val = (int)(val >> 1);
    }
    *rice_value = val;
    
    *bit_end = bit_current;
    return (bytes - bytes_start);
}

// All numbers used in a FLAC bitstream are integers; there are no floating-point representations.
// All numbers are big-endian coded. All numbers are unsigned unless otherwise specified.
// https://github.com/ietf-wg-cellar/flac-specification
// https://xiph.org/flac/format.html
// https://github.com/xiph/flac/blob/27c615706cedd252a206dd77e3910dfa395dcc49/include/FLAC/format.h
// https://github.com/xiph/flac/blob/27c615706cedd252a206dd77e3910dfa395dcc49/src/libFLAC/metadata_iterators.c
// https://github.com/xiph/flac/blob/27c615706cedd252a206dd77e3910dfa395dcc49/src/libFLAC/bitreader.c
enum flac_metadata_block_type_e
{
    FLAC_METADATA_BLOCK_TYPE_STREAMINFO     = 0,
    FLAC_METADATA_BLOCK_TYPE_PADDING        = 1,
    FLAC_METADATA_BLOCK_TYPE_APPLICATION    = 2,
    FLAC_METADATA_BLOCK_TYPE_SEEKTABLE      = 3,
    FLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT = 4,
    FLAC_METADATA_BLOCK_TYPE_CUESHEET       = 5,
    FLAC_METADATA_BLOCK_TYPE_PICTURE        = 6
};

enum flac_subframe_type_e
{
    FLAC_SUBFRAME_TYPE_CONSTANT = 0,
    FLAC_SUBFRAME_TYPE_VERBATIM = 1,
    FLAC_SUBFRAME_TYPE_FIXED    = 2,
    FLAC_SUBFRAME_TYPE_LPC      = 3
};

enum flac_residule_type_e
{
    FLAC_RESIDUAL_TYPE_RICE  = 0,
    FLAC_RESIDUAL_TYPE_RICE2 = 1
};

enum flac_channel_assignment_e
{
    // 1 channel: mono
    FLAC_CHANNEL_ASSIGNMENT_MONO = 1,
    // 2 channels: left, right
    FLAC_CHANNEL_ASSIGNMENT_LEFT_RIGHT = 2,
    // 3 channels: left, right, center
    FLAC_CHANNEL_ASSIGNMENT_LEFT_RIGHT_CENTER = 3,
    // 4 channels: front-left, front-right, back-left, back-right
    FLAC_CHANNEL_ASSIGNMENT_FLEFT_FRIGHT_BLEFT_BRIGHT = 4,
    // 5 channels: front-left, front-right, front-center, back-left, back-right
    FLAC_CHANNEL_ASSIGNMENT_FLEFT_FRIGHT_FCENTER_BLEFT_BRIGHT = 5,
    // 6 channels: front-left, front-right, front-center, low frequency effects, back-left, back-right
    FLAC_CHANNEL_ASSIGNMENT_FLEFT_FRIGHT_FCENTER_LFE_BLEFT_BRIGHT = 6,
    // 7 channels: front-left, front-right, front-center, low frequency effects, back-center, side-left, side-right
    FLAC_CHANNEL_ASSIGNMENT_FLEFT_FRIGHT_FCENTER_LFE_BCENTER_SLEFT_SRIGHT = 7,
    // 8 channels: front-left, front-right, front-center, low frequency effects, back-left, back-right, side-left, side-right
    FLAC_CHANNEL_ASSIGNMENT_FLEFT_FRIGHT_FCENTER_LFE_BLEFT_BRIGHT_SLEFT_SRIGHT = 8,
    // 2 channels: left, side difference
    FLAC_CHANNEL_ASSIGNMENT_LEFT_DIFF = 9,
    // 2 channels: side difference, right
    FLAC_CHANNEL_ASSIGNMENT_DIFF_RIGHT = 10,
    // 2 channels: middle (average), side difference (left minus right)
    FLAC_CHANNEL_ASSIGNMENT_MID_DIFF = 11,
};

struct flac_metadata_block_header_t
{
    flac_metadata_block_type_e type;
    uint32_t size;
    bool is_last;
};

struct flac_metadata_block_streaminfo_t
{
    uint32_t block_size_min;
    uint32_t block_size_max;
    uint32_t frame_size_min;
    uint32_t frame_size_max;
    uint32_t sample_rate;
    uint32_t channel_count;
    uint32_t bits_per_sample;
    uint64_t sample_count;
};

struct flac_frame_header_t
{
    uint32_t blocking_strategy;
    uint32_t block_size_inter_channel_sampels;
    uint32_t sample_rate;
    flac_channel_assignment_e channel_assignment;
    uint32_t bits_per_sample;
    uint64_t sample_number;
    uint32_t frame_number;
    uint32_t crc;
};

struct flac_subframe_header_t
{
    flac_subframe_type_e type;
    uint32_t lpc_order; // Only for FLAC_SUBFRAME_TYPE_LPC
    uint32_t wasted_bits_per_sample;
    uint32_t sample_count;
    int32_t* samples;
};

static uint64_t FLACPLoadMetadataBlockHeader(byte_t* bytes, flac_metadata_block_header_t* metadata_block_header)
{
    byte_t* bytes_start = bytes;

    flac_metadata_block_header_t tmp_header;
    // 1 : Last-metadata-block flag: '1' if this block is the last metadata block before the audio blocks, '0' otherwise.
    tmp_header.is_last = (*bytes) & 0b10000000;

    // 7 : BLOCK_TYPE
    //  0 = STREAMINFO
    //  1 = PADDING
    //  2 = APPLICATION
    //  3 = SEEKTABLE
    //  4 = VORBIS_COMMENT
    //  5 = CUESHEET
    //  6 = PICTURE
    //  7-126 = reserved
    //  127 = invalid, to avoid confusion with a frame sync code
    tmp_header.type = (flac_metadata_block_type_e)((*bytes) & 0b01111111);
    if (tmp_header.type > 6)
    {
        printf("%s:%i Invalid metadata block type\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bytes += 1;

    // 24 : Length (in bytes) of metadata to follow (does not include the size of the METADATA_BLOCK_HEADER)
    tmp_header.size = unpack_uint32_big_endian(bytes, 3);
    bytes += 3;

    *metadata_block_header = tmp_header;
    return (uint64_t)(bytes - bytes_start); // Always 4
}

static uint64_t FLACLoadMetadataBlockStreaminfo(byte_t* bytes, flac_metadata_block_streaminfo_t* metadata_block_streaminfo)
{
    byte_t* bytes_start = bytes;

    flac_metadata_block_streaminfo_t tmp_streaminfo;
    // 16 : The minimum block size (in samples) used in the stream.
    tmp_streaminfo.block_size_min = unpack_uint32_big_endian(bytes, 2);
    bytes += 2;
    // 16 : The maximum block size (in samples) used in the stream. (Minimum blocksize == maximum blocksize) implies a fixed-blocksize stream.
    tmp_streaminfo.block_size_max = unpack_uint32_big_endian(bytes, 2);
    bytes += 2;
    // 24 : The minimum frame size (in bytes) used in the stream. May be 0 to imply the value is not known.
    tmp_streaminfo.frame_size_min = unpack_uint32_big_endian(bytes, 3);
    bytes += 3;
    // 24 : The maximum frame size (in bytes) used in the stream. May be 0 to imply the value is not known.
    tmp_streaminfo.frame_size_max = unpack_uint32_big_endian(bytes, 3);
    bytes += 3;
    // 20 : Sample rate in Hz. Though 20 bits are available, the maximum sample rate is limited by the structure of frame headers to 655350Hz. Also, a value of 0 is invalid.
    tmp_streaminfo.sample_rate = unpack_uint32_big_endian(bytes, 2) << 4; // Make room for the first 4 bits
    bytes += 2;
    tmp_streaminfo.sample_rate |= ((uint32_t)(*bytes & 0b11110000)) >> 4; // Only take first 4 bits in byte, and move to first 4 bits
    // 3 : (number of channels)-1. FLAC supports from 1 to 8 channels
    tmp_streaminfo.channel_count = (((uint32_t)(*bytes & 0b00001110)) >> 1); // Only take next 3 bits, and move to first 3 bits
    tmp_streaminfo.channel_count += 1;
    // 5 : (bits per sample)-1. FLAC supports from 4 to 32 bits per sample. Currently the reference encoder and decoders only support up to 24 bits per sample.
    tmp_streaminfo.bits_per_sample = ((uint32_t)(*bytes & 0b00000001)) << 4; // Only take last bit, and make room for 4 more
    bytes += 1;
    tmp_streaminfo.bits_per_sample |= (((uint32_t)(*bytes & 0b11110000)) >> 4); // Only take first 4 bits in byte, and move to first 4 bits
    tmp_streaminfo.bits_per_sample += 1;
    // 36 : Total samples in stream. 'Samples' means inter-channel sample, i.e. one second of 44.1Khz audio will have 44100 samples regardless of the number of channels. A value of zero here means the number of total samples is unknown.
    tmp_streaminfo.sample_count = ((uint64_t)(*bytes & 0b00001111)) << 32; // Only take last 4 bits, and make room for 32 more
    bytes += 1;
    tmp_streaminfo.sample_count |= unpack_uint64_big_endian(bytes, 4);
    bytes += 4;
    assert(tmp_streaminfo.sample_count != 0);
    // 128 : MD5 signature of the unencoded audio data. This allows the decoder to determine if an error exists in the audio data even when the error does not result in an invalid bitstream.
    bytes += 16;

    *metadata_block_streaminfo = tmp_streaminfo;
    return (bytes - bytes_start); // Always 34
}

static uint64_t FLACLoadMetadataBlockVorbisComment(byte_t* bytes, song_t* song)
{
    byte_t* bytes_start = bytes;

    uint32_t vendor_string_length = *((uint32_t*)bytes);
    bytes += 4;
    bytes += vendor_string_length; // Skip vendor string
    uint32_t comment_field_count = *((uint32_t*)bytes);
    bytes += 4;
    for (uint32_t i = 0; i < comment_field_count; i++)
    {
        uint32_t comment_length = *((uint32_t*)bytes);
        bytes += 4;
        char* comment = (char*)bytes;
        if (strncmp("TITLE", comment, 5) == 0)
        {
            char* title = comment + 6; // +6 to skip "TITLE="
            strncpy(song->title, title, comment_length - 6);
            song->title[comment_length - 6] = '\0'; // Null-terminate
        }
        else if (strncmp("ALBUM", comment, 5) == 0)
        {
            char* album = comment + 6; // +6 to skip "ALBUM="
            strncpy(song->album, album, comment_length - 6);
            song->album[comment_length - 6] = '\0'; // Null-terminate
        }
        else if (strncmp("ARTIST", comment, 6) == 0)
        {
            char* artist = comment + 7; // +7 to skip "ARTIST="
            strncpy(song->artist, artist, comment_length - 6);
            song->artist[comment_length - 7] = '\0'; // Null-terminate
        }
        bytes += comment_length;
    }

    return (bytes - bytes_start);
}

static uint64_t FLACLoadFrameHeader(byte_t* bytes, flac_metadata_block_streaminfo_t* metadata_block_streaminfo, flac_frame_header_t* frame_header)
{
    byte_t* bytes_start = bytes;

    flac_frame_header_t tmp_header;
    // 14 : Sync code '11111111111110'
    uint32_t sync_code = unpack_uint32_big_endian(bytes, 1) << 6;
    bytes += 1;
    sync_code |= ((uint32_t)(*bytes & 0b11111100)) >> 2;
    if (sync_code != 0b11111111111110)
    {
        printf("%s:%i Sync code did not match 0x3FFE: 0x%X4\n", __FILE__, __LINE__, sync_code);
        exit(EXIT_FAILURE);
    }
    // 1 : Reserved
    uint32_t reserved = (uint32_t)(*bytes & 0b00000010);
    if (reserved != 0)
    {
        printf("%s:%i Invalid reserved value\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    // 1 : Blocking strategy
    //     0 = fixed-blocksize stream; frame header encodes the frame number
    //     1 = variable-blocksize stream; frame header encodes the sample number
    tmp_header.blocking_strategy = (uint32_t)(*bytes & 0b00000001);
    bytes += 1;
    // 4 : Block size in inter-channel samples:
    //     0000 = reserved
    //     0001 = 192 samples
    //     0010-0101 = 576 * (2^(n-2)) samples, i.e. 576/1152/2304/4608
    //     0110 = get 8 bit (blocksize-1) from end of header
    //     0111 = get 16 bit (blocksize-1) from end of header
    //     1000-1111 = 256 * (2^(n-8)) samples, i.e. 256/512/1024/2048/4096/8192/16384/32768
    tmp_header.block_size_inter_channel_sampels = ((uint32_t)(*bytes & 0b11110000)) >> 4;
    // 4 : Sample rate
    //     0000 = get from STREAMINFO metadata block
    //     0001 = 88.2kHz
    //     0010 = 176.4kHz
    //     0011 = 192kHz
    //     0100 = 8kHz
    //     0101 = 16kHz
    //     0110 = 22.05kHz
    //     0111 = 24kHz
    //     1000 = 32kHz
    //     1001 = 44.1kHz
    //     1010 = 48kHz
    //     1011 = 96kHz
    //     1100 = get 8 bit sample rate (in kHz) from end of header
    //     1101 = get 16 bit sample rate (in Hz) from end of header
    //     1110 = get 16 bit sample rate (in tens of Hz) from end of header
    //     1111 = invalid, to prevent sync-fooling string of 1s
    tmp_header.sample_rate = (uint32_t)(*bytes & 0b00001111);
    bytes += 1;
    // 4 : Channel assignment
    //     0000-0111 = (number of independent channels)-1. Where defined, the channel order follows SMPTE/ITU-R recommendations. The assignments are as follows:
    //         1 channel: mono
    //         2 channels: left, right
    //         3 channels: left, right, center
    //         4 channels: front left, front right, back left, back right
    //         5 channels: front left, front right, front center, back/surround left, back/surround right
    //         6 channels: front left, front right, front center, LFE, back/surround left, back/surround right
    //         7 channels: front left, front right, front center, LFE, back center, side left, side right
    //         8 channels: front left, front right, front center, LFE, back left, back right, side left, side right
    //     1000 = left/side stereo: channel 0 is the left channel, channel 1 is the side(difference) channel
    //     1001 = right/side stereo: channel 0 is the side(difference) channel, channel 1 is the right channel
    //     1010 = mid/side stereo: channel 0 is the mid(average) channel, channel 1 is the side(difference) channel
    //     1011-1111 = reserved
    uint32_t channel_assignment = ((uint32_t)(*bytes & 0b11110000)) >> 4;
    switch (channel_assignment)
    {
        //case 0b0000: {} break;
        case 0b0001:
        {
            tmp_header.channel_assignment = FLAC_CHANNEL_ASSIGNMENT_LEFT_RIGHT;
        } break;
        //case 0b0010: {} break;
        //case 0b0011: {} break;
        //case 0b0100: {} break;
        //case 0b0101: {} break;
        //case 0b0110: {} break;
        //case 0b0111: {} break;
        case 0b1000:
        {
            tmp_header.channel_assignment = FLAC_CHANNEL_ASSIGNMENT_LEFT_DIFF;
        } break;
        case 0b1001:
        {
            tmp_header.channel_assignment = FLAC_CHANNEL_ASSIGNMENT_DIFF_RIGHT;
        } break;
        case 0b1010:
        {
            tmp_header.channel_assignment = FLAC_CHANNEL_ASSIGNMENT_MID_DIFF;
        } break;
            
        default:
        {
            printf("%s:%i Invalid channel assignment: %u\n", __FILE__, __LINE__, channel_assignment);
            exit(EXIT_FAILURE);
        }
    }
    // 3 : Sample size in bits
    //     000 = get from STREAMINFO metadata block
    //     001 = 8 bits per sample
    //     010 = 12 bits per sample
    //     011 = reserved
    //     100 = 16 bits per sample
    //     101 = 20 bits per sample
    //     110 = 24 bits per sample
    //     111 = reserved
    tmp_header.bits_per_sample = ((uint32_t)(*bytes & 0b00001110)) >> 1;
    switch (tmp_header.bits_per_sample)
    {
        case 0b000:
        {
            tmp_header.bits_per_sample = metadata_block_streaminfo->bits_per_sample;
        } break;

        case 0b001:
        {
            tmp_header.bits_per_sample = 8;
        } break;
        
        case 0b010:
        {
            tmp_header.bits_per_sample = 12;
        } break;
        
        case 0b100:
        {
            tmp_header.bits_per_sample = 16;
        } break;
        
        case 0b101:
        {
            tmp_header.bits_per_sample = 20;
        } break;
        
        case 0b110:
        {
            tmp_header.bits_per_sample = 24;
        } break;

        default:
        {
            printf("%s:%i Invalid bits per sample in FLAC frame header: %u\n", __FILE__, __LINE__, tmp_header.bits_per_sample);
            exit(EXIT_FAILURE);
        } break;
    }
    assert(tmp_header.bits_per_sample == metadata_block_streaminfo->bits_per_sample);
    // 1 : Reserved
    reserved = (uint32_t)(*bytes & 0b00000001);
    if (reserved != 0)
    {
        printf("%s:%i Invalid reserved value\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bytes += 1;
    // if(variable blocksize)
    //     <8-56>:"UTF-8" coded sample number (decoded number is 36 bits) [4]
    // else
    //     <8-48>:"UTF-8" coded frame number (decoded number is 31 bits) [4]
    if (tmp_header.blocking_strategy == 1)
    {
        bytes += unpack_utf8_to_uint64(bytes, &tmp_header.sample_number);
    }
    else
    {
        bytes += unpack_utf8_to_uint32(bytes, &tmp_header.frame_number);
    }
    // Determine block size
    switch (tmp_header.block_size_inter_channel_sampels)
    {
        // 0001 = 192 samples
        case 0b0001:
        {
            tmp_header.block_size_inter_channel_sampels = 192;
        } break;
            
        // 0010-0101 = 576 * (2^(n-2)) samples, i.e. 576/1152/2304/4608
        case 0b0010:
        {
            tmp_header.block_size_inter_channel_sampels = 576 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 2));
        } break;
        case 0b0011:
        {
            tmp_header.block_size_inter_channel_sampels = 576 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 2));
        } break;
        case 0b0100:
        {
            tmp_header.block_size_inter_channel_sampels = 576 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 2));
        } break;
        case 0b0101:
        {
            tmp_header.block_size_inter_channel_sampels = 576 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 2));
        } break;
            
        // 0110 = get 8 bit (blocksize-1) from end of header
        case 0b0110:
        {
            tmp_header.block_size_inter_channel_sampels = unpack_uint32_big_endian(bytes, 1) + 1;
            bytes += 1;
        } break;
            
        // 0111 = get 16 bit (blocksize-1) from end of header
        case 0b0111:
        {
            tmp_header.block_size_inter_channel_sampels = unpack_uint32_big_endian(bytes, 2) + 1;
            bytes += 2;
        } break;
            
        // 1000-1111 = 256 * (2^(n-8)) samples, i.e. 256/512/1024/2048/4096/8192/16384/32768
        case 0b1000:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        case 0b1001:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        case 0b1010:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        case 0b1011:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        case 0b1100:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        case 0b1101:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        case 0b1110:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        case 0b1111:
        {
            tmp_header.block_size_inter_channel_sampels = 256 * (0x1 << (tmp_header.block_size_inter_channel_sampels - 8));
        } break;
        
        default:
        {
            printf("%s:%i Invalid block size value: %u\n", __FILE__, __LINE__, tmp_header.block_size_inter_channel_sampels);
            exit(EXIT_FAILURE);
        } break;
    }
    // Determine sample rate
    switch (tmp_header.sample_rate)
    {
        // 0000 = get from STREAMINFO metadata block
        case 0b0000:
        {
            tmp_header.sample_rate = metadata_block_streaminfo->sample_rate;
        } break;
        // 0001 = 88.2kHz
        case 0b0001:
        {
            tmp_header.sample_rate = 88200;
        } break;
        // 0010 = 176.4kHz
        case 0b0010:
        {
            tmp_header.sample_rate = 176400;
        } break;
        // 0011 = 192kHz
        case 0b0011:
        {
            tmp_header.sample_rate = 192000;
        } break;
        // 0100 = 8kHz
        case 0b0100:
        {
            tmp_header.sample_rate = 8000;
        } break;
        // 0101 = 16kHz
        case 0b0101:
        {
            tmp_header.sample_rate = 16000;
        } break;
        // 0110 = 22.05kHz
        case 0b0110:
        {
            tmp_header.sample_rate = 22050;
        } break;
        // 0111 = 24kHz
        case 0b0111:
        {
            tmp_header.sample_rate = 24000;
        } break;
        // 1000 = 32kHz
        case 0b1000:
        {
            tmp_header.sample_rate = 32000;
        } break;
        // 1001 = 44.1kHz
        case 0b1001:
        {
            tmp_header.sample_rate = 44100;
        } break;
        // 1010 = 48kHz
        case 0b1010:
        {
            tmp_header.sample_rate = 48000;
        } break;
        // 1011 = 96kHz
        case 0b1011:
        {
            tmp_header.sample_rate = 96000;
        } break;
        // 1100 = get 8 bit sample rate (in kHz) from end of header
        case 0b1100:
        {
            tmp_header.sample_rate = unpack_uint32_big_endian(bytes, 1);
            bytes += 1;
        } break;
        // 1101 = get 16 bit sample rate (in Hz) from end of header
        case 0b1101:
        {
            tmp_header.sample_rate = unpack_uint32_big_endian(bytes, 16);
            bytes += 2;
        } break;
        // 1110 = get 16 bit sample rate (in tens of Hz) from end of header
        case 0b1110:
        {
            printf("%s:%i What does this mean\n", __FILE__, __LINE__);
            bytes += 2;
        } break;
        default:
        {
            printf("%s:%i Invalid sample rate value: %u\n", __FILE__, __LINE__, tmp_header.sample_rate);
            exit(EXIT_FAILURE);
        } break;
    }
    assert(tmp_header.sample_rate == metadata_block_streaminfo->sample_rate);
    // 8 : CRC-8 (polynomial = x^8 + x^2 + x^1 + x^0, initialized with 0) of everything before the crc, including the sync code
    tmp_header.crc = unpack_uint32_big_endian(bytes, 1);
    bytes += 1;
    
    *frame_header = tmp_header;
    return (bytes - bytes_start);
}

static uint64_t FLACLoadSubframeHeader(byte_t* bytes, uint8_t bit_current, uint8_t* bit_end, flac_subframe_header_t* subframe)
{
    byte_t* bytes_start = bytes;
    
    flac_subframe_header_t tmp_header;
    uint32_t byte = 0;
    bytes += unpack_bits_to_uint32(bytes, 8, bit_current, &bit_current, &byte);
    // 1 : Zero bit padding, to prevent sync-fooling string of 1s
    uint32_t reserved = ((uint32_t)(byte & 0b10000000)) >> 7;
    if (reserved != 0)
    {
        printf("%s:%i Invalid reserved value\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    // 6 : Subframe type:
    //      000000 = SUBFRAME_CONSTANT
    //      000001 = SUBFRAME_VERBATIM
    //      00001x = reserved
    //      0001xx = reserved
    //      001xxx = if(xxx <= 4) SUBFRAME_FIXED, xxx=order ; else reserved
    //      01xxxx = reserved
    //      1xxxxx = SUBFRAME_LPC, xxxxx=order-1
    uint32_t subframe_type = ((uint32_t)(byte & 0b01111110)) >> 1;
    if (subframe_type == 0b000000)
    {
        tmp_header.type = FLAC_SUBFRAME_TYPE_CONSTANT;
    }
    else if ((subframe_type & 0b111111) == 0b000001)
    {
        tmp_header.type = FLAC_SUBFRAME_TYPE_VERBATIM;
        assert(0);
    }
    else if (((subframe_type & 0b111000) == 0b001000) &&
             ((subframe_type & 0b000111) <= 4))
    {
        tmp_header.type = FLAC_SUBFRAME_TYPE_FIXED;
        tmp_header.lpc_order = (subframe_type & 0b000111);
    }
    else if ((subframe_type & 0b100000) == 0b100000)
    {
        tmp_header.type = FLAC_SUBFRAME_TYPE_LPC;
        tmp_header.lpc_order = (subframe_type & 0b011111) + 1;
    }
    else
    {
        printf("%s:%i Invalid subframe header type: %u\n", __FILE__, __LINE__, subframe_type);
        exit(EXIT_FAILURE);
    }
    // <1+k> : 'Wasted bits-per-sample' flag:
    //           0 : no wasted bits-per-sample in source subblock, k=0
    //           1 : k wasted bits-per-sample in source subblock, k-1 follows, unary coded; e.g. k=3 => 001 follows, k=7 => 0000001 follows.
    tmp_header.wasted_bits_per_sample = (uint32_t)(byte & 0b00000001);
    if (tmp_header.wasted_bits_per_sample == 1)
    {
        printf("%s:%i Currently not supporting wasted bits\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    tmp_header.samples = subframe->samples;
    *subframe = tmp_header;
    return (bytes - bytes_start);
}

uint64_t FLACLoadSubframeConstant(byte_t* bytes, uint32_t bits_per_sample, uint32_t frame_block_size, uint8_t bit_current, uint8_t* bit_end, flac_subframe_header_t* subframe)
{
    byte_t* bytes_start = bytes;
    
    // <n> : Unencoded constant value of the subblock, n = frame's bits-per-sample
    int32_t sample = 0;
    bytes += unpack_bits_to_int32(bytes, bits_per_sample, bit_current, &bit_current, &sample);
    subframe->sample_count = frame_block_size;
    for (uint32_t i = 0; i < frame_block_size; i++)
    {
        subframe->samples[i] = sample;
    }
    //printf("Total samples: %u\n", frame_block_size);
    
    *bit_end = bit_current;
    return (bytes - bytes_start);
}

uint64_t FLACLoadSubframeFixed(byte_t* bytes, uint32_t bits_per_sample, uint32_t order, uint32_t frame_block_size, uint8_t bit_current, uint8_t* bit_end, flac_subframe_header_t* subframe)
{
    byte_t* bytes_start = bytes;

    // <n> : Unencoded warm-up samples (n = frame's bits-per-sample * predictor order)
    int32_t* unencoded_warmup_samples = (int32_t*)malloc(order * sizeof(int32_t));
    uint32_t mask = 0x01 << (bits_per_sample - 1); // Mark sign bit
    for (uint32_t i = 0; i < order; i++)
    {
        uint32_t tmp_value = 0;
        bytes += unpack_bits_to_uint32(bytes, bits_per_sample, bit_current, &bit_current, &tmp_value);
        // From https://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend
        tmp_value = (tmp_value ^ mask) - mask;
        unencoded_warmup_samples[i] = tmp_value;
    }
    // <2> : Residual coding method:
    //        00 : partitioned Rice coding with 4-bit Rice parameter; RESIDUAL_CODING_METHOD_PARTITIONED_RICE follows
    //        01 : partitioned Rice coding with 5-bit Rice parameter; RESIDUAL_CODING_METHOD_PARTITIONED_RICE2 follows
    //        10-11 : reserved
    uint32_t residual_type = 0;
    bytes += unpack_bits_to_uint32(bytes, 2, bit_current, &bit_current, &residual_type);
    flac_residule_type_e residual_coding_method = (flac_residule_type_e)residual_type;
    // <4> : Partition order
    uint32_t residual_partition_order = 0;
    bytes += unpack_bits_to_uint32(bytes, 4, bit_current, &bit_current, &residual_partition_order);
    // There will be 2^order partitions
    uint32_t partition_count = 0x1 << residual_partition_order; // 2^tmp_lpc.residual_partition_order
    uint32_t* rice_parameters = (uint32_t*)malloc(partition_count * sizeof(uint32_t));
    int32_t* residuals = (int32_t*)malloc((frame_block_size - order) * sizeof(int32_t));
    uint32_t residual_counter = 0;

    uint32_t total_samples = order;
    for (uint32_t i = 0; i < partition_count; i++)
    {
        switch (residual_coding_method)
        {
            case FLAC_RESIDUAL_TYPE_RICE:
            {
                // <4(+5)> : Encoding parameter:
                //            0000-1110 : Rice parameter.
                //            1111 : Escape code, meaning the partition is in unencoded binary form using n bits per sample; n follows as a 5-bit number.
                bytes += unpack_bits_to_uint32(bytes, 4, bit_current, &bit_current, &rice_parameters[i]);
                if (rice_parameters[i] == 0b1111)
                {
                    //bytes += unpack_bits_to_uint32(bytes, 5, bit_current, &bit_current, &bits_per_sample_tmp);
                    printf("%s:%i Not supported yet\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            } break;

            case FLAC_RESIDUAL_TYPE_RICE2:
            {
                // <5(+5)> : Encoding parameter:
                //            00000-11110 : Rice parameter.
                //            11111 : Escape code, meaning the partition is in unencoded binary form using n bits per sample; n follows as a 5-bit number.
                bytes += unpack_bits_to_uint32(bytes, 5, bit_current, &bit_current, &rice_parameters[i]);
                if (rice_parameters[i] == 0b11111)
                {
                    //bytes += unpack_bits_to_uint32(bytes, 5, bit_current, &bit_current, &bits_per_sample_tmp);
                    printf("%s:%i Not supported yet\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            } break;

            default:
            {
                printf("%s:%i Invalid residual type in LPC subframe: %u\n", __FILE__, __LINE__, residual_type);
                exit(EXIT_FAILURE);
            } break;
        }
        
        // <?> : Encoded residual. The number of samples (n) in the partition is determined as follows:
        //        if the partition order is zero, n = frame's blocksize - predictor order
        //        else if this is not the first partition of the subframe, n = (frame's blocksize / (2^partition order))
        //        else n = (frame's blocksize / (2^partition order)) - predictor order
        uint32_t samples_in_partition_count = 0;
        if (residual_partition_order == 0)
        {
            samples_in_partition_count = frame_block_size - order;
        }
        else if (i > 0)
        {
            samples_in_partition_count = (frame_block_size / (0x1 << residual_partition_order));
        }
        else
        {
            samples_in_partition_count = (frame_block_size / (0x1 << residual_partition_order)) - order;
        }
        total_samples += samples_in_partition_count;
        //printf("parameter [%u]=%u (%u samples)\n", i, rice_parameters[i], samples_in_partition_count);
        
        // For each sample, extract the Rice code
        for (uint32_t j = 0; j < samples_in_partition_count; j++)
        {
            bytes += unpack_rice_to_int32(bytes, bit_current, &bit_current, rice_parameters[i], &residuals[residual_counter]);
            //printf("Residual: %i\n", residuals[residual_counter]);
            residual_counter += 1;
        }
    }
    //printf("Total samples: %u\n", total_samples);
    assert(frame_block_size == total_samples);
    
    // Decode subframe
    // TODO: understand this
    subframe->sample_count = frame_block_size;
    memcpy(subframe->samples, unencoded_warmup_samples, order * sizeof(int32_t));
    switch(order) {
		case 0:
			memcpy(subframe->samples, residuals, (frame_block_size - order) * sizeof(int32_t));
			break;
		case 1:
            for(uint32_t i = 0; i < frame_block_size - order; i++)
            {
                uint32_t sample_index = i + order;
				subframe->samples[sample_index] = residuals[i] + subframe->samples[sample_index - 1];
            }
			break;
		case 2:
            for(uint32_t i = 0; i < frame_block_size - order; i++)
            {
                uint32_t sample_index = i + order;
				subframe->samples[sample_index] = residuals[i] + (2 * subframe->samples[sample_index - 1]) - subframe->samples[sample_index - 2];
            }
			break;
		case 3:
			for(uint32_t i = 0; i < frame_block_size - order; i++)
            {
                uint32_t sample_index = i + order;
				subframe->samples[sample_index] = residuals[i] + (3 * subframe->samples[sample_index - 1]) - (3 * subframe->samples[sample_index - 2]) + subframe->samples[sample_index - 3];
            }
			break;
		case 4:
			for(uint32_t i = 0; i < frame_block_size - order; i++)
            {
                uint32_t sample_index = i + order;
				subframe->samples[sample_index] = residuals[i] + (4 * subframe->samples[sample_index - 1]) - (6 * subframe->samples[sample_index - 2]) + (4 * subframe->samples[sample_index - 3]) - subframe->samples[sample_index - 4];
            }
			break;
		default:
			assert(0);
	}
    
    // Clean-up
    free(residuals);
    free(rice_parameters);
    free(unencoded_warmup_samples);
    
    *bit_end = bit_current;
    return (bytes - bytes_start);
}

// A subframe  has
//  - lpc_order warm-up samples
//  - lpc_order predictor coefficients
//  - N partitions, each of which has
//    - a Rice parameter
//    - M residual (error) samples
uint64_t FLACLoadSubframeLPC(byte_t* bytes, uint32_t bits_per_sample, uint32_t lpc_order, uint32_t frame_block_size, uint8_t bit_current, uint8_t* bit_end, flac_subframe_header_t* subframe)
{
    byte_t* bytes_start = bytes;

    // <n> : Unencoded warm-up samples (n = frame's bits-per-sample * lpc order)
    int32_t* unencoded_warmup_samples = (int32_t*)malloc(lpc_order * sizeof(int32_t));
    uint32_t mask = 0x01 << (bits_per_sample - 1); // Mark sign bit
    for (uint32_t i = 0; i < lpc_order; i++)
    {
        uint32_t tmp_value = 0;
        bytes += unpack_bits_to_uint32(bytes, bits_per_sample, bit_current, &bit_current, &tmp_value);
        // From https://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend
        tmp_value = (tmp_value ^ mask) - mask;
        unencoded_warmup_samples[i] = tmp_value;
        //printf("warmup[%u]=%i\n", i, unencoded_warmup_samples[i]);
    }
    // <4> : (Quantized linear predictor coefficients' precision in bits)-1 (1111 = invalid)
    uint32_t quantizied_linear_coefficient_bits = 0;
    bytes += unpack_bits_to_uint32(bytes, 4, bit_current, &bit_current, &quantizied_linear_coefficient_bits);
    quantizied_linear_coefficient_bits += 1;
    // <5> : Quantized linear predictor coefficient shift needed in bits (NOTE: this number is signed two's-complement)
    uint32_t quantizied_linear_coefficient_shift_bits = 0;
    bytes += unpack_bits_to_uint32(bytes, 5, bit_current, &bit_current, &quantizied_linear_coefficient_shift_bits);
    // <n> : Unencoded predictor coefficients (n = qlp coeff precision * lpc order) (NOTE: the coefficients are signed two's-complement)
    int32_t* unencoded_predictor_coefficients = (int32_t*)malloc(lpc_order * sizeof(int32_t));
    mask = 0x01 << (quantizied_linear_coefficient_bits - 1); // Mark sign bit
    for (uint32_t i = 0; i < lpc_order; i++)
    {
        uint32_t tmp_value = 0;
        bytes += unpack_bits_to_uint32(bytes, quantizied_linear_coefficient_bits, bit_current, &bit_current, &tmp_value);
        // From https://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend
        tmp_value = (tmp_value ^ mask) - mask;
        unencoded_predictor_coefficients[i] = tmp_value;
        //printf("qlp_coeff[%u]=%i\n", i, unencoded_predictor_coefficients[i]);
    }
    // <2> : Residual coding method:
    //        00 : partitioned Rice coding with 4-bit Rice parameter; RESIDUAL_CODING_METHOD_PARTITIONED_RICE follows
    //        01 : partitioned Rice coding with 5-bit Rice parameter; RESIDUAL_CODING_METHOD_PARTITIONED_RICE2 follows
    //        10-11 : reserved
    uint32_t residual_type = 0;
    bytes += unpack_bits_to_uint32(bytes, 2, bit_current, &bit_current, &residual_type);
    flac_residule_type_e residual_coding_method = (flac_residule_type_e)residual_type;
    // <4> : Partition order
    uint32_t residual_partition_order = 0;
    bytes += unpack_bits_to_uint32(bytes, 4, bit_current, &bit_current, &residual_partition_order);
    // There will be 2^order partitions
    uint32_t partition_count = 0x1 << residual_partition_order; // 2^tmp_lpc.residual_partition_order
    uint32_t* rice_parameters = (uint32_t*)malloc(partition_count * sizeof(uint32_t));
    int32_t* residuals = (int32_t*)malloc((frame_block_size - lpc_order) * sizeof(int32_t));
    uint32_t residual_counter = 0;

    uint32_t total_samples = lpc_order;
    for (uint32_t i = 0; i < partition_count; i++)
    {
        switch (residual_coding_method)
        {
            case FLAC_RESIDUAL_TYPE_RICE:
            {
                // <4(+5)> : Encoding parameter:
                //            0000-1110 : Rice parameter.
                //            1111 : Escape code, meaning the partition is in unencoded binary form using n bits per sample; n follows as a 5-bit number.
                bytes += unpack_bits_to_uint32(bytes, 4, bit_current, &bit_current, &rice_parameters[i]);
                if (rice_parameters[i] == 0b1111)
                {
                    //bytes += unpack_bits_to_uint32(bytes, 5, bit_current, &bit_current, &bits_per_sample_tmp);
                    printf("%s:%i Not supported yet\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            } break;

            case FLAC_RESIDUAL_TYPE_RICE2:
            {
                // <5(+5)> : Encoding parameter:
                //            00000-11110 : Rice parameter.
                //            11111 : Escape code, meaning the partition is in unencoded binary form using n bits per sample; n follows as a 5-bit number.
                bytes += unpack_bits_to_uint32(bytes, 5, bit_current, &bit_current, &rice_parameters[i]);
                if (rice_parameters[i] == 0b11111)
                {
                    //bytes += unpack_bits_to_uint32(bytes, 5, bit_current, &bit_current, &bits_per_sample_tmp);
                    printf("%s:%i Not supported yet\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            } break;

            default:
            {
                printf("%s:%i Invalid residual type in LPC subframe: %u\n", __FILE__, __LINE__, residual_type);
                exit(EXIT_FAILURE);
            } break;
        }
        
        // <?> : Encoded residual. The number of samples (n) in the partition is determined as follows:
        //        if the partition order is zero, n = frame's blocksize - predictor order
        //        else if this is not the first partition of the subframe, n = (frame's blocksize / (2^partition order))
        //        else n = (frame's blocksize / (2^partition order)) - predictor order
        uint32_t samples_in_partition_count = 0;
        if (residual_partition_order == 0)
        {
            samples_in_partition_count = frame_block_size - lpc_order;
        }
        else if (i > 0)
        {
            samples_in_partition_count = (frame_block_size / (0x1 << residual_partition_order));
        }
        else
        {
            samples_in_partition_count = (frame_block_size / (0x1 << residual_partition_order)) - lpc_order;
        }
        total_samples += samples_in_partition_count;
        //printf("parameter [%u]=%u (%u samples)\n", i, rice_parameters[i], samples_in_partition_count);
        
        // For each sample, extract the Rice code
        for (uint32_t j = 0; j < samples_in_partition_count; j++)
        {
            bytes += unpack_rice_to_int32(bytes, bit_current, &bit_current, rice_parameters[i], &residuals[residual_counter]);
            residual_counter += 1;
        }
    }
    //printf("Total samples: %u\n", total_samples);
    assert(frame_block_size == total_samples);
    
    // Decode subframe
    // TODO: understand this
    subframe->sample_count = frame_block_size;
    memcpy(subframe->samples, unencoded_warmup_samples, lpc_order * sizeof(int32_t));
    for (uint32_t i = lpc_order; i < frame_block_size; i++)
    {
        uint32_t sample_index = i;
        uint32_t residual_index = i - lpc_order;
        int64_t sum = 0;
        for (uint32_t j = 0; j < lpc_order; j++)
        {
            sum += (int64_t)unencoded_predictor_coefficients[j] * (int64_t)subframe->samples[sample_index - j - 1];
        }
        subframe->samples[sample_index] = residuals[residual_index] + (int32_t)(sum >> quantizied_linear_coefficient_shift_bits);
        //printf("[%u]=%i\n", i - lpc_order, subframe->samples[sample_index]);
    }
    
    // Clean-up
    free(residuals);
    free(rice_parameters);
    free(unencoded_predictor_coefficients);
    free(unencoded_warmup_samples);
    
    *bit_end = bit_current;
    return (bytes - bytes_start);
}

/*song_error_e FLACLoad(song_t* song)
{
    // Load FLAC file into memory
    FILE* flac_file = fopen(song->song_path_offset, "rb");
    if (flac_file == NULL)
    {
        printf("Failed to open %s\n", song->song_path_offset);
        exit(EXIT_FAILURE);
    }
    fseek(flac_file, 0, SEEK_END);
    long flac_file_size = ftell(flac_file);
    fseek(flac_file, 0, SEEK_SET);
    byte_t* flac_memory = (byte_t*)malloc(flac_file_size);
    fread((void*)flac_memory, flac_file_size, 1, flac_file);
    fclose(flac_file);
    byte_t* flac_memory_start = flac_memory;
    byte_t* flac_memory_end = flac_memory + flac_file_size;

    // Verify FLAC file
    if (strncmp((char*)flac_memory, "fLaC", 4) != 0)
    {
        free(flac_memory);
        exit(EXIT_FAILURE);
    }
    flac_memory += 4;

    // Parse METADATA_BLOCK_HEADER
    flac_metadata_block_header_t metadata_block_header;
    flac_memory += FLACPLoadMetadataBlockHeader(flac_memory, &metadata_block_header);
    if (metadata_block_header.type != 0)
    {
        printf("First metadata block of FLAC file must be of type STREAMINFO: %u\n", metadata_block_header.type);
        exit(EXIT_FAILURE);
    }

    // Parse STREAMINFO metadata
    flac_metadata_block_streaminfo_t metadata_block_streaminfo;
    flac_memory += FLACLoadMetadataBlockStreaminfo(flac_memory, &metadata_block_streaminfo);
    //printf("[%u,%u]\n", metadata_block_streaminfo.block_size_min, metadata_block_streaminfo.block_size_max);
    //printf("[%u,%u]\n", metadata_block_streaminfo.frame_size_min, metadata_block_streaminfo.frame_size_max);

    // Skip all following METADATA blocks
    while (metadata_block_header.is_last == false)
    {
        flac_memory += FLACPLoadMetadataBlockHeader(flac_memory, &metadata_block_header);
        if (metadata_block_header.type == FLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT)
        {
            flac_memory += FLACLoadMetadataBlockVorbisComment(flac_memory, song);
        }
        else
        {
            flac_memory += metadata_block_header.size;
        }
    }
    
    // Set up WAV info based on FLAC info
    uint32_t wav_file_channel_count = metadata_block_streaminfo.channel_count;
    uint32_t wav_file_sample_rate = 0;
    uint32_t wav_file_bits_per_sample = 16;
    uint64_t wav_file_sample_count = metadata_block_streaminfo.sample_count;
    uint64_t wav_file_sample_index = 0;
    uint64_t wav_file_size = wav_file_sample_count * wav_file_channel_count * (wav_file_bits_per_sample / 8);
    int16_t* wav_file_samples = (int16_t*)malloc(wav_file_sample_count * wav_file_channel_count * sizeof(int16_t));

    // Parse all FRAME_HEADERs
#define INT12_MAX 0x7FF
#define INT20_MAX 0x7FFFF
#define INT24_MAX 0x7FFFFF
    int64_t flac_bits_per_sample_max = 0;
    switch (metadata_block_streaminfo.bits_per_sample)
    {
        case 8:
        {
            flac_bits_per_sample_max = INT8_MAX;
        } break;
        case 12:
        {
            flac_bits_per_sample_max = INT12_MAX;
        } break;
        case 16:
        {
            flac_bits_per_sample_max = INT16_MAX;
        } break;
        case 20:
        {
            flac_bits_per_sample_max = INT20_MAX;
        } break;
        case 24:
        {
            flac_bits_per_sample_max = INT24_MAX;
        } break;
        default:
        {
            assert(0);
        } break;
    }
    flac_frame_header_t frame_header;
    flac_subframe_header_t* subframes = (flac_subframe_header_t*)malloc(metadata_block_streaminfo.channel_count * sizeof(flac_subframe_header_t));
    for (uint32_t i = 0; i < metadata_block_streaminfo.channel_count; i++)
    {
        // Guaranteed to be enough
        subframes[i].samples = (int32_t*)malloc(metadata_block_streaminfo.block_size_max * sizeof(int32_t));
    }
    uint32_t flac_file_sample_rate = 0;
    while (flac_memory < flac_memory_end)
    {
        flac_memory += FLACLoadFrameHeader(flac_memory, &metadata_block_streaminfo, &frame_header);
        if (flac_file_sample_rate != 0)
        {
            assert(frame_header.sample_rate == flac_file_sample_rate);
        }
        flac_file_sample_rate = frame_header.sample_rate;
        // Parse each subframe header and subframe
        uint8_t bit_current = 0;
        for (uint32_t i = 0; i < metadata_block_streaminfo.channel_count; i++)
        {
            uint32_t bits_per_sample = frame_header.bits_per_sample;
            // If the channel assignment includes a side difference, those warm-up samples
            // require an additioninal bit per sample (see https://hydrogenaud.io/index.php?topic=121900.0)
            if (((frame_header.channel_assignment == FLAC_CHANNEL_ASSIGNMENT_LEFT_DIFF)  && (i == 1)) ||
                ((frame_header.channel_assignment == FLAC_CHANNEL_ASSIGNMENT_DIFF_RIGHT) && (i == 0)) ||
                ((frame_header.channel_assignment == FLAC_CHANNEL_ASSIGNMENT_MID_DIFF)   && (i == 1)))
            {
                bits_per_sample += 1;
            }
            
            // Parse SUBFRAME_HEADER
            flac_memory += FLACLoadSubframeHeader(flac_memory, bit_current, &bit_current, &subframes[i]);
            
            // Parse SUBFRAME
            switch (subframes[i].type)
            {
                case FLAC_SUBFRAME_TYPE_CONSTANT:
                {
                    flac_memory += FLACLoadSubframeConstant(flac_memory, bits_per_sample, frame_header.block_size_inter_channel_sampels, bit_current, &bit_current, &subframes[i]);
                } break;

                case FLAC_SUBFRAME_TYPE_FIXED:
                {
                    flac_memory += FLACLoadSubframeFixed(flac_memory, bits_per_sample, subframes[i].lpc_order, frame_header.block_size_inter_channel_sampels, bit_current, &bit_current, &subframes[i]);
                } break;

                case FLAC_SUBFRAME_TYPE_LPC:
                {
                    flac_memory += FLACLoadSubframeLPC(flac_memory, bits_per_sample, subframes[i].lpc_order, frame_header.block_size_inter_channel_sampels, bit_current, &bit_current, &subframes[i]);
                } break;

                default:
                {
                    printf("%s:%i Invalid subframe header type: %i\n", __FILE__, __LINE__, subframes[i].type);
                    exit(EXIT_FAILURE);
                }
            }
        }
        // <?> : Zero-padding to byte alignment.
        if (bit_current > 0)
        {
            bit_current = 0;
            flac_memory += 1;
        }
        // Parse FRAME_FOOTER
        // <16> : CRC-16 (polynomial = x^16 + x^15 + x^2 + x^0, initialized with 0) of everything before the crc, back to and including the frame header sync code
        flac_memory += 2;
        
        // Convert to WAV
        switch (frame_header.channel_assignment)
        {
            // Encoding:
            //  LEFT = LEFT
            //  RIGHT = RIGHT
            // Decoding:
            //  LEFT = LEFT
            //  RIGHT = RIGHT
            case FLAC_CHANNEL_ASSIGNMENT_LEFT_RIGHT:
            {
                flac_subframe_header_t* left = &subframes[0];
                flac_subframe_header_t* right = &subframes[1];
                assert(left->sample_count == right->sample_count);
                for (uint32_t i = 0; i < left->sample_count; i++)
                {
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)left->samples[i] / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)right->samples[i] / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                }
            } break;
            
            // Endocing:
            //  LEFT = LEFT
            //  DIFF = LEFT - RIGHT
            // Decoding:
            //  LEFT = LEFT
            //  RIGHT = LEFT - DIFF = LEFT - LEFT + RIGHT = RIGHT
            case FLAC_CHANNEL_ASSIGNMENT_LEFT_DIFF:
            {
                flac_subframe_header_t* left = &subframes[0];
                flac_subframe_header_t* diff = &subframes[1];
                assert(left->sample_count == diff->sample_count);
                for (uint32_t i = 0; i < left->sample_count; i++)
                {
                    int32_t left_sample = left->samples[i];
                    int32_t diff_sample = diff->samples[i];
                    int32_t right_sample = left_sample - diff_sample;
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)left_sample / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)right_sample / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                }
            } break;
            
            // Encoding:
            //  DIFF = LEFT - RIGHT
            //  RIGHT = RIGHT
            // Decoding:
            //  LEFT = RIGHT + DIFF = RIGHT + LEFT - RIGHT = LEFT
            //  RIGHT = RIGHT
            case FLAC_CHANNEL_ASSIGNMENT_DIFF_RIGHT:
            {
                flac_subframe_header_t* diff = &subframes[0];
                flac_subframe_header_t* right = &subframes[1];
                assert(diff->sample_count == right->sample_count);
                for (uint32_t i = 0; i < diff->sample_count; i++)
                {
                    int32_t diff_sample = diff->samples[i];
                    int32_t right_sample = right->samples[i];
                    int32_t left_sample = (right_sample + diff_sample);
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)left_sample / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)right_sample / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                }
            } break;
            
            // Encoding:
            //  MID = (LEFT + RIGHT) >> 1
            //  DIFF = LEFT - RIGHT
            // Decoding:
            //  LEFT  = ((MID << 1) + DIFF) >> 1 = (LEFT + RIGHT + LEFT - RIGHT) >> 1 = 2LEFT / 2 = LEFT
            //  RIGHT = ((MID << 1) - DIFF) >> 1 = (LEFT + RIGHT - LEFT + RIGHT) >> 1 = 2RIGHT / 2 = RIGHT
            case FLAC_CHANNEL_ASSIGNMENT_MID_DIFF:
            {
                flac_subframe_header_t* mid = &subframes[0];
                flac_subframe_header_t* diff = &subframes[1];
                assert(mid->sample_count == diff->sample_count);
                for (uint32_t i = 0; i < mid->sample_count; i++)
                {
                    int32_t diff_sample = diff->samples[i];
                    int32_t mid_sample = mid->samples[i] << 1;
                    if (diff_sample & 1)
                    {
                        mid_sample += 1;
                    }
                    int32_t left_sample = (mid_sample + diff_sample) >> 1;
                    int32_t right_sample = (mid_sample - diff_sample) >> 1;
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)left_sample / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                    wav_file_samples[wav_file_sample_index++] = (int16_t)(((float)right_sample / (float)flac_bits_per_sample_max) * (float)INT16_MAX);
                }
            } break;
                
            default:
            {
                printf("Channel assignment %i\n", frame_header.channel_assignment);
                assert(0);
            } break;
        }
    }
    for (uint32_t i = 0; i < metadata_block_streaminfo.channel_count; i++)
    {
        free(subframes[i].samples);
    }
    free(subframes);
    free(flac_memory_start);
    
    // Create WAV file
    wav_file_size += 44;
    byte_t* wav_data = (byte_t*)malloc(wav_file_size * sizeof(byte_t));
    byte_t* wav_data_current = wav_data;
    const char* riff_str = "RIFF";
    memcpy(wav_data_current, riff_str, 4);
    wav_data_current += 4;
    uint32_t chunk_size = (uint32_t)(wav_file_size - 8);
    memcpy(wav_data_current, &chunk_size, 4);
    wav_data_current += 4;
    const char* wave_str = "WAVE";
    memcpy(wav_data_current, wave_str, 4);
    wav_data_current += 4;
    const char* fmt_str = "fmt ";
    memcpy(wav_data_current, fmt_str, 4);
    wav_data_current += 4;
    uint32_t subchunk_size1 = 16;
    memcpy(wav_data_current, &subchunk_size1, 4);
    wav_data_current += 4;
    uint16_t audio_format = 1;
    memcpy(wav_data_current, &audio_format, 2);
    wav_data_current += 2;
    uint16_t channel_count = wav_file_channel_count;
    memcpy(wav_data_current, &channel_count, 2);
    wav_data_current += 2;
    wav_file_sample_rate = flac_file_sample_rate;
    uint32_t sample_rate = wav_file_sample_rate;
    memcpy(wav_data_current, &channel_count, 4);
    wav_data_current += 4;
    uint32_t byte_rate = sample_rate * channel_count * (wav_file_bits_per_sample / 8);
    memcpy(wav_data_current, &byte_rate, 4);
    wav_data_current += 4;
    uint16_t block_align = channel_count * (wav_file_bits_per_sample / 8);
    memcpy(wav_data_current, &block_align, 2);
    wav_data_current += 2;
    uint16_t bits_per_sample = wav_file_bits_per_sample;
    memcpy(wav_data_current, &bits_per_sample, 2);
    wav_data_current += 2;
    const char* data_str = "data";
    memcpy(wav_data_current, data_str, 4);
    wav_data_current += 4;
    uint32_t subchunk_size2 = (uint32_t)(wav_file_size - 44);
    memcpy(wav_data_current, &subchunk_size2, 4);
    wav_data_current += 4;
    memcpy(wav_data_current, wav_file_samples, subchunk_size2);
    free(wav_file_samples);

    // Set WAV information
    song->audio_data = (byte_t*)malloc(subchunk_size2);
    memcpy(song->audio_data, wav_data + 40, subchunk_size2);
    song->audio_data_size = subchunk_size2;
    song->sample_rate = sample_rate;
    song->channel_count = channel_count;
    song->bits_per_sample = bits_per_sample;
    
    // Clean-up
    free(wav_data);

    return SONG_ERROR_NO;
}*/

