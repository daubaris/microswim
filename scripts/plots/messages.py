import argparse
import seaborn as sns
import matplotlib.pyplot as plt
import pandas as pd

plt.rcParams["font.size"] = 12
plt.rcParams["figure.figsize"] = 20.7, 5.7
plt.rcParams["axes.labelsize"] = 14
plt.rcParams["xtick.labelsize"] = 14
plt.rcParams["ytick.labelsize"] = 14

palette = [
    "#082a54",
    "#e02b35",
    "#f0c571",
]


def message_sizes(df):
    plt.rcParams["axes.labelsize"] = 20
    plt.rcParams["figure.figsize"] = 14.4, 8.1
    plt.rcParams["xtick.labelsize"] = 18
    plt.rcParams["ytick.labelsize"] = 18
    plot = sns.barplot(
        df.loc[df["type"] == "encoding"],
        x="members_per_message",
        y="message_size",
        hue="format",
        palette=palette,
        zorder=3,
    )

    plot.grid(
        True,
        which="major",
        axis="both",
        color="gray",
        linestyle="--",
        linewidth=0.5,
        zorder=0,
        alpha=0.3,
    )
    plot.set_ylim(0, 1200)
    plot.set_xlabel("Number of nodes in a message", fontname="Helvetica")
    plot.set_ylabel("Message size in bytes", fontname="Helvetica")

    for tick in plot.get_xticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    for tick in plot.get_yticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    for bar in plot.patches:
        x = bar.get_x()
        width = bar.get_width()
        centre = x + width / 2
        bar.set_width(width * 0.75)

    for container in plot.containers:
        plot.bar_label(
            container,
            fmt="%d",
            label_type="edge",
            padding=5,
            fontname="RobotoMono Nerd Font",
            fontsize=15,
        )

    legend = plot.legend(
        title="Format:",
        title_fontsize=18,
        loc="upper left",
        framealpha=0,
        facecolor="white",
        edgecolor="white",
        fontsize=16,
        columnspacing=0.8,
        ncol=3,
    )
    legend._legend_box.align = "left"
    plt.savefig("message_size.pdf", bbox_inches="tight")
    plt.close()


def message_processing(df, type):
    plt.rcParams["axes.labelsize"] = 20
    plt.rcParams["figure.figsize"] = 14.4, 8.1
    plt.rcParams["xtick.labelsize"] = 18
    plt.rcParams["ytick.labelsize"] = 18

    plot = sns.pointplot(
        df.loc[df["type"] == type],
        x="members_per_message",
        y="real_time",
        hue="format",
        palette=palette,
        zorder=3,
    )

    plot.grid(
        True,
        which="major",
        axis="both",
        color="gray",
        linestyle="--",
        linewidth=0.5,
        zorder=0,
        alpha=0.3,
    )

    ymax = max(df.loc[df["type"] == type]["real_time"])
    plot.set_ylim(0, ymax + 1)
    plot.set_xlabel("Number of nodes in a message", fontname="Helvetica")
    plot.set_ylabel("Time (µs)", fontname="Helvetica")

    for tick in plot.get_xticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    for tick in plot.get_yticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    offset = 0.2

    for i, line in enumerate(plot.lines):
        x_data = line.get_xdata()
        y_data = line.get_ydata()

        for x, y in zip(x_data, y_data):
            other_y = []
            for j, other_line in enumerate(plot.lines):
                if j != i:
                    x_other = other_line.get_xdata()
                    y_other = other_line.get_ydata()
                    if x in x_other:
                        idx = list(x_other).index(x)
                        other_y.append(y_other[idx])

            label_offset = offset
            va = "bottom"

            if any(oy <= y + offset + 0.5 and oy > y for oy in other_y):
                label_offset = -offset
                va = "top"

            plot.text(
                x,
                y + label_offset,
                f"{y:.2f}",
                ha="center",
                va=va,
                fontsize=18,
                fontname="RobotoMono Nerd Font",
            )

    legend = plot.legend(
        title="Format:",
        title_fontsize=18,
        loc="upper left",
        framealpha=0,
        facecolor="white",
        edgecolor="white",
        fontsize=16,
        columnspacing=0.8,
        ncol=3,
    )
    legend._legend_box.align = "left"
    plt.savefig(f"{type}.pdf", bbox_inches="tight")
    plt.close()


def enc_dec_trns(df, type):
    encoding = pd.Series(
        df.loc[(df["format"] == type) & (df["type"] == "encoding")]["real_time"].values,
        name="Encoding Time",
    )
    decoding = pd.Series(
        df.loc[(df["format"] == type) & (df["type"] == "decoding")]["real_time"].values,
        name="Decoding Time",
    )
    message_size = pd.Series(
        df.loc[(df["format"] == type) & (df["type"] == "encoding")][
            "message_size"
        ].values
        * 8
        / 10000000,
        name="Transmission Time",
    )

    dfs = pd.concat(
        [
            encoding / 1e6,
            decoding / 1e6,
            message_size,
        ],
        axis=1,
    )

    return dfs


def message_transmission(df):
    cbor = enc_dec_trns(df, "CBOR")
    json = enc_dec_trns(df, "JSON")
    cbor["type"] = "CBOR"
    cbor["members_per_message"] = list(range(1, 10))
    cbor["Total Latency"] = (
        cbor["Encoding Time"] + cbor["Decoding Time"] + cbor["Transmission Time"]
    ) * 1e6
    json["type"] = "JSON"
    json["members_per_message"] = list(range(1, 10))
    json["Total Latency"] = (
        json["Encoding Time"] + json["Decoding Time"] + json["Transmission Time"]
    ) * 1e6
    dfs = pd.concat([cbor, json], axis=0)

    plt.rcParams["axes.labelsize"] = 20
    plt.rcParams["figure.figsize"] = 14.4, 8.1
    plt.rcParams["xtick.labelsize"] = 18
    plt.rcParams["ytick.labelsize"] = 18

    plot = sns.pointplot(
        dfs,
        x="members_per_message",
        y="Total Latency",
        hue="type",
        palette=palette,
        zorder=3,
    )

    plot.grid(
        True,
        which="major",
        axis="both",
        color="gray",
        linestyle="--",
        linewidth=0.5,
        zorder=0,
        alpha=0.3,
    )

    ymax = max(dfs["Total Latency"])
    plot.set_ylim(0, ymax + 100)
    plot.set_xlabel("Number of nodes in a message", fontname="Helvetica")
    plot.set_ylabel("Time (µs)", fontname="Helvetica")

    for tick in plot.get_xticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    for tick in plot.get_yticklabels():
        tick.set_fontname("RobotoMono Nerd Font")

    offset = 35

    for i, line in enumerate(plot.lines):
        x_data = line.get_xdata()
        y_data = line.get_ydata()

        for x, y in zip(x_data, y_data):
            other_y = []
            for j, other_line in enumerate(plot.lines):
                if j != i:
                    x_other = other_line.get_xdata()
                    y_other = other_line.get_ydata()
                    if x in x_other:
                        idx = list(x_other).index(x)
                        other_y.append(y_other[idx])

            label_offset = offset
            va = "bottom"

            if any(oy <= y + offset + 100 and oy > y for oy in other_y):
                label_offset = -offset
                va = "top"

            plot.text(
                x,
                y + label_offset,
                f"{y:.2f}",
                ha="center",
                va=va,
                fontsize=18,
                fontname="RobotoMono Nerd Font",
            )

    legend = plot.legend(
        title="Format:",
        title_fontsize=18,
        loc="upper left",
        framealpha=0,
        facecolor="white",
        edgecolor="white",
        fontsize=16,
        columnspacing=0.8,
        ncol=3,
    )
    legend._legend_box.align = "left"
    plt.savefig(f"total_latency.pdf", bbox_inches="tight")
    plt.close()


def messages(path):
    dfs = []
    cbor = pd.read_csv(f"{path}/cbor.csv")
    json = pd.read_csv(f"{path}/json.csv")

    cbor = cbor.drop(
        columns=[
            "time_unit",
            "bytes_per_second",
            "items_per_second",
            "label",
            "error_occurred",
            "error_message",
            "cpu_time",
            "iterations",
        ]
    )
    json = json.drop(
        columns=[
            "time_unit",
            "bytes_per_second",
            "items_per_second",
            "label",
            "error_occurred",
            "error_message",
            "cpu_time",
            "iterations",
        ]
    )

    values = cbor["name"].str.split("_").str[3].str.split("/")
    cbor["type"] = values.str[0]
    cbor["members_per_message"] = values.str[1]
    cbor["format"] = "CBOR"

    values = json["name"].str.split("_").str[3].str.split("/")
    json["type"] = values.str[0]
    json["members_per_message"] = values.str[1]
    json["format"] = "JSON"

    df = pd.concat([cbor, json], ignore_index=True)
    df.rename(
        columns={
            "message_size": "Message Size in Bytes",
            "members_per_message": "Number of Members in a Message",
            "format": "Format",
        }
    )

    df["real_time"] = df["real_time"] * 0.001
    message_sizes(df)
    message_processing(df, "encoding")
    message_processing(df, "decoding")
    message_transmission(df)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", help="Path to result files", required=True)
    args = parser.parse_args()
    messages(args.path)
