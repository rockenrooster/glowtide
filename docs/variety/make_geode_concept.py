"""make_geode_concept.py [base.png] [out.png]: a concept sheet for geodes (VARIETY_DESIGN 2, 3, 13, 14).

Paints two geodes (one plain, one shielded), their numbers and the boon tray over a real frame of the game, with
callouts beside it. It is a mock: judge where things sit, what is read first and what the numbers say, not the shapes.

  python -I -B docs/variety/make_geode_concept.py [captures/p2/clash_strip.png] [docs/variety/concept_geodes.png]

The base frame is the level-25 clash picture (scripts\\review.ps1 -Set p2 renders it, 1080 x 2400).
"""
import math
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
AQUA, AQUA_LIGHT, AQUA_DARK = (84, 242, 224), (190, 255, 246), (22, 110, 122)
GOLD, GOLD_LIGHT = (255, 198, 84), (255, 238, 176)
ROSE = (255, 96, 214)
INK = (14, 14, 44)
WALL, WALL_LIGHT, WALL_DARK = (74, 40, 132), (116, 70, 196), (36, 16, 72)


def font(size, game=True):
    if game:
        return ImageFont.truetype(os.path.join(ROOT, "assets", "font", "Roboto-Black.ttf"), size)
    for f in (r"C:\Windows\Fonts\segoeui.ttf", r"C:\Windows\Fonts\arial.ttf"):
        if os.path.exists(f):
            return ImageFont.truetype(f, size)
    return ImageFont.load_default()


def glow(img, centre, radius, colour, strength=0.75, squash=1.0):
    """Adds a soft light (screen blend) at centre."""
    layer = Image.new("RGB", img.size, (0, 0, 0))
    d = ImageDraw.Draw(layer)
    cx, cy = centre
    d.ellipse([cx - radius, cy - radius * squash, cx + radius, cy + radius * squash], fill=tuple(int(c * strength) for c in colour))
    layer = layer.filter(ImageFilter.GaussianBlur(radius * 0.45))
    return ImageChops.screen(img.convert("RGB"), layer).convert("RGBA")


def crystal(d, centre, r, turn=0.0):
    """A faceted shell: an uneven octagon, each facet its own tone."""
    cx, cy = centre
    pts = []
    for k in range(8):
        a = turn + k * math.pi / 4
        rr = r * (1.0 if k % 2 == 0 else 0.86)
        pts.append((cx + rr * math.cos(a), cy - rr * 1.08 * math.sin(a)))
    tones = [AQUA_LIGHT, AQUA, AQUA, AQUA_DARK, AQUA_DARK, AQUA_DARK, AQUA, AQUA_LIGHT]
    for k in range(8):
        p, q = pts[k], pts[(k + 1) % 8]
        t = tones[k]
        d.polygon([(cx, cy - r * 0.12), p, q], fill=t + (235,))
    d.line(pts + [pts[0]], fill=AQUA_LIGHT + (255,), width=3, joint="curve")
    # the inner window the icon sits in
    d.ellipse([cx - r * 0.52, cy - r * 0.60, cx + r * 0.52, cy + r * 0.44], fill=(18, 40, 70, 235))


def icon_lantern(d, centre, s):
    cx, cy = centre
    d.rounded_rectangle([cx - s * 0.30, cy - s * 0.30, cx + s * 0.30, cy + s * 0.34], radius=s * 0.12, fill=GOLD + (255,))
    d.rectangle([cx - s * 0.14, cy - s * 0.46, cx + s * 0.14, cy - s * 0.30], fill=GOLD_LIGHT + (255,))
    d.ellipse([cx - s * 0.12, cy - s * 0.08, cx + s * 0.12, cy + s * 0.16], fill=(255, 252, 232, 255))


def icon_chevrons(d, centre, s, colour=GOLD):
    cx, cy = centre
    for k in (-1, 1):
        y = cy + k * s * 0.17
        d.line([(cx - s * 0.32, y + s * 0.14), (cx, y - s * 0.16), (cx + s * 0.32, y + s * 0.14)], fill=colour + (255,), width=max(3, int(s * 0.15)), joint="curve")


def number(d, text, centre, size, stroke):
    f = font(size)
    d.text(centre, text, font=f, fill=(255, 255, 255, 255), anchor="mm", stroke_width=max(4, size // 9), stroke_fill=stroke + (255,))


def plate(d, centre, w, h):
    cx, cy = centre
    x0, x1 = cx - w / 2, cx + w / 2
    teeth = 7
    top = []
    for k in range(teeth * 2 + 1):
        x = x0 + w * k / (teeth * 2)
        top.append((x, cy - h + (0 if k % 2 else h * 0.22) - 10 * math.sin(math.pi * k / (teeth * 2))))
    d.polygon([(x0, cy), *top, (x1, cy)], fill=WALL + (245,))
    d.polygon([(x0, cy), (x0, cy - h * 0.55), (x1, cy - h * 0.55), (x1, cy)], fill=WALL_DARK + (160,))
    d.line(top, fill=WALL_LIGHT + (255,), width=4, joint="curve")


def chip(d, box, border):
    d.rounded_rectangle(box, radius=(box[3] - box[1]) // 2, fill=INK + (215,), outline=border + (190,), width=3)


def main(argv):
    base_path = argv[0] if len(argv) > 0 else os.path.join(ROOT, "captures", "p2", "clash_strip.png")
    out_path = argv[1] if len(argv) > 1 else os.path.join(ROOT, "docs", "variety", "concept_geodes.png")
    frame = Image.open(base_path).convert("RGBA")
    assert frame.size == (1080, 2400), frame.size

    a = (770, 1600)   # the plain geode: on the quiet side, below the x2 gate
    b = (800, 1960)   # the shielded geode, nearer the lantern
    # light pools on the lane, then the bodies
    frame = glow(frame, (a[0], a[1] + 92), 120, AQUA, 0.55, 0.42)
    frame = glow(frame, (b[0], b[1] + 104), 136, AQUA, 0.55, 0.42)
    frame = glow(frame, (b[0], b[1] + 150), 190, ROSE, 0.45, 0.22)
    d = ImageDraw.Draw(frame, "RGBA")
    crystal(d, a, 72, 0.25)
    icon_lantern(d, (a[0], a[1] - 6), 78)
    crystal(d, b, 80, -0.2)
    icon_chevrons(d, (b[0], b[1] - 6), 86)
    plate(d, (b[0], b[1] + 150), 300, 84)
    frame = glow(frame, a, 96, AQUA, 0.30)
    frame = glow(frame, b, 104, AQUA, 0.30)
    d = ImageDraw.Draw(frame, "RGBA")
    number(d, "120", (a[0], a[1] - 128), 76, AQUA_DARK)
    number(d, "90", (b[0], b[1] - 138), 80, AQUA_DARK)
    number(d, "300", (b[0], b[1] + 118), 68, WALL_DARK)
    # the boon tray, bottom left, mirroring the tide counter
    chip(d, [44, 2176, 300, 2280], GOLD)
    icon_chevrons(d, (104, 2228), 60)
    d.text((152, 2228), "\u00d71", font=font(52), fill=(255, 255, 255, 255), anchor="lm")

    # the sheet: the frame and callouts
    sheet = Image.new("RGB", (1080 + 760, 2400), (18, 16, 34))
    sheet.paste(frame.convert("RGB"), (0, 0))
    s = ImageDraw.Draw(sheet)
    title, body = font(46, False), font(34, False)
    notes = [
        ((a[0] + 84, a[1]), 1480, "GEODE", ["An upgrade that drifts down the lane toward", "the lantern. Pour the stream on it to crack", "it open. This one holds +1 LANTERN."]),
        ((a[0] + 70, a[1] - 128), 1180, "ITS HEALTH", ["Every wisp it takes is a wisp that did not", "fight. It counts down as you hit it."]),
        ((b[0] + 160, b[1] + 118), 1800, "SHIELD", ["Some carry a wall with its own health.", "Break the wall first."]),
        ((300, 2228), 2082, "BOON TRAY", ["What you hold for this level:", "more lanterns, a faster or heavier shot.", "It is gone when the level ends."]),
        ((560, 1250), 560, "THE CHOICE", ["The stream is busy on the left: the best", "gates and the fight are there. The geodes", "come down the quiet side. Leave the fight", "to take one, or let it slip past."]),
    ]
    for (px, py), ty, head, lines in notes:
        tx = 1120
        s.line([(px, py), (tx - 16, ty + 24)], fill=(120, 230, 220), width=3)
        s.ellipse([px - 7, py - 7, px + 7, py + 7], fill=(120, 230, 220))
        s.text((tx, ty), head, font=title, fill=(255, 214, 120))
        for k, line in enumerate(lines):
            s.text((tx, ty + 62 + k * 44), line, font=body, fill=(226, 226, 240))
    s.text((1120, 96), "Concept: geodes (upgrade pods)", font=font(44, False), fill=(255, 255, 255))
    for k, line in enumerate(["A mock painted over a real frame of level 25.", "The shapes are placeholders: where things", "sit and what the numbers say is the point."]):
        s.text((1120, 166 + k * 44), line, font=body, fill=(170, 170, 200))
    sheet = sheet.resize((sheet.width // 2, sheet.height // 2), Image.LANCZOS)
    sheet.save(out_path, optimize=True)
    print(out_path, sheet.size)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
