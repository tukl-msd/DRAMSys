#!/usr/bin/env python3
import csv
import sys
from collections import Counter
from pathlib import Path

csv_path = Path("invalid_experiment_matrix_with_reason2.csv")

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
