import numpy as np

def find_local_maxima(signal: np.ndarray, start: int, end: int) -> int:
    window = signal[start:end]
    return start + window.argmax()
    

if __name__ == '__main__':
    signal = np.array([0, 1, 3, 7, 6, 2, 5, 10, 9, 8, 3, 2, 3, 4,5,6,7,8,9,11,14,10,7,6,4,2,1])
    start = 5
    end = 10
    print(f'the local maxima is {signal[find_local_maxima(signal, start, end)]}')
