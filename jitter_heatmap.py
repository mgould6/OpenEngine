# jitter_heatmap.py
GROUPS = {
  "root": ["root","pelvis"],
  "legs": ["thigh","shin","foot","toe"],
  "arms": ["shoulder","upper_arm","forearm","hand"],
  "spineHead": ["spine","neck","head"],
}
def group_of(bone):
    b = bone.lower()
    for g, keys in GROUPS.items():
        if any(k in b for k in keys):
            return g
    return "other"
# tally per group as you parse, then print a short summary

import re
from collections import defaultdict

LOG_FILE = "output.txt"




# Regex patterns
anim_pattern = re.compile(r"Set current animation to (\S+)", re.IGNORECASE)

# Updated regex to match current and old tags
jitter_pattern = re.compile(
    r"\[(?:JITTER-SMOOTH|PRE-BAKE-CLAMP|FIXED - SRT\+ROT|FIXED - ROT SPIKE)\]"
    r"(?: Anim=(?P<anim>\S+))? Bone=(?P<bone>\S+) Frame=(?P<frame>\d+)"
)

# Data structure: animation -> bone -> list of frames
anim_bone_frames = defaultdict(lambda: defaultdict(list))
current_anim = "UnknownAnimation"

with open(LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
    for line in f:
        anim_match = anim_pattern.search(line)
        if anim_match:
            current_anim = anim_match.group(1)
            continue

        jitter_match = jitter_pattern.search(line)
        if jitter_match:
            anim = jitter_match.group("anim") or current_anim
            bone = jitter_match.group("bone")
            frame = int(jitter_match.group("frame"))
            anim_bone_frames[anim][bone].append(frame)

# Output summary
if not anim_bone_frames:
    print("(No matching jitter suppression entries found.)")

group_counts = defaultdict(int)

for anim, bone_map in anim_bone_frames.items():
    print(f"\n=== Animation: {anim} ===")
    for bone, frames in sorted(bone_map.items(), key=lambda x: len(x[1]), reverse=True):
        unique_frames = sorted(set(frames))
        frames_str = ", ".join(map(str, unique_frames))
        print(f"{bone}: {len(unique_frames)} frames -> {frames_str}")
        group_counts[group_of(bone)] += len(unique_frames)

print("\n=== Group Summary ===")
for g, c in group_counts.items():
    print(f"{g}: {c} total suppressed frames")


