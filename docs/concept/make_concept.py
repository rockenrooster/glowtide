"""GLOWTIDE concept frame.

Renders docs/concept/concept_gameplay.png (1080x2400) and a half-size copy, with PIL only.
It is a look target for the executor and a preview for the owner, not a pixel spec: palette,
layout, density and the reading order of the screen are what matter. The camera, lane size and
colours here are the starting values quoted in GAME_DESIGN.md and TECH_SPEC.md.

Run:  python -I -B docs/concept/make_concept.py
"""
import math
import os
import random

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

W, H = 1080, 2400
OUT = os.path.dirname(os.path.abspath(__file__))
FONT = r"C:\Windows\Fonts\ARLRDBD.TTF"  # mock only; the game ships an OFL font (TECH_SPEC 5.7)
rng = random.Random(7)

# ---- palette: world 1 "Violet Dusk" (GAME_DESIGN 9.2) -------------------------------------------
SKY = [(0.00, (132, 88, 172)), (0.26, (64, 48, 128)), (0.62, (24, 24, 76)), (1.00, (10, 10, 38))]
LANE_NEAR, LANE_FAR = (30, 34, 84), (60, 68, 136)  # the far lane is blue-indigo, off the gloom's violet
RAIL = (96, 244, 232)
WISP, WISP_HI, WISP_RIM = (255, 198, 84), (255, 238, 176), (206, 112, 34)
GLOOM, GLOOM_HI, GLOOM_RIM, GLOOM_EYE = (122, 60, 200), (168, 120, 240), (42, 18, 96), (255, 156, 234)
GOOD, BAD = (84, 242, 224), (255, 84, 124)
PROP, PROP_HI, PROP_LO = (46, 60, 114), (92, 112, 178), (26, 32, 72)
SPIRE, SPIRE_HI, SPIRE_LO, CRACK = (40, 26, 84), (78, 54, 140), (22, 14, 52), (255, 96, 214)
INK = (14, 14, 44)

# ---- camera (TECH_SPEC 5.2) and lane (GAME_DESIGN 3) --------------------------------------------
CAM, TGT, VFOV = (0.0, 24.0, -11.0), (0.0, 0.0, 14.5), 56.0
HW, LEN = 5.0, 36.0  # lane half width and length, metres


def _norm(v):
    n = math.sqrt(sum(c * c for c in v))
    return tuple(c / n for c in v)


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


FWD = _norm(tuple(TGT[i] - CAM[i] for i in range(3)))
RIGHT = _norm(_cross((0, 1, 0), FWD))
UP = _cross(FWD, RIGHT)
TAN = math.tan(math.radians(VFOV / 2))


def proj(x, y, z):
    """World point -> (screen x, screen y, depth)."""
    r = (x - CAM[0], y - CAM[1], z - CAM[2])
    d = _dot(r, FWD)
    return (W / 2 + _dot(r, RIGHT) / d / TAN * H / 2, H / 2 - _dot(r, UP) / d / TAN * H / 2, d)


def ppm(depth):
    """Pixels per metre at a depth."""
    return H / 2 / TAN / depth


def lerp(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def mul(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c)


def vgrad(stops):
    col = Image.new("RGB", (1, H))
    px = col.load()
    for y in range(H):
        t = y / (H - 1)
        for (p0, c0), (p1, c1) in zip(stops, stops[1:]):
            if p0 <= t <= p1:
                px[0, y] = lerp(c0, c1, (t - p0) / (p1 - p0))
                break
    return col.resize((W, H))


_fonts = {}


def font(size):
    size = max(8, int(size))
    if size not in _fonts:
        _fonts[size] = ImageFont.truetype(FONT, size)
    return _fonts[size]


def ell(d, cx, cy, rx, ry, fill):
    d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=fill)


def star(d, cx, cy, r, fill):
    k = 0.26
    d.polygon([(cx, cy - r), (cx + r * k, cy - r * k), (cx + r, cy), (cx + r * k, cy + r * k),
               (cx, cy + r), (cx - r * k, cy + r * k), (cx - r, cy), (cx - r * k, cy - r * k)], fill=fill)


# ---- background: the camera looks down, so the backdrop is the depth below the causeway ----------
base = vgrad(SKY)
E = Image.new("RGB", (W, H))  # emissive layer: what blooms
e = ImageDraw.Draw(E)

glow = Image.new("RGB", (W, H))
gd = ImageDraw.Draw(glow)
ell(gd, W / 2, 500, 440, 340, (150, 70, 120))
ell(gd, W / 2, 450, 220, 200, (190, 96, 130))
base = ImageChops.screen(base, glow.filter(ImageFilter.GaussianBlur(120)))

mist = Image.new("RGBA", (W, H), (0, 0, 0, 0))
md = ImageDraw.Draw(mist)
for _ in range(11):
    ell(md, rng.uniform(0, W), rng.uniform(300, 2300), rng.uniform(260, 560), rng.uniform(18, 46),
        (150, 122, 214, rng.randint(14, 26)))
mist = mist.filter(ImageFilter.GaussianBlur(34))
base.paste(mist, (0, 0), mist)

d = ImageDraw.Draw(base, "RGBA")
for cx, cy, w in ((66, 700, 150), (1030, 930, 190), (40, 1500, 210), (1052, 1720, 150), (120, 2190, 120), (985, 420, 110)):
    t = cy / H  # floating islets off both sides, tinted toward the haze with distance
    body, top = lerp((58, 44, 116), (16, 16, 52), t), lerp((104, 84, 170), (40, 44, 104), t)
    d.polygon([(cx - w * 0.5, cy), (cx + w * 0.5, cy), (cx + w * 0.16, cy + w * 0.5), (cx - w * 0.08, cy + w * 0.95),
               (cx - w * 0.26, cy + w * 0.42)], fill=body)
    ell(d, cx, cy, w * 0.5, w * 0.15, top)
    for k in range(3):
        lx, ly = cx + (k - 1) * w * 0.26, cy - w * 0.05 - (k % 2) * w * 0.03
        ell(d, lx, ly, 4.5, 4.5, (255, 224, 160))
        ell(e, lx, ly, 7, 7, (255, 170, 90))

motes = Image.new("RGBA", (W, H), (0, 0, 0, 0))
bd = ImageDraw.Draw(motes)
for _ in range(170):
    c = rng.choice([(255, 214, 150), (166, 224, 255), (255, 166, 224)])
    r = rng.uniform(1.5, 6.5)
    ell(bd, rng.uniform(0, W), H * rng.random() ** 1.4, r, r, c + (rng.randint(36, 120),))
motes = motes.filter(ImageFilter.GaussianBlur(1.6))
base.paste(motes, (0, 0), motes)

# ---- the causeway ------------------------------------------------------------------------------
Z0, Z1 = -2.4, LEN + 1.7
lane_poly = [proj(-HW, 0, Z0)[:2], proj(HW, 0, Z0)[:2], proj(HW, 0, Z1)[:2], proj(-HW, 0, Z1)[:2]]
lane_mask = Image.new("L", (W, H), 0)
ImageDraw.Draw(lane_mask).polygon(lane_poly, fill=255)
y_far, y_near = proj(0, 0, Z1)[1] / H, proj(0, 0, Z0)[1] / H
base.paste(vgrad([(0.0, LANE_FAR), (y_far, LANE_FAR), (y_near, LANE_NEAR), (1.0, LANE_NEAR)]), (0, 0), lane_mask)
e.polygon(lane_poly, fill=(0, 0, 0))
d = ImageDraw.Draw(base, "RGBA")

# front face of the slab and its under-glow
d.polygon([proj(-HW, 0, Z0)[:2], proj(HW, 0, Z0)[:2], proj(HW, -1.5, Z0)[:2], proj(-HW, -1.5, Z0)[:2]], fill=(14, 16, 50))
d.line([proj(-HW, -1.5, Z0)[:2], proj(HW, -1.5, Z0)[:2]], fill=RAIL + (120,), width=3)

# tile seams and faint forward chevrons
z = 0.0
while z <= LEN:
    d.line([proj(-HW, 0, z)[:2], proj(HW, 0, z)[:2]], fill=(255, 255, 255, 15), width=2)
    z += 2.0
for i in range(-2, 3):
    d.line([proj(i * 2.0, 0, Z0)[:2], proj(i * 2.0, 0, Z1)[:2]], fill=(255, 255, 255, 10), width=2)
z = 3.0
while z < LEN - 2:
    d.line([proj(-0.9, 0, z)[:2], proj(0, 0, z + 0.9)[:2], proj(0.9, 0, z)[:2]], fill=(255, 255, 255, 13), width=5, joint="curve")
    z += 4.0

# glowing rails and edge lanterns
for sgn in (-1, 1):
    a, b = proj(sgn * HW, 0, Z0), proj(sgn * HW, 0, Z1)
    a2, b2 = proj(sgn * (HW - 0.15), 0, Z0), proj(sgn * (HW - 0.15), 0, Z1)
    d.polygon([a[:2], a2[:2], b2[:2], b[:2]], fill=RAIL)
    e.polygon([a[:2], a2[:2], b2[:2], b[:2]], fill=mul(RAIL, 0.75))
    z = 4.0
    while z < LEN:
        px, py, dep = proj(sgn * (HW + 0.3), 0.75, z)
        r = 0.2 * ppm(dep)
        fx, fy, _ = proj(sgn * (HW + 0.3), 0, z)
        d.line([(fx, fy), (px, py)], fill=PROP_HI, width=max(2, int(r * 0.5)))
        ell(d, px, py, r, r, (255, 226, 160))
        ell(e, px, py, r * 1.1, r * 1.1, (255, 190, 110))
        z += 8.0

# launcher rail slot
d.line([proj(-HW + 0.5, 0, 0.2)[:2], proj(HW - 0.5, 0, 0.2)[:2]], fill=(10, 12, 40, 220), width=14)
d.line([proj(-HW + 0.5, 0, 0.2)[:2], proj(HW - 0.5, 0, 0.2)[:2]], fill=RAIL + (70,), width=3)

# ---- the level: three rows, the stream, the flood, the enemy ------------------------------------
LX = -2.1  # launcher x
GATES = [  # x0, x1, z, label, colour, slides
    (-3.8, -0.5, 8.0, "\u00d72", GOOD, False),
    (0.7, 4.0, 8.0, "\u00f72", BAD, False),
    (-3.4, 0.2, 16.0, "\u00d73", GOOD, True),
    (-2.4, 0.4, 24.0, "\u00d75", GOOD, False),
]
BUMPERS = [(-4.0, 24.0, 0.7), (2.1, 24.0, 0.7)]
BARRIER = (1.5, 4.6, 15.3, "40")
BRUTE = (3.4, 20.4, 0.66, "12")


def front(x):
    """Where the two crowds meet: the enemy holds the right side further down the lane."""
    return 33.0 - 2.3 * max(0.0, x - 0.5)


wisps = []  # x, z, tier
z = 1.3
while z < 8.0:
    wisps.append((LX + rng.uniform(-0.07, 0.07), z, 0))
    z += 0.62
z = 8.3
while z < 16.0:
    for k in range(2):
        wisps.append((LX + (k - 0.5) * 0.8 + rng.uniform(-0.15, 0.15), z + rng.uniform(-0.2, 0.2), 0))
    z += 0.62
z = 16.3
while z < 24.0:
    for k in range(6):
        wisps.append((LX + rng.uniform(-1.5, 1.5), z + rng.uniform(-0.28, 0.28), 0))
    z += 0.62
z = 24.3
while z < LEN - 0.3:
    for k in range(6):
        x0 = LX + rng.uniform(-1.5, 1.5)
        zz = z + rng.uniform(-0.28, 0.28)
        if -2.4 <= x0 <= 0.4:  # through the x5
            for j in range(5):
                x = max(-HW + 0.4, min(HW - 0.4, x0 + rng.uniform(-2.9, 2.9)))
                zj = zz + rng.uniform(-0.3, 0.3)
                if zj < front(x):
                    roll = rng.random()
                    wisps.append((x, zj, 2 if roll < 0.03 else 1 if roll < 0.15 else 0))
        elif zz < front(x0):
            wisps.append((x0, zz, 0))
    z += 0.62

glooms = []  # x, z, radius, label
for _ in range(74):  # pouring out of the spire
    x = rng.uniform(0.7, 4.6)
    glooms.append((x, rng.uniform(front(x) + 0.5, LEN - 0.3), 0.29, None))
for _ in range(24):  # the column that slipped past on the right
    x = rng.uniform(2.7, 4.6)
    glooms.append((x, rng.uniform(9.6, front(x)), 0.29, None))
glooms.append(BRUTE)

sparks = [(x, 0.4, front(x) + rng.uniform(-0.5, 0.5), 1.0) for x in [rng.uniform(0.7, 4.5) for _ in range(26)]]
sparks += [(rng.uniform(-2.3, 0.3), rng.uniform(0.3, 1.1), LEN + 0.1, 0.6) for _ in range(10)]
sparks += [(LX + rng.uniform(-0.5, 0.5), 0.6, 8.2, 0.5) for _ in range(3)]
sparks += [(LX + rng.uniform(-1.2, 1.2), 0.6, 16.2, 0.5) for _ in range(5)]
sparks += [(rng.uniform(-2.2, 0.2), 0.6, 24.2, 0.5) for _ in range(7)]

# light thrown on the ground: by the crowd, the rails, the gates, the spire and the launcher
light = Image.new("RGB", (W, H))
ld = ImageDraw.Draw(light, "RGBA")
for sgn in (-1, 1):
    ld.line([proj(sgn * (HW - 0.3), 0, Z0)[:2], proj(sgn * (HW - 0.3), 0, Z1)[:2]], fill=RAIL + (60,), width=26)
for x0, x1, gz, _, col, _ in GATES:
    a, b = proj(x0, 0, gz - 0.7), proj(x1, 0, gz - 0.7)
    ld.line([a[:2], b[:2]], fill=col + (90,), width=int(0.9 * ppm(a[2])))
sx, sy, dep = proj(0, 0, LEN + 0.2)
ell(ld, sx, sy, 4.2 * ppm(dep), 1.3 * ppm(dep), CRACK + (110,))
sx, sy, dep = proj(LX, 0, 0.6)
ell(ld, sx, sy, 2.0 * ppm(dep), 1.2 * ppm(dep), WISP + (70,))
for x, z, tier in wisps:
    sx, sy, dep = proj(x, 0, z)
    r = (0.9 + 0.3 * tier) * ppm(dep)
    ell(ld, sx, sy, r, r * 0.62, (255, 176, 78, 34))
for x, z, rad, _ in glooms:
    sx, sy, dep = proj(x, 0, z)
    r = 3.0 * rad * ppm(dep)
    ell(ld, sx, sy, r, r * 0.62, (150, 60, 255, 30))
light = ImageChops.multiply(light.filter(ImageFilter.GaussianBlur(15)), Image.merge("RGB", [lane_mask] * 3))
base = ImageChops.add(base, light)
d = ImageDraw.Draw(base, "RGBA")
late = []  # labels: drawn after every unit, so a crowd never hides a number


def draw_wisp(x, z, tier):
    rad = 0.26 * (1.0, 1.4, 1.9)[tier]
    sx, sy, dep = proj(x, rad + rng.uniform(0.0, 0.26), z)
    r = rad * ppm(dep)
    lean = rng.uniform(-0.35, 0.35) * r
    tip = [(sx - r * 0.66, sy - r * 0.55), (sx + lean, sy - r * 1.85), (sx + r * 0.66, sy - r * 0.55)]
    rim = 1.6 + 0.4 * tier
    d.polygon([(tip[0][0] - rim, tip[0][1]), (tip[1][0], tip[1][1] - rim * 1.6), (tip[2][0] + rim, tip[2][1])], fill=WISP_RIM)
    ell(d, sx, sy, r + rim, r + rim, WISP_RIM)
    d.polygon(tip, fill=WISP)
    ell(d, sx, sy, r, r, WISP)
    ell(d, sx - r * 0.08, sy - r * 0.16, r * 0.60, r * 0.58, WISP_HI)
    ell(d, sx - r * 0.10, sy - r * 0.22, r * 0.28, r * 0.27, (255, 255, 240))
    ell(e, sx, sy, r, r, (255, 150, 44) if tier == 0 else (255, 178, 70))


def draw_gloom(x, z, rad, label):
    sx, sy, dep = proj(x, rad * 0.92 + rng.uniform(0.0, 0.14), z)
    r = rad * ppm(dep)
    for hx in (-0.52, 0.52):  # two horn bumps: a different silhouette from a wisp's single tip
        d.polygon([(sx + hx * r - r * 0.26, sy - r * 0.62), (sx + hx * r * 1.25, sy - r * 1.42),
                   (sx + hx * r + r * 0.26, sy - r * 0.62)], fill=GLOOM_RIM)
    ell(d, sx, sy, r + 1.8, r * 0.94 + 1.8, GLOOM_RIM)
    ell(e, sx, sy, r + 1.8, r * 0.94 + 1.8, (0, 0, 0))
    ell(d, sx, sy, r, r * 0.94, GLOOM)
    ell(d, sx - r * 0.12, sy - r * 0.42, r * 0.56, r * 0.34, GLOOM_HI)
    for ex in (-0.36, 0.36):
        ell(d, sx + ex * r, sy + r * 0.10, r * 0.19, r * 0.26, GLOOM_EYE)
        ell(e, sx + ex * r, sy + r * 0.10, r * 0.19, r * 0.26, (255, 120, 220))
        ell(d, sx + ex * r, sy + r * 0.16, r * 0.08, r * 0.12, GLOOM_RIM)
    if label:
        late.append(lambda: d.text((sx, sy - r * 1.95), label, font=font(r * 1.2), anchor="mm", fill=(255, 255, 255),
                                   stroke_width=max(2, int(r * 0.13)), stroke_fill=GLOOM_RIM))


def draw_gate(x0, x1, z, label, col, slides):
    h = 3.0
    p00, p10, p01, p11 = proj(x0, 0.12, z), proj(x1, 0.12, z), proj(x0, h, z), proj(x1, h, z)
    quad = [p00[:2], p10[:2], p11[:2], p01[:2]]
    d.polygon(quad, fill=col + (60,))
    lo0, lo1 = proj(x0, 0.8, z), proj(x1, 0.8, z)
    d.polygon([p00[:2], p10[:2], lo1[:2], lo0[:2]], fill=col + (48,))
    for k in range(1, 6):  # shimmer streaks
        t = k / 6 + rng.uniform(-0.05, 0.05)
        d.line([(p00[0] + (p10[0] - p00[0]) * t, p00[1]), (p01[0] + (p11[0] - p01[0]) * t, p01[1])], fill=(255, 255, 255, 22), width=3)
    e.polygon(quad, fill=mul(col, 0.28))
    s = ppm(p00[2])
    bw = max(3, int(0.10 * s))
    for a, b in ((p01, p11), (p00, p10)):
        d.line([a[:2], b[:2]], fill=col + (255,), width=bw)
        e.line([a[:2], b[:2]], fill=col, width=bw)
    for xx in (x0, x1):  # posts
        a, b = proj(xx, 0, z), proj(xx, h + 0.3, z)
        pw = 0.18 * s
        d.rounded_rectangle([a[0] - pw, b[1], a[0] + pw, a[1]], radius=pw * 0.8, fill=PROP)
        e.rounded_rectangle([a[0] - pw, b[1], a[0] + pw, a[1]], radius=pw * 0.8, fill=(0, 0, 0))
        d.rounded_rectangle([a[0] - pw * 0.35, b[1] + pw, a[0] + pw * 0.35, a[1] - pw], radius=pw * 0.3, fill=col)
        e.rounded_rectangle([a[0] - pw * 0.35, b[1] + pw, a[0] + pw * 0.35, a[1] - pw], radius=pw * 0.3, fill=mul(col, 0.8))
        ell(d, a[0], b[1], pw * 1.25, pw * 1.25, PROP_HI)
    cx, cy = (p00[0] + p11[0]) / 2, (proj(0, 1.95, z)[1])

    def label_pass():
        f = font(1.6 * s)
        d.text((cx, cy), label, font=f, anchor="mm", fill=(255, 255, 255), stroke_width=max(3, int(0.10 * s)), stroke_fill=mul(col, 0.20))
        e.text((cx, cy), label, font=f, anchor="mm", fill=(110, 110, 110))
        if slides:
            for sgn in (-1, 1):
                ax = cx + sgn * (p10[0] - p00[0]) * 0.39
                w = 0.2 * s
                d.polygon([(ax + sgn * w, cy), (ax - sgn * w * 0.4, cy - w), (ax - sgn * w * 0.4, cy + w)], fill=(255, 255, 255, 200))
    late.append(label_pass)


def draw_bumper(x, z, rad):
    sx, sy, dep = proj(x, 0, z)
    r = rad * ppm(dep)
    top = proj(x, 1.3, z)[1]
    ell(d, sx, sy, r, r * 0.62, PROP_LO)
    d.rectangle([sx - r, top, sx + r, sy], fill=PROP)
    e.rectangle([sx - r, top - r * 0.6, sx + r, sy + r * 0.6], fill=(0, 0, 0))
    d.rectangle([sx - r, top, sx - r * 0.45, sy], fill=PROP_HI)
    ell(d, sx, top, r, r * 0.62, PROP_HI)
    ell(d, sx, top, r * 0.5, r * 0.31, RAIL)
    ell(e, sx, top, r * 0.5, r * 0.31, mul(RAIL, 0.8))


def draw_barrier(x0, x1, z, label):
    a, b, c, dd = proj(x0, 0, z), proj(x1, 0, z), proj(x1, 1.6, z), proj(x0, 1.6, z)
    s = ppm(a[2])
    d.polygon([a[:2], b[:2], c[:2], dd[:2]], fill=(74, 40, 132))
    e.polygon([a[:2], b[:2], c[:2], dd[:2]], fill=(0, 0, 0))
    n = 6
    for i in range(n):  # crystal teeth along the top
        xa, xb = dd[0] + (c[0] - dd[0]) * i / n, dd[0] + (c[0] - dd[0]) * (i + 1) / n
        d.polygon([(xa, dd[1]), ((xa + xb) / 2, dd[1] - 0.5 * s), (xb, dd[1])], fill=(116, 70, 196))
    d.line([a[:2], b[:2]], fill=CRACK + (210,), width=max(2, int(0.07 * s)))
    e.line([a[:2], b[:2]], fill=mul(CRACK, 0.7), width=max(2, int(0.07 * s)))
    late.append(lambda: d.text(((a[0] + c[0]) / 2, (a[1] + c[1]) / 2), label, font=font(1.15 * s), anchor="mm", fill=(255, 255, 255),
                               stroke_width=max(2, int(0.08 * s)), stroke_fill=SPIRE_LO))


def draw_spire():
    zc = LEN + 1.4
    b_l, b_r = proj(-2.8, 0, zc), proj(2.8, 0, zc)
    m_l, m_r = proj(-1.4, 4.5, zc), proj(1.4, 4.5, zc)
    tip = proj(0.15, 6.3, zc)
    ridge_b, ridge_m = proj(-0.6, 0, zc - 0.8), proj(-0.25, 4.75, zc - 0.4)
    d.polygon([b_l[:2], ridge_b[:2], ridge_m[:2], tip[:2], m_l[:2]], fill=SPIRE_HI)
    d.polygon([ridge_b[:2], b_r[:2], m_r[:2], tip[:2], ridge_m[:2]], fill=SPIRE)
    e.polygon([b_l[:2], b_r[:2], m_r[:2], tip[:2], m_l[:2]], fill=(0, 0, 0))
    d.line([ridge_b[:2], ridge_m[:2], tip[:2]], fill=(120, 90, 190), width=3)
    s = ppm(b_l[2])
    for cx0 in (-3.8, 3.6, -2.7, 2.6):  # small crystals at the foot
        a, t, b = proj(cx0 - 0.55, 0, zc), proj(cx0 + 0.15, 1.7 if abs(cx0) > 3 else 2.5, zc), proj(cx0 + 0.65, 0, zc)
        d.polygon([a[:2], t[:2], b[:2]], fill=SPIRE_LO if cx0 < 0 else SPIRE)
    cracks = [[(-2.0, 0.2), (-1.3, 1.2), (-1.6, 2.0), (-1.0, 3.3)], [(1.9, 0.3), (1.3, 1.1), (1.6, 2.0), (1.0, 2.9), (1.1, 3.9)],
              [(-0.3, 0.2), (0.3, 0.8), (-0.1, 1.4)]]
    for line in cracks:
        pts = [proj(x, y, zc - 0.5)[:2] for x, y in line]
        d.line(pts, fill=CRACK, width=max(3, int(0.08 * s)), joint="curve")
        e.line(pts, fill=CRACK, width=max(3, int(0.10 * s)), joint="curve")
    hx, hy, _ = proj(0.05, 4.7, zc - 0.5)  # the heart
    hr = 0.36 * s
    d.polygon([(hx, hy - hr * 1.5), (hx + hr, hy), (hx, hy + hr * 1.5), (hx - hr, hy)], fill=(255, 200, 240))
    e.polygon([(hx, hy - hr * 1.6), (hx + hr * 1.1, hy), (hx, hy + hr * 1.6), (hx - hr * 1.1, hy)], fill=CRACK)
    nx, ny, _ = proj(0.0, 2.75, zc - 0.6)

    def number_pass():
        d.text((nx, ny), "2,480", font=font(1.7 * s), anchor="mm", fill=(255, 255, 255), stroke_width=max(3, int(0.12 * s)), stroke_fill=SPIRE_LO)
        e.text((nx, ny), "2,480", font=font(1.7 * s), anchor="mm", fill=(110, 80, 110))
    late.append(number_pass)


def draw_launcher():
    sx, sy, dep = proj(LX, 0, 0.2)
    s = ppm(dep)
    ell(d, sx, sy, 1.5 * s, 0.92 * s, (12, 14, 44))
    for i in range(10):  # the core's pips: 8 of 10 left
        a = math.pi * (0.06 + 0.88 * i / 9)
        px, py = sx - math.cos(a) * 1.26 * s, sy + math.sin(a) * 0.74 * s
        on = i not in (4, 5)
        ell(d, px, py, 0.13 * s, 0.11 * s, (255, 214, 120) if on else (52, 56, 100))
        if on:
            ell(e, px, py, 0.14 * s, 0.12 * s, (255, 170, 70))
    top = proj(LX, 0.9, 0.2)[1]
    m0, m1 = proj(LX, 1.0, 0.5), proj(LX, 1.4, 2.4)  # the barrel points up the lane
    bw0, bw1 = 0.44 * s, 0.38 * ppm(m1[2])
    barrel = [(m0[0] - bw0, m0[1]), (m0[0] + bw0, m0[1]), (m1[0] + bw1, m1[1]), (m1[0] - bw1, m1[1])]
    e.rectangle([sx - 0.98 * s, top - 0.62 * s, sx + 0.98 * s, sy + 0.62 * s], fill=(0, 0, 0))
    e.polygon(barrel, fill=(0, 0, 0))
    ell(d, sx, sy, 0.98 * s, 0.62 * s, (26, 78, 104))
    d.rectangle([sx - 0.98 * s, top, sx + 0.98 * s, sy], fill=(34, 104, 132))
    d.rectangle([sx - 0.98 * s, top, sx - 0.46 * s, sy], fill=(58, 150, 176))
    ell(d, sx, top, 0.98 * s, 0.62 * s, (70, 176, 200))
    d.polygon(barrel, fill=(44, 128, 156))
    d.polygon([barrel[0], (m0[0] - bw0 * 0.3, m0[1]), (m1[0] - bw1 * 0.3, m1[1]), barrel[3]], fill=(90, 196, 216))
    ell(d, m0[0], m0[1], bw0, bw0 * 0.6, (44, 128, 156))
    ell(d, m1[0], m1[1], bw1 * 1.15, bw1 * 0.72, (255, 222, 140))
    ell(e, m1[0], m1[1], bw1 * 1.3, bw1 * 0.85, (255, 190, 90))
    ell(d, m1[0], m1[1], bw1 * 0.6, bw1 * 0.38, (255, 255, 236))


# painter's order: far to near
items = [(proj(0, 0, LEN + 1.4)[2], draw_spire, ())]
items += [(proj(x, 0, z)[2], draw_wisp, (x, z, t)) for x, z, t in wisps]
items += [(proj(g[0], 0, g[1])[2], draw_gloom, g) for g in glooms]
items += [(proj(0, 0, g[2])[2] + 0.05, draw_gate, g) for g in GATES]
items += [(proj(x, 0, z)[2], draw_bumper, (x, z, r)) for x, z, r in BUMPERS]
items.append((proj(0, 0, BARRIER[2])[2], draw_barrier, BARRIER))
items.append((proj(LX, 0, 0.2)[2], draw_launcher, ()))
for _, fn, args in sorted(items, key=lambda it: -it[0]):
    fn(*args)

for x, y, z, k in sparks:
    sx, sy, dep = proj(x, y, z)
    r = rng.uniform(0.3, 0.62) * k * ppm(dep)
    c = rng.choice([(255, 244, 200), (255, 190, 240), (255, 255, 255)])
    star(d, sx, sy, r, c)
    star(e, sx, sy, r * 1.2, mul(c, 0.9))
for fn in late:
    fn()

# ---- bloom, vignette ---------------------------------------------------------------------------
for radius, gain in ((5, 0.80), (22, 0.62), (72, 0.50)):
    b = E.filter(ImageFilter.GaussianBlur(radius)).point(lambda v, g=gain: min(255, int(v * g)))
    base = ImageChops.screen(base, b)

vig = Image.new("L", (W // 8, H // 8), 0)
ImageDraw.Draw(vig).ellipse([-30, -40, W // 8 + 30, H // 8 + 40], fill=255)
vig = vig.filter(ImageFilter.GaussianBlur(26)).resize((W, H)).point(lambda v: 150 + v * 105 // 255)
base = ImageChops.multiply(base, Image.merge("RGB", [vig] * 3))

# ---- HUD (no bloom): safe-area inset at the top, touch targets of 48 dp or more -----------------
d = ImageDraw.Draw(base, "RGBA")
TOP = 96
d.rounded_rectangle([40, TOP, 318, TOP + 88], radius=44, fill=INK + (205,), outline=(255, 255, 255, 46), width=2)
d.text((179, TOP + 44), "LEVEL 12", font=font(40), anchor="mm", fill=(255, 255, 255))
d.rounded_rectangle([W - 136, TOP - 4, W - 40, TOP + 92], radius=30, fill=INK + (205,), outline=(255, 255, 255, 46), width=2)
for bx in (W - 104, W - 72):
    d.rounded_rectangle([bx - 8, TOP + 22, bx + 8, TOP + 66], radius=6, fill=(255, 255, 255))
bx0, bx1, by = 352, W - 168, TOP + 30
d.rounded_rectangle([bx0, by, bx1, by + 28], radius=14, fill=INK + (205,), outline=(255, 255, 255, 46), width=2)
d.rounded_rectangle([bx0 + 4, by + 4, bx0 + 4 + (bx1 - bx0 - 8) * 0.62, by + 24], radius=10, fill=CRACK)
d.rounded_rectangle([bx0 + 4, by + 4, bx0 + 4 + (bx1 - bx0 - 8) * 0.62, by + 13], radius=8, fill=(255, 170, 232))

cx0, cy0 = W - 338, H - 218  # the tide counter: how many wisps are on the causeway
d.rounded_rectangle([cx0, cy0, W - 40, cy0 + 96], radius=48, fill=INK + (205,), outline=WISP + (150,), width=3)
star(d, cx0 + 56, cy0 + 48, 26, WISP)
d.text((cx0 + 96, cy0 + 48), f"{len(wisps):,}", font=font(52), anchor="lm", fill=(255, 255, 255))

base.save(os.path.join(OUT, "concept_gameplay.png"))
base.resize((W // 2, H // 2), Image.LANCZOS).save(os.path.join(OUT, "concept_gameplay.small.png"))
print(f"wisps {len(wisps)}  glooms {len(glooms)}  -> {OUT}")
