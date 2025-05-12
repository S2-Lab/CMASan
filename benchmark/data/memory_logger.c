#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>

#include <dlfcn.h>

// clang -shared -fPIC -o memory_logger.so ./memory_logger.c 
static const char *MEMORY_LOGGER_DIR = "<LOGGER>";
static const char *cma_log_dir = "<PLACEHOLDER>";

typedef int (*snprintf_func_t)(char *str, size_t size, const char *format, ...);
static snprintf_func_t real_snprintf;

typedef int (*execve_func_t)(const char *filename, char *const argv[], char *const envp[]);
static execve_func_t real_execve = NULL;

// Function to check if file exists
int __cmasan_file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Function to get RSS (example, depends on the platform and implementation)
unsigned long long __cmasan_get_peak_rss() {
    struct rusage usage; 
    if(getrusage(RUSAGE_SELF, &usage)) perror("Cannot use rusage");
    return usage.ru_maxrss << 10; // ru_maxrss is in kb.
}

void __cmasan_initialize_libc() {
    void *libc_handle;

    // Load the standard C library
    libc_handle = dlopen("libc.so.6", RTLD_LAZY);
    if (!libc_handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(1);
    }

    // Get the address of snprintf from libc
    if (!real_snprintf) {
        real_snprintf = (snprintf_func_t)dlsym(libc_handle, "snprintf");
    }

    if (!real_snprintf) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        dlclose(libc_handle);
        exit(1);
    }
}

void __cmasan_log_rss() {
    unsigned long long maximum_rss = __cmasan_get_peak_rss();

    // const char *cma_log_dir = getenv("CMA_LOG_DIR");
    // if (cma_log_dir == NULL) {
    //     return;
    // }

    __cmasan_initialize_libc();

    // Get maximum RSS
    char bin_path[4096];
    ssize_t bin_path_len = readlink("/proc/self/exe", bin_path, sizeof(bin_path) - 1);
    if (bin_path_len == -1) perror("Cannot read exe");
    bin_path[bin_path_len] = '\0'; // Null-terminate the path
    // Only log for the binary not in /usr/bin
    if (strstr(bin_path, "/usr/bin") != NULL) return;

    // Write back contents
    char memory_buffer[4352];
    int len = real_snprintf(memory_buffer, sizeof(memory_buffer), "%llu\n%s\n", maximum_rss, bin_path);

    // Try to use unique filename with PID
    char log_file_dir[4096];
    for (int i = 0; i < 5; i++) {
        real_snprintf(log_file_dir, sizeof(log_file_dir), "%s/memory_p%llu_%d.log", cma_log_dir, (unsigned long long)getpid(), i);
        if (!__cmasan_file_exists(log_file_dir)) break;
    }

    // Write back memory usage
    int fd = open(log_file_dir, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        write(fd, memory_buffer, len);
        close(fd);
    } else {
        perror("Failed to open log file");
    }
}

// Override execve to enroll RSS logger
int execve(const char *filename, char *const argv[], char *const envp[]) {
    if (!real_execve) {
        real_execve = (execve_func_t)dlsym(RTLD_NEXT, "execve");
        if (!real_execve) {
            fprintf(stderr, "Error in dlsym(RTLD_NEXT, \"execve\"): %s\n", dlerror());
            exit(1);
        }
    }

    // Load CMA_LOG_DIR of current process
    // const char *env_cma_log_dir = getenv("CMA_LOG_DIR");
    // if (!env_cma_log_dir) return real_execve(filename, argv, envp);

    // Calculate the number of environment variables
    int env_count;
    int ld_preload_index = -1; // To track if LD_PRELOAD is already present
    for (env_count = 0; envp[env_count] != NULL; env_count++) {
        if (strncmp(envp[env_count], "LD_PRELOAD=", 11) == 0) {
            ld_preload_index = env_count;
        }
    }

    // Memory allocation for new environment variables
    char **new_envp = malloc((env_count + 3) * sizeof(char*));
    for (int i = 0; i < env_count; i++) {
        new_envp[i] = envp[i]; 
    }
    
    // char *cma_log_dir_entry = malloc(strlen("CMA_LOG_DIR=") + strlen(env_cma_log_dir) + 1);
    // sprintf(cma_log_dir_entry, "CMA_LOG_DIR=%s", env_cma_log_dir);
    // new_envp[env_count] = cma_log_dir_entry;
    // env_count ++;

    if (ld_preload_index >= 0) {
        // If LD_PRELOAD exists, append to it
        char *existing_ld_preload = new_envp[ld_preload_index] + 11;  // Skip "LD_PRELOAD="
        char *new_ld_preload_entry = malloc(strlen("LD_PRELOAD=") + strlen(existing_ld_preload) + 1 + strlen(MEMORY_LOGGER_DIR) + 1);
        sprintf(new_ld_preload_entry, "LD_PRELOAD=%s:%s", existing_ld_preload, MEMORY_LOGGER_DIR);
        new_envp[ld_preload_index] = new_ld_preload_entry;
    } else {
        // No LD_PRELOAD in envp, so just add it
        char *new_ld_preload_entry = malloc(strlen("LD_PRELOAD=") + strlen(MEMORY_LOGGER_DIR) + 1);
        sprintf(new_ld_preload_entry, "LD_PRELOAD=%s", MEMORY_LOGGER_DIR);
        new_envp[env_count] = new_ld_preload_entry;
        env_count++;
    }

    new_envp[env_count] = NULL;

    // Call the real execve with the new environment variables
    int ret = real_execve(filename, argv, new_envp);
    
    for (int i = 0; i < env_count; i++) {
        if (new_envp[i] && (strstr(new_envp[i], "LD_PRELOAD"))){ // || strstr(new_envp[i], "CMA_LOG_DIR"))) {
            free(new_envp[i]);
        }
    }
    free(new_envp);

    return ret;    
}

/* The address of the real main is stored here for fake_main to access */
static int (*real_main) (int, char **, char **);

/* Fake main(), spliced in before the real call to main() in __libc_start_main */
static int fake_main(int argc, char **argv, char **envp)
{	
	/* Register abort(3) as an atexit(3) handler to be called at normal
	 * process termination */
    atexit(__cmasan_log_rss);
    
	/* Finally call the real main function */
	int a = real_main(argc, argv, envp);
	return a;
}

/* LD_PRELOAD override of __libc_start_main.
 *
 * The objective is to splice fake_main above to be executed instead of the
 * program main function. We cannot use LD_PRELOAD to override main directly as
 * LD_PRELOAD can only be used to override functions in dynamically linked
 * shared libraries whose addresses are determined via the Procedure
 * Linkage Table (PLT). However, main's location is not determined via the PLT,
 * but is statically linked to the executable entry routine at __start which
 * pushes main's address onto the stack, then invokes libc's startup routine,
 * which obtains main's address from the stack. 
 * 
 * Instead, we use LD_PRELOAD to override libc's startup routine,
 * __libc_start_main, which is normally responsible for calling main. We can't
 * just run our setup code *here* because the real __libc_start_main is
 * responsible for setting up the C runtime environment, so we can't rely on
 * standard library functions such as malloc(3) or atexit(3) being available
 * yet. 
 */
int __libc_start_main(int (*main) (int, char **, char **),
		      int argc, char **ubp_av, void (*init) (void),
		      void (*fini) (void), void (*rtld_fini) (void),
		      void (*stack_end))
{
	void *libc_handle, *sym;
	/* This type punning is unfortunately necessary in C99 as casting
	 * directly from void* to function pointers is left undefined in C99.
	 * Strictly speaking, the conversion via union is still undefined
	 * behaviour in C99 (C99 Section 6.2.6.1):
	 * 
	 *  "When a value is stored in a member of an object of union type, the
	 *  bytes of the object representation that do not correspond to that
	 *  member but do correspond to other members take unspecified values,
	 *  but the value of the union object shall not thereby become a trap
	 *  representation."
	 * 
	 * However, this conversion is valid in GCC, and dlsym() also in effect
	 * mandates these conversions to be valid in POSIX system C compilers.
	 * 
	 * C11 explicitly allows this conversion (C11 Section 6.5.2.3): 
	 *  
	 *  "If the member used to read the contents of a union object is not
	 *  the same as the member last used to store a value in the object, the
	 *  appropriate part of the object representation of the value is
	 *  reinterpreted as an object representation in the new type as
	 *  described in 6.2.6 (a process sometimes called ‘‘type punning’’).
	 *  This might be a trap representation.
	 * 
	 * Some compilers allow direct conversion between pointers to an object
	 * or void to a pointer to a function and vice versa. C11's annex “J.5.7
	 * Function pointer casts lists this as a common extension:
	 * 
	 *   "1 A pointer to an object or to void may be cast to a pointer to a
	 *   function, allowing data to be invoked as a function (6.5.4).
         * 
	 *   2 A pointer to a function may be cast to a pointer to an object or
	 *   to void, allowing a function to be inspected or modified (for
	 *   example, by a debugger) (6.5.4)."
	 */
	union {
		int (*fn) (int (*main) (int, char **, char **), int argc,
			   char **ubp_av, void (*init) (void),
			   void (*fini) (void), void (*rtld_fini) (void),
			   void (*stack_end));
		void *sym;
	} real_libc_start_main;


	/* Obtain handle to libc shared library. The object should already be
	 * resident in the programs memory space, hence we can attempt to open
	 * it without loading the shared object. If this fails, we are most
	 * likely dealing with another version of libc.so */
	libc_handle = dlopen("libc.so.6", RTLD_NOLOAD | RTLD_NOW);

	if (!libc_handle) {
		fprintf(stderr, "can't open handle to libc.so.6: %s\n",
			dlerror());
		/* We dare not use abort() here because it would run atexit(3)
		 * handlers and try to flush stdio. */
		_exit(EXIT_FAILURE);
	}
	
	/* Our LD_PRELOAD will overwrite the real __libc_start_main, so we have
	 * to look up the real one from libc and invoke it with a pointer to the
	 * fake main we'd like to run before the real main function. */
	sym = dlsym(libc_handle, "__libc_start_main");
	if (!sym) {
		fprintf(stderr, "can't find __libc_start_main():%s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}

	real_libc_start_main.sym = sym;
	real_main = main;
	
	/* Close our handle to dynamically loaded libc. Since the libc object
	 * was already loaded previously, this only decrements the reference
	 * count to the shared object. Hence, we can be confident that the
	 * symbol to the read __libc_start_main remains valid even after we
	 * close our handle. In order to strictly adhere to the API, we could
	 * defer closing the handle to our spliced-in fake main before it call
	 * the real main function. */
	if(dlclose(libc_handle)) {
		fprintf(stderr, "can't close handle to libc.so.6: %s\n",
			dlerror());
		_exit(EXIT_FAILURE);
	}

	/* Note that we swap fake_main in for main - fake_main should call
	 * real_main after its setup is done. */
	return real_libc_start_main.fn(fake_main, argc, ubp_av, init, fini,
				       rtld_fini, stack_end);
}