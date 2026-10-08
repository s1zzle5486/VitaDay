#!/usr/bin/env python3
"""Reject an ELF that cannot be safely relocated by Vita's loader."""
import re
import subprocess
import sys

elf, readelf = sys.argv[1:3]
relocations = subprocess.check_output([readelf, "-rW", elf], text=True)
required = ("R_ARM_ABS32", "R_ARM_THM_MOVW_ABS_NC", "R_ARM_THM_MOVT_ABS")
missing = [kind for kind in required if not re.search(r"\b" + kind + r"\b", relocations)]
if missing:
    sys.exit("ELF relocation validation failed; missing " + ", ".join(missing) + ". Link with -Wl,-q.")
print("ELF absolute data and Thumb address relocations: PASS")
