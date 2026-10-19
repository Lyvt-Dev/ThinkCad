# Local Share-Code format TC1

No network service is involved. `ProjectShare` serializes the normal ProjectIO JSON
(format 8), removes bundled library files, adds library name/version references,
then compresses compact JSON with zlib level 9. A share code is smaller than an
uncompressed project in typical cases; compression cannot guarantee savings for
already compressed contents. Use ZIP export when actual local library files are needed.

## Envelope

`TC1:` followed by unpadded base64url of:

- ASCII `TCS1` (4 bytes), flag (0 plaintext / 1 encrypted), uncompressed JSON length
  (4-byte big endian).
- Plaintext: zlib stream, then SHA-256 over header + stream (32 bytes).
- Encrypted: salt (16 bytes), XChaCha nonce (24 bytes), encrypted zlib stream,
  Poly1305 tag (16 bytes). The complete 49-byte header is authenticated as AD.

Encryption is Monocypher 4.0.2 XChaCha20-Poly1305, with Argon2id deriving a 32-byte
key: 32768 KiB, 3 passes, 1 lane, UTF-8 password, random 16-byte salt. These
parameters are fixed by TC1; changing them requires a new format. Randomness uses
Qt's system CSPRNG. Temporary key and KDF buffers are wiped. A password should be
long and unique and passed separately. Qt UI/string copies may remain in memory;
this is not a memory-forensic protection scheme. Password processing runs off the
GUI thread. Closing a dialog discards its result, not a forcibly terminated KDF.

Plain SHA-256 detects accidental damage, **not** malicious modification or sender
identity. Encrypted codes authenticate contents under the supplied password, not
who sent them. Imported sketches are native programs in ThinkCad's existing local
compiler process; inspect code before simulation. No automatic compilation or
library download is triggered by import.

## Bounds and import behavior

- Maximum uncompressed payload: 32 MiB; maximum code: 12 MiB.
- Strict envelope/version/base64 validation, bounded exact-size decompression,
  current/older project version checks and ProjectIO structural validation.
- At most 10,000 parts, 50,000 wires and 256 library references in the share wrapper.
- QR generation: at most 1200 UTF-8 bytes, medium error correction, quiet margin.
  A large project still has a text share code; QR capacity is not increased unsafely.
- Image input: at most 32 MiB / 16 megapixels. Multiple ThinkCad QR codes are rejected.
- Camera: explicitly opened native Qt Multimedia device, permission checked, throttled
  local decoding, one frame in flight; frames are not saved or uploaded.
- Preview never retains temporary embedded component registrations. It shows project
  name, board profiles, component count, project format and library requirements.
  Editing the code/password invalidates approval. Only explicit confirmation imports.
- Import produces a new unsaved project through normal save guards. Existing project
  files are never selected as the import destination. References do not install libraries.

## Dependencies / notices

Pinned sources and SHA-256 provenance are in `third_party/*/UPSTREAM*`:
Monocypher 4.0.2 (BSD-2-Clause / CC0), Nayuki qrcodegen (MIT), quirc 1.2 (ISC).
They are small native libraries; no browser, cloud account, external cryptographic
process or new network runtime is added. zlib was already a project dependency.
