#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_COUNT_WORDS 1024
#define MAX_LEN_WORD    256

typedef struct Ht_item {
    char* key;
    int value;
} Ht_item;

typedef struct HashTable {
    Ht_item** items;
    int size;
    int count;
} HashTable;

// Объявления функций
unsigned long hash_function(const char* str, int prime, int num_buckets);
int get_hash(const char* str, int num_buckets, int attempt);
HashTable* create_table(int size);
Ht_item* create_item(const char* key, int value);
void free_item(Ht_item* item);
void free_table(HashTable* table);
__uint8_t ht_insert(HashTable* table, const char* key, int value);
int ht_search(HashTable* table, const char* key);
void ht_delete(HashTable* table, const char* key);
void normalize_word(char* word);
__uint8_t count_words(HashTable* table, const char* filename);
void print_table(HashTable* table);

static Ht_item* DELETED_ITEM = &(Ht_item){NULL, 0};

unsigned long hash_function(const char* str, int prime, int num_buckets) {
    unsigned long hash = 0;
    const int str_len = strlen(str);
    for (int i = 0; i < str_len; i++) {
        hash += (unsigned long)pow(prime, str_len - (i+1)) * str[i];
        hash = hash % num_buckets;
    }
    return hash;
}

int get_hash(const char* str, int num_buckets, int attempt) {
    const int hash_a = hash_function(str, 151, num_buckets);
    const int hash_b = hash_function(str, 163, num_buckets);
    return (hash_a + (attempt * (hash_b + 1))) % num_buckets;
}

HashTable* create_table(int size) {
    HashTable* table = (HashTable*) malloc(sizeof(HashTable));
    if (table == NULL)
    {
        printf("Ошибка 'malloc()'\n");
        return NULL;
    }
    table->size = size;
    table->count = 0;
    table->items = (Ht_item**) calloc(table->size, sizeof(Ht_item*));
    if (table->items == NULL)
    {
        free(table);
        printf("Ошибка 'calloc()'\n");
        return NULL;
    }
    return table;
}

Ht_item* create_item(const char* key, int value) {
    Ht_item* item = (Ht_item*) malloc(sizeof(Ht_item));

    if (item == NULL)
        return NULL;

    item->key = (char*) malloc(strlen(key) + 1);

    if (item->key == NULL)
    {
        free(item);
        return NULL;
    }

    strcpy(item->key, key);
    item->value = value;
    return item;
}

void free_item(Ht_item* item) {
    free(item->key);
    free(item);
}

void free_table(HashTable* table) {
    for (int i = 0; i < table->size; i++) {
        Ht_item* item = table->items[i];
        if (item != NULL && item != DELETED_ITEM) {
            free_item(item);
        }
    }
    free(table->items);
    free(table);
}

__uint8_t ht_insert(HashTable* table, const char* key, int value) {
    Ht_item* item = create_item(key, value);

    if (item == NULL)
    {
        printf("Ошибка 'create_item()'\n");
        return 1;
    }

    int index = get_hash(item->key, table->size, 0);
    Ht_item* cur_item = table->items[index];
    int i = 1;
    while (cur_item != NULL && cur_item != DELETED_ITEM) {
        if (strcmp(cur_item->key, key) == 0) {
            cur_item->value = value;
            free_item(item);
            return 0;
        }
        index = get_hash(item->key, table->size, i);
        cur_item = table->items[index];
        i++;
    }
    table->items[index] = item;
    table->count++;
    return 0;
}

int ht_search(HashTable* table, const char* key) {
    int index = get_hash(key, table->size, 0);
    Ht_item* item = table->items[index];
    int i = 1;
    while (item != NULL) {
        if (item != DELETED_ITEM) {
            if (strcmp(item->key, key) == 0) {
                return item->value;
            }
        }
        index = get_hash(key, table->size, i);
        item = table->items[index];
        i++;
    }
    return -1;
}

void ht_delete(HashTable* table, const char* key) {
    int index = get_hash(key, table->size, 0);
    Ht_item* item = table->items[index];
    int i = 1;
    while (item != NULL) {
        if (item != DELETED_ITEM) {
            if (strcmp(item->key, key) == 0) {
                free_item(item);
                table->items[index] = DELETED_ITEM;
                table->count--;
                return;
            }
        }
        index = get_hash(key, table->size, i);
        item = table->items[index];
        i++;
    }
}

void normalize_word(char* word) {
    int len = strlen(word);
    
    // Удаление знаков препинания с начала слова
    while (len > 0 && ispunct(word[0])) {
        memmove(word, word + 1, len--);
    }
    
    // Удаление знаков препинания с конца слова
    while (len > 0 && ispunct(word[len - 1])) {
        word[--len] = '\0';
    }
    
    // Приведение оставшихся символов к нижнему регистру
    for (int i = 0; i < len; i++) {
        word[i] = tolower(word[i]);
    }
    word[len] = '\0';
}

__uint8_t count_words(HashTable* table, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть файл %s\n", filename);
        return 1;
    }

    char word[MAX_LEN_WORD];
    while (fscanf(file, "%255s", word) != EOF) {
        normalize_word(word);
        int count = ht_search(table, word);
        if (count == -1) {
            if (ht_insert(table, word, 1))
            {
                printf("Ошибка 'ht_insert()'\n");
                return 1;
            }
        } else {
            if (ht_insert(table, word, count + 1))
            {
                printf("Ошибка 'ht_insert()'\n");
                return 1;
            }
        }
    }

    fclose(file);
    return 0;
}

void print_table(HashTable* table) {
    for (int i = 0; i < table->size; i++) {
        Ht_item* item = table->items[i];
        if (item != NULL && item != DELETED_ITEM) {
            printf("%s: %d\n", item->key, item->value);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Использование: %s <filename>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];

    HashTable* table = create_table(MAX_COUNT_WORDS);

    if (table == NULL)
    {
        printf("Ошибка 'create_table()'\n");
        goto END;    
    }

    if(count_words(table, filename))
    {
        printf("Ошибка 'count_words()'\n");
        goto END;
    }

    print_table(table);

END:
    free_table(table);

    return 0;
}
