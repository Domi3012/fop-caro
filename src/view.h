#pragma once

#include "model.h"
#include "controller.h"

// Quản lý tài nguyên của View
void initView(); // goi sau khi init window
void unloadView(); // goi truoc khi close window

// controller chi can goi cai ham nay de ve thoi
void renderGame(const MatchState& match, const UIState& ui);

