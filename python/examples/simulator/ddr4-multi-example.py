import json
import argparse
import copy
import polars as pl

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

config_json = Path(__file__).parent.resolve() / "configs" / "ddr4-example.json"

with open(config_json) as base_json:
    base_config = json.load(base_json)

simulators = []

page_policies = ["Open", "Closed"]
patterns = ["sequential", "random"]

for page_policy in page_policies:
    for pattern in patterns:
        simid = f"ddr4-example-{page_policy}-{pattern}"
        temp_config = copy.deepcopy(base_config)

        temp_config["simulation"]["simulationid"] = simid
        temp_config["simulation"]["mcconfig"]["PagePolicy"] = page_policy

        simdir = outdir / simid

        simulator = Simulator(dramsys, simdir, temp_config)
        simulators.append(simulator)


def simulate_and_calculate_metrics(simulator: Simulator):
    simulator.run()

    artifacts = simulator.get_arifacts()

    stats = calculate_stats(artifacts)
    stats.write_csv(outdir / "stats.csv")

    return stats


def calculate_metrics(simulator: Simulator):
    artifacts = simulator.get_arifacts()

    stats = calculate_stats(artifacts)
    stats.write_csv(outdir / "stats.csv")

    return stats


def load_metrics(simulator: Simulator):
    stats = pl.read_csv(outdir / "stats.csv")
    return stats


with Pool() as pool:
    simulation_stats = pool.map(simulate_and_calculate_metrics, simulators)

for simulator, stats in zip(simulators, simulation_stats):
    print(simulator.get_id(), stats)
