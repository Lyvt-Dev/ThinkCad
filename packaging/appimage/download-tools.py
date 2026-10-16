#!/usr/bin/env python3
"""Fetch checksum-pinned AppImage tools. Mutable upstream URLs fail closed."""
import hashlib,json,pathlib,sys,urllib.request
root=pathlib.Path(sys.argv[1]);root.mkdir(parents=True,exist_ok=True)
for item in json.loads(pathlib.Path(__file__).with_name('tools.json').read_text()):
 target=root/item['name']
 if not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest()!=item['sha256']:
  temporary=target.with_suffix(target.suffix+'.download')
  with urllib.request.urlopen(item['url'],timeout=120) as response,temporary.open('wb') as output:
   while chunk:=response.read(1024*1024):output.write(chunk)
  if hashlib.sha256(temporary.read_bytes()).hexdigest()!=item['sha256']:
   temporary.unlink();raise SystemExit('SHA256 mismatch: '+item['name'])
  temporary.replace(target)
 target.chmod(0o755)
