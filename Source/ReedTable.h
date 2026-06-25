/*
  ==============================================================================

    ReedTable.h
    Created: 25 Jun 2026 10:01:30am
    Author:  chenzuyu

  ==============================================================================
*/

#pragma once
#include <algorithm>

// ── ReedTable ────────────────────────────────────────────────────────────
// Stage 1: linear reed model (STK-style).
//
// Physical role: the reed is NOT a flow source — it is a nonlinear,
// variable REFLECTION COEFFICIENT at the mouthpiece end of the bore.
//
//   rho_hat(pressureDiff) maps the pressure difference across the reed
//   to a reflection coefficient in [-1, +1]:
//
//     rho = offset + slope * pressureDiff,  clamped to [-1, +1]
//
//   rho close to +1 -> reed nearly closed (high reflection, closed-end-like)
//   rho close to -1 -> reed fully open (no discontinuity in bore profile,
//                       open-end-like; "nearly impossible in a physical
//                       system" per STK's own comment, but mathematically
//                       the other saturation limit)
//
// IMPORTANT — range correction (was previously, incorrectly, [-1, 0]):
//   Saturating at rho=+1 makes the injected-sample equation collapse to
//   injected = breathPressure + pressureDiff*1 = -0.95*filtered (the
//   breathPressure terms cancel), which is what actually drives stable
//   self-oscillation in the verified reference implementation — the reed
//   spends most of its time saturated at this limit, only dipping into the
//   linear region briefly each cycle. Clamping to [-1, 0] instead breaks
//   this cancellation and the system converges to a DC fixed point instead
//   of oscillating. Verified by comparing against a compiled STK reference.
//
// This is deliberately the simplest possible reed: a 1D linear lookup,
// pre-solved (no Newton iteration needed). Matches STK Clarinet.cpp / STK's
// own ReedTable.h exactly (default offset=0.6, slope=-0.8; Clarinet.cpp
// overrides to offset=0.7, slope=-0.3).
class ReedTable
{
public:
    ReedTable() = default;

    // offset: reed's resting opening (embouchure-dependent, ~0.6–0.8 typical)
    // slope:  how fast the reed closes as pressureDiff increases (negative,
    //         roughly corresponds to reed stiffness; ~ -0.3 to -0.8)
    void setParameters (float newOffset, float newSlope)
    {
        offset = newOffset;
        slope  = newSlope;
    }

    void setOffset (float newOffset) { offset = newOffset; }
    void setSlope  (float newSlope)  { slope  = newSlope;  }

    float getOffset() const { return offset; }
    float getSlope()  const { return slope;  }

    // pressureDiff: pressure difference driving the reed
    // returns: nonlinear reflection coefficient rho_hat, clamped to [-1, +1]
    float tick (float pressureDiff) const
    {
        float rho = offset + slope * pressureDiff;
        return std::clamp (rho, -1.0f, 1.0f);
    }

private:
    float offset = 0.7f;
    float slope  = -0.3f;
};
