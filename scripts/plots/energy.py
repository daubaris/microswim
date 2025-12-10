import glob
import argparse
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

from matplotlib.lines import Line2D
from matplotlib.patches import Patch

palette = [
    "#082a54",
    "#e02b35",
    "#f0c571",
]

handles = [
    Patch(linewidth=1.0, facecolor="white", edgecolor="black", label="Energy"),
    Line2D(
        [0],
        [0],
        color="black",
        linestyle="--",
        marker="x",
        markeredgecolor="black",
        markeredgewidth=2,
        markersize=10,
        label="Packets",
    ),
]


protocols = ["CoAP", "MQTT", "μswim"]

plt.rcParams["font.size"] = 12
plt.rcParams["figure.figsize"] = 20.7, 5.7
plt.rcParams["axes.titlesize"] = 19
plt.rcParams["axes.labelsize"] = 16
plt.rcParams["xtick.labelsize"] = 16
plt.rcParams["ytick.labelsize"] = 16


def cumulative_energy_and_packets(path):
    coap = pd.read_csv(f"{path}/monsoon_coap.csv")
    mqtt = pd.read_csv(f"{path}/monsoon_mqtt.csv")
    gossip = pd.read_csv(f"{path}/monsoon_gossip.csv")

    coap_packets = pd.read_csv(f"{path}/coap_packets.csv")
    mqtt_packets = pd.read_csv(f"{path}/mqtt_packets.csv")
    gossip_packets = pd.read_csv(f"{path}/gossip_packets.csv")

    coap["Protocol"] = "CoAP"
    mqtt["Protocol"] = "MQTT"
    gossip["Protocol"] = "μswim"

    data = {
        "CoAP": {"energy": 0, "packets": len(coap_packets.index)},
        "MQTT": {"energy": 0, "packets": len(mqtt_packets.index)},
        "μswim": {"energy": 0, "packets": len(gossip_packets.index)},
    }

    for i, p in enumerate(protocols):
        df = pd.concat([coap, mqtt, gossip])
        df = df.loc[df["Protocol"] == p]

        df["Time_s"] = df["Time(ms)"].astype(float)
        print("Time range (s):", df["Time_s"].min(), "→", df["Time_s"].max())
        df["dt"] = df["Time_s"].diff().fillna(0)
        df.loc[df["dt"] < 0, "dt"] = 0

        df["Current_A"] = df["USB(mA)"] / 1000.0
        df["Power_W"] = df["Current_A"] * df["USB Voltage(V)"]
        df["Energy_J"] = df["Power_W"] * df["dt"]

        data[p]["energy"] = df["Energy_J"].sum()

    df = pd.DataFrame(data).T.reset_index().rename(columns={"index": "protocol"})

    fig, ax1 = plt.subplots(figsize=(8, 5), constrained_layout=True)

    ax1.set_axisbelow(True)
    ax1.grid(True, axis="both", linestyle="--", linewidth=0.5, alpha=0.3)

    sns.barplot(data=df, x="protocol", y="energy", color="white", ax=ax1)

    hatches = ["///", "xxx", "\\\\\\"]

    for bar, hatch in zip(ax1.patches, hatches):
        bar.set_hatch(hatch)
        bar.set_edgecolor("black")
        bar.set_linewidth(1.8)

    ax1.set_ylabel("Energy (J)", fontname="Helvetica")
    ax1.set_ylim(0, 400 * 1.001)

    ax1.set_xticks(range(len(df["protocol"])))
    ax1.set_xticklabels(
        df["protocol"],
        fontname="RobotoMono Nerd Font",
        fontsize=16,
    )

    bars = ax1.patches
    old_width = bars[0].get_width()
    new_width = 0.4

    for bar in bars:
        bar.set_width(new_width)
        bar.set_x(bar.get_x() + (old_width - new_width) / 2)

    ax2 = ax1.twinx()

    sns.lineplot(
        data=df,
        x="protocol",
        y="packets",
        ax=ax2,
        marker="x",
        linestyle="--",
        markeredgecolor="black",
        markeredgewidth=2,
        markersize=10,
        linewidth=1,
        color="black",
    )

    ax2.set_ylabel("Number of Packets", fontname="Helvetica")
    ax1.set_xlabel("")
    ax2.set_ylim(0, 1800)

    for tick in ax1.get_xticklabels():
        tick.set_fontname("Helvetica")

    for tick in ax1.get_yticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    for tick in ax2.get_yticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    ax1.legend_.remove() if ax1.get_legend() else None
    ax2.legend_.remove() if ax2.get_legend() else None

    ax2.margins(y=0)

    ax1.legend(
        handles=handles,
        loc="upper left",
        frameon=False,
        borderaxespad=0.0,
    )

    plt.show()


def energy(path):
    coap = pd.read_csv(f"{path}/monsoon_coap.csv")
    mqtt = pd.read_csv(f"{path}/monsoon_mqtt.csv")
    gossip = pd.read_csv(f"{path}/monsoon_gossip.csv")

    coap["Protocol"] = "CoAP"
    mqtt["Protocol"] = "MQTT"
    gossip["Protocol"] = "μswim"

    plots, axes = plt.subplots(1, 3, sharey=True)

    for i, p in enumerate(protocols):

        df = pd.concat([coap, mqtt, gossip])
        df = df.loc[df["Protocol"] == p]

        df["Time_s"] = df["Time(ms)"].astype(float)
        print("Time range (s):", df["Time_s"].min(), "→", df["Time_s"].max())
        df["dt"] = df["Time_s"].diff().fillna(0)
        df.loc[df["dt"] < 0, "dt"] = 0

        df["Current_A"] = df["USB(mA)"] / 1000.0
        df["Power_W"] = df["Current_A"] * df["USB Voltage(V)"]
        df["Energy_J"] = df["Power_W"] * df["dt"]

        print(f"[{p}] Total duration (s):", df["Time_s"].iloc[-1])
        print(f"[{p}] Total energy (J):", df["Energy_J"].sum())

        window_size = 10  # seconds
        df["window"] = (df["Time_s"] // window_size).astype(int)
        window_energy = df.groupby("window")["Energy_J"].sum().reset_index()
        window_energy["start_s"] = window_energy["window"] * window_size
        print("Number of windows:", len(window_energy))
        axes[i].set_ylim(0, 3)
        sns.lineplot(
            data=window_energy,
            x="start_s",
            y="Energy_J",
            color="black",
            ax=axes[i],
            zorder=3,
            errorbar=None,
        )
        axes[i].grid(
            True,
            which="major",
            axis="both",
            color="gray",
            linestyle="--",
            linewidth=0.5,
            zorder=0,
            alpha=0.3,
        )

        axes[i].set_xlabel("Time (s)", fontname="Helvetica")
        axes[0].set_ylabel("Energy per 10 s window (J)", fontname="Helvetica")
        axes[i].set_title(p)

        for tick in axes[i].get_xticklabels():
            tick.set_fontname("RobotoMono Nerd Font")

        for tick in axes[i].get_yticklabels():
            tick.set_fontname("RobotoMono Nerd Font")
    plt.tight_layout()
    plt.savefig("energy.pdf", bbox_inches="tight")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", help="Path to result files", required=True)
    args = parser.parse_args()

    cumulative_energy_and_packets(args.path)
