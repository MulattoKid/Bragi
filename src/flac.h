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

#ifndef FLAC_H
#define FLAC_H

#include "wav.h"

/**
 * The start of any valid FLAC file starts with the four bytes "fLaC".
 * 
 * This is then followed by a METADATA_BLOCK which consists of two sub-blocks:
 *  - METADATA_BLOCK_HEADER
 *  - METADATA_BLOCK_DATA of type STREAMINFO
 * 
 * After this initial metadata block, a number of additional metadata blocks can follow, or none.
 * 
 * After any additional metadata blocks comes the FRAME, of which there can be many,
 * which consists of at least three sub-block types:
 *  - FRAME_HEADER
 *  - SUBFRAME
 *  - FRAME_FOOTER
 * 
 * There can be several subframes, depending on the number of channel (each channel has a subframe).
 * A subframe consists of several sub-blocks:
 *  - SUBFRAME_HEADER
 *  - SUBFRAME_CONSTANT or SUBFRAME_FIXED or SUBFRAME_LPC or SUBFRAME_VERBATIM
 * 
*/
//song_error_e FLACLoad(song_t* song);

#endif