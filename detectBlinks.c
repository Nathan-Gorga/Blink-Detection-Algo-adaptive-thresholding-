#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>   
#include <limits.h>
#include <gsl/gsl_statistics.h>


#define SIGNAL_LENGTH 1000
#define REFRACTORY_PERIOD 0.2f //seconds
#define SAMPLE_FREQUENCY 250


static inline float rand_uniform() {
    return 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
}

// Fonction principale : simulation EEG avec clignements
void simulate_eeg_with_blinks(
    float duration,       // secondes
    int fs,               // fréquence d'échantillonnage
    float blink_rate,     // clignements par seconde
    float blink_amp,      // amplitude µV
    float noise_level,    // bruit µV
    float **t_out,        // pointeur vers vecteur temps
    float **eeg_out,      // pointeur vers signal EEG
    int *n_samples_out    // nombre d’échantillons
) {
    int n_samples = (int)(duration * fs);
    *n_samples_out = n_samples;

    // Allocation
    float *t = malloc(n_samples * sizeof(float));
    float *eeg = malloc(n_samples * sizeof(float));

    if (!t || !eeg) {
        fprintf(stderr, "Erreur allocation mémoire\n");
        exit(1);
    }

    // Vecteur temps et EEG de fond
    for (int i = 0; i < n_samples; i++) {
        t[i] = (float)i / fs;
        eeg[i] =
            30.0f * sinf(2.0f * M_PI * 10.0f * t[i]) +  // alpha
            15.0f * sinf(2.0f * M_PI * 6.0f * t[i]) +   // theta
            10.0f * sinf(2.0f * M_PI * 20.0f * t[i]) +  // beta
            noise_level * rand_uniform();               // bruit blanc
    }

    // Positions des clignements
    int n_blinks = (int)(duration * blink_rate);
    for (int b = 0; b < n_blinks; b++) {
        int pos = rand() % n_samples;
        int width = (int)(0.3f * fs); // 300 ms
        int half = width / 2;

        for (int j = -half; j < half; j++) {
            int idx = pos + j;
            if (idx >= 0 && idx < n_samples) {
                float win = (float)j;
                float blink = blink_amp * expf(-0.5f * powf(win / (0.1f * fs), 2.0f));
                eeg[idx] += blink;
            }
        }
    }

    *t_out = t;
    *eeg_out = eeg;
}


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

    srand((unsigned)time(NULL));

    float *t, *eeg;
    int n_samples;

    simulate_eeg_with_blinks(
        10.0f,    // durée
        256,      // fs
        0.25f,    // clignements / s
        150.0f,   // amplitude µV
        20.0f,    // bruit µV
        &t, &eeg, &n_samples
    );

    const float win_size = 3.5f;//seconds

    const float th_mult = 5.0f;

    // float eeg[SIGNAL_LENGTH] = {0.0f}; // TODO : GET OR SIMULATE THE SIGNAL

    int blink_indices[ (int) (SIGNAL_LENGTH / 10)] = {0}; 

    const int num_blinks = detectBlinksAdaptiveThresholding(eeg, n_samples, SAMPLE_FREQUENCY, win_size, blink_indices, th_mult);

    if(num_blinks < 0) return 1;

    printf("found %d blinks\n", num_blinks);

    for(int i = 0; i < num_blinks; i++){

        printf("%d ", blink_indices[i]);

    }printf("\n");

    free(t);
    free(eeg);

    return 0;
}




