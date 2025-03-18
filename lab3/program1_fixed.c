#include<stdio.h>
#include<stdlib.h>

void program1(int N) {
    int *arr = malloc(N * sizeof(int)); // Correct allocation
    if(arr == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    for(int i = 0; i < N; i++) {
        arr[i] = i * i;
        printf("arr[%d] = %d\n", i, arr[i]);
    }
    
    free(arr); // Prevent memory leak
}

int main() {
    program1(4);
}
