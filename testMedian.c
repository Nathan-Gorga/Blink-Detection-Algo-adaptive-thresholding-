#include <stdio.h>
#include <stdlib.h>
#include <gsl/gsl_statistics_float.h>



static int cmp_float(const void *a, const void *b) {
    float fa = *(const float *)a;
    float fb = *(const float *)b;
    return (fa > fb) - (fa < fb); // returns +1, 0, or -1
}

// Median function using qsort
float median(float arr[], int n) {
    if (n == 0) return 0.0f;

    // Make a copy so original array isn't modified
    float *copy = malloc(n * sizeof(float));
    if (!copy) return 0.0f; // memory allocation failed
    for (int i = 0; i < n; i++) copy[i] = arr[i];

    qsort(copy, n, sizeof(float), cmp_float);

    float med;
    if (n % 2 == 1) {
        med = copy[n / 2];
    } else {
        med = 0.5f * (copy[n / 2 - 1] + copy[n / 2]);
    }

    free(copy);
    return med;
}




int main(void){

    float arr[9] = {8.0f, 1.0f, 4.0f, 3.0f, 2.0f, 5.0f, 6.0f, 2.0f, 8.0f}; 

    printf("median of arr[] is %f\n", median(arr, 9));

    return 0;
}