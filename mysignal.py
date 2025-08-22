import numpy as np
import matplotlib.pyplot as plt

def simulate_eeg_with_blinks(
    duration: float = 10.0,
    fs: int = 256,
    blink_rate: float = 0.25,  # blinks per second
    blink_amp: float = 150.0,  # µV
    noise_level: float = 20.0  # µV
) -> tuple[np.ndarray, np.ndarray]:
    """
    Simulate EEG with blink artifacts.

    Returns:
        t: time vector
        eeg: simulated EEG signal
    """
    n_samples = int(duration * fs)
    t = np.linspace(0, duration, n_samples, endpoint=False)

    # --- Background EEG: mix of oscillations + noise ---
    eeg = (
        30 * np.sin(2 * np.pi * 10 * t) +   # alpha
        15 * np.sin(2 * np.pi * 6 * t) +    # theta
        10 * np.sin(2 * np.pi * 20 * t) +   # beta
        noise_level * np.random.randn(n_samples)  # white noise
    )

    # --- Simulate blinks ---
    blink_positions = np.random.choice(
        n_samples,
        size=int(duration * blink_rate),
        replace=False
    )

    for pos in blink_positions:
        width = int(0.3 * fs)  # 300 ms
        win = np.arange(-width//2, width//2)
        blink = blink_amp * np.exp(-0.5 * (win / (0.1*fs))**2)  # Gaussian shape
        start = max(0, pos - width//2)
        end = min(n_samples, pos + width//2)
        eeg[start:end] += blink[:end-start]

    return t, eeg

if __name__ == '__main__':
    # --- Example usage ---
    t, eeg = simulate_eeg_with_blinks(duration=10, fs=256)
    plt.figure(figsize=(12, 4))
    plt.plot(t, eeg, label="EEG + Blinks")
    plt.xlabel("Time (s)")
    plt.ylabel("Amplitude (µV)")
    plt.title("Simulated EEG with Blink Artifacts")
    plt.legend()
    plt.show()
