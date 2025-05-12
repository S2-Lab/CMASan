from utils import *

def parse_arguments():
    parser = argparse.ArgumentParser(description="CMASan Identification Helper Script.")

    parser.add_argument("-t", "--target", help="Target application name", type=str, required=True)
    parser.add_argument("-d", "--database", help="Parent directory of the database of the target (<directory>/<target>)", type=str, default="./benchmark/db")
    parser.add_argument("-o", "--output", help="Output directory for the result", type=str, default="./")
    parser.add_argument("-r", "--rerun", help="If false, CodeQL query will not be run and reuse previous result", default=True)

    return parser.parse_args()

def main():
    ### Preparation    
    args = parse_arguments()
    script_home = Path(__file__).resolve().parent
    alloc_cand_query = script_home / "alloc_candidates.ql"
    match_family_query = script_home / "match_family.ql"

    ### Build DB (only when args.database is not given)
    database_home = Path(args.database).resolve()

    ### Analyze the target
    target = args.target
    database = database_home / target

    cwd = init_cwd(path=f"./.cmasan-identification/{target}", reset=args.rerun)

    ### Gather alloc candidates
    alloc_cand_csv = cwd / f"alloc.csv"
    run_codeql(alloc_cand_query, database, alloc_cand_csv, is_rerun=args.rerun)
    alloc_cand_funcs = load_alloc_candidates(alloc_cand_csv, write_back=True)

    ### Match family for each alloc candidates
    family_result_csv = cwd / "family.csv"
    run_codeql(match_family_query, database, family_result_csv,  external_csv=alloc_cand_csv, is_rerun=args.rerun)
    family_map = load_family_map(family_result_csv, alloc_cand_funcs)

    alloc_cand_funcs = load_alloc_candidates(alloc_cand_csv)
    alloc_cand_funcs.sort(key=lambda f: (f.path, f.begin))

    alloc_identified_csv = cwd / "alloc_identified.csv"
    alloc_identified = []
    step_cache = cwd / "prev_step"
    step = 0
    while step < len(alloc_cand_funcs):
        alloc_cand = alloc_cand_funcs[step]

        ### Informations ###
        size_idx, num_idx, ptr_idx, id_idx, fid, cma_type = -1, -1, -1, -1, -1, None
        file_path, begin, n_lines = alloc_cand.path, int(alloc_cand.begin), int(alloc_cand.n_lines)
        #### Load function code (caching) ###
        file_path = get_path(file_path)

        func_code = load_func(file_path, begin, n_lines)
        if func_code is None:
            continue
        clear()
        print(f"Current step: {step}/{len(alloc_cand_funcs)}")
        print(f"Current file: {file_path}:{begin}")
        cma_info = categorize_alloc_member(alloc_cand, func_code, language="cpp")
        if not cma_info is None: alloc_identified.append(cma_info)
        step += 1

    print(alloc_identified)
    write_back_formatted_csv(alloc_identified, alloc_identified_csv)
    step_cache.write_text(str(step))

    families = []
    ### Categorize alloc functions (user interaction)
    for i in range(len(alloc_identified)):
        if not alloc_identified[i] is None:
            # Family map for this ALLOC
            family = family_map[get_funcion_info(alloc_identified[i])]
            family_entry = FamilyEntry([], family)
            # Move the identified family members into identified list
            for j in range(i, len(alloc_identified)):
                if alloc_identified[j] is None:
                    continue
                func_info = get_funcion_info(alloc_identified[j])
                if func_info in family_entry.candidates:
                    family_entry.candidates.remove(func_info) # Remove from candidates
                    family_entry.identified.append(alloc_identified[j]) # Add to identified alloc
                    alloc_identified[j] = None
            families.append(family_entry)

    clear()
    print(f"Now we are going to categorize the members of {len(families)} CMA families. Press Any key to continue...")
    ans = getchr_input()

    ### Categorize family functions (user interaction)
    for fid in range(len(families)):
        assert (len(families[fid].identified) > 0)
        alloc_info = families[fid].identified[0] # we only refer first ALLOC
        alloc = get_funcion_info(alloc_info)
        alloc_func_code = load_func(alloc.path, alloc.begin, alloc.n_lines)

        step = 0
        family_cands = families[fid].candidates

        while step < len(family_cands):
            family_cand = family_cands[step]
            cma_type, size_idx, num_idx, ptr_idx, id_idx = -1, -1, -1, -1, -1
            
            family_func_code = load_func(family_cand.path, family_cand.begin, family_cand.n_lines)
            if family_func_code is None:
                continue

            #### Print function information to user ###
            clear()
            print(f"Current family step: {step}/{len(family_cands)}")
            print(f"Current file: {family_cand.path}:{family_cand.begin}")
            cma_info = categorize_family_member(family_cand, alloc_info, family_func_code, alloc_func_code)
            if not cma_info is None: families[fid].identified.append(cma_info)
            step += 1

    # convert to JSON
    identified = []
    for i in range(len(families)):
        for j in range(len(families[i].identified)):
            info = families[i].identified[j]
            info_with_fid = CMAInfoForJSON(info.name, info.path, int(info.begin), info.size_idx, info.num_idx, info.ptr_idx, info.id_idx, i+1, info.type)
            identified.append(info_with_fid._asdict())

    write_back_cma_list(identified, target, args.output)
            
if __name__ == "__main__":
    main()
