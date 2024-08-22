#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <dirent.h>
#include <pthread.h>
#include <stdatomic.h>
#include <threads.h>

#define FILES_LIST_INIT_SZ 8
#define HASH_MAP_INIT_SZ 1024
#define URLS_STAT_SZ 10
#define REFS_STAT_SZ 10
#define URL_DEF_SZ 256
#define REF_DEF_SZ 256

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef uint32_t hash_val;

uint8_t symbols[256] = {
    0x49, 0x0a, 0x6c, 0xb4, 0x38, 0xac, 0x59, 0x2c, 0x47, 0xb9, 0xd4, 0x3d, 0x2b, 0x14, 0x33, 0x35, 0x1e, 0x4b, 0x4c,
    0x80, 0x8a, 0xb7, 0xd6, 0x15, 0xde, 0xe9, 0x6b, 0xf6, 0x0b, 0xec, 0x74, 0x06, 0x42, 0x4d, 0x63, 0x7d, 0xad, 0x65,
    0x50, 0xb8, 0xe6, 0x8e, 0x54, 0x8c, 0x03, 0x04, 0x27, 0x31, 0x46, 0xa0, 0x71, 0x75, 0xa5, 0xd5, 0x3f, 0x28, 0xd9,
    0xe3, 0x0c, 0xf5, 0x69, 0x29, 0x89, 0xba, 0x3e, 0xb2, 0xc6, 0x0f, 0xb6, 0x16, 0xfb, 0xfc, 0xfe, 0x13, 0x2a, 0x39,
    0x61, 0x6e, 0x78, 0x86, 0x92, 0xa2, 0x10, 0x34, 0x1d, 0xa9, 0x7b, 0x5b, 0xaf, 0xbb, 0xc9, 0x62, 0xc2, 0x1c, 0x36,
    0x09, 0x6d, 0x88, 0x91, 0xc3, 0xdc, 0xf1, 0xf4, 0xab, 0xd0, 0x82, 0xdd, 0x0d, 0x66, 0xfd, 0x57, 0x18, 0x45, 0xc1,
    0xaa, 0x37, 0xda, 0x70, 0xe1, 0xe2, 0x56, 0x81, 0x53, 0xa4, 0xb3, 0x19, 0x85, 0x8d, 0xb5, 0x4a, 0x99, 0x25, 0xbf,
    0xc8, 0x87, 0xca, 0x58, 0xd1, 0x21, 0x26, 0x3a, 0xd7, 0x4e, 0x76, 0xdb, 0xcc, 0xa3, 0xed, 0xee, 0xef, 0xdf, 0x7f,
    0xf3, 0x20, 0x40, 0x44, 0x7c, 0x9c, 0xae, 0x68, 0xb0, 0xa1, 0x07, 0x0e, 0x5a, 0x51, 0x9d, 0xbc, 0xc5, 0xcf, 0x24,
    0x67, 0x77, 0x97, 0xa7, 0xbe, 0xd2, 0xd3, 0xe4, 0xe7, 0x98, 0x2d, 0xe8, 0x3b, 0x05, 0x3c, 0x41, 0x43, 0x48, 0x79,
    0x83, 0x90, 0x9b, 0x9e, 0x6a, 0xbd, 0xf2, 0xd8, 0x2e, 0x1b, 0x1a, 0x00, 0x52, 0x64, 0x73, 0x7a, 0x7e, 0x9f, 0x23,
    0xb1, 0x30, 0x12, 0x5e, 0x22, 0xc0, 0x2f, 0x17, 0x5d, 0xc4, 0x11, 0x55, 0xcd, 0xce, 0x72, 0xea, 0xe5, 0xeb, 0xf0,
    0xa8, 0x1f, 0x60, 0x84, 0x94, 0x96, 0xcb, 0x9a, 0xe0, 0x32, 0x5c, 0x8b, 0xf7, 0x02, 0xf8, 0xf9, 0xfa, 0x8f, 0x08,
    0x6f, 0x01, 0x93, 0x95, 0x4f, 0x5f, 0xa6, 0xff, 0xc7};

hash_val pirsons_hash(const char* key, size_t key_len) {
    assert(key);
    assert(key_len > 0);

    hash_val hash_chunk = 0, full_hash = 0;
    for (size_t i = 0; i < sizeof full_hash; ++i) {
        hash_chunk = symbols[(key[0] + i) % 256];

        for (size_t key_idx = 0; key_idx < key_len; ++key_idx) {
            hash_chunk = symbols[(hash_chunk ^ key[key_idx]) % 256];
        }
        full_hash |= hash_chunk << i * 8;
    }
    return full_hash;
}

hash_val pirsons_hash_val(const char* key, size_t key_len, size_t count, size_t attempt_idx) {
    const hash_val hash = pirsons_hash(key, key_len);
    return (hash % count + attempt_idx * (hash % (count - 1) + 1)) % count;
}

bool are_strings_equal(const char* key1, size_t key1_len, const char* key2, size_t key2_len) {
    return key1_len == key2_len && 0 == strncmp(key1, key2, key1_len);
}

typedef struct {
    char* key;
    size_t key_len;
    int val;
} node;

typedef struct {
    node* nodes;
    size_t capacity;
    size_t size;
    hash_val (*hash_func)(const char*, size_t, size_t, size_t);
    bool (*keys_equality_func)(const char*, size_t, const char*, size_t);
} hash_map;

hash_map* create_hash_map(size_t capacity) {
    hash_map* m = (hash_map*)malloc(sizeof(hash_map));
    if (NULL == m) {
        perror("unable to allocate hash-map");
        return NULL;
    }
    memset(m, 0, sizeof(hash_map));
    m->nodes = (node*)calloc(capacity, sizeof(node));

    if (NULL == m->nodes) {
        perror("unable to allocate hash-map storage");
        free(m);
        return NULL;
    }
    m->capacity = capacity;
    m->hash_func = &pirsons_hash_val;
    m->keys_equality_func = &are_strings_equal;
    return m;
}

void remove_hash_map(hash_map** map) {
    for (size_t i = 0; i < (*map)->capacity; ++i) {
        node* n = (*map)->nodes + i;
        free(n->key);
    }
    free((*map)->nodes);
    free(*map);
    *map = NULL;
}

node* hash_map_get_node(hash_map* m, const char* key, size_t key_len) {
    size_t attempt_idx = 0;
    node* n = NULL;
    do {
        n = m->nodes + m->hash_func(key, key_len, m->capacity - 1, attempt_idx);
        assert(n);
        ++attempt_idx;
    } while (NULL != n->key && !m->keys_equality_func(n->key, n->key_len, key, key_len));
    return NULL == n->key ? NULL : n;
}

void hash_map_rehash(hash_map*, size_t);

node* hash_map_insert(hash_map* m, const char* key, size_t key_len) {
    size_t attempt_idx = 0;
    node *n = NULL, *candidate = NULL;
    do {
        candidate = m->nodes + m->hash_func(key, key_len, m->capacity - 1, attempt_idx);
        assert(candidate);
        if (NULL == candidate->key) {
            if (m->size < (size_t)((float)m->capacity * 0.33f)) {
                char* new_key = (char*)malloc(key_len + 1);

                if (NULL == new_key) {
                    perror("unable to allocate hash-map node");
                    return NULL;
                }
                strncpy(new_key, key, key_len);
                new_key[key_len] = '\0';

                candidate->key = new_key;
                candidate->key_len = key_len;
                ++m->size;
            } else {
                hash_map_rehash(m, m->capacity << 1);
                candidate = hash_map_insert(m, key, key_len);
            }
            n = candidate;
        } else if (m->keys_equality_func(candidate->key, candidate->key_len, key, key_len)) {
            n = candidate;
        } else {
            ++attempt_idx;

            if (attempt_idx > m->capacity) {
                perror("unable to get a free hash-map node");
                fprintf(stderr, "attempts: %lu\n", attempt_idx);
                return NULL;
            }
        }
    } while (NULL == n);
    return n;
}

void swap(size_t* s1, size_t* s2) {
    size_t tmp = *s1;
    *s1 = *s2;
    *s2 = tmp;
}

void hash_map_swap_data(hash_map* m1, hash_map* m2) {
    node* tmp = m1->nodes;
    m1->nodes = m2->nodes;
    m2->nodes = tmp;
    swap(&m1->capacity, &m2->capacity);
    swap(&m1->size, &m2->size);
}

void hash_map_rehash(hash_map* m, size_t new_capacity) {
    assert(new_capacity > m->capacity);

    hash_map* new_map = create_hash_map(new_capacity);
    node* n = NULL;
    for (size_t i = 0; i < m->capacity; ++i) {
        n = m->nodes + i;
        if (NULL != n->key) {
            node* inserted = hash_map_insert(new_map, n->key, n->key_len);
            assert(NULL != inserted);
            inserted->val = n->val;
        }
    }
    hash_map_swap_data(m, new_map);
    remove_hash_map(&new_map);
}

void hash_map_add(hash_map* dst, hash_map* src) {
    node* n = NULL;
    for (size_t i = 0; i < src->capacity; ++i) {
        n = src->nodes + i;

        if (NULL != n->key) {
            node* added = hash_map_insert(dst, n->key, n->key_len);
            assert(NULL != added);
            added->val += n->val;
        }
    }
}

int nodes_desc_cmp(const void* p1, const void* p2) {
    node* n1 = *(node**)p1;
    node* n2 = *(node**)p2;

    if (n1->val < n2->val)
        return 1;
    if (n1->val > n2->val)
        return -1;

    return 0;
}

typedef node** nodes_arr;
void create_nodes_sorted_arr(hash_map* m, nodes_arr* arr, size_t* arr_sz) {
    node** node_ptr_arr = malloc(m->size * sizeof(node*));
    size_t arr_idx = 0;
    for (size_t i = 0; i < m->capacity; ++i) {
        node* n = m->nodes + i;
        if (NULL != n->key) {
            node_ptr_arr[arr_idx++] = n;
        }
    }
    qsort(node_ptr_arr, m->size, sizeof(node*), nodes_desc_cmp);
    *arr = node_ptr_arr;
    *arr_sz = m->size;
}

typedef struct {
    char* url;
    size_t url_sz;
    char* referer;
    size_t referer_sz;
    size_t bytes;
} http_record;

void http_record_remove(http_record** record) {
    if (NULL == record || NULL == *record)
        return;
    free((*record)->url);
    free((*record)->referer);
    free(*record);
    *record = NULL;
}

http_record* http_rec_create(void) {
    http_record* rec = malloc(sizeof(http_record));
    assert(rec);

    rec->url = malloc(URL_DEF_SZ);
    assert(rec->url);
    memset(rec->url, 0, URL_DEF_SZ);
    rec->url_sz = URL_DEF_SZ - 1;

    rec->referer = malloc(REF_DEF_SZ);
    assert(rec->referer);
    memset(rec->referer, 0, REF_DEF_SZ);
    rec->referer_sz = REF_DEF_SZ - 1;

    rec->bytes = 0;
    return rec;
}

int http_record_get(char* line, http_record** out_rec) {
    if (NULL == *out_rec) {
        *out_rec = http_rec_create();
        assert(*out_rec);
    }
    http_record* rec = *out_rec;
    char* url_begin = strchr(line, '"');
    if (url_begin == NULL) return 1;

    char* url_val_begin = strchr(url_begin, '/');
    if (url_val_begin == NULL) return 1;

    char* url_val_end = strchr(url_val_begin, ' ');
    if (url_val_end == NULL) return 1;

    size_t url_val_sz = url_val_end - url_val_begin;

    if (url_val_sz > rec->url_sz) {
        rec->url = realloc(rec->url, url_val_sz + 1);
        assert(rec->url);
    }
    strncpy(rec->url, url_val_begin, url_val_sz);
    rec->url[url_val_sz] = '\0';
    rec->url_sz = url_val_sz;

    char* status_begin = strchr(url_val_end + 1, ' ');
    if (status_begin == NULL) return 1;

    char* end_ptr;
    char* size_begin = strchr(status_begin + 1, ' ');
    if (size_begin == NULL) return 1;

    rec->bytes = strtoul(size_begin + 1, &end_ptr, 10);

    char* referer_begin = strchr(size_begin + 1, '"');
    if (referer_begin == NULL) return 1;

    ++referer_begin;

    char* referer_end = strchr(referer_begin, '"');
    if (referer_end == NULL) return 1;

    size_t referer_val_sz = referer_end - referer_begin;
    if (0 == strncmp(referer_begin, "-", referer_val_sz)) {
        return 0;
    }

    if (referer_val_sz > rec->referer_sz) {
        rec->referer = realloc(rec->referer, referer_val_sz + 1);
        assert(rec->referer);
    }

    strncpy(rec->referer, referer_begin, referer_val_sz);
    rec->referer[referer_val_sz] = '\0';
    rec->referer_sz = referer_val_sz;
    *out_rec = rec;
    return 0;
}

typedef char** _Atomic atomic_iter;

char** files_list_create(char* dir_name, size_t* files_count) {
    DIR* dp = opendir(dir_name);
    if (NULL == dp) {
        perror(dir_name);
        exit(EXIT_FAILURE);
    }
    size_t files_list_sz = FILES_LIST_INIT_SZ;
    char** res = malloc((files_list_sz + 1) * sizeof(char*));
    assert(res);
    for (size_t i = 0; i < files_list_sz + 1; ++i) res[i] = NULL;
    size_t f_idx = 0;
    size_t entry_pos;
    struct dirent* entry;
    size_t file_name_len;
    size_t dir_name_len = strlen(dir_name);
    size_t entry_name_len;
    while (NULL != (entry = readdir(dp))) {
        if (DT_REG == entry->d_type) {
            entry_name_len = strlen(entry->d_name);
            file_name_len = dir_name_len + 1 + entry_name_len + 1;
            res[f_idx] = malloc(file_name_len);
            assert(res[f_idx]);

            entry_pos = 0;
            strncpy(res[f_idx] + entry_pos, dir_name, dir_name_len + 1);
            entry_pos += dir_name_len;

            res[f_idx][entry_pos] = '/';
            entry_pos += 1;

            strncpy(res[f_idx] + entry_pos, entry->d_name, entry_name_len + 1);
            entry_pos += entry_name_len;
            res[f_idx][entry_pos] = '\0';

            ++f_idx;

            if (f_idx >= files_list_sz) {
                files_list_sz *= 2;
                res = realloc(res, (files_list_sz + 1) * sizeof(char*));
                assert(res);
                for (size_t i = f_idx; i < files_list_sz + 1; ++i) res[i] = NULL;
            }
        }
    }
    closedir(dp);
    res[f_idx] = NULL;
    *files_count = f_idx;
    return res;
}

void files_list_remove(atomic_iter files_list) {
    char** iter = files_list;
    char* file;
    while ((file = *(iter++))) {
        free(file);
    }
    free(files_list);
}

typedef struct {
    hash_map* url;
    hash_map* referer;
    atomic_iter* files_iter;
} worker_param;

worker_param* worker_param_create(atomic_iter* files_iter) {
    worker_param* wp = (worker_param*)malloc(sizeof(worker_param));
    wp->url = create_hash_map(HASH_MAP_INIT_SZ);
    wp->referer = create_hash_map(HASH_MAP_INIT_SZ);
    wp->files_iter = files_iter;
    return wp;
}

void worker_param_remove(worker_param** wp) {
    remove_hash_map(&(*wp)->url);
    remove_hash_map(&(*wp)->referer);
    (*wp)->files_iter = NULL;
    free(*wp);
    wp = NULL;
}

void file_data_read(char* file_name, hash_map* url, hash_map* referer) {
    FILE* fd = fopen(file_name, "r");
    if (NULL == fd) {
        perror(file_name);
        exit(EXIT_FAILURE);
    }

    size_t len;
    char* line = NULL;
    http_record* record = NULL;
    while (-1 != getline(&line, &len, fd)) {
        if (1 == http_record_get(line, &record) || NULL == record)
            continue;

        if (record->url_sz > 0) {
            node* url_n = hash_map_insert(url, record->url, record->url_sz);
            assert(url_n);
            url_n->val += (int)record->bytes;
        }

        if (record->referer_sz > 0) {
            node* ref_n = hash_map_insert(referer, record->referer, record->referer_sz);
            assert(ref_n);
            ref_n->val += 1;
        }
    }
    http_record_remove(&record);
    free(line);
    fclose(fd);
}

int worker_func(void* data) {
    worker_param* wp = (worker_param*)data;
    char** files_iter;
    do {
        files_iter = atomic_load_explicit(wp->files_iter, memory_order_acquire);
        while (*files_iter && !atomic_compare_exchange_weak_explicit(wp->files_iter, &files_iter, files_iter + 1,
                                                                     memory_order_release, memory_order_relaxed))
            ;

        if (NULL == *files_iter) {
            break;
        }
        file_data_read(*files_iter, wp->url, wp->referer);
    } while (true);

    thrd_exit(0);
}

void get_stat(char* dir_name, int num_threads) {
    size_t files_count = 0;
    char** files_list = files_list_create(dir_name, &files_count);
    if (NULL == files_list[0]) {
        perror("directory is empty\n");
        exit(EXIT_FAILURE);
    }
    atomic_iter files_iter = files_list;
    num_threads = MIN((size_t)num_threads, files_count);

    worker_param** params = malloc(sizeof(worker_param*) * num_threads);
    assert(params);
    for (int i = 0; i < num_threads; ++i) params[i] = NULL;

    pthread_t* workers = malloc(sizeof(pthread_t) * num_threads);
    assert(workers);
    memset(workers, 0, sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; ++i) {
        params[i] = worker_param_create(&files_iter);
        thrd_create(workers + i, worker_func, params[i]);
    }

    thrd_join(workers[0], NULL);
    hash_map* url_result = params[0]->url;
    hash_map* ref_result = params[0]->referer;

    for (int i = 1; i < num_threads; ++i) {
        thrd_join(workers[i], NULL);
        hash_map_add(url_result, params[i]->url);
        hash_map_add(ref_result, params[i]->referer);
    }
    printf("\nstatistics calculation...\n");
    nodes_arr arr;
    size_t sz;
    create_nodes_sorted_arr(url_result, &arr, &sz);
    uint64_t total_bytes = 0;
    for (size_t i = 0; i < sz; i++)   
        total_bytes += arr[i]->val;

    printf("Total bytes: %li\n", total_bytes);
    sz = MIN(sz, URLS_STAT_SZ);

    printf("\nMost traffic URLs:\n");
    for (size_t i = 0; i < sz; ++i) {
        printf("%lu) BYTES: [%d], URL: [%s]\n", i + 1, arr[i]->val, arr[i]->key);
    }
    free(arr);

    create_nodes_sorted_arr(ref_result, &arr, &sz);
    sz = MIN(sz, REFS_STAT_SZ);

    printf("\nMost used REFERERs:\n");
    for (size_t i = 0; i < sz; ++i) {
        printf("%lu) TIMES: [%d], REFERER: [%s]\n", i + 1, arr[i]->val, arr[i]->key);
    }
    free(arr);

    for (int i = 0; i < num_threads; ++i) {
        worker_param_remove(&params[i]);
    }
    free(params);
    free(workers);
    files_list_remove(files_list);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <num_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* dir_name = argv[1];
    int num_threads = atoi(argv[2]);
    if (num_threads <= 0) {
        fprintf(stderr, "Invalid number of threads: %d\n", num_threads);
        exit(EXIT_FAILURE);
    }

    get_stat(dir_name, num_threads);
    return 0;
}
