#!/usr/bin/env python3
"""Build a crisp, high-contrast 320x176 PS3 XMB icon for STATION.
Uses the open-source DejaVu fonts installed only inside GitHub Actions.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter
S=3
W,H=320,176
im=Image.new("RGB",(W*S,H*S),(9,10,14))
p=im.load()
for y in range(H*S):
    for x in range(W*S):
        r=((x/(W*S))*.28+(y/(H*S))*.65)
        p[x,y]=(int(9+9*r),int(10+11*r),int(14+14*r))
glow=Image.new("RGBA",im.size,(0,0,0,0))
g=ImageDraw.Draw(glow)
g.ellipse((160*S,-90*S,430*S,180*S),fill=(210,12,33,105))
glow=glow.filter(ImageFilter.GaussianBlur(62*S))
im=Image.alpha_composite(im.convert("RGBA"),glow)
d=ImageDraw.Draw(im)
d.rounded_rectangle((13*S,16*S,85*S,159*S),radius=16*S,fill=(226,15,43,255))
d.rounded_rectangle((19*S,22*S,79*S,153*S),radius=13*S,fill=(28,14,22,255))
bold="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
regular="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
mark=ImageFont.truetype(bold,73*S)
brand=ImageFont.truetype(bold,27*S)
small=ImageFont.truetype(regular,11*S)
d.text((18*S,39*S),"S",font=mark,fill=(255,255,255,255))
d.rounded_rectangle((103*S,40*S,136*S,45*S),radius=2*S,fill=(237,20,47,255))
d.text((100*S,58*S),"STATION",font=brand,fill=(255,255,255,255),stroke_width=0)
d.text((102*S,100*S),"YOUR GAME LIBRARY",font=small,fill=(200,204,212,255))
d.line((103*S,128*S,295*S,128*S),fill=(79,81,93,255),width=1*S)
d.rounded_rectangle((102*S,137*S,151*S,158*S),radius=5*S,fill=(231,19,44,255))
d.text((111*S,139*S),"PS3",font=small,fill=(255,255,255,255))
d.rounded_rectangle((160*S,137*S,209*S,158*S),radius=5*S,fill=(47,49,61,255))
d.text((169*S,139*S),"PS2",font=small,fill=(255,255,255,255))
im=im.convert("RGB").resize((W,H),Image.LANCZOS)
out=Path(__file__).with_name("ICON0.PNG")
im.save(str(out),"PNG",optimize=True)
print("STATION premium XMB artwork:",out,out.stat().st_size)
