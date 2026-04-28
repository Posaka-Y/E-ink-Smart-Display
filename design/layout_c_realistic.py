"""
Layout C v2 — U8g2 実フォント制約を反映したリアル版プレビュー
日本語は全て 16px (b16_b_t_japanese1 相当) で描画
Latin はサイズ自由 (logisoso 系)
"""
from PIL import Image, ImageDraw, ImageFont

W, H = 792, 272
WHITE, BLACK = (255,255,255), (0,0,0)
RED = (220, 30, 30)

JP_PATH = "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf"
SANS_B  = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
SANS_R  = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"

_fc = {}
def f(p, s):
    k = (p, s)
    if k not in _fc: _fc[k] = ImageFont.truetype(p, s)
    return _fc[k]

def is_cjk(c):
    cp = ord(c); return (0x3000<=cp<=0x9FFF) or (0xFF00<=cp<=0xFFEF)

JP_MAX = 16  # U8g2 同梱日本語フォントの実用上限

def mtext(d, xy, s, latin_path, sz, fill=BLACK):
    """日本語は JP_MAX で頭打ち、Latin はサイズ自由"""
    x, y = xy
    lf = f(latin_path, sz)
    jp_size = min(sz, JP_MAX)
    jf = f(JP_PATH, jp_size)
    for ch in s:
        if is_cjk(ch):
            # ベースラインを Latin に合わせて少し下げる
            d.text((x, y + (sz - jp_size)), ch, font=jf, fill=fill)
            x += jf.getbbox(ch)[2]
        else:
            d.text((x, y), ch, font=lf, fill=fill)
            x += lf.getbbox(ch)[2]
    return x - xy[0]

def dotted_v(d, x, y1, y2, c=BLACK, step=4):
    for y in range(y1, y2, step): d.point((x,y), fill=c)

def layout_realistic():
    img = Image.new("RGB", (W,H), WHITE)
    d = ImageDraw.Draw(img)
    CENTER = W // 2
    dotted_v(d, CENTER, 8, H-8, BLACK, 4)

    LP = 18
    # 1) 日付 (16px JP)
    mtext(d, (LP, 6), "水 4月22日", SANS_B, 16, fill=RED)
    # 2) 時計 (Latin、巨大 - 92pxはOK)
    d.text((LP, 32), "14:32", font=f(SANS_B, 92), fill=BLACK)
    # 3) 気温 (Latin 50px + ℃16px)
    d.text((LP, 148), "22.8", font=f(SANS_B, 50), fill=BLACK)
    mtext(d, (LP+158, 168), "℃", SANS_B, 16, fill=BLACK)
    # 4) 湿度・気圧 (16px ラベル + Latin 数字 26/24px)
    SX = 215
    mtext(d, (SX,      150), "湿度", SANS_B, 16, fill=BLACK)
    d.text((SX+50,     144), "48", font=f(SANS_B, 26), fill=BLACK)
    d.text((SX+95,     152), "%",  font=f(SANS_B, 18), fill=BLACK)
    mtext(d, (SX,      188), "気圧", SANS_B, 16, fill=BLACK)
    d.text((SX+50,     184), "1013", font=f(SANS_B, 24), fill=BLACK)
    mtext(d, (SX+118,  192), "hPa",   SANS_R, 12, fill=BLACK)
    # 5) フッター
    mtext(d, (LP,      240), "更新 14:30", SANS_B, 16, fill=BLACK)
    mtext(d, (LP+135,  240), "↓気圧 低下中", SANS_B, 16, fill=RED)

    # 右半分
    R = CENTER + 12
    mtext(d, (R, 6), "今日の予定", SANS_B, 16, fill=RED)
    # event 0
    d.text((R, 38), "15:30", font=f(SANS_B, 30), fill=RED)
    mtext(d, (R+110, 44), "1on1 田中さん", SANS_B, 16, fill=RED)
    mtext(d, (R+110, 78), "@ 会議室A",     SANS_B, 16, fill=BLACK)
    # event 1
    d.text((R,     120), "17:00", font=f(SANS_B, 24), fill=BLACK)
    mtext(d, (R+90, 124), "設計レビュー", SANS_B, 16, fill=BLACK)
    mtext(d, (R+90, 150), "Meet",        SANS_R, 10, fill=BLACK)
    # event 2
    d.text((R,     180), "19:00", font=f(SANS_B, 24), fill=BLACK)
    mtext(d, (R+90, 184), "ジム", SANS_B, 16, fill=BLACK)

    return img

OUT = "/sessions/happy-intelligent-gauss/mnt/E-ink smart display (1)"
real_img = layout_realistic()
real_img.save(f"{OUT}/layout_C_v2_realistic.png")

# 3色量子化版
pal = Image.new("P",(1,1))
pal.putpalette([255,255,255, 0,0,0, 220,30,30] + [0,0,0]*253)
real_img.quantize(palette=pal, dither=Image.Dither.FLOYDSTEINBERG).convert("RGB").save(
    f"{OUT}/layout_C_v2_realistic_3color.png")

# 比較: 設計意図 vs 実機予測
intent = Image.open(f"{OUT}/layout_C_v2_preview.png")
cmp = Image.new("RGB", (W+40, (H+50)*2 + 20), WHITE)
cd = ImageDraw.Draw(cmp)
mtext(cd, (20, 5), "設計意図 (PIL: 日本語22-24px 自由)", SANS_B, 16, fill=BLACK)
cd.rectangle([20, 28, 20+W+1, 28+H+1], outline=BLACK, width=1)
cmp.paste(intent, (21, 29))
mtext(cd, (20, 28+H+15), "実機予測 (U8g2: 日本語16px上限)", SANS_B, 16, fill=RED)
cd.rectangle([20, 28+H+38, 20+W+1, 28+H+38+H+1], outline=BLACK, width=1)
cmp.paste(real_img, (21, 28+H+39))
cmp.save(f"{OUT}/layout_C_v2_intent_vs_real.png")
print("Done")
