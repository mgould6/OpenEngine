# jitter_summary.py
import re
from collections import defaultdict

LOG_FILE = "output.txt"

# Updated regex to match current and old tags
pattern = re.compile(
    r"\[(?:JITTER-SMOOTH|PRE-BAKE-CLAMP|FIXED - SRT\+ROT|FIXED - ROT SPIKE)\]"
    r"(?: Anim=(?P<anim>\S+))? Bone=(?P<bone>\S+) Frame=(?P<frame>\d+)"
)

bone_counts = defaultdict(int)
bone_frames = defaultdict(list)

with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
    for line in f:
        match = pattern.search(line)
        if match:
            bone = match.group("bone")
            frame = int(match.group("frame"))
            bone_counts[bone] += 1
            bone_frames[bone].append(frame)

# Sort bones by count
sorted_bones = sorted(bone_counts.items(), key=lambda x: x[1], reverse=True)

print("=== Jitter Suppression Summary ===")
if not sorted_bones:
    print("(No matching jitter suppression entries found.)")

for bone, count in sorted_bones:
    frames = sorted(set(bone_frames[bone]))
    frames_str = ", ".join(map(str, frames))
    print(f"{bone}: {count} frames -> {frames_str}")
