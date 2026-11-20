#pragma once
// Offline renders of the game's sound (TECH_SPEC 6): drives the mixer from a scripted event list into a 16-bit WAV.
int renderAudio(const char* scene, const char* out, float seconds);
