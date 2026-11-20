#include "gfx/palette.h"

#include <cmath>

namespace gt {

V3 hexColor(unsigned rgb) {
    return {float((rgb >> 16) & 0xFF) / 255.0f, float((rgb >> 8) & 0xFF) / 255.0f, float(rgb & 0xFF) / 255.0f};
}

namespace {

WorldPalette make(const char* name, unsigned b0, unsigned b1, unsigned b2, unsigned b3, unsigned lf, unsigned ln, unsigned rail,
                  unsigned pb, unsigned pl, unsigned pd, unsigned dawn) {
    return {name,
            {hexColor(b0), hexColor(b1), hexColor(b2), hexColor(b3)},
            hexColor(lf),
            hexColor(ln),
            hexColor(rail),
            hexColor(pb),
            hexColor(pl),
            hexColor(pd),
            hexColor(dawn)};
}

const WorldPalette kWorlds[5] = {
    make("VIOLET DUSK", 0x8458AC, 0x403080, 0x18184C, 0x0A0A26, 0x3C4488, 0x1E2254, 0x60F4E8, 0x2E3C72, 0x5C70B2, 0x1A2048,
         0xFFC9A8),
    make("DEEP TEAL", 0x2E8C9A, 0x14506E, 0x0A2844, 0x04101E, 0x1C5066, 0x0E2C42, 0x7CE0FF, 0x1E4A66, 0x4E90AC, 0x102A40,
         0xB8F4FF),
    make("ROSE DAWN", 0xE89C8C, 0xB0608E, 0x5A3278, 0x1E1440, 0x5E4078, 0x2C1C48, 0xFFD9A0, 0x5A3A78, 0xA070B0, 0x2E1E4A,
         0xFFE2C8),
    make("EMERALD AURORA", 0x48B48A, 0x1E7882, 0x102E5A, 0x080E28, 0x1E5460, 0x122848, 0xB4FF9C, 0x1E5068, 0x4A9A9C, 0x102C44,
         0xD0FFE0),
    make("EMBER", 0xF08A50, 0xB43C5A, 0x46184E, 0x140A26, 0x6E3458, 0x2A1840, 0xFFC878, 0x5A2A58, 0xA85A7C, 0x2C1638,
         0xFFE0B0),
};

}  // namespace

const WorldPalette& worldPalette(int world) { return kWorlds[(world < 1 ? 0 : (world - 1) % 5)]; }

const GamePalette& gamePalette() {
    static const GamePalette p = {
        hexColor(0xFFC654), hexColor(0xFFEEB0), hexColor(0xCE7022),                       // wisp
        hexColor(0x7A3CC8), hexColor(0xA878F0), hexColor(0x2A1260), hexColor(0xFF9CEA),  // gloom
        hexColor(0x54F2E0), hexColor(0xFF547C),                                           // good, bad
        hexColor(0x281A54), hexColor(0x4E368C), hexColor(0x160E34), hexColor(0xFF60D6),  // spire
        hexColor(0x226884), hexColor(0x46B0C8), hexColor(0x1A4E68),                       // launcher
        hexColor(0x0E0E2C), hexColor(0xFFFFFF),
    };
    return p;
}

float luminance(V3 c) {
    auto lin = [](float v) { return v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f); };
    return 0.2126f * lin(c.x) + 0.7152f * lin(c.y) + 0.0722f * lin(c.z);
}

float contrast(V3 a, V3 b) {
    const float la = luminance(a), lb = luminance(b);
    return (maxv(la, lb) + 0.05f) / (minv(la, lb) + 0.05f);
}

}  // namespace gt
