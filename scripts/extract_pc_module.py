import subprocess
import os

# Predefined constants
LLVM_OBJDUMP_SPLITTER = "\n\n"
LLVM_OBJDUMP_START = 2
PARAM_DISAS_SYMBOLS = "--disassemble-symbols="
STRIP_STRINGS = " \n\t"
CMA_LIST_MODULE_PREFIX = "[module]"
DISASSEMBLY_SYMBOL = "Disassembly"
SHARED_LIB_RESOLVED_SYMBOL = "=>"
SHARED_LIB_SYMBOL = ".so"
REFERENCE_POINT_FNAME = "CMASanPIEReferencePoint"
LDD_CMD = "ldd"
CACHE_ENTRY_FORMAT = "{} {} {} {}\n"
# Lazy initialized constants
CMASAN_HOME_PATH = None
LLVM_OBJDUMP_CMD = None
CMA_LOG_WARNING_FILE = None

# Extract function address range (start, end) using llvm-objdump
def get_addr_info(target_file, cma_list, is_in_whitelist_map):
    if len(cma_list) == 0:
        return []

    objdump_args = [LLVM_OBJDUMP_CMD, PARAM_DISAS_SYMBOLS+(",".join(cma_list)), target_file]
    objdump_process = subprocess.run(objdump_args, capture_output=True)

    disas_results = [result.strip(STRIP_STRINGS) for result in objdump_process.stdout.decode().split(LLVM_OBJDUMP_SPLITTER)[LLVM_OBJDUMP_START:]]
    infos = []

    # if len(objdump_process.stderr) != 0:
    #     with open(CMA_LOG_WARNING_FILE, "a") as f:
    #         f.write("Warning:" + objdump_process.stderr.decode())

    try:
        for i in range(len(disas_results)):
            lines = disas_results[i].split("\n")
            if lines[0][:11] == DISASSEMBLY_SYMBOL:
                continue
            
            function_name = lines[0][lines[0].find("<")+1:lines[0].find(">")] 
            function_start = int(lines[1].split(":")[0].strip(STRIP_STRINGS), 16)
            function_end = int(lines[-1].split(":")[0].strip(STRIP_STRINGS), 16)
            if function_name == REFERENCE_POINT_FNAME:
                is_in_whitelist = 0
            else:
                is_in_whitelist = is_in_whitelist_map[function_name]
            infos.append((function_start, function_end, function_name, is_in_whitelist))

            # Reference point must be placed at the front
            if function_name == REFERENCE_POINT_FNAME and i > 0: 
                temp = infos[0]
                infos[0] = infos[i]
                infos[i] = temp
    except:
        print("Error at {}:".format(target_file))
        # print("\n".join(disas_results))
        pass

    return infos

# Get all the shared libraries required by the target binary
def get_shared_libs(target_file): 
    ldd_args = [LDD_CMD, target_file]
    ldd_process = subprocess.run(ldd_args, capture_output=True)
    
    result_lines = ldd_process.stdout.decode().split("\n")
    target_libs = []
    for line in result_lines:
        if SHARED_LIB_RESOLVED_SYMBOL in line:
            lib_path = line.split(" ")[2]
            target_libs.append(lib_path)
        elif SHARED_LIB_SYMBOL in line:
            expected_lib_path = os.getcwd()+"/"+line.strip().split(" ")[0]
            if os.path.exists(expected_lib_path):
                target_libs.append(expected_lib_path)
    return target_libs


def run_pc_extractor(script_path, target_file, cma_list_path, write_path=None, is_shared_lib=False):
    global CMASAN_HOME_PATH, LLVM_OBJDUMP_CMD, CMA_LOG_WARNING_FILE
    CMASAN_HOME_PATH = "/".join(script_path.split("/")[:-2]) # exclude scripts/extract_pc.py
    LLVM_OBJDUMP_CMD = CMASAN_HOME_PATH + "/LLVM/build/bin/llvm-objdump"
    CMA_LOG_WARNING_FILE = cma_list_path + ".warning"

    # Binary or shared library itself
    cma_list = set() if is_shared_lib else set([REFERENCE_POINT_FNAME])
    is_in_whitelist = dict()
    with open(cma_list_path, "r") as f:
        cma_info = f.read()
        if len(cma_info) == 0:
            exit(0)
        for info in cma_info.split("\n"):
            if info[:len(CMA_LIST_MODULE_PREFIX)] != CMA_LIST_MODULE_PREFIX and info != "":
                name, t = info.split(" ")
                is_in_whitelist[name] = 1 if t == "0" else 0
                cma_list.add(name)
    cma_list = list(cma_list)

    info_in_target = get_addr_info(target_file, cma_list, is_in_whitelist)
    info_in_shared_libs = []
    
    # We do not recursively resolve the shared library, assuming the shared libraries containing CMAs are directly linked or dlopened at the target binary
    if not is_shared_lib:
        for shared_lib in get_shared_libs(target_file):
            info = get_addr_info(shared_lib, cma_list, is_in_whitelist)
            info_in_shared_libs.extend(info)

    # Generate caches
    if not write_path is None:
        if len(info_in_target) == 0: # target must have at least one CMA or PIE reference point
            return False
        with open(write_path, "w") as f:
            for info in info_in_target:
                f.write(CACHE_ENTRY_FORMAT.format(*info))
            if not is_shared_lib:
                f.write("-\n") # splitter between static and dynamic symbols
                for info in info_in_shared_libs:
                    f.write(CACHE_ENTRY_FORMAT.format(*info))
    else:
        for info in info_in_target:
            print(CACHE_ENTRY_FORMAT.format(*info), end="")
        if not is_shared_lib:
            print("-") # splitter between static and dynamic symbols
            for info in info_in_shared_libs:
                print(CACHE_ENTRY_FORMAT.format(*info), end="")

    return True