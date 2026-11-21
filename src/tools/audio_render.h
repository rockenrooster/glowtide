#pragma once
// Offline renders of the game's sound (TECH_SPEC 6): drives the mixer from a scripted event list into a 16-bit WAV.
// level: the level the flood replays (a late one by default).
int renderAudio(const char* scene, const char* out, float seconds, int level, int world = 1, bool boss = false);
