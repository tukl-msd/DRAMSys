#!/usr/bin/env python3
import argparse
import csv
import json
import subprocess
from pathlib import Path
from datetime import datetime


def resolve_paths():
    """Figure out where we are in the DRAMSys tree."""
    scripts_dir = Path(__file__).resolve().parent
    experiments_dir = scripts_dir.parent
    dramsys_root = experiments_dir.parent

    return {
        "scripts_dir": scripts_dir,
        "experiments_dir": experiments_dir,
        "dramsys_root": dramsys_root,
        "configs_dir": dramsys_root / "configs",
        "simconfig_dir": dramsys_root / "configs" / "simconfig",
        "traces_dir": experiments_dir / "traces",
        # NOTE: same path you used before
        "dramsys_bin": dramsys_root / "build" / "bin" / "DRAMSys",
        # New: where the tier matrices live
        "tier_matrices_dir": experiments_dir / "tier_matrices_csvs",
        "results_root": experiments_dir / "results",
    }


def build_top_config(
    sim_id: str,
    memspec_name: str,
    mcconfig_name: str,
    addrmap_name: str,
    simconfig_name: str,
    trace_path_in_config: str,
    trace_clk_mhz: int,
):
    """
    Build the top-level DRAMSys JSON config as a Python dict.

    Generated configs live in `configs/experiments/`, so to reach the standard
    config dirs we use relative paths like:

        "../memspec/...",
        "../mcconfig/...",
        "../addressmapping/...",
        "../simconfig/..."

    For the trace, we pass an *absolute* path in `trace_path_in_config`
    so DRAMSys can open it regardless of how it resolves the path.
    """

    memspec_ref   = f"../memspec/{memspec_name}"
    mcconfig_ref  = f"../mcconfig/{mcconfig_name}"
    addrmap_ref   = f"../addressmapping/{addrmap_name}"
    simconfig_ref = f"../simconfig/{simconfig_name}"

    cfg = {
        "simulation": {
            "simulationid":  sim_id,
            "simconfig":     simconfig_ref,
            "memspec":       memspec_ref,
            "addressmapping": addrmap_ref,
            "mcconfig":      mcconfig_ref,
            "tracesetup": [
                {
                    "type": "player",
                    "clkMhz": trace_clk_mhz,
                    "name": trace_path_in_config,  # absolute path
                }
            ],
        }
    }

    return cfg


def parse_int_or_default(value, default=None):
    if value is None:
        return default
    value = str(value).strip()
    if value == "":
        return default
    return int(value)


def run_single_job(job_row: dict, paths: dict):
    """
    Run one job from the jobs CSV driven by CSV.
    """
    experiments_dir = paths["experiments_dir"]
    dramsys_root = paths["dramsys_root"]
    configs_dir = paths["configs_dir"]
    simconfig_dir = paths["simconfig_dir"]
    traces_dir = paths["traces_dir"]
    dramsys_bin = paths["dramsys_bin"]
    tier_matrices_dir = paths["tier_matrices_dir"]
    results_root = paths["results_root"]

    job_id = job_row.get("job_id", "").strip()
    tier_name = job_row["tier_name"].strip()
    matrix_csv_name = job_row["matrix_csv"].strip()
    trace_name = job_row["trace_name"].strip()
    simconfig_name = (job_row.get("simconfig") or "example.json").strip()
    trace_clk_mhz = parse_int_or_default(job_row.get("trace_clk_mhz"), default=1000)
    start_id = parse_int_or_default(job_row.get("start_id"), default=0)
    end_id = parse_int_or_default(job_row.get("end_id"), default=None)

    print(f"\n=== JOB {job_id or '(no id)'} | tier={tier_name} | matrix={matrix_csv_name} | trace={trace_name} ===")

    # Resolve matrix path:
    matrix_path = Path(matrix_csv_name)
    if not matrix_path.is_absolute():
        matrix_path = tier_matrices_dir / matrix_path

    if not matrix_path.exists():
        print(f"[JOB {job_id}] ERROR: Matrix CSV not found: {matrix_path}")
        return

    # simconfig file
    simconfig_path = simconfig_dir / simconfig_name
    if not simconfig_path.exists():
        print(f"[JOB {job_id}] ERROR: simconfig file not found: {simconfig_path}")
        return

    # trace file
    trace_path = traces_dir / trace_name
    if not trace_path.exists():
        print(f"[JOB {job_id}] ERROR: trace file not found: {trace_path}")
        return

    trace_path_in_config = str(trace_path.resolve())

    # sanity check DRAMSys bin
    if not dramsys_bin.exists():
        print(f"[JOB {job_id}] ERROR: DRAMSys binary not found: {dramsys_bin}")
        return

    # Where to store generated configs and results for this tier
    tier_results_dir = results_root / tier_name
    tier_results_dir.mkdir(parents=True, exist_ok=True)

    tier_configs_dir = configs_dir / "experiments"
    tier_configs_dir.mkdir(parents=True, exist_ok=True)

    manifest_path = tier_results_dir / f"{tier_name}_runs_manifest.csv"

    # Read matrix
    with matrix_path.open("r", newline="") as fin:
        reader = csv.DictReader(fin)
        fieldnames = reader.fieldnames
        if not fieldnames:
            print(f"[JOB {job_id}] ERROR: Matrix CSV has no header: {matrix_path}")
            return

        required_cols = {"id", "memspec", "mcconfig", "addressmapping"}
        missing = required_cols - set(fieldnames)
        if missing:
            print(
                f"[JOB {job_id}] ERROR: Matrix CSV missing required columns: {missing}. "
                f"Found: {fieldnames}"
            )
            return

        rows = list(reader)

    # Determine range of *row indices* to run
    start = start_id
    end = end_id if end_id is not None else len(rows)

    if start < 0 or start >= len(rows):
        print(f"[JOB {job_id}] ERROR: start-id {start} out of range [0, {len(rows)-1}]")
        return
    if end <= start or end > len(rows):
        print(
            f"[JOB {job_id}] ERROR: end-id {end} out of range "
            f"(must be > start-id and <= {len(rows)})"
        )
        return

    # Prepare manifest for logging (append; create header only once)
    manifest_exists = manifest_path.exists()
    with manifest_path.open("a", newline="") as mf:
        writer = csv.DictWriter(
            mf,
            fieldnames=[
                "timestamp",
                "tier",
                "matrix_csv",
                "matrix_row_index",
                "config_id",
                "memspec",
                "mcconfig",
                "addressmapping",
                "simconfig",
                "trace_name",
                "trace_clk_mhz",
                "top_config_path",
                "dramsys_exit_code",
            ],
        )
        if not manifest_exists:
            writer.writeheader()

        for idx in range(start, end):
            row = rows[idx]
            config_id = row["id"]
            memspec_name = row["memspec"].strip()
            mcconfig_name = row["mcconfig"].strip()
            addrmap_name = row["addressmapping"].strip()

            # Make sim_id unique per tier + config + trace, so you don't overwrite JSON
            trace_stem = Path(trace_name).stem
            sim_id = f"{tier_name}_id{config_id}_{trace_stem}"

            # Build the JSON config
            top_cfg = build_top_config(
                sim_id=sim_id,
                memspec_name=memspec_name,
                mcconfig_name=mcconfig_name,
                addrmap_name=addrmap_name,
                simconfig_name=simconfig_name,
                trace_path_in_config=trace_path_in_config,
                trace_clk_mhz=trace_clk_mhz,
            )

            top_cfg_path = tier_configs_dir / f"{sim_id}.json"

            # Write the JSON config
            with top_cfg_path.open("w") as fcfg:
                json.dump(top_cfg, fcfg, indent=4)

            # For logging: recompute the same relative refs, just for display
            memspec_ref   = f"../memspec/{memspec_name}"
            mcconfig_ref  = f"../mcconfig/{mcconfig_name}"
            addrmap_ref   = f"../addressmapping/{addrmap_name}"
            simconfig_ref = f"../simconfig/{simconfig_name}"

            print(f"[JOB {job_id}] [{tier_name}] Row {idx} (id={config_id}):")
            print(f"  memspec        = {memspec_name} -> {memspec_ref}")
            print(f"  mcconfig       = {mcconfig_name} -> {mcconfig_ref}")
            print(f"  addressmapping = {addrmap_name} -> {addrmap_ref}")
            print(f"  simconfig      = {simconfig_name} -> {simconfig_ref}")
            print(f"  trace          = {trace_path_in_config} @ {trace_clk_mhz} MHz")
            print(f"  top config     = {top_cfg_path}")

            # Call DRAMSys with the generated config.
            result = subprocess.run(
                [str(dramsys_bin), str(top_cfg_path)],
                cwd=str(dramsys_root),
            )
            exit_code = result.returncode
            print(f"  DRAMSys exit code = {exit_code}")

            writer.writerow(
                {
                    "timestamp": datetime.now().isoformat(),
                    "tier": tier_name,
                    "matrix_csv": str(matrix_path),
                    "matrix_row_index": idx,
                    "config_id": config_id,
                    "memspec": memspec_name,
                    "mcconfig": mcconfig_name,
                    "addressmapping": addrmap_name,
                    "simconfig": simconfig_name,
                    "trace_name": trace_name,
                    "trace_clk_mhz": trace_clk_mhz,
                    "top_config_path": str(top_cfg_path),
                    "dramsys_exit_code": exit_code,
                }
            )


def main():
    paths = resolve_paths()
    experiments_dir = paths["experiments_dir"]

    parser = argparse.ArgumentParser(
        description="Run DRAMSys experiments based on a jobs CSV."
    )
    parser.add_argument(
        "--jobs-csv",
        default="experiment_jobs.csv",
        help=(
            "Path to the jobs CSV (relative to experiments/ or absolute). "
            "Default: experiment_jobs.csv"
        ),
    )

    args = parser.parse_args()

    jobs_csv_path = Path(args.jobs_csv)
    if not jobs_csv_path.is_absolute():
        jobs_csv_path = experiments_dir / jobs_csv_path

    if not jobs_csv_path.exists():
        raise SystemExit(f"Jobs CSV not found: {jobs_csv_path}")

    print(f"Loading jobs from {jobs_csv_path}")

    with jobs_csv_path.open("r", newline="") as jf:
        reader = csv.DictReader(jf)
        jobs = list(reader)

    if not jobs:
        raise SystemExit(f"No jobs found in {jobs_csv_path}")

    for job_row in jobs:
        run_single_job(job_row, paths)

    print("\nAll jobs finished.")


if __name__ == "__main__":
    main()