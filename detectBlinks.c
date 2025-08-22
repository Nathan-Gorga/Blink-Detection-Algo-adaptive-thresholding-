#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>   
#include <limits.h>
#include <gsl/gsl_statistics.h>


#define SIGNAL_LENGTH 1000
#define REFRACTORY_PERIOD 0.2f //seconds
#define SAMPLE_FREQUENCY 250


// static int cmp_float(const void *a, const void *b) {
//     float fa = *(const float *)a;
//     float fb = *(const float *)b;
//     return (fa > fb) - (fa < fb); // returns +1, 0, or -1
// }

// // Median function using qsort
// float median(float arr[], int n) {
//     if (n == 0) return 0.0f;

//     // Make a copy so original array isn't modified
//     float *copy = malloc(n * sizeof(float));
//     if (!copy) return 0.0f; // memory allocation failed
//     for (int i = 0; i < n; i++) copy[i] = arr[i];

//     qsort(copy, n, sizeof(float), cmp_float);

//     float med;
//     if (n % 2 == 1) {
//         med = copy[n / 2];
//     } else {
//         med = 0.5f * (copy[n / 2 - 1] + copy[n / 2]);
//     }

//     free(copy);
//     return med;
// }


int argmax(const float arr[], int n) {
    if (n <= 0) return -1; // invalid input

    int max_index = 0;
    float max_value = arr[0];

    for (int i = 1; i < n; i++) {
        if (arr[i] > max_value) {
            max_value = arr[i];
            max_index = i;
        }
    }
    return max_index;
}

int find_local_maxima(float segment[], const int start, const int end){
    
    const int n = end - start;

    float * window = (float *)malloc(n * sizeof(float));

    for(int i = 0; i < n; i++){
        window[i] = segment[start + i];
    }

    return start + argmax(window, n);
}


float robust_threshold(float * segment, const int segment_length,  const float th_mult){

    const float med = gsl_stats_median(segment, 1, segment_length);

    float * temp = (float *)malloc(segment_length * sizeof(float));

    for(int i = 0; i < segment_length; i++){
        temp[i] = segment[i] - med;
    }

    const float mad = 1.4826f * gsl_stats_median(fabsf(temp), 1, segment_length);

    free(temp);

    return med + th_mult * mad;
}


int detectBlinksAdaptiveThresholding(float eeg[], const int signal_length, const int sample_freq, const float win_size, int * blink_indices, const float th_mult){

    const bool use_abs = true;
    
    float * signal = (float *)malloc(signal_length * sizeof(float));//FIXME : heap is slow

    const int n_samples = signal_length;

    const int win_len = (int) (win_size * sample_freq);

    const int refractory_samples = (int) (REFRACTORY_PERIOD * sample_freq);

    // float * thresholds = (float *)malloc(( (int)( signal_length / win_len )) * sizeof(float)); // FIXME : heap is slow

    int last_index =  INT_MIN;

    int peaks_count = 0;

    int count = 0;

    float * peaks = (float *)malloc(signal_length * sizeof(float));

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

        const float threshold = robust_threshold(segment, end_of_window - i , th_mult);
        
        for(int j = i; j < end_of_window; j++){
            if(segment[j] > threshold){

                const int end = i + refractory_samples;

                const int peak_index = find_local_maxima(segment, i, end);

                peaks[peaks_count++] = peak_index;

                j += end;
            
            }
        }

        for(int p = 0; p < peaks_count; p++){
            
            if(peaks[p] > last_index){
            
                last_index = peaks[p] + refractory_samples;
            
                blink_indices[count++] = peaks[p];
            
            }
        }
    }


    free(peaks);
    free(segment);
    // free(thresholds);
    free(signal);

    return count;
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




