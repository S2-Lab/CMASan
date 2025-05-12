
import os
import re
import sys
import json
import shutil
import argparse
from pathlib import Path
from collections import namedtuple

from rich.console import Console, Group
from rich.syntax import Syntax
from rich.panel import Panel
from rich.columns import Columns

import questionary as q
from questionary import Choice

### Data structures ###

Function = namedtuple('Function', ['name', 'path', 'begin', 'n_lines'])
CMAInfo = namedtuple('CMAInfo', ['name', 'path', 'begin', 'n_lines', 'size_idx', 'num_idx', 'ptr_idx', 'id_idx', 'fid', 'type'])
FamilyEntry = namedtuple('FamilyEntry', ['identified', 'candidates'])
CMAInfoForJSON = namedtuple('CMAInfoForJSON', ['funcname', 'file', 'begin', 'sizeIdx', 'numIdx', 'ptrIdx', 'idIdx', 'family', 'type'])

### Basic utils ###
def init_cwd(path="./.cmasan-identification", reset=True):
    cwd = Path(path)
    if reset:
        shutil.rmtree(path, ignore_errors=True)
        cwd.mkdir()
    return cwd.resolve()

def getchr():
    import sys, termios

    fd = sys.stdin.fileno()
    orig = termios.tcgetattr(fd)    

    new = termios.tcgetattr(fd)
    new[3] = new[3] &~ termios.ICANON
    new[6][termios.VMIN] = 1
    new[6][termios.VTIME] = 0

    try:
        termios.tcsetattr(fd, termios.TCSAFLUSH, new)
        return sys.stdin.read(1).lower()
    finally:
        termios.tcsetattr(fd, termios.TCSAFLUSH, orig)

def getchr_input():
    c = getchr()
    print()
    return c

### CodeQL ###

def run_codeql(query, db, out, is_rerun=False, external_csv=None):
    # default args codeql database analyze db/$P ../cmasan-identification/match_family.ql --format=csv --output=hi.csv --rerun --external=getAnAllocFunction=a.csv && cat hi.csv
    codeql_args = ["codeql", "database", "analyze", str(db), str(query), "--format=csv", f"--output={str(out)}", f"--threads={os.cpu_count()}"]
    if is_rerun:
        codeql_args.append("--rerun")
    else: 
        return
    if external_csv is not None:
        codeql_args.extend(["--external", f"getAnExternal={external_csv}"])
    
    print(" ".join(codeql_args))
    os.system(" ".join(codeql_args))

def load_formated_csv(csv: Path, write_back=False):
    lines = csv.read_text().splitlines()

    result = set()
    for l in lines:
        if l[0] == '"':
            result.add(l.split('","')[1])
        elif l[-1] == '"':
            result.add(l.split('","')[0])
        else:
            result.add(l)
    if write_back:
        csv.write_text("\n".join(result))
    result = [r.split("|") for r in result]
    return result

def load_alloc_candidates(alloc_cand_csv, write_back=True):
    result = load_formated_csv(alloc_cand_csv, write_back=write_back)
    funcs = [Function(*l) for l in result]
    return funcs

def load_family_map(family_result_csv, alloc_cand_funcs, write_back=False):
    result = load_formated_csv(family_result_csv, write_back=write_back)
    
    family_map = dict()
    
    # Initialize family entries
    for alloc_cand in alloc_cand_funcs:
        family_map[alloc_cand] = []
    
    # Construct family map
    for l in result:
        alloc_cand = Function(*(l[:4]))
        family_func = Function(*(l[4:]))            
        
        family_map[alloc_cand].append(family_func)
    
    # Collect surrounding functions
    for alloc_cand in family_map.keys():
        if len(family_map[alloc_cand]) < 100:
            continue
        path_map = {}
        for func in family_map[alloc_cand]:
            if not func.path in path_map:    
                path_map[func.path] = []
            path_map[func.path].append(func)
        
        for path in path_map.keys():
            path_map[path].append(alloc_cand)
            path_map[path].sort(key=lambda f: int(f.begin))
            i = path_map[path].index(alloc_cand)
            s = max(0, i-10)
            e = min(i+10, len(path_map[path]))
            path_map[path] = path_map[path][s:i] + path_map[path][i+1:e]
        result = []
        for v in path_map.values():
            result.extend(list(v))
        result = list(set(result))

        family_map[alloc_cand].clear()
        family_map[alloc_cand].extend(result)

    return family_map

def write_back_formatted_csv(dict_list, csv_file):
    with open(csv_file, "w") as f:
        f.write("\n".join(["|".join(list(map(str, list(d)))) for d in dict_list]))

def get_funcion_info(info: CMAInfo):
    return Function(info.name, info.path, info.begin, info.n_lines)

### File Utils ###

'''
if cache exists, return the current step and cache.
'''
def cache_or_empty():
    if is_cache_exists:
        with open(cache_file_path, "r") as f:
            cache = f.read()
            idx = cache.find("\n")
            step = int(cache[:idx])
            return step, list(json.loads(cache[idx:]))
    return 0, []

'''
Find closest matching file path
'''
def get_path(target_path):
    return Path(target_path)

'''
Load function by (path, begin, end) information
'''

loaded_files = {}
def load_file(file_path):
    global loaded_files
    if not file_path in loaded_files.keys():
        try:
            with open(file_path) as f: # Create a cache
                loaded_files[file_path] = f.read()
        except:
            result = find_realpath(file_path)
            if not result is None:
                with open(result) as f: # Use found result, but key as original path
                    loaded_files[file_path] = f.read()
            else:
                return None
    return loaded_files[file_path]

def load_func(file_path, beg, n_lines, beg_pad=2, end_pad=1):
    content = load_file(file_path)
    beg, n_lines = int(beg), int(n_lines)
    if content is None:
        print(f"WARNING: source code not found for ({file_path}). Skip Analysis.")
        return None
    file = content.split("\n")
    end = beg + n_lines
    beg = max(0, beg - beg_pad)
    end = min(end + end_pad, len(file))

    return "\n".join(file[beg:end])

def write_back_cma_list(cma_list, target, out_dir):
    output_file = f"{out_dir}/{target}.json"
    with open(output_file, "w") as f:
        json_str = json.dumps(cma_list, indent=4)
        f.write(json_str)
    print(f"CMA list of {target} is written to {output_file}")
'''
Write back the result
'''
def write_back(processed_func_list, step, is_cache=False):
    if is_cache:
        path = cache_file_path
    else:
        path = out_file_path
    with open(path, "w") as f:
        if is_cache:
            f.write(str(step) + "\n")
        f.write(json.dumps(processed_func_list, sort_keys=True, indent=4))


### Interactive Utils ###

DESCRIPTION_RULE_SIZE = '''There is an integer (size) argument used for the object allocation by:
    (a) Pointer arithmetic (including array indexing)
    (b) Comparison with internal metadata
If the product of two arguments satisfies the above rules, one argument is size, and the other is granularity'''

DESCRIPTION_RULE_FREE_AND_CLEAR_AND_FP = '''1. [bold]Free API[/bold]: There is a pointer argument to be stored in the internal storage, which is used for allocating an object in the matched ALLOC candidates.
2. [bold]Clear API[/bold]: it satisfies (a) or (b)
    (a) reset base pointer, which is used as an arena of this CMA family, to the first position
    (b) destroys the CMA family by releasing all the resources the CMA allocated
3. [bold]Object accessing API[/bold]: The given function accesses objects allocated by the CMA family
4. [bold]Size querying API[/bold]: The given function receives an object argument and returns the size of the object
5. [bold]Object collecting API[/bold]: The given function collects the objects into internal structures under specific conditions (i.e., reference counting).'''

DESCRIPTION_RULE_INSTANCE = '''There is an argument that serves as the source for the internal storage, responsible for storing and providing the object'''

DESCRIPTION_ANNOTATION = '''
1. [bold]Alloc[/bold]: There is an integer (size) argument used for the object allocation
2. [bold]Relloc[/bold]: Alloc + it resizes the given object
3. [bold]Calloc[/bold]: Alloc + it has a granularity argument (used in size*granularity)
4. [bold]Free[/bold]: There is a pointer argument to be stored in the internal storage, which is used for allocating an object in the matched ALLOC candidates.
5. [bold]Clear[/bold]: it satisfies (a) or (b)
    (a) reset base pointer, which is used as an arena of this CMA family, to the first position
    (b) destroys the CMA family by releasing all the resources the CMA allocated
6. [bold]Object accessing[/bold]: The given function accesses objects allocated by the CMA family
7. [bold]Size querying[/bold]: The given function receives an object argument and returns the size of the object
8. [bold]Object collecting[/bold]: The given function collects the objects into internal structures under specific conditions (i.e., reference counting).'''

QUESTIONARY_STYLE = q.Style([
        ("highlighted", "bold"),  # style for a token which should appear highlighted
])

console = Console()

def clear():
    os.system('clear')
    console.clear()

def categorize_alloc_member(alloc_func: Function,
                            alloc_code: str,
                            language: str = "cpp"):
    size_idx, num_idx, ptr_idx, id_idx, fid, cma_type = -1, -1, -1, -1, -1, "ALLOC"

    alloc_syntax = Syntax(alloc_code, language, line_numbers=True, word_wrap=True)
    group = Group(
        Panel(alloc_syntax, title="ALLOC candidate", border_style="cyan"),
        Panel(DESCRIPTION_RULE_SIZE, title="Rule size", border_style="green"),
    )
    console.print(group)
    # console.print(group)

    result = q.select(
        "Does this ALLOC candidate satisfy Rule size?",
        choices=[
            Choice(title="No. Move to the next function", value="SKIP"),
            Choice(title="Yes. It has a size argument that satisfies the rule", value="ALLOC_CAND"),
        ],
        style=QUESTIONARY_STYLE
    ).ask()

    if result == "SKIP":
        return None

    size_idx = q.text(
        "index of [size] argument (e.g., 0th):",
        validate=lambda t: t.isdigit(),
    ).ask()
    
    print()

    num_idx = q.text(
        "(Optional - CALLOC) index of [granularity] argument (used in size*granularity), or ENTER if none:",
        validate=lambda t: t.isdigit() or t == "",
    ).ask()
    if num_idx != "": cma_type = "CALLOC"
    else: num_idx = -1

    print()

    ptr_idx = q.text(
        "Does this function resize the given object?\n  (Optional - REALLOC) index of [object] argument, or ENTER if none:",
        validate=lambda t: t.isdigit() or t == "",
    ).ask()
    if ptr_idx != "": cma_type = "REALLOC"
    else: ptr_idx = -1

    print()

    console.print(Panel(DESCRIPTION_RULE_INSTANCE, title="Rule instance", border_style="green"))
    id_idx = q.text(
        "Is there an [instance] argument satisfying Rule instance?\n  (Optional) index of the [instance] argument, or ENTER if none:",
        validate=lambda t: t.isdigit() or t == "",
    ).ask()
    if id_idx == "": id_idx = -1
    return CMAInfo(*alloc_func, int(size_idx), int(num_idx), int(ptr_idx), int(id_idx), fid, cma_type)


def categorize_family_member(family_func: Function,
                            alloc_func: Function,
                            family_code: str,
                            alloc_code: str,
                            language: str = "cpp"):
    size_idx, num_idx, ptr_idx, id_idx, fid, cma_type = -1, -1, -1, -1, -1, None

    family_syntax = Syntax(family_code, language, line_numbers=True, word_wrap=True)
    alloc_syntax = Syntax(alloc_code, language, line_numbers=True, word_wrap=True)

    cols = Columns(
        [
            Panel(family_syntax, title="Family member candidate", border_style="magenta"), # width=int(console.width*0.45)),
            Panel(alloc_syntax, title="ALLOC of this family", border_style="cyan"), # width=int(console.width*0.45)),
        ],
        equal=False,
        expand=True,
    )
    group = Group(
        cols,
        Panel(DESCRIPTION_RULE_FREE_AND_CLEAR_AND_FP, title="Rules", border_style="green"),
    )
    console.print(group)

    cma_type = q.select(
        "Select if this family member falls in any API above",
        choices=[
            Choice(title="Move to the next Function", value="SKIP"),
            Choice(title="Free", value="FREE"),
            Choice(title="Clear", value="CLEAR"),
            Choice(title="Object-accessing", value="NONE"),
            Choice(title="Size-querying", value="SIZE"),
            Choice(title="Object-collecting", value="GC"),
        ],
        style=QUESTIONARY_STYLE
    ).ask()

    if cma_type == "SKIP":
        return None

    ptr_idx = -1
    if cma_type in ("FREE", "SIZE"):
        ptr_idx = q.text(
            "index of the [pointer] argument:",
            validate=lambda t: t.isdigit(),
        ).ask()

    if not cma_type in "NONE":
        console.print(Panel(DESCRIPTION_RULE_INSTANCE, title="Rule instance", border_style="green"))
        id_idx = q.text(
            "Is there an [instance] argument satisfying Rule instance?\n  (Optional) index of the [instance] argument, or ENTER if none:",
            validate=lambda t: t.isdigit() or t == "",
        ).ask()
        if id_idx == "": id_idx = -1

    return CMAInfo(*family_func, int(size_idx), int(num_idx), int(ptr_idx), int(id_idx), alloc_func.fid, cma_type)

def annotate_member(func: Function,
                    code: str,
                    language: str = "cpp"):
    size_idx, num_idx, ptr_idx, id_idx, fid, cma_type = -1, -1, -1, -1, -1, None

    family_syntax = Syntax(code, language, line_numbers=True, word_wrap=True)

    group = Group(
        Panel(family_syntax, title="Family member candidate", border_style="cyan"),
        Panel(DESCRIPTION_ANNOTATION, title="CMA Members", border_style="green"),
    )
    console.print(group)

    cma_type = q.select(
        "Select the type of this function",
        choices=[
            Choice(title="Alloc", value="ALLOC"),
            Choice(title="Realloc", value="REALLOC"),
            Choice(title="Calloc", value="CALLOC"),
            Choice(title="Free", value="FREE"),
            Choice(title="Clear", value="CLEAR"),
            Choice(title="Object-accessing", value="NONE"),
            Choice(title="Size-querying", value="SIZE"),
            Choice(title="Object-collecting", value="GC"),
        ],
        style=QUESTIONARY_STYLE
    ).ask()

    if cma_type == "ALLOC" or cma_type == "REALLOC" or cma_type == "CALLOC":
        size_idx = q.text(
            "index of the [size] argument:",
            validate=lambda t: t.isdigit(),
        ).ask()

    if cma_type == "CALLOC":
        num_idx = q.text(
            "index of the [granularity] argument:",
            validate=lambda t: t.isdigit(),
        ).ask()

    if cma_type == "FREE" or cma_type == "SIZE" or cma_type == "REALLOC":
        ptr_idx = q.text(
            "index of the [pointer] argument:",
            validate=lambda t: t.isdigit(),
        ).ask()

    if cma_type != "NONE":
        console.print(Panel(DESCRIPTION_RULE_INSTANCE, title="Rule instance", border_style="green"))
        id_idx = q.text(
            "Is there an [instance] argument satisfying Rule instance?\n  (Optional) index of the [instance] argument, or ENTER if none:",
            validate=lambda t: t.isdigit() or t == "",
        ).ask()
        if id_idx == "": id_idx = -1

    return CMAInfoForJSON(func.name, str(func.path), int(func.begin), int(size_idx), int(num_idx), int(ptr_idx), int(id_idx), 1, cma_type)
