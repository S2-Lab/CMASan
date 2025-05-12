import os
import sys
import re
from extract_pc_module import run_pc_extractor

if len(sys.argv) < 3:
    print("Usage: python3 extract_pc_all.py [Project directory] [RT_CMA_LIST]")
    exit(0)

SCRIPT_PATH = sys.argv[0]
TARGET_DIR = sys.argv[1]
CMA_LIST_PATH = sys.argv[2]
SHARED_LIB_EXTENSION_PATTERN = re.compile(r"\.so(\.\d+)*$")

def find_binaries_and_shared_libs(directory):
    target_files = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            filepath = os.path.join(root, file)
            if SHARED_LIB_EXTENSION_PATTERN.search(file):
                target_files.append((filepath, True))
            elif os.access(filepath, os.X_OK):  # Check if file is executable
                target_files.append((filepath, False))
    return target_files

target_files = find_binaries_and_shared_libs(TARGET_DIR)
for target_path, is_shared_lib in target_files:
    cache_path = target_path + ".cmasan.pc"
    if run_pc_extractor(SCRIPT_PATH, target_path, CMA_LIST_PATH, cache_path, is_shared_lib):
        print("Generated {}".format(cache_path))

