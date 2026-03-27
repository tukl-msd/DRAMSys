import json
import subprocess
import os
import re

from pathlib import Path
from dataclasses import dataclass


@dataclass(frozen=True)
class SimulationArtifacts:
    stdout: Path
    stderr: Path
    databases: dict[int, Path]


class Simulator:
    def __init__(self, dramsys: Path, out_dir: Path, config: dict, parameters):
        self.dramsys = dramsys
        self.config = config
        self.simulation_dir = out_dir
        self.parameters = parameters

        self.stdout_path = self.simulation_dir / "stdout.txt"
        self.stderr_path = self.simulation_dir / "stderr.txt"

    def run(self):
        config_path = self.__generate_config_json()

        with (
            open(self.stdout_path, "w", encoding="utf-8") as stdout_file,
            open(self.stderr_path, "w", encoding="utf-8") as stderr_file,
        ):
            command = [self.dramsys.absolute(), config_path.absolute()]
            subprocess.run(
                command,
                cwd=self.simulation_dir,
                stdout=stdout_file,
                stderr=stderr_file,
                check=True,
            )

    def get_id(self) -> str:
        return self.config["simulation"]["simulationid"]

    def get_arifacts(self) -> SimulationArtifacts:
        databases = {}

        for file in os.listdir(self.simulation_dir):
            if file.endswith(".tdb"):
                m = re.search("(?<=ch)[0-9]+", file)
                channel = int(m.group(0)) if m else -1

                databases[channel] = self.simulation_dir / file

        return SimulationArtifacts(self.stdout_path, self.stderr_path, databases)

    def __generate_config_json(self) -> Path:
        simulation_dir = self.simulation_dir
        simulation_dir.mkdir(parents=True, exist_ok=True)

        config_path = simulation_dir / "config.json"

        with open(config_path, "w") as f:
            json.dump(self.config, f, indent=4)

        return config_path
