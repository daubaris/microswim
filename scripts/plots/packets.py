import glob
import argparse
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

palette = [
    "#082a54",
    "#e02b35",
    "#f0c571",
]

protocols = ["CoAP", "MQTT", "μswim"]

plt.rcParams["font.size"] = 12
plt.rcParams["figure.figsize"] = 20.7, 5.7
plt.rcParams["axes.titlesize"] = 19
plt.rcParams["axes.labelsize"] = 20
plt.rcParams["xtick.labelsize"] = 20
plt.rcParams["ytick.labelsize"] = 20


def packets(path):
    coap = pd.read_csv(f"{path}/coap_io.csv")
    mqtt = pd.read_csv(f"{path}/mqtt_io.csv")
    gossip = pd.read_csv(f"{path}/gossip_io.csv")

    coap["Protocol"] = "CoAP"
    mqtt["Protocol"] = "MQTT"
    gossip["Protocol"] = "μswim"

    df = pd.concat([coap, mqtt, gossip])

    plots, axes = plt.subplots(1, 3, sharey=True)

    for i, p in enumerate(protocols):
        sns.lineplot(
            data=df.loc[df["Protocol"] == p],
            x="Interval start",
            y="Filtered packets",
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
        axes[0].set_ylabel("Packets per second", fontname="Helvetica")
        axes[i].set_title(p)

        for tick in axes[i].get_xticklabels():
            tick.set_fontname("RobotoMono Nerd Font")

        for tick in axes[i].get_yticklabels():
            tick.set_fontname("RobotoMono Nerd Font")

    plt.tight_layout()
    plt.savefig("packets.pdf", bbox_inches="tight")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", help="Path to result files", required=True)
    args = parser.parse_args()

    packets(args.path)
