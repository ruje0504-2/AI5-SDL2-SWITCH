#!/usr/bin/env python3
"""Extract an original PE icon and convert it to the Switch 256px JPEG icon."""
import argparse,struct,subprocess,shutil
from pathlib import Path
from extract_cursors import read_resources
p=argparse.ArgumentParser();p.add_argument('exe');p.add_argument('output');args=p.parse_args()
resources=read_resources(args.exe)
groups=sorted((key,value) for key,value in resources.items() if key[0]==14)
if not groups:raise SystemExit('No RT_GROUP_ICON resources')
key,group=groups[0]
reserved,kind,count=struct.unpack_from('<HHH',group)
assert reserved==0 and kind==1 and count>0
entries=[];images=[];offset=6+16*count
for i in range(count):
 entry=group[6+i*14:20+i*14]
 size,identifier=struct.unpack_from('<IH',entry,8)
 image=resources[(3,identifier,key[2])];assert len(image)==size
 entries.append(entry[:12]+struct.pack('<I',offset));images.append(image);offset+=size
out=Path(args.output);out.parent.mkdir(parents=True,exist_ok=True)
ico=out.with_suffix('.ico');ico.write_bytes(struct.pack('<HHH',0,1,count)+b''.join(entries)+b''.join(images))
if shutil.which('sips'):
    subprocess.run(['sips','-s','format','jpeg','-s','formatOptions','95','-z','256','256',str(ico),'--out',str(out)],check=True)
elif shutil.which('magick'):
    subprocess.run(['magick',str(ico)+'[0]','-background','white','-alpha','remove','-resize','256x256!','-quality','95',str(out)],check=True)
else:
    raise SystemExit('Install ImageMagick (magick) or use macOS sips to convert the original icon')
print(f'Original group {key[1]}, language {key[2]}, {count} image(s) -> {out}')
