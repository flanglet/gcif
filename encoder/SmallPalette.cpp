/*
	Copyright (c) 2013 Game Closure.  All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	* Neither the name of GCIF nor the names of its contributors may be used
	  to endorse or promote products derived from this software without
	  specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include "SmallPalette.hpp"
#include "../decoder/EndianNeutral.hpp"
#include "Log.hpp"
#include "../decoder/Enforcer.hpp"
using namespace cat;
using namespace std;


//// SmallPalette

bool SmallPalette::generatePalette() {
	u32 hist[SMALL_PALETTE_MAX] = {0};

	const u32 *color = reinterpret_cast<const u32 *>( _rgba );
	int palette_size = 0;

	for (int y = 0; y < _size_y; ++y) {
		for (int x = 0, xend = _size_x; x < xend; ++x) {
			u32 c = *color++;

			// Determine palette index
			int index;
			if (_map.find(c) == _map.end()) {
				// If ran out of palette slots,
				if (palette_size >= SMALL_PALETTE_MAX) {
					return false;
				}

				index = palette_size;
				_map[c] = index;
				_palette.push_back(c);

				++palette_size;
			} else {
				index = _map[c];
			}

			// Record how often each palette index is used
			hist[index]++;
		}
	}

	// If palette size is degenerate,
	if (palette_size <= 0) {
		CAT_DEBUG_EXCEPTION();
		return false;
	}

	// Store the palette size
	_palette_size = palette_size;

	return true;
}

void SmallPalette::generateImage() {
	if (_palette_size > 4) { // 3-4 bits/pixel
		/*
		 * Combine pairs of pixels on the same scanline together.
		 * Final odd pixel in each row is encoded in the low bits.
		 */
		_pack_x = (_size_x + 1) >> 1;
		_pack_y = _size_y;
		const int image_size = _pack_x * _pack_y;
		_image.resize(image_size);

		const u32 *color = reinterpret_cast<const u32 *>( _rgba );
		u8 *image = _image.get();

		// For each scanline,
		for (int y = 0; y < _size_y; ++y) {
			u8 b = 0;
			int packed = 0;

			// For each pixel,
			for (int x = 0, xend = _size_x; x < xend; ++x) {
				// Lookup palette index
				u32 c = *color++;
				u8 p = _map[c];

				// Pack pixel
				b <<= 4;
				b |= p;
				++packed;

				// If it is time to write,
				if (packed >= 2) {
					*image++ = b;
					b = 0;
					packed = 0;
				}
			}

			// If there is a partial packing,
			if (packed > 0) {
				*image++ = b;
			}
		}
	} else if (_palette_size > 2) { // 2 bits/pixel
		/*
		 * Combine blocks of 4 pixels together:
		 *
		 * 0 0 1 1 2 2 3
		 * 0 0 1 1 2 2 3 <- example 7x3 image
		 * 4 4 5 5 6 6 7
		 *
		 * Each 2x2 block is packed like so:
		 * 0 1  -->  HI:[ 3 3 2 2 1 1 0 0 ]:LO
		 * 2 3
		 */
		_pack_x = (_size_x + 1) >> 1;
		_pack_y = (_size_y + 1) >> 1;
		const int image_size = _pack_x * _pack_y;
		_image.resize(image_size);

		const u32 *color = reinterpret_cast<const u32 *>( _rgba );
		u8 *image = _image.get();

		for (int y = 0; y < _size_y; y += 2) {
			for (int x = 0, xend = _size_x; x < xend; x += 2) {
				// Lookup palette index
				u32 c0 = color[0];
				u8 p0 = _map[c0], p1 = 0, p2 = 0, p3 = 0;

				// Read off palette indices and increment color pointer
				if (y < _size_y-1) {
					u32 c2 = color[_size_x];
					p2 = _map[c2];
				}
				if (x < _size_x-1) {
					if (y < _size_y-1) {
						u32 c3 = color[_size_x + 1];
						p3 = _map[c3];
					}

					u32 c1 = color[1];
					p1 = _map[c1];
					++color;
				}
				++color;

				// Store packed pixels
				*image++ = (p3 << 6) | (p2 << 4) | (p1 << 2) | p0;
			}
		}
	} else if (_palette_size > 1) { // 1 bit/pixel
		/*
		 * Combine blocks of 8 pixels together:
		 *
		 * 0 0 0 0 1 1 1 1 2 2 2
		 * 0 0 0 0 1 1 1 1 2 2 2 <- example 11x3 image
		 * 3 3 3 3 4 4 4 4 5 5 5
		 *
		 * Each 4x2 block is packed like so:
		 * 0 1 2 3  -->  HI:[ 7 6 5 4 3 2 1 0 ]:LO
		 * 4 5 6 7
		 */
		_pack_x = (_size_x + 3) >> 2;
		_pack_y = (_size_y + 1) >> 1;
		const int image_size = _pack_x * _pack_y;
		_image.resize(image_size);

		const u32 *color = reinterpret_cast<const u32 *>( _rgba );
		u8 *image = _image.get();

		for (int y = 0; y < _size_y; y += 2) {
			for (int x = 0, xend = _size_x; x < xend; x += 4) {
				u8 b = 0;

				for (int jj = 0; jj < 2; ++jj) {
					for (int ii = 0; ii < 4; ++ii) {
						int px = x + ii, py = y + jj;

						b <<= 1;

						if (px < _size_x && py < _size_y) {
							u32 c = color[px + py * _size_x];

							b |= (u8)_map[c];
						}
					}
				}

				*image++ = b;
			}
		}
	}
	// Else: 0 bits per pixel, just need to transmit palette
}

int SmallPalette::init(const u8 *rgba, int size_x, int size_y, const GCIFKnobs *knobs) {
	_knobs = knobs;
	_rgba = rgba;
	_size_x = size_x;
	_size_y = size_y;

	// Off by default
	_palette_size = 0;

	// If palette was generated,
	if (generatePalette()) {
		// Generate palette raster
		generateImage();
	}

	return GCIF_WE_OK;
}

void SmallPalette::write(ImageWriter &writer) {
	if (enabled()) {
		writer.writeBit(1);
		writeTable(writer);
	} else {
		writer.writeBit(0);
	}
}

void SmallPalette::writeTable(ImageWriter &writer) {
	int bits = 0;

	CAT_DEBUG_ENFORCE(SMALL_PALETTE_MAX <= 16);

	writer.writeBits(_palette_size - 1, 4);
	bits += 4;

	for (int ii = 0; ii < _palette_size; ++ii) {
		u32 color = getLE(_palette[ii]);

		writer.writeWord(color);
		bits += 32;
	}

#ifdef CAT_COLLECT_STATS
	Stats.overhead_bits = bits;
#endif
}

#ifdef CAT_COLLECT_STATS

bool SmallPalette::dumpStats() {
	if (!enabled()) {
		CAT_INANE("stats") << "(Small Palette) Disabled.";
	} else {
		CAT_INANE("stats") << "(Small Palette)  Size : " << Stats.palette_size << " colors";
		CAT_INANE("stats") << "(Small Palette) Table : " << Stats.overhead_bits / 8 << " bytes";
	}

	return true;
}

#endif
