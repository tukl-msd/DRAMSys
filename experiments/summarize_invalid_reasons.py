#!/usr/bin/env python3
import csv
import sys
from collections import Counter
from pathlib import Path

"""
    Run this from DRAMSys/experiments:
    cd /Users/m.akhtari/JLR/DRAMSys/experiments
    python3 summarize_invalid_reasons.py
"""

csv_path = Path("invalid_experiment_matrix_with_reason.csv")

counts = Counter()

with csv_path.open("r", newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        reason = (row.get("invalid_reasons") or "").strip()
        if not reason:
            reason = "<EMPTY_REASON_FIELD>"
        counts[reason] += 1

total = sum(counts.values())

try:
    for reason, n in counts.most_common():
        print(f"{n:5d}  {reason}")
    print("-" * 60)
    print(f"Total invalid configs counted: {total}")
except BrokenPipeError:
    sys.exit(0)
