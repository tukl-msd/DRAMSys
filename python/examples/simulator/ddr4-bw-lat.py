import json
import argparse
import copy
import numpy as np
import polars as pl
import seaborn as sns
import matplotlib.pyplot as plt

from pathlib import Path
from multiprocessing import Pool
from dramsys.simulation.simulator import Simulator
from dramsys.simulation.simstats import calculate_stats

parser = argparse.ArgumentParser(description="Example simulation")
parser.add_argument("simulator", type=str, help="path to the simulator executable")
parser.add_argument(
    "-o",
    "--outdir",
    type=str,
    default="simout-multi",
    help="path to the output directory",
)

options = parser.parse_args()
outdir = Path(options.outdir)
dramsys = Path(options.simulator)

config_json = Path(__file__).parent.resolve() / "configs" / "ddr4-bw-lat.json"

with open(config_json) as base_json:
    base_config = json.load(base_json)

simulators = []

frequencies = np.logspace(0, 2, num=100)
rw_ratios = np.linspace(0.5, 1.0, num=6)
# patterns = ["sequential", "random"]
patterns = ["sequential"]

for pattern in patterns:
    for rw_ratio in rw_ratios:
        for frequency in frequencies:
            simid = f"ddr4-example-{pattern}-{rw_ratio}-{frequency}"
            temp_config = copy.deepcopy(base_config)

            temp_config["simulation"]["simulationid"] = simid

            cores = 4
            channels = 1

            for i in range(cores):
                addr_start = i * 8589934592 * channels / cores
                addr_end = (i + 1) * 8589934592 * channels / cores - 1

                generator = {
                    "type": "generator",
                    "name": "gen0",
                    "numRequests": 2000000,
                    "maxPendingReadRequests": 8,
                    "maxPendingWriteRequests": 8,
                    "minAddress": addr_start,
                    "maxAddress": addr_end,
                }

                generator["addressDistribution"] = pattern
                generator["rwRatio"] = rw_ratio
                generator["clkMhz"] = frequency

                temp_config["simulation"]["tracesetup"].append(generator.copy())

            parameters = {
                "pattern": pattern,
                "rw_ratio": rw_ratio,
                "frequency": frequency,
            }

            simdir = outdir / simid

            simulator = Simulator(dramsys, simdir, temp_config, parameters)
            simulators.append(simulator)


def simulate_and_calculate_metrics(simulator: Simulator):
    simulator.run()

    artifacts = simulator.get_arifacts()

    stats = calculate_stats(artifacts)
    stats.write_csv(simulator.simulation_dir / "stats.csv")

    return stats


def calculate_metrics(simulator: Simulator):
    artifacts = simulator.get_arifacts()

    stats = calculate_stats(artifacts)
    stats.write_csv(simulator.simulation_dir / "stats.csv")

    return stats


def load_metrics(simulator: Simulator):
    stats = pl.read_csv(simulator.simulation_dir / "stats.csv")
    return stats


with Pool() as pool:
    # simulation_stats = pool.map(simulate_and_calculate_metrics, simulators)
    simulation_stats = pool.map(load_metrics, simulators)

all_stats = []

for simulator, stats in zip(simulators, simulation_stats):
    # Add parameters to dataframe TODO do it in simulator class?
    for key, value in simulator.parameters.items():
        stats = stats.with_columns(pl.lit(value).alias(key))

    all_stats.append(stats)

df = pl.concat(all_stats, how="vertical")
print(df)

# df = df.filter(pl.col("channel") < 6)
df = df.group_by(["frequency", "rw_ratio", "pattern"], maintain_order=True).agg(
    [pl.mean("avg_latency"), pl.sum("bandwidth")]
)
# df = df.with_columns(pl.col("bandwidth") * 0.1164153218) # Gbit/s -> GiB/s
df = df.with_columns(pl.col("bandwidth") / 8)  # Gbit/s -> GiB/s
df = df.sort("frequency")

# df = df.filter(pl.col("rw_ratio") == 0.6)
print(df)

ax = sns.FacetGrid(
    df,
    col="pattern",
    hue="rw_ratio",
    palette=sns.color_palette("light:b", n_colors=10, desat=0.8),
    sharey=True,
    margin_titles=True,
)
ax.map(sns.lineplot, "bandwidth", "avg_latency", alpha=0.7, sort=False)
# ax.set(xscale="log")
plt.legend()

# df_seq = df.filter(pl.col("pattern") == "sequential")
# df_ran = df.filter(pl.col("pattern") == "random")

# ax = sns.lineplot(
#     df,
#     x="bandwidth",
#     y="avg_latency",
#     hue="rw_ratio",
#     alpha=0.7,
# )

ax.set(xlabel="Bandwidth [GB/s]", ylabel="Average Latency [ns]")

plt.ylim(bottom=0)
plt.show()
