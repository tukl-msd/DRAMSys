import polars as pl
import sqlite3

from typing import Callable, Any
from dataclasses import dataclass
from dramsys.simulation.simulator import SimulationArtifacts


@dataclass(frozen=True)
class DatabaseStatistic:
    name: str
    calculate_function: Callable[[sqlite3.Connection], Any]


def calculate_stats(artifacts: SimulationArtifacts, stats: list[DatabaseStatistic]) -> pl.DataFrame:
    data: dict = {}

    for channel, database in artifacts.databases.items():
        data.setdefault("channel", []).append(channel)

        connection = sqlite3.connect(database)

        for statistic in stats:
            try:
                value = statistic.calculate_function(connection)
            except Exception as error:
                print(
                    f"Warning: Could not calculate metric {statistic.name} for {database}: {error}"
                )

            data.setdefault(statistic.name, []).append(value)

    df = pl.DataFrame(data)
    return df
