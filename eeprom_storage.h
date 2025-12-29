#pragma once

#include <stdint.h>
struct TVFOState;

void storeState(const TVFOState &state);
void loadState(TVFOState &state);
