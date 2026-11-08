#pragma once

#ifndef JSON_MEMORY_H
#define JSON_MEMORY_H

// Makes cJSON allocate from PSRAM, falling back to internal RAM if PSRAM is missing or full.
// Call once at boot, before anything creates or parses JSON.
void json_memory_prefer_psram(void);

#endif // JSON_MEMORY_H
