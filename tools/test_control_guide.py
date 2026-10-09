#!/usr/bin/env python3
"""Exercise the actual native guide worker with native pixel loading and GPU/thread mocks."""
from pathlib import Path
import os,subprocess
root=Path(__file__).resolve().parent.parent
out=root/'work';out.mkdir(exist_ok=True)
prefix=(root/'tests/control_guide_render_prefix.cpp').read_text().replace('"../src/','"'+str(root/'src')+'/')
pos=prefix.index('static std::string guideFile(')
prefix=prefix[:pos]+'static const std::string assetRoot="'+str(root/'assets')+'";\n'+prefix[pos:]
main=(root/'src/main.cpp').read_text().split('static vita2d_texture* controlGuideTexture=',1)[1].split('static vita2d_texture* roundedTexture=',1)[0].replace('controlGuidePath(controlGuideJobLanguage)','guideFile(controlGuideJobLanguage)')
source=out/'control_guide_render.cpp';source.write_text(prefix+'static vita2d_texture* controlGuideTexture='+main+(root/'tests/control_guide_render_suffix.cpp').read_text())
binary=out/'control_guide_render'
subprocess.run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined',str(source),'-o',str(binary)],check=True)
subprocess.run([str(binary)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True)
