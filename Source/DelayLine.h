/*
  ==============================================================================

    DelayLine.h
    Created: 24 Apr 2026 4:27:12pm
    Author:  chenzuyu


   Two-phase design:
   prepare(maxDelaySamples) — allocates buffer once; call at note-on or
                               prepareToPlay. Clears buffer to zero.
   setDelay(int)            — repositions the read pointer within the
                               already-allocated buffer without clearing.
                               Safe to call every sample for portamento /
                               vibrato / pitch bend.
==============================================================================
*/
#pragma once
#include <vector>
#include <algorithm>

class DelayLine
{
public:
 // ── Allocation ────────────────────────────────────────────────────────────
 // Call once with the MAXIMUM delay you will ever need (sized for the lowest
 // frequency the string can play). Clears the buffer and resets pointers.
 void prepare(int maxDelaySamples)
 {
     size   = std::max(maxDelaySamples, 2);
     buffer.assign(size, 0.0f);
     writePtr = 0;
     readPtr  = 0;
 }

 // ── Real-time retune ──────────────────────────────────────────────────────
 // Repositions the read pointer for a new integer delay length.
 // Does NOT clear the buffer — the string keeps ringing across the retune.
 // newDelay must be in [1, size-1]; values outside are clamped.
 void setDelay(int newDelay)
 {
     int d    = std::clamp(newDelay, 1, size - 1);
     readPtr  = (writePtr - d + size) % size;
 }

 // ── Per-sample I/O ────────────────────────────────────────────────────────
 float read() const
 {
     return buffer[readPtr];
 }

 void write(float x)
 {
     buffer[writePtr] = x;
 }

 void advance()
 {
     writePtr = (writePtr + 1) % size;
     readPtr  = (readPtr  + 1) % size;
 }

 // ── Utilities ─────────────────────────────────────────────────────────────
 void clear()
 {
     std::fill(buffer.begin(), buffer.end(), 0.0f);
     writePtr = 0;
     readPtr  = 0;
 }

 int getSize() const { return size; }

private:
 std::vector<float> buffer;
 int size     = 0;
 int writePtr = 0;
 int readPtr  = 0;
};
