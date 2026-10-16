frames.webm is a synthetic, silent, one-second VP9 test pattern (32×32, 4 FPS).
Generated with: ffmpeg -f lavfi -i testsrc2=size=32x32:rate=4:duration=1 -c:v libvpx-vp9 -lossless 1 -an frames.webm
The fixture contains no third-party video content. It exercises real decoding,
bitmap export and installed multimedia plugin availability, without requiring
an encoder executable on the user's machine.
