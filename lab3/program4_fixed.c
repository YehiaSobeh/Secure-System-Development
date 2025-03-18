#include<stdio.h>
#include<stdlib.h>
#include<string.h>

char* getString() {
    char* ret = malloc(100 * sizeof(char)); // Heap allocation
    strcpy(ret, "Hello World!");
    return ret;
}

void program4() {
    char* str = getString();
    printf("String: %s\n", str);
    free(str); // Free allocated memory
}

int main() {
    program4();
}
