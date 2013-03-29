gcif
====

Game Closure Image Format

This is a work-in-progress towards a new RGBA image format that works well for
our spritehseets.

It is expected to produce files about half the size of the equivalent PNG
formatted images.  And it is expected to decompress three times faster than the
equivalent PNG formatted image (with PNGCRUSH) using libpng.


What works right now
====================

Currently, 1-bit alpha channel compression works, which is a major advance we
are pushing over BCIF for the final version of the codec.  You can try it out
with the test images and view statistics during compression and decompression
at the console.

Here's an example:

~~~
 $ ./gcif
USAGE: ./gcif [options] [output file path]

Options:
  --[h]elp                             Print usage and exit.
  --[v]erbose                          Verbose console output
  --[s]ilent                           No console output (even on errors)
  --[c]ompress <input PNG file path>   Compress the given .PNG image.
  --[d]ecompress <input GCI file path> Decompress the given .GCI image
  --[t]est <input PNG file path>       Test compression to verify it is lossless

Examples:
  ./gcif -tv ./original.png
  ./gcif -c ./original.png test.gci
  ./gcif -d ./test.gci decoded.png
~~~

Compression:

~~~
 $ ./gcif -c original.png test.gci
[Dec 31 16:00] <png> Inflate took 45773 usec
[Dec 31 16:00] <png> Processed input at 29.38 MB/S
[Dec 31 16:00] <png> Generated at 91.6551 MB/S
[Dec 31 16:00] <stats> (Mask Encoding)     Post-RLE Size : 24949 bytes
[Dec 31 16:00] <stats> (Mask Encoding)      Post-LZ Size : 21953 bytes
[Dec 31 16:00] <stats> (Mask Encoding) Post-Huffman Size : 18041 bytes (144325 bits)
[Dec 31 16:00] <stats> (Mask Encoding)        Table Size : 66 bytes (521 bits) [Golomb pivot = 0 bits]
[Dec 31 16:00] <stats> (Mask Encoding)      Filtering : 96 usec (4.64891 %total)
[Dec 31 16:00] <stats> (Mask Encoding)            RLE : 345 usec (16.707 %total)
[Dec 31 16:00] <stats> (Mask Encoding)             LZ : 1475 usec (71.4286 %total)
[Dec 31 16:00] <stats> (Mask Encoding)      Histogram : 20 usec (0.968523 %total)
[Dec 31 16:00] <stats> (Mask Encoding) Generate Table : 9 usec (0.435835 %total)
[Dec 31 16:00] <stats> (Mask Encoding)   Encode Table : 6 usec (0.290557 %total)
[Dec 31 16:00] <stats> (Mask Encoding)    Encode Data : 114 usec (5.52058 %total)
[Dec 31 16:00] <stats> (Mask Encoding)        Overall : 2065 usec
[Dec 31 16:00] <stats> (Mask Encoding) Throughput : 63.4731 MBPS (input bytes)
[Dec 31 16:00] <stats> (Mask Encoding) Throughput : 8.76804 MBPS (output bytes)
[Dec 31 16:00] <stats> (Mask Encoding) Ratio : 13.8138% (18106 bytes) of original data set (131072 bytes)
[Dec 31 16:00] <main> Wrote test.gci
~~~

Decompression:

~~~
 $ ./gcif -d test.gci output.png
[Dec 31 16:00] <stats> (Mask Decoding) Table Pivot : 0
[Dec 31 16:00] <stats> (Mask Decoding) Initialization : 7 usec (1.4 %total)
[Dec 31 16:00] <stats> (Mask Decoding)  Read Codelens : 4 usec (0.8 %total)
[Dec 31 16:00] <stats> (Mask Decoding)  Setup Huffman : 6 usec (1.2 %total)
[Dec 31 16:00] <stats> (Mask Decoding)     Huffman+LZ : 250 usec (50 %total)
[Dec 31 16:00] <stats> (Mask Decoding)     RLE+Filter : 233 usec (46.6 %total)
[Dec 31 16:00] <stats> (Mask Decoding)        Overall : 500 usec
[Dec 31 16:00] <stats> (Mask Decoding) Throughput : 36.216 MBPS (input bytes)
[Dec 31 16:00] <stats> (Mask Decoding) Throughput : 262.144 MBPS (output bytes)
[Dec 31 16:00] <main> Writing output image file: output.png
[Dec 31 16:00] <main> Read success!
 $ open decoded_mono.png
~~~

Note the output is actually dumped to "decoded_mono.png" and not the file you
specify.  This is just for testing the monochrome compression and will be
removed as the codec matures into full RGBA.

Some interesting things to note:

The PNG decoding rate is about 100 MB/s and our decoding rate is over twice as
fast.  This is about how it will be in the full RGBA version as well.

The compression ratio achieved by our monochrome encoder on this data is better
than PNG on all the files we've tested so far.  Sometimes by a little sometimes
by a lot.  Note this doesn't indicate performance for full RGBA data though.

