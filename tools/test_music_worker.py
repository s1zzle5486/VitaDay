#!/usr/bin/env python3
"""Exercise the actual audio worker with desktop SDK shims, not a reimplementation.
This checks state/queue/ownership behavior; real Vita BGM and power require hardware.
"""
from pathlib import Path
import os,subprocess,shlex
root=Path(__file__).resolve().parent.parent
out=root/'work/music-worker';out.mkdir(parents=True,exist_ok=True)
library=out/'library'
source=(root/'src/music_vita.hpp').read_text().replace('#pragma once','')
source='\n'.join(line for line in source.splitlines() if not line.startswith('#include <psp2/'))
source=source.replace('#include "music.hpp"',f'#include "{root}/src/music.hpp"').replace('#include "dr_mp3.h"',f'#include "{root}/src/dr_mp3.h"')
source=source.replace('ux0:data/VitaDay/music',str(library)).replace('ux0:music',str(library))
prefix=(root/'tests/music_worker_prefix.hpp').read_text()
suffix=(root/'tests/music_worker_suffix.hpp').read_text().replace('work/music/library',str(library)).replace('outputs/VitaDay/tests/fixtures/',str(root/'tests/fixtures')+'/')
# The fixture generator isn't a runtime dependency; original tone MP3s are committed.
file=out/'test.cpp';file.write_text(prefix+'\n#include <functional>\n'+source+'\n'+suffix)
binary=out/'test'
subprocess.run(shlex.split(os.environ.get('CXX','c++'))+['-std=c++17','-O1','-g','-fsanitize=address,undefined',str(file),'-o',str(binary)],check=True)
subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
