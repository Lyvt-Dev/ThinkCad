#!/usr/bin/env python3
"""Create matching source and Gentoo overlay release archives from the Git commit."""
import hashlib
import lzma
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile

version, output = sys.argv[1:]
out = Path(output).resolve()
out.mkdir(parents=True, exist_ok=True)
source = out / f"ThinkCad-{version}-Source.tar.xz"
with lzma.open(source, "wb", preset=6) as stream:
    stream.write(subprocess.check_output(["git", "archive", f"--prefix=ThinkCad-{version}/", "HEAD"]))
overlay = out / "gentoo-overlay"
shutil.copytree("packaging/gentoo", overlay, dirs_exist_ok=True)
data = source.read_bytes()
manifest = (f"DIST {source.name} {len(data)} BLAKE2B {hashlib.blake2b(data).hexdigest()} "
            f"SHA512 {hashlib.sha512(data).hexdigest()}\n")
(overlay / "media-gfx/thinkcad/Manifest").write_text(manifest)
with tarfile.open(out / f"ThinkCad-{version}-Gentoo-overlay.tar.xz", "w:xz") as archive:
    archive.add(overlay, arcname="thinkcad")
shutil.rmtree(overlay)
