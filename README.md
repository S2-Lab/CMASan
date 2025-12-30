# CMASan: Custom Memory Allocator-aware Address Sanitizer

<a href="https://s2-lab.github.io/assets/CMASan_S&P25.pdf" target="_blank"><img src="https://raw.githubusercontent.com/junwha/junwha/refs/heads/main/cmasan.png" style="padding-left:10px" align="right" width="230"></a>

This is the artifact of the paper CMASan: Custom Memory Allocator-aware Address Sanitizer, which is accepted at the IEEE Symposium on Security and Privacy 2025.

CMASan is a Custom Memory Allocator-aware Address Sanitizer that detects memory bugs on objects allocated by Custom Memory Allocators (CMAs), which are missed by the native Address Sanitizer (ASan).

Unlike ASan, CMASan preserves the internal behavior of CMAs while providing redzone instrumentation, instance-specific quarantine zones, and false positive suppression. As a result, CMASan significantly improves the bug detection capability for real-world CMA memory bugs with acceptable runtime overhead.

The following instructions describe how to build CMASan and apply it to various real-world targets using custom allocators.

## Citation
```
@inproceedings{hong2025cmasan,
  title={CMASan: Custom Memory Allocator-aware Address Sanitizer},
  author={Hong, Junwha and Jang, Wonil and Kim, Mijung and Yu, Lei and Kwon, Yonghwi and Jeon, Yuseok},
  booktitle={2025 IEEE Symposium on Security and Privacy (SP)},
  pages={740--757},
  year={2025},
  organization={IEEE}
}
````

## Code Structure
CMASan is implemented on the top of Address Sanitizer of LLVM version 15.0.6.

```
CMASan/
├── LLVM/                          # CMASan's LLVM source
│   └── src/                       # CMASan source code
├── LLVM-log/                      # CMASan compiled with logging utilities
├── LLVM-pure/                     # LLVM with native ASan
├── benchmark/                     # Benchmark suite for CMASan evaluation
│   ├── scripts/                   # Application build scripts
│   ├── cma_list/                  # CMA_LIST files for each application
│   ├── src/                       # Benchmark application source code
│   ├── result/                    # Benchmark result output
│   └── data/                      # Application workload data
├── scripts/                       # Utility scripts
│   ├── cma-identification         # CMA identification helper scripts
|   ├── evaluation                 # CMASan evaluation scripts
│   └── ...                        # Other CMASan-related scripts
└── test/                          # Tests for CMASan
```

# Ⅰ. Environment Setup
### Using Docker (Recommended)
You can use our prebuilt image on DockerHub
```bash
docker pull s2lab/cmasan
```

Or build a docker image by yourself:
```bash
git clone https://github.com/S2-Lab/CMASan && cd CMASan
docker build -t s2lab/cmasan ./
```

Then, create a container:
```bash
xhost +local:docker
docker run -it \
  --env DISPLAY=$DISPLAY \
  --env QT_X11_NO_MITSHM=1 \
  --env XAUTHORITY=/root/.Xauthority \
  --volume /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --volume $XAUTHORITY:/root/.Xauthority:ro \
  --privileged \
  --gpus all \
  s2lab/cmasan
```
- if you don't have a gpu, it's okay to run without `--gpus all`
- if you have no GUI in host, it's okay to run without X11-related arguments, but we recommend this for GUI applications (e.g., godot).
- we recommend to set volume to `/root/.cache` and `/root/CMASan/benchmark/src` (e.g., `-v<cache directory on host>:/root/.cache -v<source code directory on host>:/root/CMASan/benchmark/src`)

### Building from source

For the custom use of CMASan, use the following script.
```bash
git clone https://github.com/S2-Lab/CMASan && cd CMASan
./scripts/build_cmasan.sh
```

For the running the evaluation targets of CMASan:
```bash
git clone https://github.com/S2-Lab/CMASan && cd CMASan
./scripts/build_for_evaluation.sh
```

# Ⅱ. How to use

If you want to apply CMASan to your own application, please refer to [Usage Guide](docs/GUIDE.md) for detailed instructions.

# Ⅲ. Evaluation
Below are instructions for running CMASan's benchmark target applications.

To run the experiments in the paper, we used  a 32-core Intel(R) Core(TM) i9-13900KF CPU with 128GB DDR4 RAM and an NVIDIA GeForce RTX 4090 GPU, running on Ubuntu 22.04.1 LTS (GNU/Linux 6.2.0) with GUI.

CMASan is evaluated with the following 12 popular C/C++ open-source applications in Github:
| Identifier   | Github repo                                         | Description                                                                                                      |
|--------------|-----------------------------------------------------|------------------------------------------------------------------------------------------------------------------|
| grpc         | [grpc/grpc](https://github.com/grpc/grpc)           | The C based gRPC (C++, Python, Ruby, Objective-C, PHP, C#)                                                      |
| php-src      | [php/php-src](https://github.com/php/php-src)       | The PHP Interpreter                                                                                             |
| redis        | [redis/redis](https://github.com/redis/redis)       | Redis is an in-memory database that persists on disk. It is a high-performance key-value store.                |
| folly        | [facebook/folly](https://github.com/facebook/folly) | An open-source C++ library developed and used at Facebook. |
| micropython  | [micropython/micropython](https://github.com/micropython/micropython) | MicroPython - a lean and efficient Python implementation for microcontrollers and constrained systems           |
| ncnn         | [Tencent/ncnn](https://github.com/Tencent/ncnn)     | High-performance neural network inference framework optimized for mobile platforms                              |
| cocos2d-x    | [cocos2d/cocos2d-x](https://github.com/cocos2d/cocos2d-x) | Open-source, cross-platform, game-development tools used by thousands of developers worldwide                   |
| swoole-src   | [swoole/swoole-src](https://github.com/swoole/swoole-src) | Event-driven asynchronous & coroutine-based concurrency networking engine for PHP                               |
| taichi       | [taichi-dev/taichi](https://github.com/taichi-dev/taichi) | Productive & portable programming language for high-performance, sparse & differentiable computing              |
| rocksdb      | [facebook/rocksdb](https://github.com/facebook/rocksdb) | Embeddable, persistent key-value store for fast storage                                                         |
| tensorflow   | [tensorflow/tensorflow](https://github.com/tensorflow/tensorflow) | An Open Source Machine Learning Framework for Everyone                                                          |
| godot        | [godotengine/godot](https://github.com/godotengine/godot) | Godot Engine – Multi-platform 2D and 3D game engine                                                             |


## Detection Coverage (Table I)
### Build for logging
First, build the target application with the logger-enabled CMASan (i.e., `CMASan/LLVM-log`).
```bash 
./scripts/evaluation/build_with_log.sh
```

### Run
Then, run the applications and check the result.
```bash
./scripts/evaluation/run_workload.sh
python3 ./scripts/evaluation/print_detection_coverage.py
```

## Performance Overhead (Table II)
### Build with CMASan & ASan
First, build the target application with CMASan and ASan (i.e., `CMASan/LLVM` and `CMASan/LLVM-pure`).

```bash 
./scripts/evaluation/build.sh
```

### Performance Overhead
Measure the performance overhead and check the result.
```bash
./scripts/evaluation/run_workload.sh
python3 ./scripts/evaluation/print_perf.py
```

### Memory Overhead
Measure the memory overhead and and check the result.
```bash
./scripts/evaluation/run_workload_with_memory.sh
python3 ./scripts/evaluation/print_memory.py
```

## False Positive Avoidance (Table III)
### Build for logging
First, build the target application with the logger-enabled CMASan (i.e., `CMASan/LLVM-log`).
```bash 
./scripts/evaluation/build_with_log.sh
```

### Run
The false positive avoidance can be turned off by the option `CMASAN_FP_AVOIDANCE=0`. 
The following script will turn off the option and run the corresponding applications without our false positive avoidance approaches.

```bash
./scripts/evaluation/run_fp_avoidance.sh
```

## Bug Detection Capability (Table IV)
The following script will build and run the different versions of `php` and `imagemagick` for each bugs with CMASan and ASan, respectively. the result will be located at `unknown_bug_detection/results`

```bash
./scripts/evaluation/run_detection_capability.sh
```




