# Getting Started with CMASan

This guide explains how to apply **CMASan** to a new application. The overall workflow consists of the following steps:

1. Writing a build script  
2. Building a CodeQL database  
3. Generating the CMA list (CMA Identification)  
4. Building the application  
5. Running the instrumented application  

---

## 1. Write a Build Script

CMASan provides template scripts for common build systems under `benchmark/scripts/templates`.  
(If you've suffered from complex build setups with other sanitizer-based tools, feel free to reuse these in your project.)

Supported build systems:
- CMake/Make (`cmake.sh`)
- Bazel (`bazel.sh`)
- SCons (`scons.sh`)
- C/C++ Python Extension (`on_python.sh`)

Use these templates to write a build script for your application and place it under:
`benchmark/scripts/<your_application_name>.sh`


## 2. Build the CodeQL Database

_(If you already have a known list of CMA APIs, skip this process)_

Run the following command to generate a CodeQL database:

```bash
codeql database create $(pwd)/db/<your_application_name> \
  --language=c-cpp \
  --command="$(pwd)/scripts/run_in_codeql.sh <your_application_name>" \
  --overwrite \
  --source-root=/ \
  -j$(nproc)
````

For more information, refer to the [official CodeQL documentation](https://docs.github.com/en/code-security/codeql-cli/getting-started-with-the-codeql-cli/preparing-your-code-for-codeql-analysis).


## 3. Generate the CMA List (CMA Identification)

If you have a known list of CMA APIs, create a text file listing each function location on a separate line in the format `<filepath>:<begin>`, and manually annotate them using:
```bash
python3 scripts/cma-identification/annotate.py -t <your application name> -i <your cma list location>
```

Otherwise, you can use our helper script to identify candidate CMA APIs semi-automatically with your response:

```bash
python3 scripts/cma-identification/analyze.py -t <your application name>
```

Annotation or Identification script will generate a CMA list at: `./<your_application_name>.json`

Check out the generated JSON and move them into `benchmark/cma_list`


## 4. Build the Application

Run the build script you prepared in Step 1:

```bash
./scripts/<your_application_name>
```

## 5. Run the Application

Run the following script to test the instrumented application:

```bash
./result/<your_application_name>/single_run.sh
```

Or, load the test environment manually and run commands interactively:

```bash
source ./result/<your_application_name>/test_env
```
