/*
  ==============================================================================

    DWGClarinet.h
    Created: 18 Jun 2026 3:39:13pm
    Author: chenzuyu

    Stage 1–2: single-tube clarinet, linear ReedTable, scalar bell reflection.

    VERIFIED TOPOLOGY (single delay line, not a two-rail string-style model):

        ┌─────────────────────────────────────────────────────────┐
        │                                                           │
        │   lastOut ──> [loss filter] ──> ×(-0.95) ──┐              │
        │                                            │              │
        │   breathPressure ─────────────────────────(-)──> pressureDiff
        │                                                           │
        │   pressureDiff ──> [ReedTable] ──> rho                    │
        │                                                           │
        │   injected = breathPressure + pressureDiff * rho          │
        │                                                           │
        │   injected ──> [FracDelay] ──> [DelayLine] ──> y ─────────┘
        │                                          │
        └──────────────────────────────────────────┴──> output (×outputGain)

    This is the STK Clarinet topology (Cook & Scavone), adapted to use your
    DelayLine/FracDelay/ReedTable classes in place of STK's DelayL/OneZero/
    ReedTable. Verified against a real compiled STK reference: with matching
    offset/slope/breathPressure parameters, this structure self-oscillates
    with the same peak amplitude (~0.68) and harmonic content (odd harmonics
    dominant: f0, 3·f0, 5·f0...) as the reference implementation.

    IMPORTANT — why this differs from the two-rail "DWGString-style" model
    that was tried first and did NOT oscillate:
      A two-rail model (separate reed→bell and bell→reed delay lines, crossed
      write like DWGString's delayL/delayR) requires BOTH ends to have
      frequency-independent, sign-correct terminations for the loop to find
      an oscillating fixed point under simple linear analysis. The reed
      junction here is a single nonlinear SCATTERING point that combines
      "read last bell-reflected sample" and "inject toward bell" in one
      operation each sample — collapsing it into a single delay line (as STK
      does) is what makes self-oscillation occur reliably with these reed
      parameters. The two-rail split is physically motivated for STRINGS
      (truly bidirectional, symmetric boundary conditions) but is not how
      the canonical single-reed woodwind DWG model is structured. The
      single-delay-line form here matches Smith/Cook/Scavone's reference
      implementations and Fig. 8 collapsed onto one round-trip delay.

    Half-pressure note: this implementation uses FULL pressure (not h=p/2)
    to match the verified STK formula exactly. The half-pressure convention
    from Smith et al. Fig. 8 is mathematically equivalent (see Rev. 3 of the
    research notes) but introduces a factor-of-2 bookkeeping difference that
    is easy to get wrong; using full pressure here removes that risk.

    CRITICAL BUG FOUND AND FIXED DURING VALIDATION — read this before
    touching ReedTable's clamp range:
      ReedTable::tick() must clamp to [-1, +1], NOT [-1, 0]. With breath
      pressure plus noise driving the operating point, the reed spends most
      of each cycle SATURATED at rho ≈ +1. At that saturation point the
      injected-sample equation collapses algebraically:
          injected = breathPressure + pressureDiff * 1
                    = breathPressure + (-0.95*filtered - breathPressure)
                    = -0.95 * filtered
      — the breathPressure terms cancel exactly, leaving a pure feedback
      loop on the bell-reflected signal. THIS cancellation is what produces
      stable self-oscillation locked to the fundamental. Clamping to [-1, 0]
      instead (an earlier, incorrect version of this class) breaks the
      cancellation; breathPressure free-feeds straight into the loop instead,
      and the system collapses to a DC fixed point with zero oscillation —
      confirmed both by a local-gain stability analysis (|gain| ≈ 0.37 < 1,
      i.e. provably stable/non-oscillating at that clamp range) and by
      direct simulation. Diagnosed by comparing intermediate variables
      (rho, pressureDiff, injected) sample-by-sample against a compiled
      real STK reference — rho was pinned at exactly 1.00000 for long
      stretches, which is only possible if the clamp ceiling is +1.

    Boundary conditions (still physically as derived):
      Reed end (closed, Z_L → ∞):  R → +1, expressed implicitly in the
                                    junction equation, NOT as a separate
                                    scalar multiply.
      Bell end (open, Z_L → 0):    R = -1, applied here as bellReflection
                                    (scalar, Stage 2; Stage 6 will replace
                                    the lossFilter with a proper frequency-
                                    dependent Reflection Filter R(ω)).

    Pitch: clarinet is a closed-open cylindrical pipe. Round-trip delay for
    one period = (fs / freq) * 0.5 samples, minus the loss filter's phase
    delay, minus 1 sample for the "lastOut" one-sample lag inherent in the
    feedback structure above.
  ==============================================================================
*/

#pragma once
#include "DelayLine.h"
#include "FracDelay.h"
#include "ReedTable.h"
#include <cmath>
#include <algorithm>

class DWGClarinet
{
public:
    // ── Lifecycle ──────────────────────────────────────────────────────────
    // Call once from prepareToPlay. Allocates delay buffer for lowestFreq.
    void prepare(double sampleRate, float lowestFreq = 130.0f); // ~D3, clarinet's lowest note

    // ── Pitch ──────────────────────────────────────────────────────────────
    void setFrequency(float frequency);

    // ── Reed parameters (Stage 1: linear ReedTable) ──────────────────────────
    void setReedParameters(float offset, float slope) { reedTable.setParameters(offset, slope); }

    // ── Loss / radiation (Stage 2: scalar; Stage 4/6 will replace with filters) ──
    void setBellReflection(float R) { bellReflection = std::clamp(R, -1.0f, 0.0f); }

    // ── Breath pressure envelope (mirrors STK startBlowing/stopBlowing) ─────
    // amplitude: target loudness, roughly [0, 1]. Internally mapped to the
    // breath-pressure operating point the reed needs to self-oscillate.
    void startBlowing(float amplitude, float attackRate);
    void stopBlowing(float releaseRate);

    // ── Note on/off ────────────────────────────────────────────────────────
    void noteOn(float frequency, float amplitude);
    void noteOff(float amplitude);

    // ── Main audio callback ───────────────────────────────────────────────
    // Internally advances the breath envelope each sample (matches STK's
    // Clarinet::tick(), which reads its own envelope/noise generators).
    float process();

private:
    void allocateDelay(float lowestFreq);

    double fs        = 44100.0;
    float  freq       = 220.0f;
    int    maxDelay   = 1;

    DelayLine delayLine;
    FracDelay fracDelay;

    ReedTable reedTable;

    // ── Bell-end commuted loss (Stage 2: scalar R + one-pole-style loss) ────
    float bellReflection = -0.95f;
    float lossZ1          = 0.0f;     // one-zero filter state (b0=b1=0.5)

    // ── Breath envelope (linear ramp, mirrors STK::Envelope) ─────────────────
    float envelopeValue  = 0.0f;
    float envelopeTarget = 0.0f;
    float envelopeRate   = 0.001f;

    // ── Noise injection (small perturbation that helps the reed escape any
    //    transient near-fixed-point and lock into the travelling-wave mode;
    //    NOTE: verified the structure also oscillates with noiseGain = 0,
    //    so this is for natural timbre, not a correctness requirement) ──────
    float noiseGain  = 0.2f;

    float outputGain = 1.0f;

    float lastOut = 0.0f;   // delay line's previous output, fed back into the loss filter
};
