#!/usr/bin/env python3
"""
Generate unfiltered experiment matrix with all possible DRAM configurations
"""

import os
import csv
import itertools

# Paths to configuration directories
CONFIG_BASE = "configs"
MEMSPEC_DIR = os.path.join(CONFIG_BASE, "memspec")
MCCONFIG_DIR = os.path.join(CONFIG_BASE, "mcconfig")
ADDRESSMAPPING_DIR = os.path.join(CONFIG_BASE, "addressmapping")

# Output file
OUTPUT_FILE = "experiments/unfiltered_experiment_matrix.csv"

def get_config_files(directory):
    """Get all JSON config files from a directory"""
    files = []
    for file in os.listdir(directory):
        if file.endswith('.json'):
            files.append(file)
    return sorted(files)

def generate_unfiltered_matrix():
    """Generate all possible combinations of DRAM configurations"""
    
    # Get all available configurations
    memspecs = get_config_files(MEMSPEC_DIR)
    mcconfigs = get_config_files(MCCONFIG_DIR)
    addressmappings = get_config_files(ADDRESSMAPPING_DIR)
    
    print(f"Found {len(memspecs)} memspecs, {len(mcconfigs)} mcconfigs, {len(addressmappings)} address mappings")
    
    # Generate all combinations
    combinations = list(itertools.product(memspecs, mcconfigs, addressmappings))
    print(f"Generated {len(combinations)} total combinations")
    
    # Write to CSV
    with open(OUTPUT_FILE, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['id', 'memspec', 'mcconfig', 'addressmapping'])
        
        for i, (memspec, mcconfig, addressmapping) in enumerate(combinations):
            writer.writerow([i, memspec, mcconfig, addressmapping])
    
    print(f"Unfiltered experiment matrix saved to: {OUTPUT_FILE}")
    return len(combinations)

if __name__ == "__main__":
    count = generate_unfiltered_matrix()
    print(f"✅ Generated {count} configuration combinations")