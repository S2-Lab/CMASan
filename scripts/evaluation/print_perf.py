#!/usr/bin/env python3

import sys
import re
import os
import json 

# Full path to the result file
def get_result_path(app_name, log_name="time.log"):
    return os.path.join("benchmark", os.path.join("result", app_name, log_name))

# read with various encodings and line endings
def read_file_lines(filepath):
    try:
        with open(filepath, encoding="utf-8") as f:
            return f.read().splitlines()
    except UnicodeDecodeError:
        try:
            with open(filepath, encoding="latin1") as f:  # ISO-8859-1 호환
                return f.read().splitlines()
        except UnicodeDecodeError:
            with open(filepath, encoding="utf-16") as f:
                return f.read().splitlines()

# import chardet
# def read_file_lines(filepath):
#     with open(filepath, "rb") as f:
#         raw = f.read()
#     encoding = chardet.detect(raw)["encoding"] or "utf-8"
#     text = raw.decode(encoding, errors="replace")
#     text = text.replace('\r\n', '\n').replace('\r', '\n')
#     return text.split('\n')

# Parses lines like '123 seconds' and accumulates the integer part
def parse_redis_time(app_name):
    filepath = get_result_path(app_name)
    total = 0.0
    pattern = re.compile(r'(\d+)\s+ms')
    for line in read_file_lines(filepath):
        match = pattern.search(line)
        if match:
            total += float(match.group(1))
    return total

# Parses lines like 'avg =  12.34' and accumulates the float value
def parse_ncnn_time(app_name):
    filepath = get_result_path(app_name)
    total = 0.0
    pattern = re.compile(r'avg =\s*([0-9]+\.[0-9]+)')
    for line in read_file_lines(filepath):
        match = pattern.search(line)
        if match:
            total += float(match.group(1))
    return total

# Parses lines like 'PASSED in 12.34' and accumulates the float value
def parse_grpc_time(app_name):
    filepath = get_result_path(app_name)
    total = 0.0
    pattern = re.compile(r'PASSED in\s+([0-9]+\.[0-9]+)')
    for line in read_file_lines(filepath):
        match = pattern.search(line)
        if match:
            total += float(match.group(1))
    return total

# Parses JSON-like lines such as '"time":123.45'
def parse_godot_time(app_name):
    elapsed = 0.0
    filepath = get_result_path(app_name, "bench_result.json")
    with open(filepath) as f:
        logs = json.load(f)["benchmarks"]
        for log in logs:
            elapsed += float(log["results"]["time"])
    return elapsed

# Parses lines like 'Time taken      : 12.34'
# If unit suffixes like 's' exist, they're stripped off
def parse_php_time(app_name):
    filepath = get_result_path(app_name)
    elapsed = 0.0
    pattern = re.compile(r'Time taken\s*:\s*([0-9]*\.?[0-9]+)')
    for line in read_file_lines(filepath):
        match = pattern.search(line)
        if match:
            val_str = match.group(1).strip().lower().replace('s', '')
            elapsed = float(val_str)
            break

    return elapsed

# Parses lines like 'Total Test time (real) = 123.45 sec'
def parse_ctest_time(app_name):
    filepath = get_result_path(app_name)
    elapsed = 0.0
    pattern = re.compile(r'Total Test time \(real\) = ([0-9]+\.[0-9]+)')
    for line in read_file_lines(filepath):
        match = pattern.search(line)
        if match:
            elapsed = float(match.group(1))
            break
    return elapsed

# Parses lines like '* failed, * passed in 123.45s'
def parse_taichi_time(app_name):
    filepath = get_result_path(app_name)
    elapsed = 0.0
    pattern = re.compile(r'passed in\s+([0-9]+\.[0-9]+)s')
    for line in read_file_lines(filepath):
        match = pattern.search(line)
        if match:
            elapsed = float(match.group(1))
            break
    return elapsed

# Parses lines like 'Elapsed (wall clock) time (h:mm:ss or m:ss): 0:40.49'
# Converts time format to seconds and accumulates
def parse_wall_clock_time(app_name):
    filepath = get_result_path(app_name)
    total = 0.0
    pattern = re.compile(r'Elapsed \(wall clock\) time \(h:mm:ss or m:ss\):\s+([0-9:]+\.[0-9]+)')
    for line in read_file_lines(filepath):
        match = pattern.search(line)
        if match:
            timestr = match.group(1)
            parts = timestr.split(':')
            try:
                if len(parts) == 2:
                    total += float(parts[0]) * 60 + float(parts[1])
                elif len(parts) == 3:
                    total += float(parts[0]) * 3600 + float(parts[1]) * 60 + float(parts[2])
                else:
                    total += float(timestr)
            except ValueError:
                pass
    return total


# Parser mappings
PARSERS = {
    "redis"       : parse_redis_time,
    "ncnn"        : parse_ncnn_time,
    "grpc"        : parse_grpc_time,
    "godot"       : parse_godot_time,
    "php-src"     : parse_php_time,
    "folly"       : parse_ctest_time,
    "rocksdb"     : parse_ctest_time,
    "taichi"      : parse_taichi_time,
    "tensorflow"  : parse_wall_clock_time,
    "micropython" : parse_wall_clock_time,
    "cocos2d-x"   : parse_wall_clock_time,
    "swoole-src"  : parse_wall_clock_time,
}

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {os.path.basename(sys.argv[0])} <application1> [application2 ...]")
        print(f"Supported apps are:\n  " + ", ".join(PARSERS.keys()))
        sys.exit(1)

    app_names = sys.argv[1:]

    result = []
    for app_name in app_names:
 
        if not app_name in PARSERS:
            print(f"[ERROR] Unknown application '{app_name}'. Skipped.")
            continue
        parser_function = PARSERS[app_name]

        try:
            cmasan_time = parser_function(app_name)
            asan_time = parser_function(app_name + "_asan")
            result.append((app_name, (cmasan_time/asan_time*100)-100))
        except FileNotFoundError:
            print(f"[ERROR] File not found for '{app_name}'. Check log path.")
        except Exception as e:
            print(f"[ERROR] Failed to parse '{app_name}': {str(e)}")


    print(f"================== Table ===================")
    print(f"|  Application  | Performance Overhead (%) |")
    for app_name, overhead in result:
        print("| {:>13} | {:>24.2f} |".format(app_name, overhead))

if __name__ == "__main__":
    main()
