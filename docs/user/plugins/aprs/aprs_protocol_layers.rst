APRS Protocol and Signal Layers
===============================

This document describes the layers involved in APRS reception/transmission and
how the APRS SDR integration test synthesizes Bell 202 I/Q samples.

Layer overview (high to low)
----------------------------

APRS sits on top of AX.25, which is framed with HDLC and carried over Bell 202
2FSK. In the SDR plugin we ultimately operate on complex I/Q samples produced by
an RTL-SDR.

::

   APRS information field (payload)
     ↓
   AX.25 UI frame (addresses, control=0x03, PID=0xF0, info, FCS)
     ↓
   HDLC framing (0x7E flags, bit stuffing between flags)
     ↓
   NRZI line coding (transition/no-transition)
     ↓
   Bell 202 2FSK (mark=1200 Hz, space=2200 Hz, 1200 baud)
     ↓
   RF/IF placement and sampling (complex I/Q at rf_sample_rate)

APRS (application layer)
------------------------

APRS defines the semantics of the information field inside an AX.25 UI frame.
In the integration test, the payload is:

::

   !5132.00N/00007.00W-Test

AX.25 (link layer)
------------------

AX.25 UI frames contain:

- Destination address (6 chars + SSID byte, shifted left 1 bit)
- Source address (6 chars + SSID byte, shifted left 1 bit)
- Control (UI = ``0x03``)
- PID (no layer 3 = ``0xF0``)
- Information field (APRS payload)
- FCS (CRC-CCITT) appended little-endian

In the test, the encoded payload (including AX.25 header) begins with:

::

   82 A0 A4 A6 40 40 60 96 8E 6A B0 B0 B0 61 03 F0 ...

HDLC framing (flags + bit stuffing)
-----------------------------------

AX.25 is sent in an HDLC-like bitstream:

- A flag byte ``0x7E`` (binary ``01111110``) delimits frames.
- Between flags, after any run of five consecutive ``1`` data bits, a ``0`` is
  inserted (bit stuffing). The receiver must remove that ``0`` (de-stuffing).
- Flags themselves are never stuffed.

NRZI line coding
----------------

AX.25 uses NRZI (Non-Return-to-Zero Inverted) on the wire:

- Data bit ``0`` ⇒ transition (toggle line state)
- Data bit ``1`` ⇒ no transition (keep line state)

The integration test (and the DSP) assume initial line state ``1`` (idle mark).

Bell 202 (physical layer: 2FSK)
-------------------------------

Bell 202 (AFSK1200 when used over audio) corresponds to two tones at 1200 baud:

- Mark:  1200 Hz
- Space: 2200 Hz

In this codebase, the NRZI *line state* maps directly to tone selection:

- line ``1`` ⇒ mark (1200 Hz)
- line ``0`` ⇒ space (2200 Hz)

Synthetic IQ generation (integration test)
------------------------------------------

The integration test generates a full end-to-end synthetic signal in
``navit/plugin/aprs/tests/test_aprs_sdr_integration.c`` using these steps.

1. Build an AX.25 UI frame (bytes + FCS)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Function: ``encode_ax25_frame()``

- Builds the AX.25 header, control, PID, and APRS information field.
- Computes CRC-CCITT (poly ``0x1021``, init ``0xFFFF``, non-reflected) over the
  payload and appends the 2-byte FCS little-endian.

2. Add preamble/opening/closing flags and apply bit stuffing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Function: ``append_flag_and_stuff_bits()``

- Prepends **16** preamble flags (``0x7E``), then an opening flag.
- Appends all frame bytes (payload + FCS) as bits LSB-first with bit stuffing.
- Appends a closing flag.

Important boundary condition:

- The encoder intentionally does **not** stuff bits inside flag bytes
  (see ``encode_flag_bits()``).

3. NRZI encode (data bits → line bits)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Function: ``nrzi_encode_bits()``

- Initial line state: ``1``
- For each input data bit:
  - if bit == 0: toggle state
  - if bit == 1: keep state
- Output array contains the **line state per bit** (one element per bit).

4. Generate RF-rate complex I/Q for Bell 202 with optional IF offset
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Function: ``generate_fsk_iq()``

Configuration used by the test:

- RF sample rate: ``192000`` Hz
- Baud: ``1200`` bps
- Samples per bit: ``192000 / 1200 = 160`` RF samples/bit
- Mark: ``1200`` Hz
- Space: ``2200`` Hz
- IF offset: ``if_offset_hz`` (``100000.0`` for the IF test, ``0.0`` for DC-centered)

For each NRZI line bit:

- Pick ``audio_freq`` (mark or space).
- Compute ``rf_freq = if_offset_hz + audio_freq``.
- Emit 160 complex samples of a unit phasor rotating at ``rf_freq``:

  - \( I = \cos(\phi) \)
  - \( Q = \sin(\phi) \)
  - \( \phi \leftarrow \phi + 2\pi \cdot rf\_freq / rf\_sample\_rate \)

The emitted samples are converted to unsigned bytes with center 128:

- Scale by 80 and clamp to [-128,127]
- Store interleaved bytes: ``I0 Q0 I1 Q1 ...`` as ``(value + 128)``

Noise:

- The generator includes a Gaussian noise helper, but ``noise_std`` is currently
  set to 0.0 for the deterministic test run (see ``generate_fsk_iq()``).

Padding/alignment:

- The RF sample buffer is padded so that the downsampled audio length (48 kHz)
  is a multiple of 40 samples/bit.
- Extra RF samples (if needed) are filled with an idle mark tone.
- Finally, the byte array is padded to a multiple of 4 bytes by appending
  ``128`` (zero-IQ) bytes.

How the DSP consumes the synthetic IQ
------------------------------------

The SDR DSP then:

- Mixes down by ``if_offset_hz`` to baseband.
- Applies DC blocking.
- Runs a phase-difference FM discriminator to produce one audio sample per
  decimated sample.
- Makes one bit decision per 40 audio samples by averaging discriminator output
  and comparing to a threshold.
- NRZI-decodes and performs HDLC/AX.25 flag search, de-stuffing, and byte
  assembly, then calls the frame callback.

