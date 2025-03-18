#include<stdio.h>
#include<stdlib.h>
#include<string.h>

void* program3(unsigned N) {
    void *arr = malloc(N * sizeof(int)); // Correct allocation size
    if((N < 1) || (arr == NULL)) {       // Fixed comparison (==)
        printf("%s\n", "Memory allocation failed!");
        return NULL;
    }
    printf("%s\n", "Memory allocation success!");
    return arr;
}

int main() {
    int* arr = (int*)program3(4); 
    free(arr); // Now actually frees the memory
}
