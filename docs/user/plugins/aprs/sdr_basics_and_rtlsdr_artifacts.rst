SDR Basics and RTL-SDR Hardware Artifacts
========================================

This document explains how a software-defined radio (SDR) receiver works at a
signal-processing level, and why **real RTL-SDR hardware** benefits from using a
non-zero IF offset when receiving APRS.

The context in this repository is the APRS SDR pipeline:

- Synthetic test signal generation in:
  ``navit/plugin/aprs/tests/test_aprs_sdr_integration.c``
- DSP demodulation in:
  ``navit/plugin/aprs_sdr/aprs_sdr_dsp.c``

What an SDR produces (IQ samples)
---------------------------------

An SDR front-end samples the RF spectrum and provides complex baseband samples:

- I (in-phase) component
- Q (quadrature) component

The output stream is typically an interleaved byte stream:

::

   I0 Q0 I1 Q1 I2 Q2 ...

Conceptually, each complex sample is:

- \( x[n] = I[n] + j Q[n] \)

If the SDR is tuned to a center frequency \(f_c\), then signals near \(f_c\) are
shifted into a low-frequency band around 0 Hz (baseband). Frequency offsets
relative to \(f_c\) appear as positive or negative frequencies in the complex
spectrum.

Digital downconversion (why the mixer exists)
---------------------------------------------

In practice, you often tune the hardware a little away from the desired signal
(an IF offset), then digitally shift the desired signal back to DC using a
complex mixer:

- Multiply the IQ stream by a complex sinusoid
  \(e^{-j 2\pi f_{\text{mix}} n / f_s}\).

This shifts the spectrum by \(f_{\text{mix}}\).

In ``aprs_sdr_dsp.c`` the mixer is implemented via:

- \( (I + jQ) \cdot (\cos(\phi) + j\sin(\phi)) \)
- with phase increment:
  \( \Delta\phi = -2\pi \cdot f_{\text{offset}} / f_s \)

After mixing, the APRS signal is centered where the downstream demod expects it.

Decimation (RF rate → audio rate)
---------------------------------

The RTL-SDR produces IQ at a high sample rate (e.g. 192 kHz in the tests).
The APRS demodulation logic operates at a lower rate (e.g. 48 kHz).

Decimation means keeping every Nth sample (optionally after filtering) to reduce
the sample rate. In the integration tests:

- RF sample rate: 192000 Hz
- Decimation: 4
- Audio sample rate: 48000 Hz

FM discriminator (turn phase change into an "audio" stream)
-----------------------------------------------------------

For 2FSK APRS (Bell 202), we can treat the complex baseband as an FM-like signal
where instantaneous frequency is proportional to the change in phase between
successive samples.

One common discriminator is:

- \( y[n] = \arg(x[n] \cdot \overline{x[n-1]}) \)

This produces a scalar stream \(y[n]\) where the average value corresponds to
frequency. In the integration test's synthetic signal:

- mark (1200 Hz) tends to produce a discriminator average around ~0.17
- space (2200 Hz) tends to produce a discriminator average around ~0.29

Bit decisions from discriminator output (why we average)
--------------------------------------------------------

The discriminator output is not a 1200/2200 Hz sinusoid in this pipeline.
It is a scalar proportional to frequency. The APRS SDR DSP therefore decides
mark vs space by:

- averaging discriminator samples over one bit period
- thresholding the average

For 48 kHz audio and 1200 baud:

- samples per bit = 48000 / 1200 = 40

The threshold used in the synthetic integration tests is 0.23, which is the
midpoint between the observed clusters (~0.17 and ~0.29). For real hardware this
threshold is signal-dependent and may require tuning and/or hysteresis.

RTL-SDR hardware artifacts (why DC-centered reception is problematic)
---------------------------------------------------------------------

DC spike problem
~~~~~~~~~~~~~~~~

RTL-SDR devices commonly have a strong hardware artifact at exactly 0 Hz in the
complex baseband output. This is caused by local oscillator leakage and related
imperfections in the analog front-end.

When the desired signal is tuned to sit at 0 Hz (DC-centered reception), this
DC spike lands on top of the signal and can corrupt it. For APRS, this often
makes DC-centered reception nearly unusable on real hardware.

IQ imbalance
~~~~~~~~~~~~

Real hardware rarely has perfect I/Q matching:

- I and Q gains differ slightly
- the phase offset is not exactly 90 degrees

IQ imbalance creates a mirror (image) of the spectrum: energy from positive
frequencies leaks into negative frequencies and vice versa. Near DC, this mirror
image folds back close to the desired signal and degrades SNR.

Why the synthetic test still passes at 0 Hz
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The synthetic integration test signal is mathematically ideal:

- no DC spike
- no IQ imbalance
- no front-end distortion
- deterministic generation (and in practice, no added noise)

The IQ is generated directly using \(\cos(\phi)\) and \(\sin(\phi)\), which
bypasses the hardware imperfections that dominate at DC. Therefore
``test_aprs_sdr_dc_centered`` passing only demonstrates that the DSP math works
with ``if_offset=0`` in a perfect simulation. It does **not** guarantee that
DC-centered reception will work well on real RTL-SDR hardware.

What happens to the mixer at 0 Hz offset
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When ``if_offset_hz = 0``:

- mixer phase increment is 0
- \(\cos(0) + j\sin(0) = 1 + 0j\)

So the mixer multiply does nothing. The signal passes through unchanged, and the
FM discriminator operates directly on the baseband IQ. This is correct
mathematically in the ideal synthetic case.

For real hardware, always use an IF offset
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The +100 kHz offset used in ``test_aprs_sdr_if_offset`` reflects real-world RTL-SDR
practice:

- Tune the RTL-SDR center frequency above the APRS channel by an IF offset.
- The DC spike then appears at \(-f_{\text{offset}}\) in baseband, away from the
  desired signal.
- A digital downmixer shifts the desired signal back to baseband for the FM
  discriminator and bit decision logic.

Practical guidance:

- A minimum safe IF offset is typically around **50–100 kHz** for RTL-SDR.
- The goal is to push the DC spike and mirror images well outside the signal's
  effective bandwidth before downmixing.

