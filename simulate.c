#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Fonction pour générer un flottant uniforme entre -1 et 1
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

// Exemple d’utilisation
int main() {
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

    // Afficher les 10 premiers points
    for (int i = 0; i < 100; i++) {
        printf("t=%.3f s, eeg=%.3f µV\n", t[i], eeg[i]);
    }

    free(t);
    free(eeg);
    return 0;
}
