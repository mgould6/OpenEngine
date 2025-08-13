import itertools, subprocess, sys, json, re
from pathlib import Path

ANIMS = ["animations/Stance1.fbx"]  # extend as needed
BONES = ["root","DEF-toe.L","DEF-toe.R","DEF-upper_arm.L","DEF-upper_arm.R"]  # start with hotspots

# Experiment grid: small, safe ranges around current defaults
t_vals  = [0.0015, 0.0020, 0.0030]
r_vals  = [0.25, 0.35, 0.50]
wins    = [2]

# Your engine should read these from env or a json file at startup.
cfg_path = Path("jitter_config.json")

best = {}

def run_engine(cfg):
    cfg_path.write_text(json.dumps(cfg))
    # Run your exe so it loads jitter_config.json and logs to output.txt
    subprocess.run(["OpenEngine.exe"], check=True)

def score_output():
    log = Path("output.txt").read_text(encoding="utf-8", errors="ignore")
    # Lower is better: total number of JITTER-SMOOTH hits (proxy for suppression)
    hits = len(re.findall(r"\[(?:JITTER-SMOOTH|PRE-BAKE-CLAMP|FIXED - SRT\+ROT|FIXED - ROT SPIKE)\]", log))
    # Penalty for DRIFT warnings (over-smoothing smell)
    drift = len(re.findall(r"\[DRIFT\]", log))
    return hits + 3*drift

for anim in ANIMS:
    best[anim] = None, 1e9
    for t, r, w in itertools.product(t_vals, r_vals, wins):
        cfg = {
          "default": {"t": t, "rDeg": r, "window": w},
          "overrides": {
              # you can pin early hypotheses per bone here if desired
          }
        }
        run_engine(cfg)
        s = score_output()
        if s < best[anim][1]:
            best[anim] = (cfg, s)
            print("NEW BEST", anim, cfg, "score", s)
print("FINAL", best)
