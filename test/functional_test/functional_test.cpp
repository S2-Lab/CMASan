#include<stdio.h>
#include<map>

std::map<void*, int> size_map;

#define SIZE 10000000
#define OFFSET 3

void* pool;
char* base;

void Init() {
    base = new char[SIZE];
    pool = (void*) (base + OFFSET);
}

void Destroy() {
    delete[] base;
}

void access2(void* ptr) {
    ((int*)ptr)[0] = 1;
}

void access1(void* ptr) {
    access2(ptr);
}

void access(void* ptr) {
    access1(ptr);
}

size_t GetSize(void* ptr) {
    return size_map[ptr];
}

void Dealloc(void* ptr) {
    access(ptr);
    printf("[CMA] %p was freed\n", ptr);
}

void* AllocNN(size_t size) {
    void* ptr = pool;
    pool = (void*)((char*)pool + size);
    printf("[CMA] %p was allocated with size 0x%x\n", ptr, size);
    size_map.insert({ptr, size});
    return ptr;
}

void* Alloc(size_t size) {
    return AllocNN(size);
}

void* Calloc(size_t size, size_t num) {
    return AllocNN(size*num);
}

typedef struct a {
    int a;
    float b;
} sa;

typedef struct b {
    int a;
    float b;
    sa _sa;
} sb; 


sb* TypedAlloc(size_t num) {
    return (sb*)AllocNN(num*sizeof(sb));
}

void* Realloc(size_t size, void* ptr) {
    void* new_ptr = AllocNN(size);
    Dealloc(ptr);
    return new_ptr;
}

void ClearChunks() {
    pool = base;
}

int main() {
    bool run = true;
    int op;
    int ptr_n;
    void* chunks[20];
    size_t n = 0, size, num, loc;
    sb _sb = {1, 3.2, {1, 2.2}};
    Init();
    while (run) {
        printf("0.Alloc 1.TypedAlloc 2. Calloc 3.Realloc 4.Dealloc 5.Access 6.TypedAccess 7.GetSize 8.Clear 9.Exit: ");
        scanf("%d", &op);
        switch(op) {
            case 0:
                printf("size: ");
                scanf("%lu", &size);
                chunks[n++] = Alloc(size);
                printf("[User] allocate on %lu\n", n-1);
                break;
            case 1:
                printf("size: ");
                scanf("%lu", &size);
                chunks[n++] = (void*) TypedAlloc(size);
                printf("[User] allocate on %lu\n", n-1);
                break;
            case 2:
                printf("size: ");
                scanf("%lu", &size);
                printf("num: ");
                scanf("%lu", &num);
                chunks[n++] = Calloc(size, num);
                printf("[User] allocate on %lu\n", n-1);
                break;
            case 3:
                printf("size: ");
                scanf("%lu", &size);
                printf("ptr#: ");
                scanf("%d", &ptr_n);
                chunks[ptr_n] = Realloc(size, chunks[ptr_n]);
                printf("[User] allocate on %lu\n", ptr_n);
                break;
            case 4:
                printf("ptr#: ");
                scanf("%d", &ptr_n);
                Dealloc(chunks[ptr_n]);
                break;
            case 5:
                printf("ptr#: ");
                scanf("%d", &ptr_n);
                printf("loc: ");
                scanf("%lu", &loc);
                ((char*)chunks[ptr_n])[loc] = 1;
                break;
            case 6:
                printf("ptr#: ");
                scanf("%d", &ptr_n);
                printf("loc: ");
                scanf("%lu", &loc);
                ((sb*)chunks[ptr_n])[loc] = _sb;
                break;
            case 7:
                printf("ptr#: ");
                scanf("%d", &ptr_n);
                size = GetSize(chunks[ptr_n]);
                printf("size of chunk %d: %lu\n", ptr_n, size);
                break;
            case 8:
                printf("cleared all the chunks\n");
                ClearChunks();
                break;
            case 9:
                printf("exited\n");
                run = false;
                break;
        }
    }
    Destroy();
}
