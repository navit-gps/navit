APRS SDR DSP — Expected Decode Pipeline Output
==============================================

This document provides **bit stream examples for every stage** of the synthetic
integration test in ``navit/plugin/aprs/tests/test_aprs_sdr_integration``.

Target payload expectation (what the DSP must ultimately deliver to the callback):

::

   frame_callback: length=40 first4=82 A0 A4 A6

Important
---------

The demodulation pipeline makes **bit decisions from the FM discriminator output**
using **per-bit averaging + threshold**, not Goertzel.

Conventions used below
----------------------

- **Bit order**: AX.25 bytes are transmitted **LSB-first**.
- **Mark/space**: mark = 1200 Hz, space = 2200 Hz.
- **NRZI line bit**: 1 means mark, 0 means space (tone selection).
- **NRZI data rule**: transition ⇒ data 0, no transition ⇒ data 1.
- **Samples per bit**:
  - RF: 192000 / 1200 = 160 RF samples/bit
  - Audio (after decimation by 4): 48000 / 1200 = 40 audio samples/bit

The bit examples focus on the start of the stream (preamble flags and the first
payload byte) because it is the most sensitive region for off-by-one errors.

Stage 1/12. AX.25 bytes (pre-stuff, pre-NRZI)
---------------------------------------------

The first payload bytes (after the opening flag) begin with destination address:

::

   payload bytes start: 82 A0 A4 A6 ...

Stage 2/12. AX.25 data bits (LSB-first)
---------------------------------------

AX.25 flag ``0x7E`` (LSB-first) and first payload byte ``0x82`` (LSB-first):

::

   0x7E bits: 0 1 1 1 1 1 1 0
   0x82 bits: 0 1 0 0 0 0 0 1

Combined example (flag then first payload byte):

::

   data bits:  0 1 1 1 1 1 1 0  0 1 0 0 0 0 0 1
              ^ flag (0x7E)      ^ first payload byte (0x82)

Stage 3/12. Bit stuffing (TX) / de-stuffing (RX)
------------------------------------------------

- **Flags are never stuffed.**
- Stuffing only applies to the frame content between flags.

Boundary condition (critical)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The de-stuffer must not be allowed to discard bits while you are assembling a
flag candidate byte (i.e. the 8 bits used to decide whether a completed byte is
``0x7E``).

If a stuffed 0 is discarded during flag assembly, the decoder slips by one bit
and the *very next* payload byte will be wrong. The classic symptom is that the
first payload byte becomes ``0x45`` instead of ``0x82``.

Practical implementation rule (RX):

- Only apply bit de-stuffing to the **payload bit stream between flags**.
- Do not run the de-stuffer on bits that belong to a flag byte (opening, preamble
  flush flags, or closing flag).

Example of what stuffing looks like in the data bit stream:

::

   before stuffing: ... 1 1 1 1 1 1 ...
   after stuffing:  ... 1 1 1 1 1 0 1 ...
                                   ^ stuffed 0

On the RX side, the de-stuffer must discard the 0 after five consecutive 1s.

Stage 4/12. NRZI line bits (TX output)
--------------------------------------

For a flag ``0 1 1 1 1 1 1 0`` and initial NRZI state = 1 (idle mark),
the NRZI **line bits** (tone selection per bit period) are:

::

   data bits:  0 1 1 1 1 1 1 0
   line bits:  0 0 0 0 0 0 0 1

Explanation:

- data 0 toggles the line state
- data 1 keeps the line state

Stage 5/12. Tone stream derived from the NRZI line bits
-------------------------------------------------------

Mapping from line bit to tone:

- line 1 ⇒ mark (1200 Hz)
- line 0 ⇒ space (2200 Hz)

For the example flag:

::

   line bits:  0 0 0 0 0 0 0 1
   tones:      S S S S S S S M

Stage 6/12. FM discriminator sample stream (audio-rate)
-------------------------------------------------------

The discriminator produces a scalar proportional to instantaneous frequency.

Expected sanity check lines (values vary slightly per run):

::

   verify[0]: ... audio=0.000000 freq=0.0Hz
   verify[1]: ... audio=0.289064 freq=2208.3Hz

Interpretation (approximate):

- space ≈ 0.29
- mark  ≈ 0.17

``verify[0]`` note
~~~~~~~~~~~~~~~~~~

``verify[0]`` exists only to seed the discriminator's previous-sample state; it
is not a meaningful frequency estimate. The first valid discriminator output is
``verify[1]`` (and subsequent).

Stage 7/12. Per-bit averaging (40 audio samples per bit)
--------------------------------------------------------

For each bit period, average 40 discriminator samples.
The resulting per-bit averages should cluster near the space/mark values:

::

   avg_disc[0..7] (example): 0.29 0.29 0.29 0.29 0.29 0.29 0.29 0.17
                              S    S    S    S    S    S    S    M

Stage 8/12. Raw bit decision (threshold on averaged discriminator)
------------------------------------------------------------------

Threshold used by the DSP: **0.23**.

Why 0.23?
~~~~~~~~~

In this synthetic test, the averaged discriminator output clusters around:

- mark ≈ 0.17
- space ≈ 0.29

The threshold is the midpoint: \((0.17 + 0.29) / 2 \approx 0.23\).

Real hardware note:

- The discriminator units and separation are **signal-dependent** (gain,
  filtering, deviation, DC removal).
- Real RTL-SDR reception may need threshold tuning and/or hysteresis.

Decision rule:

::

   raw_bit = 1 (mark)  if avg < 0.23
   raw_bit = 0 (space) if avg >= 0.23

Using the example averages:

::

   avg_disc: 0.29 0.29 0.29 0.29 0.29 0.29 0.29 0.17
   raw_bit:  0    0    0    0    0    0    0    1

Stage 9/12. NRZI decode (RX) from raw_bit stream
------------------------------------------------

NRZI decode rule used by the DSP:

- no transition (same raw_bit as last_bit) ⇒ decoded 1
- transition ⇒ decoded 0

With initial ``last_bit = 1`` and the example flag raw bits:

::

   last_bit init: 1
   raw_bit:       0 0 0 0 0 0 0 1
   decoded:       0 1 1 1 1 1 1 0

Which reconstructs the flag bits for ``0x7E``.

Stage 10/12. Flag search bit window
-----------------------------------

Flag pattern searched (LSB-first): ``0 1 1 1 1 1 1 0``.

At startup, the DSP is expected to lock a flag with these diagnostics:

::

   FLAG_FOUND at decoded=8
   FLAG_FOUND at decoded=8 goertzel_blk=8
   FLAG_FOUND last_bit=1

Note: log fields still use ``goertzel_*`` names historically, but the counters
now correspond to **bit periods** (one per 40 audio samples).

Stage 11/12. AX.25 in-frame bit stream (post flag lock)
-------------------------------------------------------

Immediately after the opening flag, the next 8 decoded bits must be the first
payload byte (``0x82``), LSB-first:

::

   expected decoded bits for 0x82: 0 1 0 0 0 0 0 1

For a stronger end-to-end check, the first 4 payload bytes must be
``82 A0 A4 A6``. As a **decoded bit stream** (still LSB-first per byte), that is:

::

   0x82: 0 1 0 0 0 0 0 1
   0xA0: 0 0 0 0 0 1 0 1
   0xA4: 0 0 1 0 0 1 0 1
   0xA6: 0 1 1 0 0 1 0 1

Combined (32 bits, grouped by byte):

::

   0 1 0 0 0 0 0 1   0 0 0 0 0 1 0 1   0 0 1 0 0 1 0 1   0 1 1 0 0 1 0 1

Stage 12/12. Frame callback bytes
---------------------------------

The completed frame (with FCS removed before callback) must start with:

::

   frame_callback: length=40 first4=82 A0 A4 A6

Final DSP stats (must match)
----------------------------

Each test case prints one final stats line that must match:

::

   APRS SDR DSP stats: rf_samples=76960 audio_samples=19240 decim_factor=4 samples_per_bit=40 goertzel_block=40 goertzel_blocks=481 raw_bits=481 decoded_bits=481 flags_found=2 frames=1

``flags_found`` counts both:

- opening flag (flag search lock)
- closing flag (end-of-frame flag)

Test result (must match)
------------------------

::

   All APRS SDR integration tests passed!

Implementation footguns (read before debugging)
----------------------------------------------

No static locals for HDLC/AX.25 state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These must be stored as fields in the DSP instance struct (not ``static`` locals),
otherwise state can leak across instances/tests and corrupt decoding:

- ``flag_pos``
- ``in_frame_bit_pos``
- ``in_frame_current_byte``
- ``bit_stuff_count``

Preamble flush rule and the >= 15 guard
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When a completed byte equals ``0x7E`` while already ``in_frame``:

- If ``frame_buffer_pos < 15``: treat it as a **preamble flag** and reset byte
  assembly + stuffing state, but keep ``in_frame = 1`` (stay in frame).
- If ``frame_buffer_pos >= 15``: treat it as the **closing flag**, deliver the
  frame (without the 2-byte FCS), then reset and exit frame.

The guard prevents early false termination if a spurious ``0x7E`` appears before
any realistic payload has accumulated.

Quick Fault Reference
---------------------

===========================================  ==============================================
Symptom                                       Likely cause
===========================================  ==============================================
``flags_found=0``                              Raw-bit polarity wrong (mark/space swapped),
                                             or discriminator threshold wrong
``flags_found=2 frames=0``                     Closing-flag handling broken (guard too strict/loose),
                                             or stuck in repeated preamble flush
Many ``frame_byte[*]=0x7E`` then no payload    Preamble flush rule wrong (should keep ``in_frame=1``),
                                              or threshold too close to boundary causing bit flips
``length`` too small (truncated frame)         Closing flag detected too early (guard too loose) or
                                              bit averaging/threshold unstable (spurious 0x7E)
``length`` too large / never closes            Closing flag never recognized (guard too strict), or
                                              raw bit decisions unstable so 0x7E pattern never forms
First payload byte ``0x45`` (expected ``0x82``) De-stuffer ran during flag assembly (stuffed-zero discard
                                             slipped the boundary between flag and payload)
Payload byte ``0xBE``                          Byte/bit position not reset on preamble flush, or state
                                             persisted (static locals)
``DISCARD stuffed zero`` during flags          De-stuffer incorrectly active while assembling a flag byte
Wrong ``first4`` (not ``82 A0 A4 A6``)         Bit averaging window misaligned, threshold wrong, or
                                             destuff/NRZI boundary violated
``first4=82 A0 A4 A6``                         Correct
===========================================  ==============================================
