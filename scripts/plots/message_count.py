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

convergence_columns = [
    "ID",
    "Port",
    "Gossip fanout",
    "Members",
    "Members in an update",
    "Gossip rounds",
    "Received messages count",
    "Received messages size",
    "Time until convergence",
]

plt.rcParams["font.size"] = 12
plt.rcParams["figure.figsize"] = 20.7, 5.7

plt.rcParams["axes.titlesize"] = 19
plt.rcParams["axes.labelsize"] = 20
plt.rcParams["xtick.labelsize"] = 20
plt.rcParams["ytick.labelsize"] = 20


def gossip_message_count_comparison(files):
    dfs = []
    for file in files:
        x = pd.read_csv(
            file,
            names=convergence_columns,
            header=None,
        )
        x.drop(x.columns[[0]], axis=1, inplace=True)
        iteration = int(file.split("_")[-1].split(".")[0])
        x["Iteration"] = iteration
        dfs.append(x)

    combined = pd.concat(dfs, ignore_index=True)
    combined = combined.drop(columns=["Port"])
    combined["Members in an update"] = combined["Members in an update"] + 1

    groups = combined.groupby(
        ["Gossip fanout", "Members", "Members in an update", "Iteration"]
    )
    groups = groups.mean().reset_index()

    df_fanout_2 = groups.loc[groups["Gossip fanout"] == 2]
    df_fanout_3 = groups.loc[groups["Gossip fanout"] == 3]
    df_fanout_4 = groups.loc[groups["Gossip fanout"] == 4]

    rounds_fanout_2 = df_fanout_2.groupby(["Members", "Members in an update"])[
        "Received messages count"
    ].mean()
    rounds_fanout_3 = df_fanout_3.groupby(["Members", "Members in an update"])[
        "Received messages count"
    ].mean()
    rounds_fanout_4 = df_fanout_4.groupby(["Members", "Members in an update"])[
        "Received messages count"
    ].mean()

    df = pd.concat(
        [rounds_fanout_2, rounds_fanout_3, rounds_fanout_4],
        axis=1,
        keys=["fanout2", "fanout3", "fanout4"],
    )

    df["dec_f3"] = (df["fanout2"] - df["fanout3"]) / df["fanout2"] * 100
    df["dec_f4"] = (df["fanout2"] - df["fanout4"]) / df["fanout2"] * 100
    dec_f3 = ((df["fanout2"] - df["fanout3"]) / df["fanout2"] * 100).rename("Fanout 3")
    dec_f4 = ((df["fanout2"] - df["fanout4"]) / df["fanout2"] * 100).rename("Fanout 4")

    table = pd.concat([dec_f3, dec_f4], axis=1)
    table = table.unstack(level=1)
    table.index.name = "Nodes"
    table.columns = pd.MultiIndex.from_product(
        [["Fanout of 3 nodes", "Fanout of 4 nodes"], ["2 N/M", "3 N/M", "4 N/M"]]
    )

    print(table.round(2))


def gossip_message_count(files):
    dfs = []
    for file in files:
        x = pd.read_csv(
            file,
            names=convergence_columns,
            header=None,
        )
        x.drop(x.columns[[0]], axis=1, inplace=True)
        iteration = int(file.split("_")[-1].split(".")[0])
        x["Iteration"] = iteration
        dfs.append(x)

    combined = pd.concat(dfs, ignore_index=True)
    combined = combined.drop(columns=["Port"])
    combined["Members in an update"] = combined["Members in an update"] + 1

    groups = combined.groupby(
        ["Gossip fanout", "Members", "Members in an update", "Iteration"]
    )
    groups = groups.mean().reset_index()

    df_fanout_2 = groups.loc[groups["Gossip fanout"] == 2]
    df_fanout_3 = groups.loc[groups["Gossip fanout"] == 3]
    df_fanout_4 = groups.loc[groups["Gossip fanout"] == 4]

    plots, axes = plt.subplots(1, 3, sharey=True)

    sns.barplot(
        data=df_fanout_2,
        x="Members",
        y="Received messages count",
        hue="Members in an update",
        palette=palette,
        ax=axes[0],
        zorder=3,
        errorbar=None,
    )
    sns.barplot(
        data=df_fanout_3,
        x="Members",
        y="Received messages count",
        hue="Members in an update",
        palette=palette,
        ax=axes[1],
        zorder=3,
        errorbar=None,
    )
    sns.barplot(
        data=df_fanout_4,
        x="Members",
        y="Received messages count",
        hue="Members in an update",
        palette=palette,
        ax=axes[2],
        zorder=3,
        errorbar=None,
    )

    for i in range(0, 3):
        for container in axes[i].containers:
            axes[i].bar_label(
                container,
                fmt="%.2f",
                label_type="edge",
                padding=5,
                fontname="RobotoMono Nerd Font",
                fontsize=14,
                rotation=50,
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
        axes[i].set_ylim(0, 400)
        axes[i].set_xlabel("Nodes", fontname="Helvetica")
        axes[i].set_title(f"Gossip fanout: {i+2}")

        for tick in axes[i].get_xticklabels():
            tick.set_fontname("RobotoMono Nerd Font")

        for tick in axes[i].get_yticklabels():
            tick.set_fontname("RobotoMono Nerd Font")

        for bar in axes[i].patches:
            x = bar.get_x()
            width = bar.get_width()
            centre = x + width / 2
            bar.set_width(width * 0.75)

        legend = axes[i].legend(
            title="Nodes per update:",
            title_fontsize=16,
            loc="upper left",
            framealpha=0,
            facecolor="white",
            edgecolor="white",
            fontsize=14,
            columnspacing=0.8,
            ncol=3,
        )
        legend._legend_box.align = "left"

    plt.tight_layout()
    plt.savefig("gossip_message_count.pdf", bbox_inches="tight")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", help="Path to result files", required=True)
    args = parser.parse_args()

    files = list(glob.glob(f"{args.path}/*.csv"))
    gossip_message_count(files)
    gossip_message_count_comparison(files)
