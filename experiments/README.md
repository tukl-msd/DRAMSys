# **DRAM Configuration Generation & Validation Pipeline**

This document explains the full workflow used to generate, validate, categorize, and execute DRAM configuration experiments using DRAMSys.

It includes:

1. Generating the unfiltered experiment matrix
2. Filtering valid vs. invalid DRAM configurations
3. Summarizing reasons for invalid configs
4. Splitting valid configs by DRAM family
5. Running DRAMSys simulations for each configuration

All commands assume:

```bash
cd ~/JLR/dram-explore/DRAMSys/experiments
```

and that your Python virtual environment is active.

---

# **Pipeline Overview**

| Step  | Script                            | Purpose                                               | Output                                                                        |
| ----- | --------------------------------- | ----------------------------------------------------- | ----------------------------------------------------------------------------- |
| **1** | `generate_unfiltered_matrix.py`   | Produces all possible configuration combinations      | `unfiltered_experiment_matrix.csv`                                            |
| **2** | `filter_valid_configs.py`         | Splits valid vs. invalid configs                      | `valid_experiment_matrix2.csv` + `invalid_experiment_matrix_with_reason2.csv` |
| **3** | `summarize_invalid_reasons.py`    | Summarizes invalid reasons                            | `invalid_reasons_summary.txt`                                                 |
| **4** | `split_valid_matrix_by_family.py` | Categorizes valid configs into DRAM families          | `tier_*_valid_configs_matrix.csv`                                             |
| **5** | `run_experiment_matrix.py`        | Runs DRAMSys simulations using selected tier matrices | JSON configs + DRAMSys results                                                |

---

# ------------------------------------------

# **STEP 1 – Generate the Unfiltered Matrix**

# ------------------------------------------

### **Script**

```
/Users/m.akhtari/JLR/dram-explore/DRAMSys/experiments/scripts/generate_unfiltered_matrix.py
```

### **Purpose**

Generates **every possible combination** of:

* memspec
* mcconfig
* addressmapping

No validation is done at this stage.

### **Run**

```bash
python3 scripts/generate_unfiltered_matrix.py
```

### **Output**

```
/Users/m.akhtari/JLR/dram-explore/DRAMSys/experiments/unfiltered_experiment_matrix.csv
```

This CSV contains *all* potential DRAM configurations, valid or not.

---

# ---------------------------------------------

# **STEP 2 – Filter Valid vs. Invalid Configs**

# ---------------------------------------------

### **Script**

```
/Users/m.akhtari/JLR/dram-explore/DRAMSys/experiments/scripts/filter_valid_configs.py
```

### **Purpose**

Validates each configuration against:

* Row/bank/column bit rules
* Power-of-two constraints
* Refresh policy rules
* DRAM family compatibility
* Address mapping limits

### **Run**

```bash
python3 scripts/filter_valid_configs.py
```

### **Outputs**

1. **Valid configurations**

```
valid_experiment_matrix2.csv
```

2. **Invalid configurations + reason(s)**

```
invalid_experiment_matrix_with_reason2.csv
```

The second file contains rows like:

```
id,memspec,mcconfig,addressmapping,invalid_reasons
12,HBM2.json,fifo.json,am_ddr3..., "BankGroup bits mismatch | Column bits mismatch | Family mismatch"
```

---

# -----------------------------------------------------

# **STEP 3 – Summarize Invalid Reasons (Optional but Recommended)**

# -----------------------------------------------------

### **Script**

```
/Users/m.akhtari/JLR/dram-explore/DRAMSys/experiments/summarize_invalid_reasons.py
```

### **Purpose**

Counts how many times **each exact invalid reason** appears.

This helps understand:

* The most common configuration failures
* Whether filters behave as expected
* Whether invalid counts match total rows

### **Run**

```bash
python3 summarize_invalid_reasons.py
```

### **Output (saved manually)**

```bash
python3 summarize_invalid_reasons.py > invalid_reasons_summary.txt
```

Saved to:

```
invalid_reasons_summary.txt
```

This file lists:

```
432   Row bits mismatch: memspec rows=16384 but addrmap ROW_BIT entries=15
216   RefreshPolicy='SameBank' only allowed for DDR5
...
------------------------------------------------------------
Total invalid configs counted: N
```

---

# ---------------------------------------------------

# **STEP 4 – Split Valid Configs by DRAM Family**

# ---------------------------------------------------

### **Script**

```
/Users/m.akhtari/JLR/dram-explore/DRAMSys/experiments/scripts/split_valid_matrix_by_family.py
```

### **Purpose**

Groups valid configurations into *tiers* based on DRAM memory type:

* DDR3
* DDR4
* HBM2
* LPDDR4
* MRAM
* WIDEIO
* WIDEIO2

### **Run**

```bash
python3 scripts/split_valid_matrix_by_family.py
```

### **Outputs (one CSV per DRAM family)**

Stored in:

```
experiments/tier_matrices_csvs/
```

Files generated:

```
tier_ddr3_valid_configs_matrix.csv
tier_ddr4_valid_configs_matrix.csv
tier_hbm2_valid_configs_matrix.csv
tier_lpddr4_valid_configs_matrix.csv
tier_mram_valid_configs_matrix.csv
tier_wideio_valid_configs_matrix.csv
tier_wideio2_valid_configs_matrix.csv
```

Each file contains *only* valid configurations for that DRAM family.

---

# -----------------------------------------------------

# **STEP 5 – Run DRAMSys for Selected Tier Matrices**

# -----------------------------------------------------

### **Script**

```
/Users/m.akhtari/JLR/dram-explore/DRAMSys/experiments/scripts/run_experiment_matrix.py
```

### **Purpose**

Runs DRAMSys simulations for each row in a tier matrix CSV.

This script:

1. Builds a **top-level DRAMSys JSON config**
2. Writes it under:

   ```
   DRAMSys/configs/experiments/
   ```
3. Calls the DRAMSys binary with the JSON config
4. Logs results into a manifest

---

## **What `run_experiment_matrix.py` Does Internally**

### **1. Reads the job description**

Each job defines:

* `tier_name` (e.g., `tier_ddr3`)
* `matrix_csv` (e.g., `tier_ddr3_valid_configs_matrix.csv`)
* `trace_name` (e.g., `resnet_cpu.stl`)
* `trace_clk_mhz`
* `simconfig`
* `start_id`, `end_id`

---

### **2. Resolves paths**

| Resource   | Resolved From                                         |
| ---------- | ----------------------------------------------------- |
| Matrix CSV | `DRAMSys/experiments/tier_matrices_csvs/<matrix_csv>` |
| Trace file | `DRAMSys/experiments/traces/<trace_name>`             |
| Simconfig  | `DRAMSys/configs/simconfig/<simconfig>`               |

---

### **3. For each selected config row (start_id → end_id−1)**

The script:

1. Loads: `memspec`, `mcconfig`, `addressmapping`
2. Builds a top-level DRAMSys config:

   ```
   DRAMSys/configs/experiments/<tier_name>_id<config_id>_<trace_stem>.json
   ```
3. Calls DRAMSys:

   ```bash
   DRAMSys <JSON_FILE>
   ```

   with working directory set to the **DRAMSys root**.

---

# ✔ Summary of All Outputs

### **Matrix generation**

* `unfiltered_experiment_matrix.csv`

### **Validation**

* `valid_experiment_matrix2.csv`
* `invalid_experiment_matrix_with_reason2.csv`

### **Invalid summaries**

* `invalid_reasons_summary.txt`

### **Family separation**

* `tier_*_valid_configs_matrix.csv`

### **Simulation**

* JSON configs in `configs/experiments/`
* DRAMSys results in tier-specific output folders
* Manifest CSV inside `experiments/results/<tier_name>/`




to run summarize_invalid_reasons.py: Run this in DRAMSys/experiments:
python3 summarize_invalid_reasons.py | tee invalid_reasons_summary.txt | head