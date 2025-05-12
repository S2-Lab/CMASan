#ifndef DISABLE_CMASAN
#define MAPATH 1000

#ifndef ASAN_utils
#define ASAN_utils

// #define EXPERIMENT_COVERAGE_INCREMENT //! if you want to enable log, uncomment this macro
// #define CMASAN_LOG_CHECK

#include "sanitizer_common/sanitizer_atomic.h"
#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_libc.h"
#include "sanitizer_common/sanitizer_symbolizer.h"
#include "sanitizer_common/sanitizer_file.h"

#define CMASAN_DEBUG_LEVEL 0
#define CMASAN_LOG_LINE_SIZE 8192
#define CMASAN_LOG_MAX_CONFLICT_TRIAL 5
#define CMASAN_LOG_PID_FIELD_SIZE 64

#ifndef ASAN_SHADOW_GRANULARITY
#define ASAN_SHADOW_GRANULARITY SHADOW_GRANULARITY
#endif
#ifndef ASAN_SHADOW_OFFSET
#define ASAN_SHADOW_OFFSET SHADOW_OFFSET
#endif

using namespace __sanitizer;

#define tlt_topLayerSize 0x400000ULL
// 22 bits
#define tlt_secondLayerPageNumbers 0x800000ULL
#define tlt_secondLayerNumbers 1

static const char* CMA_LOG_DIR_VARNAME = "CMA_LOG_DIR";
static const char* CMA_LOG_CONFLICT_FNAME = "/cmasan_log_conflict";
static const char* CMA_LOG_FNAME_FORMAT = "/cma_p%d_%d.log";

typedef struct tlt_chunk_info {
  uptr start_addr;
  uptr chunk_size;
  u32 rz_size;
  u32 status;
  uptr cma_id;
  u32 alloc_stack_id;
  u32 alloc_thread_id;
  u32 free_stack_id;
  u32 free_thread_id;
  uptr cnt;
} tlt_chunk_info; // 56 bytes, 8 byte granularity

typedef struct tlt_page {
  tlt_chunk_info* chunk_info;
} tlt_page;

typedef struct second_layer {
  tlt_page pages[tlt_secondLayerPageNumbers];
} second_layer;

namespace __asan
{

// concat the file name to prefix directory
const char* strcat_filename_to_dir(const char* dir, const char* file_name);
#ifdef EXPERIMENT_COVERAGE_INCREMENT
#define DEFINE_INCREMENT_METHOD(fieldName) \
    void inc_##fieldName() { \
        ++fieldName; \
    }

class TltLogger
  {
  private:
    uptr cma_alloc_cnt;
    uptr cma_alloc_realloc_cnt;

    uptr cma_dealloc_cnt;
    uptr cma_dealloc_realloc_cnt;

    uptr quarantine_dealloc_cnt;
    uptr quarantine_realloc_cnt;

    uptr cma_check_cnt;
    uptr asan_check_cnt;

    uptr base_mmap_cnt;
    uptr base_asan_cnt;

    uptr cma_chunk_from_unknown_cnt;
    uptr cma_chunk_from_asan_cnt;
    uptr cma_chunk_from_mmap_cnt;

    uptr activated_instance_cnt;

    const char* cma_log_dir;

  public:
    TltLogger () :
      cma_alloc_cnt(0), cma_alloc_realloc_cnt(0), cma_dealloc_cnt(0), cma_dealloc_realloc_cnt(0),
      quarantine_dealloc_cnt(0), quarantine_realloc_cnt(0), cma_check_cnt(0),
      base_mmap_cnt(0), base_asan_cnt(0),
      cma_chunk_from_unknown_cnt(0), cma_chunk_from_asan_cnt(0), cma_chunk_from_mmap_cnt(0),
      activated_instance_cnt(0)
    {
      cma_log_dir = GetEnv(CMA_LOG_DIR_VARNAME);
      if (cma_log_dir == nullptr) cma_log_dir = GetPwd();
    }

    ~TltLogger () {
      write_back_logs();
    }

    DEFINE_INCREMENT_METHOD(cma_alloc_cnt)
    DEFINE_INCREMENT_METHOD(cma_alloc_realloc_cnt)
    DEFINE_INCREMENT_METHOD(cma_dealloc_cnt)
    DEFINE_INCREMENT_METHOD(cma_dealloc_realloc_cnt)
    DEFINE_INCREMENT_METHOD(quarantine_dealloc_cnt)
    DEFINE_INCREMENT_METHOD(quarantine_realloc_cnt)
    DEFINE_INCREMENT_METHOD(cma_check_cnt)
    DEFINE_INCREMENT_METHOD(asan_check_cnt)
    DEFINE_INCREMENT_METHOD(base_mmap_cnt)
    DEFINE_INCREMENT_METHOD(base_asan_cnt)
    DEFINE_INCREMENT_METHOD(cma_chunk_from_unknown_cnt)
    DEFINE_INCREMENT_METHOD(cma_chunk_from_asan_cnt)
    DEFINE_INCREMENT_METHOD(cma_chunk_from_mmap_cnt)
    DEFINE_INCREMENT_METHOD(activated_instance_cnt)
    
    
    void write_back_logs() {
      const char* cma_conflict_dir = strcat_filename_to_dir(cma_log_dir, CMA_LOG_CONFLICT_FNAME);

      fd_t fd;

      // Try to use unique filename with PID
      char log_file_name[CMASAN_LOG_PID_FIELD_SIZE+sizeof(CMA_LOG_FNAME_FORMAT)];
      const char* log_file_dir;
      for (int i=0; i<CMASAN_LOG_MAX_CONFLICT_TRIAL; i++) {
        internal_snprintf(log_file_name, CMASAN_LOG_PID_FIELD_SIZE+sizeof(CMA_LOG_FNAME_FORMAT), CMA_LOG_FNAME_FORMAT, internal_getpid(), i);
        log_file_dir = strcat_filename_to_dir(cma_log_dir, log_file_name);
        if (!FileExists(log_file_dir)) break;
      }
      // Failed to resolve conflict
      if (FileExists(log_file_dir)) {
        fd = OpenFile(cma_conflict_dir, WrOnly);
        char *conflict_buf = "1";
        WriteToFile(fd, conflict_buf, 1);
        CloseFile(fd);
        return;
      }

      const char* format = "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n";
      char buffer[CMASAN_LOG_LINE_SIZE];
      fd = OpenFile(log_file_dir, WrOnly);
      int len = internal_snprintf(buffer, CMASAN_LOG_LINE_SIZE, format,
        cma_alloc_cnt, cma_alloc_realloc_cnt, cma_dealloc_cnt, cma_dealloc_realloc_cnt, 
        quarantine_dealloc_cnt, quarantine_realloc_cnt, 
        cma_check_cnt, asan_check_cnt,
        base_mmap_cnt, base_asan_cnt,
        cma_chunk_from_unknown_cnt, cma_chunk_from_asan_cnt, cma_chunk_from_mmap_cnt,
        activated_instance_cnt
      );
      WriteToFile(fd, buffer, len);
      CloseFile(fd);
      
      cma_alloc_cnt = 0; cma_alloc_realloc_cnt = 0; cma_dealloc_cnt = 0; cma_dealloc_realloc_cnt = 0; 
      quarantine_dealloc_cnt = 0; quarantine_realloc_cnt = 0; 
      cma_check_cnt = 0; asan_check_cnt = 0;
      base_mmap_cnt = 0; base_asan_cnt = 0;
      cma_chunk_from_unknown_cnt = 0; cma_chunk_from_asan_cnt = 0; cma_chunk_from_mmap_cnt = 0;
      activated_instance_cnt = 0;
    }
  };

typedef struct CMASanMMapList {
  public:
    typedef struct CMASanMMapNode {
        uptr addr;
        uptr size;
        bool is_checked;
        struct CMASanMMapNode* next;
    } CMASanMMapNode;

    CMASanMMapNode* createCMASanMMapNode(uptr addr, uptr size) {
        CMASanMMapNode* newCMASanMMapNode = (CMASanMMapNode*) InternalAlloc(sizeof(CMASanMMapNode));
        if (newCMASanMMapNode) {
            newCMASanMMapNode->addr = addr;
            newCMASanMMapNode->size = size;
            newCMASanMMapNode->is_checked = false;
            newCMASanMMapNode->next = nullptr;
        }
        return newCMASanMMapNode;
    }

    void insertCMASanMMapNode(uptr addr, uptr size) {
        CMASanMMapNode* newCMASanMMapNode = createCMASanMMapNode(addr, size);
        if (newCMASanMMapNode) {
            newCMASanMMapNode->next = head;
            head = newCMASanMMapNode;
        }
    }

    void removeCMASanMMapNode(uptr addr) {
        CMASanMMapNode* current = head;
        CMASanMMapNode* prev = nullptr;

        // Traverse the list to find the CMASanMMapNode with the given address
        while (current != nullptr && current->addr != addr) {
            prev = current;
            current = current->next;
        }

        // If the CMASanMMapNode is found, remove it from the list
        if (current != nullptr) {
            if (prev != nullptr) {
                prev->next = current->next;
            } else {
                head = current->next;
            }
            InternalFree(current);
        }
    }

    // 0: not in mmap, 1: in mmap and checked now, 2: in mmap and checked already
    char isInMMapChunk(uptr addr) {
        CMASanMMapNode* current = head;
        while (current != nullptr) {
            if (addr >= current->addr && addr < current->addr + current->size) {
              if (!current->is_checked) {
                current->is_checked = true;
                return 1;
              } else {
                return 2;
              }
            }
            current = current->next;
        }
        return 0;
    }

  private:
    CMASanMMapNode* head;
} CMASanMMapList;
#endif
// | 22 bits | 23 bits | 3bits |
// 0000000000 0000000000 0000000000 0000000000 00000000 00000000 00000000

#define tlt_topLayerShift 26 // 22 remain?
#define tlt_22bit_upper_mask 0xFFFFFC000000 // 5*4+2=22 | 2+6*4=26
#define tlt_topLayerMask 0x3FFFFF // 2+5*4=22
#define tlt_secondLayerMask 0x7FFFFF // 3+5*4 = 23 (26 - tlt_info_granularity_shamt)
#define tlt_info_granularity_shamt 3
#define tlt_info_granularity 1 << tlt_info_granularity_shamt

enum tlt_allocation_status {
  ALLOCATED,
  DEALLOCATED,
  IN_REALLOC,
  NONE
};


// Logging functions
#ifdef EXPERIMENT_COVERAGE_INCREMENT
TltLogger* get_tlt_logger();
void tlt_write_back_logs();
extern "C" void CMASanLogCheck (uptr addr, bool is_write, uptr access_size, u32 exp);
#endif
// CMASan routines
extern "C" void CMASanPIEReferencePoint()  __attribute__((optimize("Og")));
void InitializeCMASan();
void CMASanOnDlOpen(void* handle, const char* filename);
void FinalizeCMASan();

// Environment variables
bool is_quarantine_zone_on();
bool is_munmap_flush_on();
bool is_report_always_off();
bool is_fp_avoidance_off();
uptr get_quarantine_size();
uptr get_quarantine_num();
uptr get_alloczone_n();
char get_cma_context_size();
extern "C" void __asan_log_cma_related_function();
uptr get_num_tlt_second_table();

// Helper methods
bool isCMAInCallStackSkipOneCMA(BufferedStackTrace *stack);
bool IsPoisonAccessFalsePositive(uptr addr, uptr pc, uptr bp);

// tlt API
void tlt_update (uptr key, uptr size, tlt_allocation_status status, tlt_allocation_status expected_status, u32 stack_id);
void tlt_release (uptr key, uptr size, tlt_allocation_status expected_status);
tlt_chunk_info* tlt_insert (uptr key, uptr chunk_size, u32 rz_size, uptr cma_id, u32 alloc_stack_id);
tlt_chunk_info* tlt_get (uptr key);

void tlt_lock_chunk_info();
void tlt_unlock_chunk_info();

bool isCMASanInited();

};


template <typename T>
struct CMASanSimpleMap {
private:
    struct MapEntry {
        uptr key;
        T* value;
        MapEntry* next;
    };

    MapEntry** map;
    uptr _capacity;
    uptr size;
    bool _initialized = false;

    // Improved hash function
    uptr hash(uptr key) {
        // Use a combination of bitwise XOR and bit shifting for better distribution
        key = (key >> 3) ^ (key << 11);
        if (key == 0 || _capacity == 0) return 0;
        return key % _capacity;
    }

public:
    void initialize_once(uptr capacity = 500) {
        if (_initialized) return;
        else _initialized = true;
        _capacity = capacity;
        size = 0;
        map = static_cast<MapEntry**>(InternalAlloc(_capacity * sizeof(MapEntry*)));
        if (map == nullptr) {
            // Handle allocation failure
            Printf("[CMASan] Warning: allocation of CMASanSimpleMap failed");
            CHECK(false);
        }
        for (uptr i = 0; i < _capacity; ++i) {
            map[i] = nullptr;
        }
    }

    void insert(uptr key, T* value) {
        uptr index = hash(key);
        MapEntry* entry = map[index];
        // Check if key already exists, update value if it does
        while (entry != nullptr) {
            if (entry->key == key) {
                entry->value = value;
                return;
            }
            entry = entry->next;
        }

        // Key doesn't exist, create a new entry
        entry = static_cast<MapEntry*>(InternalAlloc(sizeof(MapEntry)));
        entry->key = key;
        entry->value = value;
        entry->next = map[index];
        map[index] = entry;
        size++;
    }

    bool find(uptr key, T** value) {
        uptr index = hash(key);
        MapEntry* entry = map[index];

        while (entry != nullptr) {
            if (entry->key == key) {
                *value = entry->value;
                return true;
            }
            entry = entry->next;
        }

        return false;
    }
};
#endif
#endif
