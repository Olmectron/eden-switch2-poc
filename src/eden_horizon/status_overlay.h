// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

// A few lines of text in the top left corner of the game, drawn through the renderer's
// overlay panels (common/overlay_panels.h). The frontend has no other way to put
// anything on screen while a game runs.
namespace StatusOverlay {

/// Shows `text` (lines separated by '\n'). Only digits, a few capitals and ". %" exist
/// in the built-in font; other characters come out blank. An empty text hides the panel.
void Show(std::string_view text);

} // namespace StatusOverlay
