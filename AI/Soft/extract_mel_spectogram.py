import librosa
import numpy as np

def waveform_to_melspec(waveform, sr=16000, n_mels=40, n_fft=400, hop_length=160):
    S = librosa.feature.melspectrogram(y=waveform, sr=sr, n_fft=n_fft,
            hop_length=hop_length, n_mels=n_mels, win_length=400)
    S_dB = librosa.power_to_db(S, ref=np.max)
    S_scaled = normalize_and_scale_spec(S_dB)

    return S_scaled

def normalize_and_scale_spec(spec, eps=1e-6):
    spec_norm = (spec - spec.mean()) / (spec.std() + eps)
    
    # Scale to [0, 1]
    spec_min, spec_max = spec_norm.min(), spec_norm.max()
    spec_scaled = (spec_norm - spec_min) / (spec_max - spec_min)

    return spec_scaled