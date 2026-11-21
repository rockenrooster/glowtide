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

const WorldPalette kWorlds[kPaletteCount] = {
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
    // VARIETY_DESIGN 15's five: pale blue to indigo; slate and violet with a yellow rail; the lightest backdrop the lint
    // allows; deep green with a silver rail; red-violet. Each was picked from three candidates (A, B, C) on the V5 sheet
    // (PROGRESS.md, V5 choices); the two not picked stay below as the world's alternatives.
    make("FROST GLASS", 0x88B2DA, 0x5A7CB8, 0x2A3470, 0x0E1234, 0x30487E, 0x182650, 0xFFFFFF, 0x40609A, 0x9CC4EC, 0x1C2850,
         0xE8F8FF),  // B
    make("STORM INDIGO", 0x5C6090, 0x30306A, 0x16163E, 0x08081C, 0x2E3262, 0x16183A, 0xFFD84A, 0x343666, 0x6A6EA4, 0x18183A,
         0xFFE8A8),  // B
    make("GOLD NOON", 0xD4AA5C, 0xA86C48, 0x5A2E40, 0x1C0E20, 0x4A2E48, 0x26162C, 0xFFF2C0, 0x6A3E50, 0xC08A70, 0x301828,
         0xFFF6D8),  // A
    make("MOONLIT JADE", 0x347A64, 0x164A40, 0x0A2422, 0x020E0C, 0x16423E, 0x0C2624, 0xC8D4DC, 0x1A463E, 0x50907A, 0x0C221E,
         0xD8F8E8),  // C
    make("CRIMSON DUSK", 0xB44C70, 0x782252, 0x360E38, 0x100418, 0x461C48, 0x240E28, 0xFF98B8, 0x501C48, 0x984474, 0x260C24,
         0xFFD0DC),  // C
};

// The two candidates not picked for each new world, for `--palette-alt 1|2` and the owner's sheet.
const WorldPalette kAlts[5][2] = {
    {make("FROST GLASS A", 0x84B0DC, 0x4A68A8, 0x22285E, 0x0C0E2C, 0x2C4478, 0x16224A, 0xE8F6FF, 0x3A5488, 0x8AB4E0, 0x1A2448,
          0xE0F4FF),
     make("FROST GLASS C", 0x78A0D0, 0x3C4E94, 0x1E1C54, 0x0A0A26, 0x283C70, 0x141E44, 0xC8ECFF, 0x34487E, 0x7C9CD4, 0x182040,
          0xD8ECFF)},
    {make("STORM INDIGO A", 0x6A6E9C, 0x3C3A74, 0x1C1A46, 0x0A0A22, 0x34386A, 0x1A1C40, 0xFFE45C, 0x3A3C6E, 0x7478B0, 0x1C1C40,
          0xFFF0B8),
     make("STORM INDIGO C", 0x7A70A8, 0x46387C, 0x221848, 0x0C0820, 0x383466, 0x1C183E, 0xF4F07A, 0x403868, 0x8070B0, 0x201A40,
          0xFFF4C8)},
    {make("GOLD NOON B", 0xC89E58, 0x9A6044, 0x502A3C, 0x180C1C, 0x442A44, 0x22142A, 0xFFEAB0, 0x623A4C, 0xB4806A, 0x2C1626,
          0xFFF0D0),
     make("GOLD NOON C", 0xD2A866, 0xB07A58, 0x64384A, 0x201024, 0x4C3050, 0x281830, 0xFFF8D8, 0x704458, 0xC8967C, 0x341C30,
          0xFFFAE8)},
    {make("MOONLIT JADE A", 0x3E8A6E, 0x1C5448, 0x0C2A28, 0x041210, 0x1A4A44, 0x0E2A28, 0xD8E4EC, 0x1E4E44, 0x5A9A84, 0x0E2622,
          0xE0FFF0),
     make("MOONLIT JADE B", 0x4A9A7C, 0x226052, 0x10322E, 0x061614, 0x1E5048, 0x10302C, 0xE8F0F4, 0x22564A, 0x64A890, 0x102A26,
          0xE8FFF4)},
    {make("CRIMSON DUSK A", 0xC85A7C, 0x8A2A5E, 0x3E1240, 0x14061C, 0x4E2050, 0x28102C, 0xFFB0C8, 0x5A2050, 0xA85080, 0x2A0E28,
          0xFFD8E0),
     make("CRIMSON DUSK B", 0xD06884, 0x963468, 0x461648, 0x180820, 0x542456, 0x2C1230, 0xFFC0D4, 0x622458, 0xB45C8C, 0x2E1030,
          0xFFE0E8)},
};

}  // namespace

const WorldPalette& worldPalette(int world) { return kWorlds[(world < 1 ? 0 : (world - 1) % kPaletteCount)]; }

// No two neighbouring worlds share a sky feature or a prop family, and no two worlds share both.
int skyFeature(int world) {
    static const int sky[kPaletteCount] = {1, 3, 2, 1, 4, 3, 4, 3, 2, 1};
    return sky[(world < 1 ? 0 : (world - 1) % kPaletteCount)];
}

int propFamily(int world) {
    static const int family[kPaletteCount] = {0, 1, 2, 1, 0, 2, 1, 0, 1, 2};
    return family[(world < 1 ? 0 : (world - 1) % kPaletteCount)];
}

const WorldPalette& worldPaletteAlt(int world, int alt) {
    if (world >= 6 && world <= 10 && alt >= 1 && alt <= 2) return kAlts[world - 6][alt - 1];
    return worldPalette(world);
}

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
