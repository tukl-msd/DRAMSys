#!/usr/bin/env python3
"""
Split the valid experiment matrix into separate CSV files,
one per DRAM family (DDR3, DDR4, LPDDR4, LPDDR5, HBM2, etc.)

Input:
    valid_experiment_matrix.csv

Output folder:
    tier_matrices_csvs/

Output files:
    tier_<family>_valid_configs_matrix.csv
"""

import csv
from pathlib import Path
from collections import defaultdict


# ------------------------------------------------------
# Paths
# ------------------------------------------------------

EXPERIMENTS_DIR = Path("/Users/m.akhtari/JLR/DRAMSys/experiments")
INPUT_CSV = EXPERIMENTS_DIR / "valid_experiment_matrix.csv"
OUTPUT_DIR = EXPERIMENTS_DIR / "tier_matrices_csvs"

OUTPUT_DIR.mkdir(exist_ok=True)


# ------------------------------------------------------
# Helper: detect DRAM family from memspec filename
# ------------------------------------------------------

def extract_family_from_memspec(memspec_filename: str) -> str:
    s = memspec_filename.lower()

    # Order matters (lpddr before ddr, gddr before ddr)
    if "lpddr5" in s:
        return "lpddr5"
    if "lpddr4" in s:
        return "lpddr4"
    if "lpddr3" in s:
        return "lpddr3"
    if "ddr5" in s and "lpddr" not in s:
        return "ddr5"
    if "ddr4" in s and "lpddr" not in s:
        return "ddr4"
    if "ddr3" in s and "lpddr" not in s:
        return "ddr3"
    if "hbm2" in s:
        return "hbm2"
    if "wideio2" in s:
        return "wideio2"
    if "wideio" in s:
        return "wideio"
    if "gddr6" in s:
        return "gddr6"
    if "gddr5x" in s:
        return "gddr5x"
    if "gddr5" in s:
        return "gddr5"
    if "mram" in s or "stt-mram" in s:
        return "mram"

    # Fallback
    return "unknown"


# ------------------------------------------------------
# Split the CSV into multiple family-based files
# ------------------------------------------------------

def main():
    print(f"Reading: {INPUT_CSV}")
    if not INPUT_CSV.is_file():
        raise SystemExit(f"Input CSV not found: {INPUT_CSV}")

    # family → list of rows
    family_rows = defaultdict(list)

    with INPUT_CSV.open("r", newline="") as f:
        reader = csv.DictReader(f)
        header = reader.fieldnames

        if header is None:
            raise SystemExit("ERROR: valid_experiment_matrix.csv has no header row")

        for row in reader:
            memspec = row["memspec"]
            family = extract_family_from_memspec(memspec)
            family_rows[family].append(row)

    # Write one CSV per family
    for family, rows in family_rows.items():
        output_path = OUTPUT_DIR / f"tier_{family}_valid_configs_matrix.csv"

        with output_path.open("w", newline="") as f_out:
            writer = csv.DictWriter(f_out, fieldnames=header)
            writer.writeheader()
            writer.writerows(rows)

        print(f"Wrote {len(rows):4d} rows → {output_path}")

    print("\nDone!")


if __name__ == "__main__":
    main()
