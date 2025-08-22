#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>   
#include <limits.h>

#define SIGNAL_LENGTH 1000
#define REFRACTORY_PERIOD 0.2f //seconds
#define SAMPLE_FREQUENCY 250


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



float robust_threshold(float * segment, const int segment_length,  const float th_mult){
    const float med = median(segment, segment_length );
}


int detectBlinksAdaptiveThresholding(float eeg[], const int signal_length, const int sample_freq, const float win_size, int * blink_indices, const float th_mult){

    const bool use_abs = true;
    
    float * signal = (float *)malloc(signal_length * sizeof(float));//FIXME : heap is slow

    const int n_samples = signal_length;

    const int win_len = (int) (win_size * sample_freq);

    const int refractory_samples = (int) (REFRACTORY_PERIOD * sample_freq);

    float * thresholds = (float *)malloc(( (int)( signal_length / win_len )) * sizeof(float)); // FIXME : heap is slow

    int last_index =  INT_MIN;


    float * segment = (float *)malloc(win_len * sizeof(float));

    if(use_abs){

        for(int i = 0; i < signal_length; i++){
            signal[i] = fabsf(eeg[i]);
        }

    } else {

        for(int i = 0; i < signal_length; i++){
            signal[i] = eeg[i];
        }

    }

    for(int i = 0; i < n_samples; i += win_len){
        
        const int end_of_window = min(i + win_len, n_samples);

        for(int j = i; j < end_of_window; j++){
            segment[j - i] = signal[j];
        }

        const float threshold = robust_threshold(segment,end_of_window - i , th_mult);
    }





    free(segment);
    free(thresholds);
    free(signal);

}



int main(void){

    const float win_size = 3.5f;//seconds
    const float th_mult = 5.0f;

    float eeg[SIGNAL_LENGTH] = {0.0f}; // TODO : GET OR SIMULATE THE SIGNAL

    int blink_indices[ (int) (SIGNAL_LENGTH / 10)] = {0}; 

    const int num_blinks = detectBlinksAdaptiveThresholding(eeg ,SIGNAL_LENGTH, SAMPLE_FREQUENCY, win_size, blink_indices, th_mult);


    if(num_blinks < 0) return 1;

    printf("found %d blinks\n", num_blinks);


    for(int i = 0; i < num_blinks; i++){
        printf("%d ", blink_indices[i]);
    }printf("\n");

    return 0;
}




