# Signalsmith audio dependencies

Unmodified implementation headers, vendored for offline/reproducible builds:

- Signalsmith Stretch 1.3.2: https://github.com/Signalsmith-Audio/signalsmith-stretch
  Commit `a670068d9aeb64913331d5cc29337b19a457a7df`.
  `signalsmith-stretch.h` is the upstream root header.
- Signalsmith Linear: https://github.com/Signalsmith-Audio/linear
  Commit `146b26f91b48835d665fa0498d64d9617818d27a`.
  `signalsmith-linear/stft.h` and `fft.h` are the upstream root implementation headers.

Both libraries are MIT licensed. Their complete license texts are retained here
and installed with the application. No optional external FFT backend is enabled.
