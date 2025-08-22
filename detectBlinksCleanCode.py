import numpy as np
import matplotlib.pyplot as plt
from mysignal import simulate_eeg_with_blinks
from scipy.signal import butter, filtfilt, find_peaks
import time

# solution 1 : Band-limit + smooth before thresholding (blink band: ~0.1–8 Hz)
def lowpass(data, fs, fc=8.0, order=4):
    b, a = butter(order, fc/(fs/2), btype='low', analog=False)
    return filtfilt(b, a, data)

def preprocess_for_blinks(eeg: np.ndarray, fs: int, smoothing_factor = 4) -> np.ndarray:
    x = lowpass(eeg, fs, fc=8.0, order=4)
    # light moving-average smoothing (~40 ms)
    k = max(1, int(0.01 * smoothing_factor * fs))
    kernel = np.ones(k)/k
    return np.convolve(x, kernel, mode='same')



# solution 2 : Robust (MAD/quantile) thresholds instead of mean±k·std

def robust_threshold1(segment: np.ndarray, mult: float = 2) -> float:
    med = np.median(segment)
    mad = 1.4826 * np.median(np.abs(segment - med))  # ≈ robust σ
    return med + mult * mad





# solution 5 : Envelope / RMS Gating    

def moving_rms(x, fs, win=0.1):
    k = max(1, int(win*fs))
    c = np.convolve(x*x, np.ones(k)/k, mode='same')
    return np.sqrt(c)

def find_local_maxima(signal: np.ndarray, start: int, end: int) -> int:
    window = signal[start:end]
    return start + window.argmax()

def detect_blinks_adaptive(eeg, fs, win_size=2.0, th_mult=2, refractory=0.2, use_abs=True):
    """
    Detect blinks using adaptive thresholding.
 
    Args:
        eeg       : 1D EEG signal (single channel).
        fs        : Sampling frequency (Hz).
        win_size  : Window size in seconds for threshold estimation.
        th_mult   : Threshold multiplier (mean + th_mult*std).
        refractory: Minimum time (s) between detected blinks.
        use_abs   : Whether to use absolute amplitude (recommended).
 
    Returns:
        blink_indices (list of sample indices)
    """
    signal = np.abs(eeg) if use_abs else eeg

    n_samples = len(signal)

    win_len = int(win_size * fs)

    refractory_samples = int(refractory * fs)
 
    blink_indices = []

    thresholds = []

    last_idx = -np.inf
 
    for start in range(0, n_samples, win_len):
        end = min(start + win_len, n_samples)
        segment = signal[start:end]


        # Adaptive threshold
        threshold = robust_threshold1(segment, mult=th_mult)  #max(robust_threshold1(segment), np.percentile(segment, 90))
        thresholds.append(threshold) 
        



        

        if np.any(segment > threshold):
            # Find peaks above threshold in this segment



            # MAKE THE CODE BELOW FASTER, MAYBE CAN JUST SWEEP THROUGH AND TEST LOCAL MAXIMUMS
            # start = time.perf_counter()
            # peaks, _ = find_peaks(segment, height=threshold, distance=refractory_samples)

            peaks = []

            for i in range(len(segment)):
                if segment[i] > threshold:

                    end = int(i + refractory * fs)
                    p = find_local_maxima(segment, i, end)
                    peaks.append(p)
                    i += end



            # end = time.perf_counter()
            # print(f'condition and find peaks took {end - start} seconds') 

            # Map local peak indices → global signal indices
            for p in peaks:
                blink_idx = start + p
                # global refractory check
                if blink_idx - last_idx > refractory_samples:
                    blink_indices.append(blink_idx)
                    last_idx = blink_idx


 
    return blink_indices, thresholds    




if __name__ == '__main__':


    sample_freq = 256
    win_size = 3.5  # seconds — MUST match the detector's win_size

    t, eeg = simulate_eeg_with_blinks(duration=30, fs=sample_freq, blink_rate=0.3)

    start = time.time()
    # rms = moving_rms(preprocess_for_blinks(eeg, sample_freq, smoothing_factor = 8), sample_freq)
    # rms = preprocess_for_blinks(eeg, sample_freq, smoothing_factor = 8)


    # # new_eeg = preprocess_for_blinks(eeg, sample_freq)

    blink_indices, thresholds = detect_blinks_adaptive(
        eeg, sample_freq, win_size=win_size, th_mult=5
    )


    end = time.time()
    print(f"Time taken: {(end - start) * 1000}")


    plt.figure(figsize=(10, 4))
    plt.plot(t, eeg, label="EEG + Blinks")

    # --- plot adaptive thresholds per window (green segments) ---
    n_samples = len(eeg)
    win_len = int(win_size * sample_freq)

    for i, th in enumerate(thresholds):
        start = i * win_len
        end = min(start + win_len, n_samples)
        if end > start:  # guard against empty slice
            # draw a horizontal segment at 'th' across the window's time span
            plt.plot(t[start:end], [th] * (end - start),
                     color='g', linestyle='-', linewidth=2,
                     label="Adaptive threshold" if i == 0 else "")

    # --- detected blinks (red verticals) ---
    for j, bt in enumerate(blink_indices):
        bt_sec = bt / sample_freq
        plt.axvline(bt_sec, color='r', linestyle='--', alpha=0.7,
                    label="Detected blink" if j == 0 else "")

    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude (µV)")
    plt.title("Simulated EEG with Blink Artifacts + Adaptive Thresholds")
    plt.legend()
    plt.tight_layout()
    plt.show()


    
    # print(blink_indices)
    
    
    