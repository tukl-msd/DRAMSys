#!/usr/bin/env python3
"""
Filter DRAMSys experiment matrix to keep only *valid* combinations of:

- memspec (geometry + memoryType)
- mcconfig (RefreshPolicy, etc.)
- addressmapping (BANK/ROW/COLUMN/RANK/CHANNEL bits)

Input:
  /Users/m.akhtari/JLR/DRAMSys/experiments/unfiltered_experiment_matrix.csv

Outputs (in same experiments folder):
  valid_experiment_matrix.csv
  invalid_experiment_matrix_with_reason.csv
"""

import csv
import json
import math
from pathlib import Path
from typing import Dict, Any, List, Tuple, Optional

# ----------------------------------------------------------------------
# Paths (adapted to *your* directory layout)
# ----------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
EXPERIMENTS_DIR = SCRIPT_DIR.parent            # .../DRAMSys/experiments
DRAMSYS_ROOT = EXPERIMENTS_DIR.parent          # .../DRAMSys

INPUT_CSV = EXPERIMENTS_DIR / "unfiltered_experiment_matrix.csv"
OUTPUT_VALID_CSV = EXPERIMENTS_DIR / "valid_experiment_matrix.csv"
OUTPUT_INVALID_CSV = EXPERIMENTS_DIR / "invalid_experiment_matrix_with_reason.csv"

MEMSPEC_DIR = DRAMSYS_ROOT / "configs" / "memspec"
MCCONFIG_DIR = DRAMSYS_ROOT / "configs" / "mcconfig"
ADDRMAP_DIR = DRAMSYS_ROOT / "configs" / "addressmapping"


# ----------------------------------------------------------------------
# Generic helpers
# ----------------------------------------------------------------------

def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r") as f:
        return json.load(f)


def log2_exact(n: int) -> Optional[int]:
    """Return integer log2(n) if n is a positive power-of-two, else None."""
    if n is None or n <= 0:
        return None
    if (n & (n - 1)) != 0:
        return None
    return int(math.log2(n))


def normalize_mem_type(raw: Optional[str]) -> Optional[str]:
    """Normalize memspec.memoryType to a simple lowercase token."""
    if not raw:
        return None
    s = raw.lower()
    # order matters (lpddr before ddr)
    if "lpddr5" in s:
        return "lpddr5"
    if "lpddr4" in s:
        return "lpddr4"
    if "lpddr3" in s:
        return "lpddr3"
    if "ddr5" in s:
        return "ddr5"
    if "ddr4" in s:
        return "ddr4"
    if "ddr3" in s:
        return "ddr3"
    if "hbm2" in s:
        return "hbm2"
    if "wideio2" in s or "wide i/o 2" in s:
        return "wideio2"
    if "wideio" in s or "wide i/o" in s:
        return "wideio"
    if "gddr5x" in s:
        return "gddr5x"
    if "gddr6" in s:
        return "gddr6"
    if "gddr5" in s:
        return "gddr5"
    if "stt-mram" in s or "mram" in s:
        return "stt-mram"
    return s.strip()


def infer_family_from_addrmap_filename(fname: str) -> Optional[str]:
    """Guess memory family from addressmapping file name."""
    s = fname.lower()
    # order matters (lpddr before ddr, wideio2 before wideio)
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
    if "gddr5x" in s:
        return "gddr5x"
    if "gddr6" in s:
        return "gddr6"
    if "gddr5" in s:
        return "gddr5"
    if "stt-mram" in s or "mram" in s:
        return "stt-mram"
    return None


# ----------------------------------------------------------------------
# Parsers for your DRAMSys JSON schemas
# ----------------------------------------------------------------------

def parse_memspec_geometry(memspec_json: Dict[str, Any]) -> Dict[str, Any]:
    """
    Extract geometry + type from memspec JSON, following your example.
    """
    root = memspec_json.get("memspec", {})
    arch = root.get("memarchitecturespec", {})

    geom = {
        "banks": arch.get("nbrOfBanks"),
        "rows": arch.get("nbrOfRows"),
        "columns": arch.get("nbrOfColumns"),
        "ranks": arch.get("nbrOfRanks"),
        "channels": arch.get("nbrOfChannels"),
        "bank_groups": arch.get("nbrOfBankGroups"),
        "width": arch.get("width"),
        "burst_length": arch.get("burstLength"),
        "devices": arch.get("nbrOfDevices"),
        "memory_type_raw": root.get("memoryType"),
        "memory_type": normalize_mem_type(root.get("memoryType")),
        "memory_id": root.get("memoryId"),
    }
    return geom


def parse_address_mapping(addrmap_json: Dict[str, Any]) -> Dict[str, Any]:
    """
    Parse addressmapping and return bit lists + counts.
    """
    am = addrmap_json.get("addressmapping", {})

    def bit_count(key: str) -> Optional[int]:
        if key not in am:
            return None
        v = am.get(key)
        if not isinstance(v, list):
            return None
        return len(v)

    mapping = {
        "BANK_BIT": am.get("BANK_BIT", []),
        "ROW_BIT": am.get("ROW_BIT", []),
        "COLUMN_BIT": am.get("COLUMN_BIT", []),
        "BYTE_BIT": am.get("BYTE_BIT", []),
        "RANK_BIT": am.get("RANK_BIT", []),
        "CHANNEL_BIT": am.get("CHANNEL_BIT", []),
        "BANKGROUP_BIT": am.get("BANKGROUP_BIT", []),

        "num_bank_bits": bit_count("BANK_BIT") or 0,
        "num_row_bits": bit_count("ROW_BIT") or 0,
        "num_col_bits": bit_count("COLUMN_BIT") or 0,
        "num_byte_bits": bit_count("BYTE_BIT") or 0,
        "num_rank_bits": bit_count("RANK_BIT") or 0,
        "num_channel_bits": bit_count("CHANNEL_BIT") or 0,
        "num_bg_bits": bit_count("BANKGROUP_BIT") or 0,
    }
    return mapping


def parse_mcconfig(mcconfig_json: Dict[str, Any]) -> Dict[str, Any]:
    """
    Parse mcconfig (we only care about a few fields for validity).
    """
    root = mcconfig_json.get("mcconfig", {})
    return {
        "PagePolicy": root.get("PagePolicy"),
        "Scheduler": root.get("Scheduler"),
        "RefreshPolicy": root.get("RefreshPolicy"),
        "raw": root,
    }


# ----------------------------------------------------------------------
# Validation rules
# ----------------------------------------------------------------------

def validate_memspec_geometry(geom: Dict[str, Any]) -> List[str]:
    """
    Basic sanity checks on memspec geometry fields.

    - Ensures banks/rows/cols/ranks/channels are positive ints.
    - Treats nbrOfBankGroups as *optional*:
      * If None/missing → no bank-group check.
      * If present     → must be >0 and power-of-two.
    """
    reasons: List[str] = []

    banks = geom.get("banks")
    rows = geom.get("rows")
    cols = geom.get("columns")
    ranks = geom.get("ranks")
    channels = geom.get("channels")
    bank_groups = geom.get("bank_groups")

    # Basic >0 checks (where present)
    if banks is None or not isinstance(banks, int) or banks <= 0:
        reasons.append(f"Invalid nbrOfBanks={banks!r}")
    if rows is None or not isinstance(rows, int) or rows <= 0:
        reasons.append(f"Invalid nbrOfRows={rows!r}")
    if cols is None or not isinstance(cols, int) or cols <= 0:
        reasons.append(f"Invalid nbrOfColumns={cols!r}")
    if ranks is not None and (not isinstance(ranks, int) or ranks <= 0):
        reasons.append(f"Invalid nbrOfRanks={ranks!r}")
    if channels is not None and (not isinstance(channels, int) or channels <= 0):
        reasons.append(f"Invalid nbrOfChannels={channels!r}")

    # Bank groups are optional — only check if present
    if bank_groups is not None:
        if not isinstance(bank_groups, int) or bank_groups <= 0:
            reasons.append(f"Invalid nbrOfBankGroups={bank_groups!r}")
        else:
            bg_log = log2_exact(bank_groups)
            if bg_log is None:
                reasons.append(
                    f"nbrOfBankGroups={bank_groups} is not a positive power of two"
                )

    return reasons


def validate_geometry_vs_mapping(geom: Dict[str, Any],
                                 mapping: Dict[str, Any]) -> List[str]:
    """
    Enforce that the number of BANK / ROW / COLUMN / RANK / CHANNEL bits
    matches the memspec geometry (power-of-two assumptions).

    For bank-grouped memories (DDR4/DDR5/HBM2/GDDR5+/WideIO2),
    the effective bank index is composed of BANK_BIT + BANKGROUP_BIT.
    """
    reasons: List[str] = []

    # --- Pull out geometry ---
    banks        = geom.get("banks")
    rows         = geom.get("rows")
    cols         = geom.get("columns")
    ranks        = geom.get("ranks")
    channels     = geom.get("channels")
    bank_groups  = geom.get("bank_groups")
    mem_type     = geom.get("memory_type")   # normalized (lpddr4, ddr4, hbm2, ...)

    # --- Pull out mapping bit counts ---
    bank_bits    = mapping.get("num_bank_bits", 0)
    row_bits     = mapping.get("num_row_bits", 0)
    col_bits     = mapping.get("num_col_bits", 0)
    byte_bits    = mapping.get("num_byte_bits", 0)
    rank_bits    = mapping.get("num_rank_bits", 0)
    ch_bits      = mapping.get("num_channel_bits", 0)
    bg_bits      = mapping.get("num_bg_bits", 0)

    # ------------------------------------------------------------------
    # BANKS + BANK GROUPS
    # ------------------------------------------------------------------
    # Families where bank-grouping is a real architectural concept.
    bank_group_families = {"ddr4", "ddr5", "hbm2", "gddr5", "gddr5x", "gddr6", "wideio2"}

    if banks is not None:
        bank_log = log2_exact(int(banks))
        if bank_log is None:
            reasons.append(f"nbrOfBanks={banks} is not a power of two")
        else:
            # Case 1: bank-grouped memories (e.g., DDR4/DDR5/HBM2/WideIO2)
            if mem_type in bank_group_families and (bank_groups or 1) > 1:
                # sanity: bank_groups must match BANKGROUP_BIT
                if bank_groups is not None:
                    bg_log = log2_exact(int(bank_groups))
                    if bg_log is None or bg_log != bg_bits:
                        reasons.append(
                            f"BankGroup bits mismatch: memspec has {bank_groups} bank groups "
                            f"(log2={bg_log}) but addressmapping has {bg_bits} BANKGROUP_BIT entries"
                        )

                effective_bank_bits = bank_bits + bg_bits
                if effective_bank_bits != bank_log:
                    reasons.append(
                        f"Bank bits mismatch (with bank-groups): memspec has {banks} banks "
                        f"(log2={bank_log}) but addressmapping has "
                        f"{bank_bits} BANK_BIT + {bg_bits} BANKGROUP_BIT = {effective_bank_bits} bits"
                    )

            else:
                # Case 2: non-bank-grouped memories (DDR3, LPDDR4, etc.)
                if bank_bits != bank_log:
                    reasons.append(
                        f"Bank bits mismatch: memspec has {banks} banks (log2={bank_log}) "
                        f"but addressmapping has {bank_bits} BANK_BIT entries"
                    )

    # Extra sanity: addrmap uses BANKGROUP_BIT but memspec says 1 or 0 groups
    if (bank_groups is None or bank_groups <= 1) and bg_bits > 0:
        reasons.append(
            f"addrmap defines {bg_bits} BANKGROUP_BIT entries but memspec has "
            f"{bank_groups if bank_groups is not None else 1} bank groups"
        )

    # ------------------------------------------------------------------
    # ROWS
    # ------------------------------------------------------------------
    if rows is not None and row_bits > 0:
        row_log = log2_exact(int(rows))
        if row_log is None:
            reasons.append(f"nbrOfRows={rows} is not a power of two")
        elif row_log != row_bits:
            reasons.append(
                f"Row bits mismatch: memspec has {rows} rows (log2={row_log}) "
                f"but addressmapping has {row_bits} ROW_BIT entries"
            )

    # ------------------------------------------------------------------
    # COLUMNS
    # ------------------------------------------------------------------
    if cols is not None and col_bits > 0:
        col_log = log2_exact(int(cols))
        if col_log is None:
            reasons.append(f"nbrOfColumns={cols} is not a power of two")
        elif col_log != col_bits:
            reasons.append(
                f"Column bits mismatch: memspec has {cols} columns (log2={col_log}) "
                f"but addressmapping has {col_bits} COLUMN_BIT entries"
            )

    # ------------------------------------------------------------------
    # RANKS (optional)
    # ------------------------------------------------------------------
    if ranks is not None and rank_bits > 0:
        rank_log = log2_exact(int(ranks))
        if rank_log is None:
            reasons.append(f"nbrOfRanks={ranks} is not a power of two")
        elif rank_log != rank_bits:
            reasons.append(
                f"Rank bits mismatch: memspec has {ranks} ranks (log2={rank_log}) "
                f"but addressmapping has {rank_bits} RANK_BIT entries"
            )

    # ------------------------------------------------------------------
    # CHANNELS (optional)
    # ------------------------------------------------------------------
    if channels is not None:
        ch_log = log2_exact(int(channels))
        if ch_log is None:
            reasons.append(f"nbrOfChannels={channels} is not a power of two")
        else:
            if ch_bits != ch_log:
                reasons.append(
                    f"Channel bits mismatch: memspec has {channels} channels (log2={ch_log}) "
                    f"but addressmapping has {ch_bits} CHANNEL_BIT entries"
                )

    # ------------------------------------------------------------------
    # Sanity limits
    # ------------------------------------------------------------------
    if row_bits > 32:
        reasons.append(f"ROW_BIT count {row_bits} seems unrealistically large (>32)")
    if col_bits > 16:
        reasons.append(f"COLUMN_BIT count {col_bits} seems unrealistically large (>16)")

    return reasons


def validate_family_compatibility(geom: Dict[str, Any],
                                  addrmap_filename: str) -> List[str]:
    """
    Enforce that memspec.memoryType family matches the addressmapping family
    (e.g., LPDDR4 memspec should not use an am_ddr3_... mapping).
    """
    reasons: List[str] = []

    mem_type = geom["memory_type"]
    mem_id = geom["memory_id"]
    map_family = infer_family_from_addrmap_filename(addrmap_filename)

    if mem_type is None or map_family is None:
        # Not enough info to complain
        return reasons

    if mem_type != map_family:
        reasons.append(
            f"Memory family mismatch: memspec memoryType '{geom['memory_type_raw']}' "
            f"(normalized '{mem_type}', id='{mem_id}') vs "
            f"addressmapping file '{addrmap_filename}' (family '{map_family}')"
        )
    return reasons


def validate_refresh_policy(geom: Dict[str, Any],
                            mc: Dict[str, Any]) -> List[str]:
    """
    Enforce rules from README:

    - RefreshPolicy 'PerBank' only allowed with LPDDR4, Wide I/O 2, GDDR5/5X/6, or HBM2.
    - RefreshPolicy 'SameBank' only allowed with DDR5.
    """
    reasons: List[str] = []

    mem_type = geom["memory_type"]
    rp = mc.get("RefreshPolicy")

    if not rp:
        return reasons  # no info

    if mem_type is None:
        reasons.append(
            f"RefreshPolicy='{rp}' used but memspec.memoryType is missing; cannot verify legality"
        )
        return reasons

    allowed_perbank = {"lpddr4", "wideio2", "gddr5", "gddr5x", "gddr6", "hbm2"}
    if rp == "PerBank":
        if mem_type not in allowed_perbank:
            reasons.append(
                f"RefreshPolicy='PerBank' is only allowed with LPDDR4, WideIO2, GDDR5/5X/6 or HBM2, "
                f"but memspec.memoryType='{geom['memory_type_raw']}' (normalized '{mem_type}')"
            )

    if rp == "SameBank":
        if mem_type != "ddr5":
            reasons.append(
                f"RefreshPolicy='SameBank' is only allowed with DDR5, "
                f"but memspec.memoryType='{geom['memory_type_raw']}' (normalized '{mem_type}')"
            )

    return reasons


def validate_configuration(memspec_path: Path,
                           mcconfig_path: Path,
                           addrmap_path: Path) -> Tuple[bool, List[str]]:
    """
    Validate one triple (memspec, mcconfig, addressmapping).
    Return (is_valid, invalid_reasons_list).
    """
    reasons: List[str] = []

    # --- 1) File existence ---
    if not memspec_path.is_file():
        reasons.append(f"memspec file not found: {memspec_path}")
        return False, reasons

    if not mcconfig_path.is_file():
        reasons.append(f"mcconfig file not found: {mcconfig_path}")
        return False, reasons

    if not addrmap_path.is_file():
        reasons.append(f"addressmapping file not found: {addrmap_path}")
        return False, reasons

    # --- 2) JSON parsing ---
    try:
        memspec_json = load_json(memspec_path)
    except Exception as e:
        reasons.append(f"Failed to parse memspec JSON '{memspec_path.name}': {e}")
        return False, reasons

    try:
        mcconfig_json = load_json(mcconfig_path)
    except Exception as e:
        reasons.append(f"Failed to parse mcconfig JSON '{mcconfig_path.name}': {e}")
        return False, reasons

    try:
        addrmap_json = load_json(addrmap_path)
    except Exception as e:
        reasons.append(f"Failed to parse addressmapping JSON '{addrmap_path.name}': {e}")
        return False, reasons

    # --- 3) Structured info ---
    geom = parse_memspec_geometry(memspec_json)
    mc = parse_mcconfig(mcconfig_json)
    addrmap = parse_address_mapping(addrmap_json)

    # --- 4) Apply validation rules ---
    reasons += validate_memspec_geometry(geom)
    reasons += validate_geometry_vs_mapping(geom, addrmap)
    reasons += validate_family_compatibility(geom, addrmap_path.name)
    reasons += validate_refresh_policy(geom, mc)

    is_valid = len(reasons) == 0
    return is_valid, reasons


# ----------------------------------------------------------------------
# Main driver
# ----------------------------------------------------------------------

def main():
    if not INPUT_CSV.is_file():
        raise SystemExit(f"Input CSV not found: {INPUT_CSV}")

    with INPUT_CSV.open("r", newline="") as f_in, \
         OUTPUT_VALID_CSV.open("w", newline="") as f_valid, \
         OUTPUT_INVALID_CSV.open("w", newline="") as f_invalid:

        reader = csv.DictReader(f_in)
        fieldnames = reader.fieldnames
        if fieldnames is None:
            raise SystemExit("Input CSV has no header row")

        # Your CSV headers are: id,memspec,mcconfig,addressmapping
        if not {"id", "memspec", "mcconfig", "addressmapping"}.issubset(set(fieldnames)):
            raise SystemExit(
                f"Expected CSV headers: id, memspec, mcconfig, addressmapping. Got: {fieldnames}"
            )

        # Writers
        valid_writer = csv.DictWriter(f_valid, fieldnames=fieldnames)
        valid_writer.writeheader()

        invalid_fieldnames = fieldnames + ["invalid_reasons"]
        invalid_writer = csv.DictWriter(f_invalid, fieldnames=invalid_fieldnames)
        invalid_writer.writeheader()

        total = 0
        valid_count = 0
        invalid_count = 0

        for row in reader:
            total += 1

            memspec_file = row["memspec"]
            mcconfig_file = row["mcconfig"]
            addrmap_file = row["addressmapping"]

            memspec_path = MEMSPEC_DIR / memspec_file
            mcconfig_path = MCCONFIG_DIR / mcconfig_file
            addrmap_path = ADDRMAP_DIR / addrmap_file

            is_valid, reasons = validate_configuration(
                memspec_path, mcconfig_path, addrmap_path
            )

            if is_valid:
                valid_writer.writerow(row)
                valid_count += 1
            else:
                row_out = dict(row)
                row_out["invalid_reasons"] = " | ".join(reasons)
                invalid_writer.writerow(row_out)
                invalid_count += 1

        print(f"Total configs:  {total}")
        print(f"Valid configs:  {valid_count}")
        print(f"Invalid configs:{invalid_count}")
        print(f"Valid CSV written to:   {OUTPUT_VALID_CSV}")
        print(f"Invalid CSV written to: {OUTPUT_INVALID_CSV}")


if __name__ == "__main__":
    main()
