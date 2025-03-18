#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

// Returns the index within MAP_MAX bounds
int HashIndex(const char* key) {
    int sum = 0; // CWE-457: Initialize sum
    for (const char* c = key; *c != '\0'; c++) { // Fix loop condition
        sum += *c;
    }
    return sum % MAP_MAX; // CWE-125: Prevent OOB access
}

HashMap* HashInit() {
    HashMap* map = malloc(sizeof(HashMap));
    if (map == NULL) { // CWE-252: Check malloc return
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    memset(map->data, 0, sizeof(map->data)); // Initialize pointers
    return map;
}

void HashAdd(HashMap *map, PairValue *value) {
    int idx = HashIndex(value->KeyName);
    PairValue* existing = HashFind(map, value->KeyName);
    
    if (existing) { // CWE-768: Handle duplicates
        existing->ValueCount++;
        return;
    }
    
    // Prepend to the linked list
    value->Next = map->data[idx];
    map->data[idx] = value;
}

PairValue* HashFind(HashMap *map, const char* key) {
    unsigned idx = HashIndex(key);
    
    for (PairValue* val = map->data[idx]; val != NULL; val = val->Next) {
        if (strcmp(val->KeyName, key) == 0) { // CWE-480: Fix strcmp check
            return val;
        }
    }
    return NULL;
}

void HashDelete(HashMap *map, const char* key) {
    unsigned idx = HashIndex(key);
    PairValue *prev = NULL, *current = map->data[idx];

    while (current != NULL) {
        if (strcmp(current->KeyName, key) == 0) {
            if (prev) {
                prev->Next = current->Next;
            } else {
                map->data[idx] = current->Next;
            }
            return;
        }
        prev = current;
        current = current->Next;
    }
}

void HashDump(HashMap *map) {
    for (unsigned i = 0; i < MAP_MAX; i++) {
        for (PairValue* val = map->data[i]; val != NULL; val = val->Next) {
            printf("%s\n", val->KeyName); // CWE-134: Safe format string
        }
    }
}

int main() {
    HashMap* map = HashInit();
    printf("HashInit() Successful\n");
    
    PairValue pv1 = { .KeyName = "test_key", .ValueCount = 1, .Next = NULL };
    PairValue pv2 = { .KeyName = "other_key", .ValueCount = 1, .Next = NULL };
    
    printf("HashAdd(map, '%s')\n", pv1.KeyName);
    HashAdd(map, &pv1);

    printf("HashAdd(map, '%s')\n", pv1.KeyName);
    HashAdd(map, &pv1); // ValueCount becomes 2

    printf("HashAdd(map, '%s')\n", pv2.KeyName);
    HashAdd(map, &pv2);

    printf("HashFind(map, %s) = ", pv1.KeyName);
    PairValue* result = HashFind(map, pv1.KeyName);
    if (result) {
        printf("{'%s': %d}\n", result->KeyName, result->ValueCount);
    } else {
        printf("Not found\n");
    }
    
    printf("HashDump(map) = ");
    HashDump(map);

    printf("HashDelete(map, '%s')\n", pv1.KeyName);
    HashDelete(map, pv1.KeyName);

    printf("HashFind(map, %s) = ", pv1.KeyName);
    result = HashFind(map, pv1.KeyName);
    if (result) {
        printf("{'%s': %d}\n", result->KeyName, result->ValueCount);
    } else {
        printf("Not found\n");
    }

    printf("HashDump(map) = ");
    HashDump(map);

    free(map);
}
