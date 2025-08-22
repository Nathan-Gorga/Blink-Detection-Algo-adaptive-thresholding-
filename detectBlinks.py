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


def robust_threshold2(segment, th_mult=2.0):
    median = np.median(segment)
    mad = np.median(np.abs(segment - median))
    return median + th_mult * mad



def trimmed_threshold(segment, th_mult=2.0, trim_perc=0.1):
    # Remove top/bottom 10% of values
    lo, hi = np.percentile(segment, [100*trim_perc, 100*(1-trim_perc)])
    trimmed = segment[(segment >= lo) & (segment <= hi)]
    return np.mean(trimmed) + th_mult * np.std(trimmed)


def normal_threshold(segment, th_mult):
    return np.mean(segment) + th_mult * np.std(segment)


# solution 5 : Envelope / RMS Gating    

def moving_rms(x, fs, win=0.1):
    k = max(1, int(win*fs))
    c = np.convolve(x*x, np.ones(k)/k, mode='same')
    return np.sqrt(c)


# solution 8 : Quantile-based adaptive threshold per window

def window_quantile_threshold(seg, q=95):
    return np.quantile(seg, q)


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

        # Find peaks above threshold in this segment
        peaks, _ = find_peaks(segment, height=threshold, distance=refractory_samples)

        # Map local peak indices → global signal indices
        for p in peaks:
            blink_idx = start + p
            # global refractory check
            if blink_idx - last_idx > refractory_samples:
                blink_indices.append(blink_idx)
                last_idx = blink_idx
 
    return blink_indices, thresholds    


def detect_blinks_peakbased(eeg, fs):
    # x = preprocess_for_blinks(eeg, fs)
    x_abs = np.abs(eeg)
    # robust baseline for prominence
    base = np.median(x_abs)
    mad = 1.4826 * np.median(np.abs(x_abs - base))
    prom = 4.5 * mad                      # robust prominence
    width = (0.12*fs, 0.45*fs)            # 120–450 ms

    peaks, _ = find_peaks(x_abs, prominence=prom, width=width, distance=int(0.2*fs))
    return peaks.tolist()


if __name__ == '__main__':


    sample_freq = 256
    win_size = 3.5  # seconds — MUST match the detector's win_size

    t, eeg = simulate_eeg_with_blinks(duration=30, fs=sample_freq, blink_rate=0.3)

    rms = moving_rms(preprocess_for_blinks(eeg, sample_freq, smoothing_factor = 8), sample_freq)
    start = time.time()


    # # new_eeg = preprocess_for_blinks(eeg, sample_freq)

    # blink_indices, thresholds = detect_blinks_adaptive(
    #     rms, sample_freq, win_size=win_size, th_mult=10
    # )



    blink_indices = detect_blinks_peakbased(rms, sample_freq)
    end = time.time()
    print(f"Time taken: {end - start}s")

    plt.figure(figsize=(10, 4))
    plt.plot(t, rms, label="EEG + Blinks")

    # # --- plot adaptive thresholds per window (green segments) ---
    # n_samples = len(eeg)
    # win_len = int(win_size * sample_freq)

    # for i, th in enumerate(thresholds):
    #     start = i * win_len
    #     end = min(start + win_len, n_samples)
    #     if end > start:  # guard against empty slice
    #         # draw a horizontal segment at 'th' across the window's time span
    #         plt.plot(t[start:end], [th] * (end - start),
    #                  color='g', linestyle='-', linewidth=2,
    #                  label="Adaptive threshold" if i == 0 else "")

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
    
    
    