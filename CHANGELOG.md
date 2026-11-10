# Changelog

## 1.1.0 — 2026-11-10

- Add macOS Audio Unit and VST3 builds, distributed as universal Apple Silicon/Intel bundles.
- Add reproducible CMake build configuration and manual build/validation instructions.
- Save and restore all eight processor parameters with host project/plugin state.
- Restrict supported bus layouts to the 64 discrete inputs and outputs required by the DSP.
- Add AU state/impulse tests and an isolated REAPER VST3 project-state regression test.

The VST3/AU plugin identity is **TVFDN 64** (`Tv64` / `Tvfn`); the legacy VST2 release is not replaced automatically. Keep VST2 installed for projects that already use it. State restoration applies to parameter values, not the ringing reverb tail, and cannot recover settings omitted by older builds.

The macOS bundles target macOS 11 or newer and are ad-hoc signed, not Developer ID signed or notarized. Runtime validation was performed on Apple Silicon; Intel code was compiled but not runtime-tested. No updated Windows or VST2 binaries are included. The existing first-call-only DSP preparation behavior remains; use a fresh instance when changing sample rate.

## 1.0.0-beta — 2025-08-28

- Publish the initial Windows and macOS VST2 binaries.
