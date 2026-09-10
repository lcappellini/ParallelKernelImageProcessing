import csv
import os
from collections import defaultdict
from matplotlib.ticker import ScalarFormatter

import matplotlib.pyplot as plt
import seaborn as sns

resolutions = {
    (640, 360): "nHD",
    (960, 540): "qHD",
    (1280, 720): "HD",
    (1366, 768): "WXGA",
    (1600, 900): "HD+",
    (1920, 1080): "Full HD",
    (2560, 1440): "QHD",
    (3840, 2160): "4K UHD",
    (5120, 2880): "5K",
    (7680, 4320): "8K UHD",
    (15360, 8640): "16K UHD",
}

def median(lst):
    lst = tuple(sorted(lst))
    n = len(lst)
    if n % 2 == 0:
        return (lst[n//2] + lst[n//2-1]) / 2
    return lst[n//2]

sns.set_theme(style="whitegrid", context="talk")

CSV_FILES = [e for e in os.listdir() if e.endswith(".csv") and e.startswith("timings_")]

data = defaultdict(lambda: defaultdict(list))
aosvssoa = defaultdict(lambda: defaultdict(list))
aosvssoa_noavx2 = defaultdict(lambda: defaultdict(list))
image_sizes = {}

for filename in CSV_FILES:
    t = filename[len("timings_"):-len(".csv")].replace("_", "+")

    with open(filename, "r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)

        for row in reader:
            width, height = map(int, row["size"].split("x"))
            n = width * height
            image_sizes[n] = (width, height)
            ms = float(row["microseconds"]) / 1000

            if t == "AoSvsSoA":
                aosvssoa[row["mode"]][n].append(ms)
            elif t == "AoSvsSoA+noAVX2":
                aosvssoa_noavx2[row["mode"]][n].append(ms)
            else:
                data[t][n].append(ms)

avg = defaultdict(dict)
for t, pixels in data.items():
    for n, times in pixels.items():
        avg[t][n] = median(times)

avg_aosvssoa = defaultdict(dict)
for mode, pixels in aosvssoa.items():
    for n, times in pixels.items():
        avg_aosvssoa[mode][n] = median(times)

avg_aosvssoa_noavx2 = defaultdict(dict)
for mode, pixels in aosvssoa_noavx2.items():
    for n, times in pixels.items():
        avg_aosvssoa_noavx2[mode][n] = median(times)

techniques = ["BASE", "AVX2", "OpenMP", "OpenMP+AVX2", "CUDA"]
sorted_pixels = {t: sorted(avg[t].keys()) for t in techniques}

palette = sns.color_palette("tab10", len(techniques))
colors = dict(zip(techniques, palette))

mode_palette = sns.color_palette("tab10", 2)
mode_colors = {"AoS": mode_palette[0], "SoA": mode_palette[1]}


# Graph for each technique
##for t in techniques:
##    x = sorted_pixels[t]
##    y = [avg[t][n] for n in x]
##
##    plt.figure(figsize=(10, 6))
##    plt.plot(x, y, linewidth=2.5, color=colors[t], label=t)
##
##    plt.xlabel("Pixel number")
##    plt.ylabel("Elapsed time (ms)")
##    plt.title(t)
##
##    plt.tight_layout()
##    plt.savefig("graphs\\Graph " + t + ".png")


# Graph AoS vs SoA
plt.figure(figsize=(11, 7))

for mode in ["AoS", "SoA"]:
    x = sorted(avg_aosvssoa[mode].keys())
    y = [avg_aosvssoa[mode][n] for n in x]
    plt.plot(x, y, linewidth=2.5, color=mode_colors[mode], label=mode)

plt.xlabel("Pixel number")
plt.ylabel("Elapsed time (ms)")
plt.title("AoS vs SoA")
plt.legend()

plt.tight_layout()
plt.savefig("graphs\\Graph AoS vs SoA.png")


# Graph AoS vs SoA without AVX2
plt.figure(figsize=(11, 7))

for mode in ["AoS", "SoA"]:
    x = sorted(avg_aosvssoa_noavx2[mode].keys())
    y = [avg_aosvssoa_noavx2[mode][n] for n in x]
    plt.plot(x, y, linewidth=2.5, color=mode_colors[mode], label=mode)

plt.xlabel("Pixel number")
plt.ylabel("Elapsed time (ms)")
plt.title("AoS vs SoA - no AVX2")
plt.legend()

plt.tight_layout()
plt.savefig("graphs\\Graph AoS vs SoA no AVX2.png")


# Graph All techniques
plt.figure(figsize=(12, 7))

for t in techniques:
    x = sorted_pixels[t]
    y = [avg[t][n] for n in x]
    plt.plot(x, y, linewidth=2.5, color=colors[t], label=t)

plt.xlabel("Pixel number")
plt.ylabel("Elapsed time (ms)")
plt.title("All techniques")
plt.legend()

plt.tight_layout()
plt.savefig("graphs\\Graph All.png")


# Bar graph All techniques
plt.figure(figsize=(14, 7))

bar_data = {"technique": [], "size": [], "ms": []}

for t in techniques:
    for n in sorted_pixels[t]:
        bar_data["technique"].append(t)
        bar_data["size"].append(resolutions[image_sizes[n]])
        bar_data["ms"].append(avg[t][n])

sns.barplot(data=bar_data, x="size", y="ms", hue="technique", hue_order=techniques, palette=colors, errorbar=None)

plt.xlabel("Image size")
plt.ylabel("Elapsed Time (ms) - Log scale")
plt.yscale("log")
plt.gca().yaxis.set_major_formatter(ScalarFormatter())
plt.title("All techniques")
plt.legend()

plt.tight_layout()
plt.savefig("graphs\\Graph All bar.png")


techniques_no_base = [t for t in techniques if t != "BASE"]

# Graph BASE vs <tecnica>, con speedup annotato su ogni punto
##for t in techniques_no_base:
##    x = sorted(set(avg["BASE"].keys()) & set(avg[t].keys()))
##    base_y = [avg["BASE"][n] for n in x]
##    t_y = [avg[t][n] for n in x]
##    speedups = [avg["BASE"][n] / avg[t][n] for n in x]
##
##    plt.figure(figsize=(12, 7))
##    plt.plot(x, base_y, linewidth=2.5, color=colors["BASE"], label="BASE")
##    plt.plot(x, t_y, linewidth=2.5, color=colors[t], label=t)
##
##    for n, y, s in zip(x, t_y, speedups):
##        plt.annotate(f"{s:.2f}x", (n, y), textcoords="offset points", xytext=(0, 8), ha="center", fontsize=10)
##
##    plt.xlabel("Pixel number")
##    plt.ylabel("Elapsed time (ms)")
##    plt.title(f"Base vs {t}")
##    plt.legend()
##    plt.tight_layout()
##    plt.savefig(f"graphs\\Graph BASE vs {t}.png")


# Grafico unico riassuntivo dello speedup, tutte le tecniche
plt.figure(figsize=(12, 7))

for t in techniques_no_base:
    x = sorted(set(avg["BASE"].keys()) & set(avg[t].keys()))
    speedups = [avg["BASE"][n] / avg[t][n] for n in x]
    plt.plot(x, speedups, marker="o", linewidth=2.5, color=colors[t], label=t)

plt.axhline(y=1, color=palette[0], linestyle="--", linewidth=1.5, label="Base")
plt.xlabel("Pixel number")
plt.ylabel("Speedup")
plt.title("Speedup - All techniques")
plt.legend()

plt.tight_layout()
plt.savefig("graphs\\Graph All Speedup.png")
