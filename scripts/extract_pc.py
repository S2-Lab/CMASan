import sys
from extract_pc_module import run_pc_extractor

if len(sys.argv) < 3:
    print("Usage: python3 extract_pc.py [target binary] [RT_CMA_LIST]")
    exit(0)

SCRIPT_PATH = sys.argv[0]
TARGET_BINARY = sys.argv[1]
CMA_LIST_PATH = sys.argv[2]
WRITE_PATH = (TARGET_BINARY + ".cmasan.pc") if len(sys.argv) == 3 else None

run_pc_extractor(SCRIPT_PATH, TARGET_BINARY, CMA_LIST_PATH, WRITE_PATH)
