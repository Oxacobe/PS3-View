#!/usr/bin/env python3
"""STATION NOVA: premium 320x176 XMB artwork.
No binary font or artwork files are needed at build time. Generated with
Pillow at 4x resolution and downsampled for excellent PS3 icon clarity.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math

S=4
W,H=320,176
w,h=W*S,H*S
im=Image.new("RGB",(w,h))
px=im.load()
# Deep graphite gradient; just a hint of warm reflected light.
for y in range(h):
    for x in range(w):
        fx,fy=x/w,y/h
        radial=max(0.0,1.0-math.hypot(fx-.36,fy-.45)*1.7)
        halo=max(0.0,1.0-math.hypot(fx-.95,fy+.05)*2.4)
        noise=int(radial*11+halo*13)
        px[x,y]=(int(8+noise+fy*4),int(10+noise*.57+fy*3),int(16+noise*.35+fy*3))
im=im.convert("RGBA")
glow=Image.new("RGBA",im.size)
g=ImageDraw.Draw(glow)
g.ellipse((0*S,0*S,180*S,188*S),fill=(235,45,12,140))
glow=glow.filter(ImageFilter.GaussianBlur(31*S))
im=Image.alpha_composite(im,glow)
# Glowing rounded glass icon panel.
panel=Image.new("RGBA",im.size)
d=ImageDraw.Draw(panel)
d.rounded_rectangle((19*S,17*S,159*S,159*S),radius=25*S,fill=(14,16,23,248),
                    outline=(134,77,62,255),width=2*S)
d.rounded_rectangle((23*S,20*S,155*S,154*S),radius=21*S,
                    outline=(58,59,71,255),width=1*S)
d.rounded_rectangle((32*S,27*S,142*S,30*S),radius=S,fill=(92,99,118,100))
im=Image.alpha_composite(im,panel)
# A custom folded N: draw neon bloom and then polished orange ribbons.
shape=Image.new("RGBA",im.size)
d=ImageDraw.Draw(shape)
poly_left=[(44,119),(44,54),(63,44),(66,114)]
poly_mid=[(61,48),(80,37),(138,110),(116,126)]
poly_right=[(114,49),(136,42),(136,116),(117,127)]
for coords in (poly_left,poly_mid,poly_right):
    p=[(x*S,y*S) for x,y in coords]
    d.polygon(p,fill=(255,64,16,255))
bloom=shape.filter(ImageFilter.GaussianBlur(10*S))
bloom.putalpha(bloom.getchannel("A").point(lambda v:int(v*.45)))
im=Image.alpha_composite(im,bloom)
d=ImageDraw.Draw(im)
d.polygon([(x*S,y*S) for x,y in poly_left],fill=(241,54,14,255))
d.polygon([(x*S,y*S) for x,y in poly_mid],fill=(255,136,29,255))
d.polygon([(x*S,y*S) for x,y in poly_right],fill=(229,39,16,255))
d.line([(61*S,48*S),(80*S,37*S),(138*S,110*S)],fill=(255,202,92,250),width=2*S)
d.line([(43*S,118*S),(43*S,55*S)],fill=(255,125,62,220),width=2*S)
# Modern typography, crisp and intentionally spacious.
bold="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
reg="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
B=lambda n:ImageFont.truetype(bold,int(n*S))
R=lambda n:ImageFont.truetype(reg,int(n*S))
d.text((179*S,37*S),"NOVA",font=B(29),fill=(252,251,249,255),stroke_width=0)
d.rounded_rectangle((181*S,80*S,275*S,83*S),radius=S,fill=(255,104,33,255))
d.text((180*S,95*S),"THE GAME LIBRARY",font=R(10),fill=(205,205,210,255))
d.text((180*S,117*S),"PS3    /    PS2",font=B(11),fill=(249,126,55,255))
d.text((180*S,144*S),"STATION",font=R(8),fill=(120,123,136,255))
d.line((20*S,169*S,300*S,169*S),fill=(62,41,40,255),width=S)
im=im.convert("RGB").resize((W,H),Image.Resampling.LANCZOS)
out=Path(__file__).with_name("ICON0.PNG")
im.save(out,"PNG",optimize=True)
print("NOVA cinema XMB icon:",out,out.stat().st_size)
