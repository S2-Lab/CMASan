# Environment variables
#### `CMA_LIST=<application>.json`
JSON file describing CMAs to instrument. Can be generated using xxxxx or a CMA identification script.

#### `RT_CMA_LIST=<application>.txt`  
File to store instrumented CMA addresses during compile time. Must exist (empty) before build.  

#### `CMASAN_QUARANTINE_SIZE_I=I`  
Sets quarantine zone size as 2^I bytes. Default: `18` (256KB).  

#### `CMASAN_QUARANTINE_SIZE_N=N`  
Maximum number of freed objects in the quarantine zone. Default: `8192`.  

#### `CMASAN_QUARANTINE_ZONE_OFF=1`  
Disables our instance-specific free-delaying quarantine zone.  

#### `CMASAN_EXTRACTOR_PATH=<path to extractor_pc.py>`  
Path to the dynamic CMA address extractor. Usually default in Docker.  

#### `CMASAN_ALLOCZONE_N=N`  
Max number of tracked objects in the alloc zone. Default: `10000`.  

#### `CMASAN_FP_AVOIDANCE_OFF=1`  
Disables false positive avoidance logic.

#### `RT_CMA_PC_LIST=<application>.cmasan.pc`  
Used internally for FP avoidance. Usually auto-generated and auto-set.  
