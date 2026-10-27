#!/usr/bin/env python3
"""Audio-continuity regression test for the Garou recompilation.

Runs a scripted session (default: tests/play_long.txt) and checks that the
captured WAV does not contain a long silent run after the boot/attract
period.  The bug this guards against was the Z80 M1 bank mapper reading
64 KB too high, which corrupted the sound driver's music data and produced
long BGM dropouts.

Usage:
  python tests/audio_regression.py --exe build/Garou --rom-path build/roms
  python tests/audio_regression.py --exe Garou.exe --rom-path roms \
      --input tests/play_long.txt --frames 19000

Requires only the Python standard library.  The game and ROMs must already
be available; this is an integration test, not a CI unit test.
"""
import argparse
import array
import math
import os
import subprocess
import sys
import tempfile
import wave


def per_second_rms(path):
    w = wave.open(path, "rb")
    ch = w.getnchannels()
    rate = w.getframerate()
    n = w.getnframes()
    samples = array.array("h")
    samples.frombytes(w.readframes(n))
    w.close()
    out = []
    for start in range(0, n, rate):
        end = min(n, start + rate)
        # Every 8th interleaved sample is enough for an energy check.
        seg = samples[start * ch:end * ch:8 * ch]
        if not seg:
            continue
        rms = math.sqrt(sum(x * x for x in seg) / len(seg))
        out.append((start / rate, rms))
    peak = max(abs(x) for x in samples) if samples else 0
    return out, peak, n / rate


def silent_runs(windows, threshold=100.0):
    runs = []
    start = prev = None
    for t, rms in windows:
        if rms < threshold:
            if start is None:
                start = t
            prev = t
        else:
            if start is not None:
                runs.append((start, prev))
                start = None
    if start is not None:
        runs.append((start, prev))
    return runs


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True, help="path to Garou/Garou.exe")
    ap.add_argument("--rom-path", required=True, help="prepared ROM directory")
    ap.add_argument("--input", default=os.path.join(here, "play_long.txt"))
    ap.add_argument("--frames", type=int, default=11000,
                    help="frames to run (default 11000, through character "
                         "select and the early match but before the continue "
                         "screen's intentional silence)")
    ap.add_argument("--interp", action="store_true",
                    help="run the 68000 in the interpreter")
    ap.add_argument("--max-silence", type=float, default=3.0,
                    help="maximum allowed silent run after --skip seconds")
    ap.add_argument("--skip", type=float, default=20.0,
                    help="ignore boot/initial loading before this time")
    args = ap.parse_args()

    for p in (args.exe, args.input):
        if not os.path.exists(p):
            print(f"error: missing {p}", file=sys.stderr)
            return 2

    with tempfile.TemporaryDirectory(prefix="garou-audio-") as td:
        wav = os.path.join(td, "audio.wav")
        cmd = [args.exe, "--headless", "--rom-path", args.rom_path,
               "--frames", str(args.frames), "--input", args.input,
               "--wav", wav]
        if args.interp:
            cmd.insert(2, "--interp")
        print("running:", " ".join(cmd))
        run = subprocess.run(cmd, text=True, capture_output=True)
        if run.stderr:
            print(run.stderr.rstrip())
        if run.returncode != 0:
            print(f"error: player exited {run.returncode}", file=sys.stderr)
            return 2
        if not os.path.exists(wav):
            print("error: no WAV was written", file=sys.stderr)
            return 2

        windows, peak, duration = per_second_rms(wav)

    runs = silent_runs(windows)
    bad = [(a, b) for a, b in runs if b >= args.skip and (b - a + 1) > args.max_silence]
    longest = max(((b - a + 1), a, b) for a, b in runs) if runs else (0, 0, 0)

    print(f"duration {duration:.1f}s, peak {peak}, "
          f"{len(windows)} one-second windows")
    print(f"silent windows (RMS<100): {len(runs)}; "
          f"longest {longest[0]:.0f}s at {longest[1]:.0f}-{longest[2]:.0f}s")
    if bad:
        print("FAIL: long silent run(s) after the boot period:", file=sys.stderr)
        for a, b in bad:
            print(f"  {a:.0f}-{b:.0f}s ({b - a + 1:.0f}s)", file=sys.stderr)
        return 1
    print("PASS: audio is continuous after the boot period")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
