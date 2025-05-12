#ifndef DISABLE_CMASAN
#include "asan_utils.h"

#include "asan_poisoning.h"
#include "asan_stack.h"
#include "asan_report.h"

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_symbolizer.h"
#include "sanitizer_common/sanitizer_file.h"
#include "sanitizer_common/sanitizer_stackdepot.h"

#include <unistd.h>
#include <dlfcn.h>
#include <link.h>

using namespace __sanitizer;


#define CMA_MAX_N 500
#define MAX_FNAME_LENGTH 255
#define UNIX_MAX_PATH_LENGTH 4096
#define CMASAN_MAX_CMD_LINE 8192
#define CMASAN_MAX_RT_SIZE 1 << 17
#define CMASAN_MINIMUM_CHUNK_SIZE 32 // min_chunk_sz(8) + min_rz_sz(16) + min_pad_sz(8) = 32

namespace __asan
{
  static const char* RT_CMA_LIST_LOCATION_VARNAME = "RT_CMA_LIST";
  static const char* RT_CMA_PC_LIST_LOCATION_VARNAME = "RT_CMA_PC_LIST";
  static const char* CMASAN_QUARANTINE_ZONE_VARNAME = "CMASAN_QUARANTINE_ZONE_OFF";
  static const char* CMASAN_QUARANTINE_SIZE_I_VARNAME = "CMASAN_QUARANTINE_SIZE_I";
  static const char* CMASAN_QUARANTINE_SIZE_N_VARNAME = "CMASAN_QUARANTINE_SIZE_N";
  static const char* CMASAN_EXTRACTOR_PATH_VARNAME = "CMASAN_EXTRACTOR_PATH";
  static const char* CMASAN_LOCK_OFF_VARNAME = "CMASAN_LOCK_OFF";
  static const char* CMASAN_MUNMAP_FLUSH_ON_VARNAME = "CMASAN_MUNMAP_FLUSH_ON";
  static const char* CMASAN_ALLOCZONE_N_VARNAME = "CMASAN_ALLOCZONE_N";
  static const char* CMASAN_REPORT_ALWAYS_OFF_VARNAME = "CMASAN_REPORT_ALWAYS_OFF";
  static const char* CMASAN_FP_AVOIDANCE_OFF_VARNAME = "CMASAN_FP_AVOIDANCE_OFF";

  static bool _is_quarantine_zone_on = true;
  static uptr _quarantine_size = 1 << 18; // default: 256KB
  static uptr _quarantine_num = 8192; // default: 8192
  static bool _is_lock_off = false;
  static bool _is_munmap_flush_on = false;
  static bool _is_report_always_off = false;
  static uptr _alloczone_n = 10000;
  static bool _is_fp_avoidance_off = false;

  static uptr cma_pc_range[CMA_MAX_N][2];
  static uptr whitelist_pc_range[CMA_MAX_N][2];
  static int cma_count = 0;
  static int whitelist_count = 0;
  static uptr base_address = 0;
  static uptr _cmasan_max_stacktrace_size = 10;

  static tlt_page **TopLayerTable;
  static second_layer* SecondLayerTable;
  static uptr tlt_numSecondTable = 0;
#ifdef EXPERIMENT_COVERAGE_INCREMENT
  TltLogger tlt_logger;
#endif
  StaticSpinMutex _tlt_chunk_info_mutex;

  static bool cmasanInited = false;
  //* ------------------------------ *//
  //* Non tlt function (maybe users) *//
  //* ------------------------------ *//
  uptr get_num_tlt_second_table() {
    return tlt_numSecondTable;
  }
  const char* strcat_filename_to_dir(const char* dir, const char* file_name) {
    int log_dir_len = internal_strlen(dir);
    int file_name_len = internal_strlen(file_name);

    char* file_dir = (char*) InternalAlloc(log_dir_len+file_name_len+1);
    internal_strlcpy(file_dir, dir, log_dir_len+1);
    internal_strlcat(file_dir, file_name, log_dir_len+file_name_len+1);

    return file_dir;
  }

  void cachePCBySubprocess(char **buffer) {
    *buffer = (char*) InternalAlloc(CMASAN_MAX_RT_SIZE);
    const char* cmaListFileName = GetEnv(RT_CMA_LIST_LOCATION_VARNAME);
    if (cmaListFileName == nullptr) {
      Printf("Set Environment Variable %s\n", RT_CMA_LIST_LOCATION_VARNAME);
      cmaListFileName = "/home/qbit/cma.txt";
    }

    const char* cmasanExtractorPath = GetEnv(CMASAN_EXTRACTOR_PATH_VARNAME);
    if (cmasanExtractorPath == nullptr) {
      cmasanExtractorPath = "/root/CMASan/scripts/extract_pc.py";
    }

    // Parse full path of the binary itself
    char bin_path[UNIX_MAX_PATH_LENGTH];
    int bin_path_len = internal_readlink("/proc/self/exe", bin_path, UNIX_MAX_PATH_LENGTH-1);

    if (bin_path_len != -1) {
      bin_path[bin_path_len] = '\0';
    } else {
      Printf("Failed to retrieve the absolute path\n");
      return;
    }

    char script_command[CMASAN_MAX_CMD_LINE];
    int script_command_len = internal_snprintf(script_command, CMASAN_MAX_CMD_LINE,
      "/usr/bin/python3 %s %s %s", cmasanExtractorPath, bin_path, cmaListFileName);

    CHECK(script_command_len < CMASAN_MAX_CMD_LINE && "[CMASan] the script command length is too long");

    const char *parser_argv[] = {
      "/bin/sh",
      "-c",
      script_command,
      (char *) 0
    };
    char **parser_env = GetEnviron();

    // int pipe_fds[2];
    // if(pipe(pipe_fds)) {
    //   Printf("Failed to open pipes\n");
    // }

    pid_t pid = StartSubprocess(parser_argv[0], parser_argv, parser_env, /* stdin */ kInvalidFd, /* stdout */ kInvalidFd);

    if (pid == -1) {
      Printf("Could not open CMA Info parser process\n");
      return;
    }

    while(IsProcessRunning(pid));

    // internal_close(pipe_fds[0]);
    // internal_close(pipe_fds[1]);
  }

  void InitializeCMAInformation() {
    const char *dump;

    if (GetEnv(CMASAN_QUARANTINE_ZONE_VARNAME)) {
      _is_quarantine_zone_on = false;
    }

    if (GetEnv(CMASAN_LOCK_OFF_VARNAME)) {
      _is_lock_off = true;
    }

    if (GetEnv(CMASAN_MUNMAP_FLUSH_ON_VARNAME)) {
      _is_munmap_flush_on = true;
    }
    
    if (GetEnv(CMASAN_REPORT_ALWAYS_OFF_VARNAME)) {
      _is_report_always_off = true;
    }

    const char* quarantine_size_i_str = GetEnv(CMASAN_QUARANTINE_SIZE_I_VARNAME);
    if (quarantine_size_i_str) {
      uptr i = internal_simple_strtoll(quarantine_size_i_str, &dump, 10);
      _quarantine_size = 1 << i;
    }

    const char* quarantine_size_n_str = GetEnv(CMASAN_QUARANTINE_SIZE_N_VARNAME);
    if (quarantine_size_n_str) {
      _quarantine_num = internal_simple_strtoll(quarantine_size_n_str, &dump, 10);
    }


    const char* alloczone_size_str = GetEnv(CMASAN_ALLOCZONE_N_VARNAME);
    if (alloczone_size_str) {
      uptr n = internal_simple_strtoll(alloczone_size_str, &dump, 10);
      _alloczone_n = n;
    }
#ifdef EXPERIMENT_COVERAGE_INCREMENT
    if (GetEnv(CMASAN_FP_AVOIDANCE_OFF_VARNAME)) {
      _is_fp_avoidance_off = true;
    }
#endif
    char *buffer;
    uptr buffer_size;
    uptr len;

    /* Parsing runtime address range of CMA APIs */
    const char* runtime_cma_list_file_name = GetEnv(RT_CMA_PC_LIST_LOCATION_VARNAME);
    if (runtime_cma_list_file_name) { // Use user 
      if (!ReadFileToBuffer(runtime_cma_list_file_name, &buffer, &buffer_size, &len)){
        Printf("Unable to open RT_CMA_PC_LIST: %s\n", runtime_cma_list_file_name);
        return;
      }
    } else {
      char bin_path[UNIX_MAX_PATH_LENGTH];
      int bin_path_len = internal_readlink("/proc/self/exe", bin_path, UNIX_MAX_PATH_LENGTH-1);

      if (bin_path_len != -1) {
        bin_path[bin_path_len] = '\0';
      } else {
        Printf("Failed to retrieve the absolute path\n");
        return;
      }
      char cache_pc_path[UNIX_MAX_PATH_LENGTH];
      int cache_pc_path_len = internal_snprintf(cache_pc_path, UNIX_MAX_PATH_LENGTH, "%s.cmasan.pc", bin_path);

      // Generate cache
      if (!FileExists(cache_pc_path)) {
        cachePCBySubprocess(&buffer);
      }

      if (!ReadFileToBuffer(cache_pc_path, &buffer, &buffer_size, &len)){
        // Printf("Unable to open cached CMA PC ranges from: %s\n", cache_pc_path);
        return;
      }
    }

    if (!buffer || len == 0) {
      Printf("Runtime CMA LIST has no contents or error\n");
      return;
    }

    char *p = buffer;
    const char *arg0 = 0, *arg1 = 0, *arg2 = 0, *arg3 = 0;
    uptr line_len = 0;
    bool is_entry_complete = false;
    bool is_base_initialized = false;
    bool is_shared_lib_mode = false;
    while (*p != '\0') {  // will happen at the \0\0 that terminates the buffer
      if (*p == '-') {
        is_shared_lib_mode = true;
        p += 2;
        line_len = 0;
        continue;
      }
      if (*p == '\n') {
        if (!is_entry_complete) {
          Printf("RT_CMA_PC_LIST has an incorrect line\n");
          CHECK(false);
        }

        arg0 = p-line_len;
        *p = '\0'; // Set the end of second

        // Parse info
        uptr function_start = internal_simple_strtoll(arg0, &dump, 10);
        uptr function_end = internal_simple_strtoll(arg1, &dump, 10);
        const char *function_name = arg2;
        bool is_only_in_whitelist = *arg3 == '1';
        if (!is_shared_lib_mode) {
          if (is_base_initialized) { // Relocate addresses
            // Printf("%s (static): %llu %llu\n", function_name, function_start, function_end);
            whitelist_pc_range[whitelist_count][0] = base_address + function_start;
            whitelist_pc_range[whitelist_count][1] = base_address + function_end;
            whitelist_count++;
            if (!is_only_in_whitelist) {
              cma_pc_range[cma_count][0] = base_address + function_start;
              cma_pc_range[cma_count][1] = base_address + function_end;
              cma_count++;
            }
            CHECK((cma_count < CMA_MAX_N && whitelist_count < CMA_MAX_N) && "# of CMA/whitelist exceeded the limit");
          } else { // Derive base of .text from reference point
            // Printf("%s (ref): %llu %llu\n", function_name, function_start, function_end);
            uptr pie_reference_real = (uptr) CMASanPIEReferencePoint;
            uptr pie_reference_offset = function_start;
            base_address = pie_reference_real - pie_reference_offset;
            is_base_initialized = true;
          }
        } else { // dynamically resolve the addresses in shared libraries
          uptr len = function_end - function_start;
          void *sfunction_start = dlsym(RTLD_DEFAULT, function_name);
          if (sfunction_start) { // otherwise, it does not in the default loaded shared libs
            // Printf("%s (dynamic): %llu %llu -> %llu %llu\n", function_name, function_start, function_end, sfunction_start, sfunction_start+len);
            whitelist_pc_range[whitelist_count][0] = (uptr) sfunction_start;
            whitelist_pc_range[whitelist_count][1] = whitelist_pc_range[whitelist_count][0] + len;
            whitelist_count++;
            if (!is_only_in_whitelist) {
              cma_pc_range[cma_count][0] = (uptr) sfunction_start;
              cma_pc_range[cma_count][1] = cma_pc_range[cma_count][0] + len;
              cma_count++;
            }
            CHECK((cma_count < CMA_MAX_N && whitelist_count < CMA_MAX_N) && "# of CMA/whitelist exceeded the limit");
          }
        }

        is_entry_complete = false;
        arg1 = 0;
        arg2 = 0;
        line_len = 0;
        p++;
        continue;
      } else if (*p == ' ') {
        *p = '\0';
        if (arg1 == 0) {
          arg1 = p+1;
        } else if (arg2 == 0) {
          arg2 = p+1;
        } else {
          arg3 = p+1;
          is_entry_complete = true;
        }
      }
      line_len++;
      p++;
    }
  }

  void InitializeTLT() {
    uptr len = tlt_topLayerSize * sizeof (void*);
  #ifdef USE_MMAP_FOR_TLT
    TopLayerTable = (tlt_page **) MmapOrDie (len, __func__);
    SecondLayerTable = (second_layer *) MmapOrDie (sizeof(second_layer)*tlt_secondLayerNumbers, __func__);
  #else
    TopLayerTable = (tlt_page **) __asan::InternalAlloc (len);
    SecondLayerTable = (second_layer*) __asan::InternalAlloc (sizeof(second_layer)*tlt_secondLayerNumbers);
  #endif
    CHECK ((TopLayerTable != 0 && SecondLayerTable != 0)
          && "Memory space is not enough to save metadata");
  }
  
  /* Parsing runtime address range of CMA APIs for the dlopend shared libraries*/ 
  void CMASanOnDlOpen(void* handle, const char* filename) {
    if (handle == nullptr || filename == nullptr || dlerror()) return;
    // Find absolute path of the library
    struct link_map *lm;
    if(dlinfo(handle, RTLD_DI_LINKMAP, &lm) != 0) return; 

    if (dlerror()) return;
 
    char cache_pc_path[UNIX_MAX_PATH_LENGTH];
    int cache_pc_path_len = internal_snprintf(cache_pc_path, UNIX_MAX_PATH_LENGTH, "%s/%s.cmasan.pc", lm->l_name, filename);
    
    // Check the existence of the cache
    if (!FileExists(cache_pc_path)) {
      return;
    }

    char *buffer;
    uptr buffer_size;
    uptr len;
    
    if (!ReadFileToBuffer(cache_pc_path, &buffer, &buffer_size, &len)){
      // Printf("Unable to open cached CMA PC ranges from: %s\n", cache_pc_path);
      return;
    }


    if (!buffer || len == 0) {
      Printf("Runtime CMA LIST has no contents or error\n");
      return;
    }

    char *p = buffer;
    const char *arg0 = 0, *arg1 = 0, *arg2 = 0, *arg3 = 0;
    uptr line_len = 0;
    bool is_entry_complete = false;
    const char *dump;

    while (*p != '\0') {  // will happen at the \0\0 that terminates the buffer
      if (*p == '\n') {
        if (!is_entry_complete) {
          Printf("RT_CMA_PC_LIST has an incorrect line\n");
          CHECK(false);
        }

        arg0 = p-line_len;
        *p = '\0'; // Set the end of second

        // Parse info
        uptr function_start = internal_simple_strtoll(arg0, &dump, 10);
        uptr function_end = internal_simple_strtoll(arg1, &dump, 10);
        const char *function_name = arg2;
        bool is_only_in_whitelist = *arg3 == '1';

        uptr len = function_end - function_start;
        
        // dynamically resolve the addresses in shared libraries
        void *sfunction_start = dlsym(handle, function_name);
        if (sfunction_start) { // otherwise, it does not in the default loaded shared libs
          // Printf("%s (dynamic): %llu %llu -> %llu %llu\n", function_name, function_start, function_end, sfunction_start, sfunction_start+len);
          whitelist_pc_range[whitelist_count][0] = (uptr) sfunction_start;
          whitelist_pc_range[whitelist_count][1] = cma_pc_range[whitelist_count][0] + len;
          cma_count++;
          if (!is_only_in_whitelist) {
            cma_pc_range[cma_count][0] = (uptr) sfunction_start;
            cma_pc_range[cma_count][1] = cma_pc_range[cma_count][0] + len;
            cma_count++;
          }
          CHECK((cma_count < CMA_MAX_N && whitelist_count < CMA_MAX_N) && "# of CMA/whitelist exceeded the limit");
        }

        is_entry_complete = false;
        arg1 = 0;
        line_len = 0;
        p++;
        continue;
      } else if (*p == ' ') {
        *p = '\0';
        if (arg1 == 0) {
          arg1 = p+1;
        } else if (arg2 == 0){
          arg2 = p+1;
        } else {
          arg3 = p+1;
          is_entry_complete = true;
        }
      }
      line_len++;
      p++;
    }
  }

  void InitializeCMASan() {
    InitializeCMAInformation();
    InitializeTLT();
    CMASanPIEReferencePoint();
    cmasanInited = true;;
  }

  bool isCMASanInited() {
    return cmasanInited;
  }

  void FinalizeCMASan() {
#ifdef EXPERIMENT_COVERAGE_INCREMENT
    tlt_logger.write_back_logs();
#endif
  }

  extern "C" void  CMASanPIEReferencePoint() {
    return;
  }

  bool is_quarantine_zone_on() {
    return _is_quarantine_zone_on;
  }

  bool is_munmap_flush_on() {
    return _is_munmap_flush_on;
  }

  bool is_report_always_off() {
    return _is_report_always_off;
  }

  bool is_fp_avoidance_off() {
    return _is_fp_avoidance_off;
  }

  uptr get_quarantine_size() {
    return _quarantine_size;
  }

  uptr get_quarantine_num() {
    return _quarantine_num;
  }

  uptr get_alloczone_n() {
    return _alloczone_n;
  }

  extern "C" void __asan_log_cma_related_function() {
    Printf("CMA related function called!\n");
  }

  bool isCMAInCallStack(BufferedStackTrace *stack) {
#ifdef EXPERIMENT_COVERAGE_INCREMENT
    if(is_fp_avoidance_off()) return false;
#endif
    for (uptr i = 0; i < stack->size && stack->trace[i]; i++) {
      // PCs in stack traces are actually the return addresses, that is,
      // addresses of the next instructions after the call.
      uptr pc = stack->GetPreviousInstructionPc(stack->trace[i]);
      for (int k=0; k<whitelist_count; k++) { // Check if PC is in a CMA function
        if (whitelist_pc_range[k][0] <= pc && pc <= whitelist_pc_range[k][1]) {
          return true;
        }
      }
    }

    return false;
  }

  bool isCMAInCallStackSkipOneCMA(BufferedStackTrace *stack) {
#ifdef EXPERIMENT_COVERAGE_INCREMENT
    if(is_fp_avoidance_off()) return false;
#endif
    uptr self = 0;
    for (uptr i = 0; i < stack->size && stack->trace[i]; i++) {
      // PCs in stack traces are actually the return addresses, that is,
      // addresses of the next instructions after the call.
      uptr pc = stack->GetPreviousInstructionPc(stack->trace[i]);
      for (int k=0; k<cma_count; k++) { // Check if PC is in a CMA function    
        if (cma_pc_range[k][0] <= pc && pc <= cma_pc_range[k][1]) {
          if (self == 0) {
            self = cma_pc_range[k][0];
          } else {
            if (self != cma_pc_range[k][0]) return true;
          }
        }
      }
    }
    return false;
  }

  // Check UAF False positive and re-poison chunk as kASanHeapFreeMagic
  bool IsPoisonAccessFalsePositive(uptr addr, uptr pc, uptr bp) {
    if (addr == 0) return false;
  
    BufferedStackTrace stack;                        \
    stack.Unwind(pc, bp, nullptr, true); // fast unwind using frame pointer

    if (!isCMAInCallStack(&stack)) {
      return false;
    } else {
      return true;
    }
  }

#ifdef EXPERIMENT_COVERAGE_INCREMENT
  // [User] log access
  extern "C" void CMASanLogCheck (uptr addr, bool is_write, uptr access_size, u32 exp)
  {
    if (addr == 0) return; 
    if (tlt_get(addr)) get_tlt_logger()->inc_cma_check_cnt();
    else get_tlt_logger()->inc_asan_check_cnt();
  }

  TltLogger* get_tlt_logger() {
    return &tlt_logger;
  }
#endif

  //* --------------- *//
  //* Locking methods *//
  //* --------------- *//

  // [API] for synchronizing the accesses on tlt_chunk_info
  // TODO(junwha): consider fine-grained lock
  void tlt_lock_chunk_info() {
    if (!_is_lock_off) _tlt_chunk_info_mutex.Lock();
  }

  void tlt_unlock_chunk_info() {
    if (!_is_lock_off) _tlt_chunk_info_mutex.Unlock();
  }

  //* ----------------------------------------------------- *//
  //* Internal functions (user must sychronize page access) *//
  //* ----------------------------------------------------- *//

  // Get or create page on the address
  inline tlt_page * _tlt_get_or_create_page (uptr key)
  {
    CHECK ((TopLayerTable != 0)
          && "Top Layer Table was not initialized");
    uptr top_idx
      = (((uptr)key & tlt_22bit_upper_mask) >> tlt_topLayerShift)
          & tlt_topLayerMask;
    uptr second_idx
      = (((uptr)key) >> tlt_info_granularity_shamt) & tlt_secondLayerMask;

    if (!TopLayerTable[top_idx])
    {
      if (tlt_numSecondTable < tlt_secondLayerNumbers) {
              TopLayerTable[top_idx] = SecondLayerTable[tlt_numSecondTable].pages;
      } else {
            #ifdef USE_MMAP_FOR_TLT
              second_layer* layer = (second_layer*) MmapOrDie (sizeof(second_layer), __func__);
            #else
              second_layer* layer = (second_layer*) __asan::InternalAlloc (sizeof(second_layer));
            #endif
              CHECK(layer && "Memory Space is not enough to save metadata");
              TopLayerTable[top_idx] = layer->pages;
      }
      // Printf("Top Layer table is 0x%x\n", &TopLayerTable[top_idx]);
      // Printf("Create Second Layer table [%d] on 0x%x\n", tlt_numSecondTable, TopLayerTable[top_idx]);
      tlt_numSecondTable++;

      //CHECK (tlt_numSecondTable <= tlt_secondLayerNumbers &&
       // "Memory space is not enough to save metadata");
    }

    return (TopLayerTable[top_idx]) + second_idx;
  }

  // Get page for the key(address). it can be null
  inline tlt_page * _tlt_get_page_may_null (uptr key)
  {
    CHECK ((TopLayerTable != 0)
          && "Top Layer Table was not initialized");
    uptr top_idx
      = (((uptr)key & tlt_22bit_upper_mask) >> tlt_topLayerShift)
          & tlt_topLayerMask;
    uptr second_idx
      = (((uptr)key) >> tlt_info_granularity_shamt) & tlt_secondLayerMask;

    if (!TopLayerTable[top_idx]) return nullptr;

    return (TopLayerTable[top_idx]) + second_idx;
  }

  // Release cma chunk info from the page
  inline void _tlt_release_chunk_info(tlt_chunk_info *chunk_info)
  {
    chunk_info->cnt--;
    if (chunk_info->cnt == 0) __asan::InternalFree(chunk_info);
  }

  // [Internal] Get CMA chunk information for the key(address)
  // caller must check the nullity of the key
  inline tlt_chunk_info* _tlt_get_internal (uptr key)
  {
    // if (GetEnv("CMASAN_TLT_OFF") || GetEnv("CMASAN_SIZE_OFF")) return nullptr;
    tlt_page *AccessP = _tlt_get_page_may_null(key);
    if (AccessP == nullptr) return nullptr;
    if (AccessP->chunk_info && AccessP->chunk_info->status == NONE) return nullptr;
    return AccessP->chunk_info;
  }


  // [Internal] for the given target addr, release the chunk info along whole chunk
  inline uptr _tlt_release_whole_chunk(uptr target_addr, tlt_allocation_status expected_status) {
    tlt_chunk_info* info = _tlt_get_internal(target_addr);
    if (info) {
      info->status = NONE;
      uptr end_addr = info->start_addr + info->chunk_size + info->rz_size;
      return end_addr - tlt_info_granularity;
    }
    return target_addr;
  }

  //* ----------------------------------------------------- *//
  //* External API (user must sychronize chunk info access) *//
  //* ----------------------------------------------------- *//

  // [API] Get CMA chunk information for the key(address)
  tlt_chunk_info* tlt_get (uptr key)
  {
    if (key == 0) return nullptr;
    tlt_chunk_info* result = _tlt_get_internal(key);
    return result;
  }

  // [API] Update status and free_stack_id only when status is equal to exptected
  void tlt_update (uptr key, uptr size, tlt_allocation_status status, tlt_allocation_status expected_status, u32 free_stack_id)
  {
    if (key == 0) return;
    uptr aligned_key = RoundDownTo(key, ASAN_SHADOW_GRANULARITY);
    AsanThread *t = GetCurrentThread();
    for (uptr i=0; i<size; i+=8) {
      tlt_chunk_info* chunk_info = _tlt_get_internal(aligned_key+i);
      if (chunk_info && chunk_info->status == expected_status) {
        chunk_info->status = status;
        chunk_info->free_stack_id = free_stack_id;
        chunk_info->free_thread_id = t ? t->tid() : 0;
        uptr aligned_size = chunk_info->chunk_size+chunk_info->rz_size;
        i += RoundUpTo(aligned_size, ASAN_SHADOW_GRANULARITY);
      }
    }
  }

  // Release cma informations in the given memry address range.
  // If the status is not equal to expected status, log warning.
  void tlt_release (uptr key, uptr size, tlt_allocation_status expected_status)
  {
    if (key == 0) return;
    uptr aligned_key = RoundDownTo(key, ASAN_SHADOW_GRANULARITY);

    for (uptr i=0; i<size; i+=CMASAN_MINIMUM_CHUNK_SIZE) {
      i = _tlt_release_whole_chunk(aligned_key + i, expected_status);
    }
    // For the last reer bytes(< CMASAN_MINIMUM_CHUNK_SIZE), check each unit bytes
    for (uptr i=RoundDownTo(size, CMASAN_MINIMUM_CHUNK_SIZE); i<size; i+=tlt_info_granularity) {
      i = _tlt_release_whole_chunk(aligned_key + i, expected_status);
    }

  }

  // Insert new chunk information into two layer table
  // if the previous information is there and the status is not equal to expected status, it will return that chunk information (duplicated allocation detected)
  // otherwise, it will finish the insertion on the memory range, and return the null pointer.
  tlt_chunk_info* tlt_insert (uptr key, uptr chunk_size, u32 rz_size, uptr cma_id, u32 alloc_stack_id)
  {
    if (key == 0) return nullptr;
    // Chunk end must be aligned to shadow granularity
    CHECK(IsAligned(key + chunk_size + rz_size, ASAN_SHADOW_GRANULARITY));

    AsanThread *t = GetCurrentThread();

    tlt_chunk_info data = {key, chunk_size, rz_size, ALLOCATED, cma_id, alloc_stack_id, (t ? t->tid() : 0), 0, kInvalidTid, 0};

    tlt_chunk_info* chunk_info = (tlt_chunk_info*) __asan::InternalAlloc(sizeof(tlt_chunk_info)); // share one chunk
    internal_memcpy(chunk_info, &data, sizeof(tlt_chunk_info));
    uptr aligned_key = RoundDownTo(key, ASAN_SHADOW_GRANULARITY);
    uptr aligned_size = RoundUpTo(chunk_size+rz_size, ASAN_SHADOW_GRANULARITY);

    for (uptr offset=0; offset < aligned_size;) {
      tlt_page *page = _tlt_get_or_create_page(aligned_key + offset); // page of aligned_key or first page of the 2nd-level table
      uptr idx_in_page = ((aligned_key + offset) >> tlt_info_granularity_shamt)
                       & tlt_secondLayerMask; // calculate idx of current page in the 2nd-level table
      uptr remain_in_page = tlt_secondLayerPageNumbers - idx_in_page; // remaining pages in the current 2nd-level table
      uptr pages_to_fill = Min(remain_in_page,
                      (aligned_size - offset) >> 3);
                      
      for (uptr i= 0; i < pages_to_fill; ++i) {
        tlt_page *page_i = page + i;
        if (page_i->chunk_info) _tlt_release_chunk_info(page_i->chunk_info);
        page_i->chunk_info = chunk_info;
      }
      offset += pages_to_fill << 3;
      chunk_info->cnt += pages_to_fill;
    }
    return nullptr;
  }
#ifdef EXPERIMENT_COVERAGE_INCREMENT
  void tlt_write_back_logs() {
    get_tlt_logger()->write_back_logs();
  }
#endif

}
#endif
