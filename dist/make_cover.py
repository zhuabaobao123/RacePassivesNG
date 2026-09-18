from PIL import Image, ImageDraw, ImageFont, ImageFilter
import random, os

W, H = 1280, 720
OUT = r"E:\RacePassivesNG\dist\cover.png"

# ---- background: cold vertical gradient ----
img = Image.new("RGB", (W, H), (10, 14, 20))
d = ImageDraw.Draw(img)
for y in range(H):
    t = y / H
    r = int(11 + 12 * t)
    g = int(16 + 16 * t)
    b = int(26 + 26 * t)
    d.line([(0, y), (W, y)], fill=(r, g, b))

# ---- soft glow behind title ----
glow = Image.new("L", (W, H), 0)
gd = ImageDraw.Draw(glow)
gd.ellipse([W / 2 - 430, -180, W / 2 + 430, 400], fill=70)
glow = glow.filter(ImageFilter.GaussianBlur(130))
img = Image.composite(Image.new("RGB", (W, H), (42, 74, 116)), img, glow)

# ---- vignette ----
vig = Image.new("L", (W, H), 0)
vd = ImageDraw.Draw(vig)
vd.ellipse([-260, -220, W + 260, H + 220], fill=255)
vig = vig.filter(ImageFilter.GaussianBlur(150))
img = Image.composite(img, Image.new("RGB", (W, H), (4, 6, 10)), vig)

d = ImageDraw.Draw(img)

# ---- fine grain ----
px = img.load()
random.seed(11)
for _ in range(90000):
    x = random.randrange(W); y = random.randrange(H)
    r, g, b = px[x, y]
    n = random.randint(-9, 9)
    px[x, y] = (max(0, min(255, r + n)), max(0, min(255, g + n)), max(0, min(255, b + n)))
d = ImageDraw.Draw(img)

# ---- double border ----
d.rectangle([34, 34, W - 34, H - 34], outline=(118, 138, 164), width=2)
d.rectangle([43, 43, W - 43, H - 43], outline=(64, 80, 100), width=1)

FD = "C:/Windows/Fonts/"
f_kicker = ImageFont.truetype(FD + "segoeuib.ttf", 20)
f_title = ImageFont.truetype(FD + "COPRGTB.ttf", 104)
f_sub = ImageFont.truetype(FD + "georgia.ttf", 27)
f_small = ImageFont.truetype(FD + "COPRGTB.ttf", 19)
f_race = ImageFont.truetype(FD + "COPRGTB.ttf", 20)
f_foot = ImageFont.truetype(FD + "georgia.ttf", 21)


def tracked(draw, y, text, font, fill, tracking=0, shadow=None):
    widths = [draw.textlength(ch, font=font) for ch in text]
    total = sum(widths) + tracking * (len(text) - 1)
    x = (W - total) / 2
    if shadow:
        sx = x + shadow[0]; sy = y + shadow[1]
        for ch, cw in zip(text, widths):
            draw.text((sx, sy), ch, font=font, fill=shadow[2])
            sx += cw + tracking
    for ch, cw in zip(text, widths):
        draw.text((x, y), ch, font=font, fill=fill)
        x += cw + tracking


# ---- text block ----
tracked(d, 118, "SKYRIM SPECIAL EDITION", f_kicker, (150, 176, 204), 7)
tracked(d, 168, "RACE PASSIVES", f_title, (243, 246, 251), 7, shadow=(3, 3, (5, 8, 14)))
tracked(d, 312, "Racial passives for every race — set the strength yourself",
        f_sub, (186, 201, 221), 0)

# ---- slider motif ----
ty = 424
x0, x1 = 316, 964
d.rounded_rectangle([x0, ty - 9, x1, ty + 9], radius=9,
                    fill=(36, 46, 60), outline=(92, 108, 128), width=2)
ratio = 0.75
fx = x0 + int((x1 - x0) * ratio)
d.rounded_rectangle([x0, ty - 9, fx, ty + 9], radius=9, fill=(92, 152, 208))
# 100% midpoint tick
mx = x0 + int((x1 - x0) * 0.5)
d.line([(mx, ty + 14), (mx, ty + 22)], fill=(92, 108, 128), width=2)
d.text((mx - 24, ty + 26), "100%", font=f_small, fill=(120, 138, 160))
# knob
d.ellipse([fx - 21, ty - 21, fx + 21, ty + 21], fill=(238, 244, 251),
          outline=(126, 156, 190), width=3)
# knob value label
kv = "150%"
kw = d.textlength(kv, font=f_small)
d.text((fx - kw / 2, ty - 58), kv, font=f_small, fill=(198, 220, 242))
d.text((x0 - 62, ty - 15), "0%", font=f_small, fill=(158, 178, 200))
d.text((x1 + 20, ty - 15), "200%", font=f_small, fill=(158, 178, 200))

# ---- race row ----
races = "NORD  ·  ORC  ·  BRETON  ·  DUNMER  ·  ALTMER  ·  KHAJIIT"
races2 = "ARGONIAN  ·  REDGUARD  ·  BOSMER  ·  IMPERIAL"
tracked(d, 520, races, f_race, (146, 164, 186), 2)
tracked(d, 556, races2, f_race, (146, 164, 186), 2)

# ---- footer ----
tracked(d, 616, "in-game menu  ·  cross-save settings  ·  no conflicts",
        f_foot, (128, 145, 165), 0)

img.save(OUT, "PNG")
print("saved", OUT, img.size)

# square card version (Nexus mod-list thumbnail)
side = 640
sq = Image.new("RGB", (side, side), (10, 14, 20))
sq.paste(img.crop((320, 0, 960, 640)), (0, 0))
spath = r"E:\RacePassivesNG\dist\cover_square.png"
sq.save(spath, "PNG")
print("saved", spath, sq.size)
