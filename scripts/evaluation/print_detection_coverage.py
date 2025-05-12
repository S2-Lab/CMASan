import sys
import os
from tqdm import tqdm

ASAN_PREFIX = "asan.crash"
LOG_SUFFIX = ".log"

LOG_CONFLICT_FNAME = "cmasan_log_conflict"


if len(sys.argv) < 2:
        print(f"Usage: {os.path.basename(sys.argv[0])} <application1> [application2 ...]")
        sys.exit(1)

applications = sys.argv[1:]

outputs = []
for application in applications:
    print(f"================================================= {application:^12} ==================================================")
    result_dir = "./result/" + application #+ "_log"
    all_files = os.listdir(result_dir)

    log_files = [file for file in all_files if file.endswith(LOG_SUFFIX)]
    crash_files = [file for file in all_files if file.startswith(ASAN_PREFIX)]

    # Check if processes conflicted during logging
    if os.path.exists(result_dir+"/"+LOG_CONFLICT_FNAME):
        print("fatal: log was trimmed")
        exit(-1)

    # Collect logs
    log_cnt = [0]*14
    max_n_instances = 0
    for i in tqdm(range(len(log_files))):
        file = log_files[i]
        filedir = result_dir + "/" + file
        if file == "time.log":
            continue
        with open(filedir) as f:
            lines = f.readlines()

            if len(lines) > 1:
                print("Warning: {} has more than one line".format(filedir))
            log_result = lines[0].split()
            if len(log_cnt) != len(log_result):
                print("fatal: log {} has fewer than {} entries".format(filedir, len(log_cnt)))
                continue
                exit(-1)
            for j in range(len(log_cnt)):
                log_cnt[j] += int(log_result[j])

            max_n_instances = max(max_n_instances, int(log_result[-1]))
    
    # Print out asan crashes
    if len(crash_files) > 0:
        print("Warning: following asan crashes are there")
        for file in crash_files:
            filedir = result_dir + "/" + file
            print(filedir)

    # Print out collected logs
    log_cnt[0] = log_cnt[0] + log_cnt[1] # ALLOC + REALLOC
    log_cnt[2] = log_cnt[2] + log_cnt[3] # FREE + REALLOC
    log_cnt[4] = log_cnt[4] + log_cnt[5] # FREE + REALLOC (QZ)
    print()
    avg_cnt = [cnt/len(log_files) for cnt in log_cnt]
    all_cnt = [0]*2*len(log_cnt)
    for i in range(len(log_cnt)):
        all_cnt[2*i] = log_cnt[i]
        all_cnt[2*i+1] = avg_cnt[i]

    application = application
    n_cma_objects = log_cnt[0]
    n_base_chunks = log_cnt[8] + log_cnt[9]
    n_cma_free = log_cnt[2]
    asan_checks_on_cma = log_cnt[6]
    asan_checks_on_cma_ratio = asan_checks_on_cma/(asan_checks_on_cma+log_cnt[7])*100
    max_n_instances = max_n_instances

    format_with_commas = lambda x: f"{x:,}" if isinstance(x, int) else x
    output = list(map(format_with_commas, (application, n_cma_objects, n_base_chunks, n_cma_free, asan_checks_on_cma, asan_checks_on_cma_ratio, max_n_instances)))
    outputs.append(output)
    
print(f"==================================================== Table =======================================================")
print(f"|  Application  | CMA Objects | Base Chunks | Freed Objects |   load/store Checks (ratio)   | Max. CMA Instances |")
for output in outputs:
    print("| {:>13} | {:>11} | {:>11} | {:>13} | {:>20} ({:05.2f}%) | {:>18} |".format(*output))
