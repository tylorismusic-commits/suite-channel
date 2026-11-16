#!/usr/bin/env python3
"""Comp Suite measurement harness.

Renders a reference JSFX (or our CLAP build) through REAPER headlessly and
reports the numbers the models are tuned against: static compression curve,
attack/release timing, and harmonic content.

SPDX-License-Identifier: GPL-3.0-or-later
"""

import math
import os
import struct
import subprocess
import sys
import wave

SR = 48000
OUT = "/tmp/opencode/cs/measure"


def write_wav(path, left, right=None, sr=SR):
    right = right if right is not None else left
    with wave.open(path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(3)
        w.setframerate(sr)
        frames = bytearray()
        for a, b in zip(left, right):
            for v in (a, b):
                iv = int(max(-1.0, min(1.0, v)) * 8388607)
                frames += struct.pack("<i", iv)[0:3]
        w.writeframes(bytes(frames))


def read_wav(path):
    with wave.open(path, "rb") as w:
        n = w.getnframes()
        ch = w.getnchannels()
        sw = w.getsampwidth()
        raw = w.readframes(n)
    out = [[], []]
    if sw == 3:
        for i in range(n):
            for c in range(min(ch, 2)):
                b = raw[(i * ch + c) * 3:(i * ch + c) * 3 + 3]
                v = int.from_bytes(b, "little", signed=False)
                if v & 0x800000:
                    v -= 1 << 24
                out[c].append(v / 8388608.0)
    elif sw == 4:
        for i in range(n):
            for c in range(min(ch, 2)):
                v = struct.unpack("<i", raw[(i * ch + c) * 4:(i * ch + c) * 4 + 4])[0]
                out[c].append(v / 2147483648.0)
    else:
        raise SystemExit("unsupported sample width %d" % sw)
    return out[0], out[1]


def db_to_amp(db):
    return 10.0 ** (db / 20.0)


def amp_to_db(a):
    return 20.0 * math.log10(max(a, 1e-12))


def rms(x, start, end):
    if end <= start:
        return 0.0
    s = 0.0
    for i in range(start, end):
        s += x[i] * x[i]
    return math.sqrt(s / (end - start))


def tune(name, freqs):
    """Sum of sine magnitudes near the given frequencies in a window."""
    n = len(freqs)
    return [0.0] * n


def bin_freq(bin_index, n=32768):
    """A frequency that lands exactly on a DFT bin, so leakage cannot be
    mistaken for distortion."""
    return bin_index * SR / n


def hann_window(n):
    return [0.5 - 0.5 * math.cos(2 * math.pi * i / (n - 1)) for i in range(n)]


def spectrum_peaks(x, start, length, freqs):
    """Goertzel DFT at each frequency.  Rectangular window: use bin-aligned
    frequencies so there is no leakage to confuse with distortion."""
    seg = x[start:start + length]
    win = [1.0] * len(seg)
    out = []
    for f in freqs:
        w = 2 * math.pi * f / SR
        c = 2 * math.cos(w)
        s1 = s2 = 0.0
        for i, v in enumerate(seg):
            s0 = v * win[i] + c * s1 - s2
            s2, s1 = s1, s0
        power = s1 * s1 + s2 * s2 - c * s1 * s2
        out.append(math.sqrt(max(power, 0.0)) / (len(seg) * 0.25))
    return out


# ---------------------------------------------------------------------------
# Test signals
# ---------------------------------------------------------------------------

LEVELS = [-48, -42, -36, -30, -24, -18, -12, -6, -3]


def make_levels(path):
    """Staircase of 1 kHz tones, one level per 0.75 s step."""
    step = int(0.75 * SR)
    left = []
    for db in LEVELS:
        a = db_to_amp(db)
        for i in range(step):
            left.append(a * math.sin(2 * math.pi * 1000 * i / SR))
    write_wav(path, left)
    return step


SEG = [(1.6, -30), (2.0, -10), (2.0, -30)]


def make_step(path):
    """-30 dBFS bell then a -10 dBFS step then back down."""
    seg = SEG
    left = []
    for dur, db in seg:
        a = db_to_amp(db)
        n = int(dur * SR)
        for i in range(n):
            left.append(a * math.sin(2 * math.pi * 1000 * i / SR))
    write_wav(path, left)
    return seg


def make_tone(path, freq, db, dur):
    a = db_to_amp(db)
    n = int(dur * SR)
    write_wav(path, [a * math.sin(2 * math.pi * freq * i / SR) for i in range(n)])


# ---------------------------------------------------------------------------
# REAPER rendering
# ---------------------------------------------------------------------------

def rpp_js(fx_path, sliders):
    vals = " ".join(str(v) for v in sliders)
    return ('    <JS "%s" ""\n'
            '      %s - - -\n'
            '    >\n' % (fx_path, vals))


def rpp_clap(identifier, params):
    """CLAP FX chunk. params is {clap_param_name: value}."""
    lines = ''.join(
        '      <PARAM id="%s" value="%s" />\n' % (k, repr(float(v)).lstrip('0') or '0')
        for k, v in params.items())
    return ('    <VST "" "CompSuite: CompSuite" 0 "" 1919247213<565354...> ""\n'
            '    %s    >\n' % lines)


RPP_TEMPLATE = """<REAPER_PROJECT 0.1 "7.84/linux-x86_64" 1700000000
  RIPPLE 0
  GROUPOVERRIDE 0 0 0
  AUTOXFADE 129
  SAMPLERATE {sr} 0 0
  RENDER_FILE "{outdir}"
  RENDER_PATTERN "{pattern}"
  RENDER_FMT 0 2 0
  RENDER_1X 0
  RENDER_RANGE 1 0 0 18 1000
  RENDER_ADDTOPROJ 0
  RENDER_DITHER 0
  <TRACK
    NAME test
    VOLPAN 1 0 -1 -1 1
    MUTESOLO 0 0 0
    IPHASE 0
    ISBUS 0 0
    NCHAN 2
    FX 1
    TRACKID {{11111111-2222-3333-4444-555555555555}}
    <FXCHAIN
{fx}    >
    <ITEM
      POSITION 0
      SNAPOFFS 0
      LENGTH {length:.6f}
      LOOP 1
      ALLTAKES 0
      FADEIN 1 0 0 1 0 0 0
      FADEOUT 1 0 0 1 0 0 0
      <SOURCE WAVE
        FILE "{wav}"
      >
    >
  >
>
"""


def build_rpp(path, wav, fx_chunks, length, render_file):
    outdir = os.path.dirname(render_file)
    pattern = os.path.basename(render_file)[:-4]
    fx = "".join(fx_chunks)
    with open(path, "w") as f:
        f.write(RPP_TEMPLATE.format(sr=SR, outdir=outdir, pattern=pattern,
                                    fx=fx, length=length, wav=wav))


def render(rpp, wav_path, fx_chunks, out_name):
    left, _ = read_wav(wav_path)
    length = len(left) / SR
    out = os.path.join(OUT, out_name + ".wav")
    if os.path.exists(out):
        os.remove(out)
    build_rpp(rpp, wav_path, fx_chunks, length, out)
    cmd = ["xvfb-run", "-a", "reaper", "-renderproject", rpp]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
    if not os.path.exists(out):
        print(r.stdout[-3000:])
        print(r.stderr[-3000:])
        raise SystemExit("render failed: " + out)
    return out


# ---------------------------------------------------------------------------
# Reference plugin settings and our model settings
# ---------------------------------------------------------------------------

TK = "Tukan/"

# Each reference: plugin, full slider list, and its drive control.
#   (slider_index, lo, hi, more_drive_gives_more_gr)
REFS = {
    "76blue": dict(fx=TK + "NC76 S2 (Tukan)",
                   sliders=[0, 1, 0, 0, 100, 250, 0, 0, 0, 0],   # 4:1, 100us, 250ms
                   drive=(2, -20.0, 30.0, True)),
    "76black": dict(fx=TK + "NC76B (Tukan)",
                    sliders=[0, 5, -10, 100, 250, 100, 0.77, 0, 0, 0, 4.9, 0, 0, 1],
                    drive=(7, 0.05, 4.0, True)),               # linear input gain
    "2a": dict(fx=TK + "LA-1A (Tukan)",
               sliders=[-40, 4, 0, 3, 0, 0, 0, 1, 20, 0, 5, 1, 0, 0],
               drive=(9, 0.0, 60.0, True)),                    # peak reduction
    "3a": dict(fx=TK + "LA-2KAN S2 (Tukan)",
               sliders=[33.333, 0, 33.333, 0, 0, 1, 0, 0, 0, 10],
               drive=(0, 0.0, 100.0, True)),                   # peak reduction
    "xpress": dict(fx=TK + "Dis-Treasure (Tukan)",
                   sliders=[0, 0, 0, 4, 5, 5, 15, 775, 0, 0, 0, 0, 0, 0],
                   drive=(4, 0.0, 10.0, True)),                # in gain
    "vca": dict(fx=TK + "Bus Comp S2 (Tukan)",
                sliders=[-10, 4, 5, 100, 0, 0, 0, 1,
                         -10, 4, 5, 100, 0, 0],
                drive=(0, -30.0, 0.0, False)),                 # threshold
}

# Ours: model index plus the control used as drive.
OURS = {
    "76blue": dict(model=0, drive=("input", True)),
    "76black": dict(model=1, drive=("input", True)),
    "2a": dict(model=2, drive=("threshold", True)),
    "3a": dict(model=3, drive=("threshold", True)),
    "xpress": dict(model=4, drive=("input", True)),
    "vca": dict(model=5, drive=("threshold", False)),
}


def write_raw(path, left, right=None, sr=SR):
    right = right if right is not None else left
    with open(path, "wb") as f:
        for a, b in zip(left, right):
            f.write(struct.pack("<ff", a, b))


def read_raw(path):
    with open(path, "rb") as f:
        data = f.read()
    n = len(data) // 8
    left = [0.0] * n
    right = [0.0] * n
    for i in range(n):
        left[i], right[i] = struct.unpack_from("<ff", data, i * 8)
    return left, right


def render_ref(name, wav, out_name, drive_value=None):
    spec = REFS[name]
    sliders = list(spec["sliders"])
    if drive_value is not None:
        sliders[spec["drive"][0]] = drive_value
    return render(os.path.join(OUT, name + ".rpp"), wav,
                  [rpp_js(spec["fx"], sliders)], out_name)


def render_ours(name, wav, out_name, drive_value=None):
    """Renders through our DSP via raw float files, then writes a WAV."""
    src_l, src_r = read_wav(wav)
    raw_in = os.path.join(OUT, "in.f32")
    raw_out = os.path.join(OUT, "out.f32")
    write_raw(raw_in, src_l, src_r)
    args = ["model=%d" % OURS[name]["model"]]
    if drive_value is not None:
        args.append("%s=%.6f" % (OURS[name]["drive"][0], drive_value))
    cmd = ["build/comp_offline_render", raw_in, raw_out, str(SR)] + args
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit("offline render failed: " + r.stderr)
    l, rr = read_raw(raw_out)
    out = os.path.join(OUT, out_name + ".wav")
    write_wav(out, l, rr)
    return out


def analyse_levels(wav, step, dry_wav=None):
    """Wet level per input step plus the local slope (effective ratio)."""
    wet, _ = read_wav(wav)
    dry, _ = read_wav(dry_wav) if dry_wav else (wet, wet)
    rows = []
    n = min(len(wet), len(dry))
    for i, db in enumerate(LEVELS):
        a = int(i * step + step * 0.35)
        b = int(i * step + step * 0.9)
        if b > n:
            b = n
        rows.append((db, amp_to_db(rms(wet, a, b))))
    return rows


def slopes(rows):
    """d(output)/d(input) between adjacent steps, 1.0 = no compression."""
    out = []
    for (d0, o0), (d1, o1) in zip(rows, rows[1:]):
        dd = d1 - d0
        out.append((o1 - o0) / dd if dd else 1.0)
    return out


def analyse_step(wav, seg, dry_wav):
    """Attack and release from the -30 -> -10 -> -30 step.

    Times are referenced to the gain reduction 500 ms after the step, not to
    the final value: program-dependent units keep increasing reduction for
    seconds, and a 90%-of-final rule then never triggers.
    """
    wet, _ = read_wav(wav)
    dry, _ = read_wav(dry_wav)
    n = min(len(wet), len(dry))
    n1 = int(seg[0][0] * SR)
    n2 = n1 + int(seg[1][0] * SR)
    win = 480

    def gr(i):
        a = max(0, i - win // 2)
        b = min(n, i + win // 2)
        return amp_to_db(rms(dry, a, b)) - amp_to_db(rms(wet, a, b))

    quiet = gr(n1 - 1000)
    ref = gr(n1 + int(0.5 * SR))
    settled = gr(n2 - 2000)

    t_attack = None
    if ref > 0.5:
        for i in range(n1, min(n2, n1 + int(1.0 * SR))):
            if gr(i) >= 0.63 * ref:
                t_attack = (i - n1) / SR * 1000.0
                break
    goal = max(quiet, 0.0) + 1.0
    t_release = None
    for i in range(n2, n):
        if gr(i) <= goal:
            t_release = (i - n2) / SR * 1000.0
            break
    return quiet, settled, t_attack, t_release


def effective_gr(wav, dry_wav):
    """Gain reduction at the loud step relative to the quiet step.  Measuring
    from a below-threshold level cancels static auto-makeup, which would
    otherwise make a heavily made-up unit look like an expander."""
    wet, _ = read_wav(wav)
    dry, _ = read_wav(dry_wav)
    n = min(len(wet), len(dry))
    n1 = int(SEG[0][0] * SR)
    n2 = n1 + int(SEG[1][0] * SR)

    def gain(start, end):
        end = min(end, n)
        a = max(0, end - 4800)
        return amp_to_db(rms(wet, a, end)) - amp_to_db(rms(dry, a, end))

    return gain(n1 - 4800, n1 - 480) - gain(n2 - 4800, n2 - 480)


def clipped(wav):
    """True when a render hit full scale, which makes harmonic figures
    meaningless because the WAV writer has hard-clipped them."""
    left, right = read_wav(wav)
    return max(max(abs(v) for v in left), max(abs(v) for v in right)) >= 0.999


def analyse_harmonics(wav, freq, db, dur):
    left, _ = read_wav(wav)
    start = int(len(left) * 0.6)
    length = 32768
    if start + length > len(left):
        start = max(0, len(left) - length)
    mags = spectrum_peaks(left, start, length, [freq * k for k in range(1, 8)])
    if mags[0] <= 0:
        return 0.0, 0.0, 0.0
    fund = mags[0]
    h2 = 20 * math.log10(max(mags[1] / fund, 1e-9))
    h3 = 20 * math.log10(max(mags[2] / fund, 1e-9))
    thd = math.sqrt(sum(m * m for m in mags[1:])) / fund
    series = [20 * math.log10(max(m / fund, 1e-9)) for m in mags[1:]]
    return h2, h3, 20 * math.log10(max(thd, 1e-9)), series


def search_ref_drive(name, step_wav, target_gr, iterations=7):
    idx, lo, hi, more_is_up = REFS[name]["drive"]
    for _ in range(iterations):
        mid = 0.5 * (lo + hi)
        out = render_ref(name, step_wav, name + "_search", mid)
        gr = effective_gr(out, step_wav)
        if (gr < target_gr) == more_is_up:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def search_ours_drive(name, step_wav, target_gr, iterations=22):
    param, more_is_up = OURS[name]["drive"]
    lo, hi = 0.0, 1.0
    for _ in range(iterations):
        mid = 0.5 * (lo + hi)
        out = render_ours(name, step_wav, name + "_search", mid)
        gr = effective_gr(out, step_wav)
        if (gr < target_gr) == more_is_up:
            lo = mid
        else:
            hi = mid
    return 0.5 * (lo + hi)


def match(models, target_gr=8.0):
    """Compare each pair at the same operating point: the drive that gives
    `target_gr` dB of gain reduction on the test step."""
    os.makedirs(OUT, exist_ok=True)
    step_wav = os.path.join(OUT, "match_step.wav")
    make_step(step_wav)
    f0 = bin_freq(70)
    tone_wav = os.path.join(OUT, "match_tone.wav")
    make_tone(tone_wav, f0, -10, 2.0)

    print("target: %.1f dB gain reduction\n" % target_gr)
    print("%-8s %10s %10s %6s  %8s %8s  %8s %8s" %
          ("model", "ref drive", "our drive", "GR", "ref H2", "our H2",
           "ref THD", "our THD"))
    for name in models:
        ref_drive = search_ref_drive(name, step_wav, target_gr)
        our_drive = search_ours_drive(name, step_wav, target_gr)
        param, _ = OURS[name]["drive"]
        ref_step = render_ref(name, step_wav, name + "_ref_st", ref_drive)
        our_step = render_ours(name, step_wav, name + "_our_st", our_drive)
        ref_tone = render_ref(name, tone_wav, name + "_ref_hn", ref_drive)
        our_tone = render_ours(name, tone_wav, name + "_our_hn", our_drive)

        rh2, _, rthd, _ = analyse_harmonics(ref_tone, f0, -10, 2.0)
        oh2, _, othd, _ = analyse_harmonics(our_tone, f0, -10, 2.0)
        gr = effective_gr(our_step, step_wav)
        print("%-8s %10.2f %10.2f %6.1f  %8.1f %8.1f  %8.1f %8.1f" %
              (name, ref_drive, our_drive, gr, rh2, oh2, rthd, othd))
        if clipped(ref_tone) or clipped(our_tone):
            print("         [clipped at this operating point]")
        # Timing at the matched setting.
        for label, path in (("ref", ref_step), ("ours", our_step)):
            q, l, ta, tr = analyse_step(path, SEG, step_wav)
            print("         %-4s attack %-7s release %-7s" %
                  (label, "%.1f ms" % ta if ta else "-",
                   "%.0f ms" % tr if tr else "-"))
        print()


def main():
    which = sys.argv[1] if len(sys.argv) > 1 else "smoke"
    if which == "smoke":
        os.makedirs(OUT, exist_ok=True)
        tone = os.path.join(OUT, "tone.wav")
        make_tone(tone, 1000, -10, 2.0)
        sliders = [0, 3, 12, 0, 100, 250, 0, 0, 0, 0]
        out = render(os.path.join(OUT, "smoke.rpp"), tone,
                     [rpp_js("Tukan/NC76 S2 (Tukan)", sliders)], "smoke")
        l, _ = read_wav(out)
        print("in rms %.2f dB  out rms %.2f dB" % (
            amp_to_db(rms(read_wav(tone)[0], SR // 2, SR)),
            amp_to_db(rms(l, SR // 2, SR))))
    elif which == "match":
        match(sys.argv[2:] or ["76blue"])
    else:
        models = sys.argv[2:] or ["76blue"]
        suite(models)


if __name__ == "__main__":
    main()
