#!/usr/bin/env python3
"""Build and run offline desktop checks from any working directory."""
import os
from pathlib import Path
import shlex
import subprocess

root = Path(__file__).resolve().parent.parent
out = root / "work" / "host-tests"
out.mkdir(parents=True, exist_ok=True)
(root / "work").mkdir(exist_ok=True)
compiler = shlex.split(os.environ.get("CXX", "c++"))
for source in sorted((root / "tests").glob("*_test.cpp")):
    name = source.stem
    command = compiler + ["-std=c++17", "-O1", "-g", "-fsanitize=address,undefined", str(source), "-o", str(out / name)]
    if name == "cjk_bitmap_test":
        command += ["-lz"]
    if name in ["png_test", "control_icons_test"]:
        command += [str(root / "src/lodepng.cpp"), "-DLODEPNG_NO_COMPILE_ENCODER"]
    if "network.hpp" in source.read_text():
        command += ["-lcurl"]
    print("Building", name, flush=True)
    subprocess.run(command, cwd=root, check=True)
    args = [str(out / name)]
    if name == "png_test":
        args += [str(root / "assets/icons")]
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    subprocess.run(args, cwd=root, env=env, check=True)
print("All offline desktop tests passed.")
