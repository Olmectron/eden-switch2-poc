// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "eden_horizon/status_overlay.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <string>
#include <vector>

#include "common/common_types.h"
#include "common/overlay_panels.h"

namespace StatusOverlay {

namespace {

constexpr u32 GlyphWidth = 5;
constexpr u32 GlyphHeight = 7;
constexpr u32 Scale = 3;
constexpr u32 Padding = 6;
constexpr u32 CellWidth = (GlyphWidth + 1) * Scale;
constexpr u32 LineHeight = (GlyphHeight + 2) * Scale;

struct Glyph {
    char character;
    std::array<std::string_view, GlyphHeight> rows; ///< '1' is a lit pixel
};

constexpr std::array Font{
    Glyph{'0', {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}},
    Glyph{'1', {"00100", "01100", "00100", "00100", "00100", "00100", "01110"}},
    Glyph{'2', {"01110", "10001", "00001", "00010", "00100", "01000", "11111"}},
    Glyph{'3', {"11110", "00001", "00001", "01110", "00001", "00001", "11110"}},
    Glyph{'4', {"00010", "00110", "01010", "10010", "11111", "00010", "00010"}},
    Glyph{'5', {"11111", "10000", "11110", "00001", "00001", "10001", "01110"}},
    Glyph{'6', {"00110", "01000", "10000", "11110", "10001", "10001", "01110"}},
    Glyph{'7', {"11111", "00001", "00010", "00100", "01000", "01000", "01000"}},
    Glyph{'8', {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}},
    Glyph{'9', {"01110", "10001", "10001", "01111", "00001", "00010", "01100"}},
    Glyph{'.', {"00000", "00000", "00000", "00000", "00000", "01100", "01100"}},
    Glyph{'%', {"11001", "11010", "00010", "00100", "01000", "01011", "10011"}},
    Glyph{'A', {"01110", "10001", "10001", "11111", "10001", "10001", "10001"}},
    Glyph{'C', {"01110", "10001", "10000", "10000", "10000", "10001", "01110"}},
    Glyph{'D', {"11110", "10001", "10001", "10001", "10001", "10001", "11110"}},
    Glyph{'E', {"11111", "10000", "10000", "11110", "10000", "10000", "11111"}},
    Glyph{'F', {"11111", "10000", "10000", "11110", "10000", "10000", "10000"}},
    Glyph{'H', {"10001", "10001", "10001", "11111", "10001", "10001", "10001"}},
    Glyph{'K', {"10001", "10010", "10100", "11000", "10100", "10010", "10001"}},
    Glyph{'L', {"10000", "10000", "10000", "10000", "10000", "10000", "11111"}},
    Glyph{'N', {"10001", "11001", "10101", "10011", "10001", "10001", "10001"}},
    Glyph{'O', {"01110", "10001", "10001", "10001", "10001", "10001", "01110"}},
    Glyph{'P', {"11110", "10001", "10001", "11110", "10000", "10000", "10000"}},
    Glyph{'S', {"01111", "10000", "10000", "01110", "00001", "00001", "11110"}},
    Glyph{'X', {"10001", "10001", "01010", "00100", "01010", "10001", "10001"}},
};

const Glyph* FindGlyph(char character) {
    const auto it = std::ranges::find(Font, character, &Glyph::character);
    return it != Font.end() ? &*it : nullptr;
}

} // namespace

void Show(std::string_view text) {
    auto& panel = Common::GetOverlayPanel(Common::OverlayPanelId::Players);
    std::scoped_lock lock{panel.mutex};
    if (text.empty()) {
        if (panel.enabled) {
            panel.enabled = false;
            ++panel.version;
        }
        return;
    }

    std::vector<std::string_view> lines;
    std::size_t longest = 0;
    for (std::size_t start = 0; start <= text.size();) {
        const std::size_t end = (std::min)(text.find('\n', start), text.size());
        lines.push_back(text.substr(start, end - start));
        longest = (std::max)(longest, end - start);
        start = end + 1;
    }

    const u32 width = static_cast<u32>(longest) * CellWidth + Padding * 2;
    const u32 height = static_cast<u32>(lines.size()) * LineHeight + Padding * 2;
    // Translucent black behind opaque white text.
    std::vector<u8> pixels(std::size_t{width} * height * 4);
    for (std::size_t i = 3; i < pixels.size(); i += 4) {
        pixels[i] = 150;
    }

    for (std::size_t line = 0; line < lines.size(); ++line) {
        for (std::size_t column = 0; column < lines[line].size(); ++column) {
            const Glyph* const glyph = FindGlyph(lines[line][column]);
            if (glyph == nullptr) {
                continue;
            }
            const u32 origin_x = Padding + static_cast<u32>(column) * CellWidth;
            const u32 origin_y = Padding + static_cast<u32>(line) * LineHeight;
            for (u32 gy = 0; gy < GlyphHeight * Scale; ++gy) {
                for (u32 gx = 0; gx < GlyphWidth * Scale; ++gx) {
                    if (glyph->rows[gy / Scale][gx / Scale] != '1') {
                        continue;
                    }
                    u8* const pixel =
                        &pixels[(std::size_t{origin_y + gy} * width + origin_x + gx) * 4];
                    pixel[0] = pixel[1] = pixel[2] = pixel[3] = 255;
                }
            }
        }
    }

    panel.pixels = std::move(pixels);
    panel.width = width;
    panel.height = height;
    panel.enabled = true;
    ++panel.version;
}

} // namespace StatusOverlay
