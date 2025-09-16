import librosa
import numpy as np

def waveform_to_melspec(waveform, sr=16000, n_mels=40, n_fft=400, hop_length=160):    
    # pad or truncate accordingly
    wav = pad_or_truncate(waveform, target_frames=sr)

    S = librosa.feature.melspectrogram(y=wav, sr=sr, n_fft=n_fft,
            hop_length=hop_length, n_mels=n_mels, win_length=400)
    S_dB = librosa.power_to_db(S, ref=np.max)
    S_scaled = normalize_and_scale_spec(S_dB)

    return S_scaled

def pad_or_truncate(wav, target_frames):
    if wav.shape[0] < target_frames:
        new_wav = np.pad(wav, int(np.ceil((target_frames - wav.shape[0]) / 2)), mode='reflect')
    else:
        new_wav = wav[:target_frames]
    
    return new_wav

def normalize_and_scale_spec(spec, eps=1e-6):
    spec_norm = (spec - spec.mean()) / (spec.std() + eps)
    
    # Scale to [0, 1]
    spec_min, spec_max = spec_norm.min(), spec_norm.max()
    spec_scaled = (spec_norm - spec_min) / (spec_max - spec_min)

    return spec_scaled