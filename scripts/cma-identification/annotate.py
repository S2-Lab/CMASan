from utils import *

def parse_arguments():
    parser = argparse.ArgumentParser(description="CMASan Annotation Helper Script.")

    parser.add_argument("-t", "--target", help="Target application name", type=str, required=True)     
    parser.add_argument("-d", "--input", help="List of the CMA Family member list. (<file>:<begin>)", type=str, default="./cma_list.txt")
    parser.add_argument("-o", "--output", help="Output directory for the result", type=str, default="./")
    parser.add_argument("-n", "--nlines", help="Number of lines to print", type=int, default=50)

    return parser.parse_args()

def main():
    args = parse_arguments()
    cma_list = Path(args.input).resolve().read_text().splitlines()
    cma_list = [i.split(":") for i in cma_list]
    cma_list = [(i[0], int(i[1])) for i in cma_list]

    cma_annotated_list = []
    for file_path, begin in cma_list:
        file_path = get_path(file_path)
        func_code = load_func(file_path, begin, args.nlines)
        symbol = " ".join(func_code.split("\n")[:2]).strip()
        func = Function(symbol, file_path, begin, args.nlines)
        if func_code is None:
            continue
        clear()
        cma_annotated_list.append(annotate_member(func, func_code)._asdict())
    write_back_cma_list(cma_annotated_list, args.target, args.output)

if __name__ == "__main__":
    main()
