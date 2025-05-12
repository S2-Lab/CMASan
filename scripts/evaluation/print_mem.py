#!/usr/bin/env python3

import sys
import re
import os
from tqdm import tqdm

def get_result_path(app_name):
    return os.path.join("benchmark", os.path.join("result", app_name, "mem"))

def accumulate_memory(app_name):
    filepath = get_result_path(app_name)
    all_files = os.listdir(filepath)
    log_files = [file for file in all_files if file.startswith("memory")]
    
    # Collect logs
    accumulated_rss = 0
    for j in tqdm(range(len(log_files))):
        file = log_files[j]
        filedir = filepath + "/" + file
        with open(filedir) as f:
            lines = f.readlines()
            if len(lines) > 2:
                print("Warning: {} has more than two line".format(filedir))
                print(lines)
                continue
            elif len(lines) == 0:
                print("Warning: {} has no content".format(filedir))
                continue
            if "process-wrapper" in lines[1]: # a test util of gRPC, not ASan or CMASan instrumented.
                continue
            accumulated_rss += int(lines[0])/(2**20) # Convert to MB
            
    return accumulated_rss

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {os.path.basename(sys.argv[0])} <application1> [application2 ...]")
        sys.exit(1)

    app_names = sys.argv[1:]

    result = []
    for app_name in app_names:
        try:
            cmasan_mem = accumulate_memory(app_name)
            asan_mem = accumulate_memory(app_name + "_asan")
            result.append((app_name, (cmasan_mem/asan_mem*100)-100))
        except FileNotFoundError:
            print(f"[ERROR] File not found for '{app_name}'. Check log path.")
        except Exception as e:
            print(f"[ERROR] Failed to parse '{app_name}': {str(e)}")


    print(f"================ Table ================")
    print(f"|  Application  | Memory Overhead (%) |")
    for app_name, overhead in result:
        print("| {:>13} | {:>24.2f} |".format(app_name, overhead))

if __name__ == "__main__":
    main()
