import json
import argparse

from pathlib import Path
from dramsys.simulation.simulator import Simulator
from dramsys.simulation.simstats import calculate_stats

parser = argparse.ArgumentParser(description="Example simulation")
parser.add_argument("simulator", type=str, help="path to the simulator executable")
parser.add_argument(
    "-o",
    "--outdir",
    type=str,
    default="simout",
    help="path to the output directory",
)

options = parser.parse_args()
outdir = Path(options.outdir)
dramsys = Path(options.simulator)

config_json = Path(__file__).parent.resolve() / "configs" / "ddr4-example.json"

with open(config_json) as base_json:
    base_config = json.load(base_json)

simulator = Simulator(dramsys, outdir, base_config, [])
simulator.run()

artifacts = simulator.get_arifacts()

stats = calculate_stats(artifacts, [])
print(stats)

stats.write_csv(outdir / "stats.csv")
