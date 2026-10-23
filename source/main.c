/*
 * STATION 0.3 — cover-first PS3 + PS2 Classics game library.
 * Non-destructive scanner. Explicit webMAN handoff for mounting games.
 * GPL-3.0-or-later app code; Tiny3D and PSL1GHT retain upstream licences.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/file.h>
#include <sysmodule/sysmodule.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>
#include <pngdec/pngdec.h>
#include <tiny3d.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/socket.h>
#include <net/net.h>
#include <fcntl.h>
#include "ui_font_generated.h"

#define MAX_GAMES 600
#define PER_PAGE 16
#define COLUMNS 8
#define GRID_COLS 4
#define FONT_W 512
#define FONT_H 256
#define TEX_BUDGET (20u * 1024u * 1024u)
#define RGBA(r,g,b,a) ((((u32)(r))<<24)|(((u32)(g))<<16)|(((u32)(b))<<8)|(u32)(a))

#define PAD_LEFT    0x8000
#define PAD_DOWN    0x4000
#define PAD_RIGHT   0x2000
#define PAD_UP      0x1000
#define PAD_CROSS   0x0040
#define PAD_SQUARE  0x0080
#define PAD_SELECT  0x0100
#define PAD_L2      0x0001
#define PAD_R2      0x0002
#define PAD_CIRCLE  0x0020
#define PAD_TRIANGLE 0x0010
#define PAD_R1      0x0008
#define PAD_L1      0x0004

typedef struct {
    char title[120];
    char path[480];
    char cover[512];
} Entry;

static Entry games[2][MAX_GAMES];
static int count[2]={0,0}, tab=1, chosen[2]={0,0}, offset[2]={0,0};
static int matching[2][MAX_GAMES],matches[2]={0,0};
static char query[24]="";
static int search_mode=0,keypos=0,exit_frames=0;
static int focus_row=0; /* screen rows 0,1 for PS3 and 2,3 for PS2 */
static const char keyboard[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int show_details=0, keep_running=1;
static int loaded_off[2]={-1,-1};
static u32 *texture_mem=0;
static u32 cover_offset[2*PER_PAGE];
static pngData cover_data[2*PER_PAGE];
static u32 font_offset=0;
static char message[128]="SELECT A GAME TO PLAY";
static int show_error=0;
static int launch_mode_manual=0;
static int network_ready=0;

static const u32 C_BG=RGBA(4,4,6,255);
static const u32 C_CARD=RGBA(22,23,30,255);
static const u32 C_EDGE=RGBA(50,51,63,255);
static const u32 C_ACCENT=RGBA(255,116,45,255);
static const u32 C_TEXT=RGBA(246,247,250,255);
static const u32 C_MUTED=RGBA(166,169,181,255);


/* Font atlas: GPU TINY3D_TEX_FORMAT_A8R8G8B8 uses AARRGGBB texels;
 * drawn quads use RRGGBBAA vertex colour. Do NOT swap these formats. */
static void rect(float x,float y,float w,float h,u32 color) {
    tiny3d_SetPolygon(TINY3D_QUADS);
    tiny3d_VertexPos(x,y,65535.f);tiny3d_VertexColor(color);
    tiny3d_VertexPos(x+w,y,65535.f);
    tiny3d_VertexPos(x+w,y+h,65535.f);
    tiny3d_VertexPos(x,y+h,65535.f);
    tiny3d_End();
}
static void font_setup(void) {
    u32 *out;
    u32 *pixels;
    int i,x,y;
    out=tiny3d_AllocTexture(FONT_W*FONT_H*4);
    if(!out)return;
    pixels=(u32 *)calloc(FONT_W*FONT_H,4);
    if(!pixels)return;
    for(i=0;i<96;i++)for(y=0;y<32;y++)for(x=0;x<24;x++) {
        unsigned char a=station_font_alpha[i*24*32+y*24+x];
        int tx=(i%16)*24+x,ty=(i/16)*32+y;
        pixels[ty*FONT_W+tx]=(((u32)a)<<24) | 0x00FFFFFFu;
    }
    RSX_MEMCPY(out,pixels,FONT_W*FONT_H*4);
    font_offset=tiny3d_TextureOffset(out);
    free(pixels);
}
static float line_width(const char *s,float sz,int maximum) {
    float w=0;
    int n=0;
    while(*s && n++<maximum) {
        unsigned char c=(unsigned char)*s++;
        if(c<32 || c>127)c='?';
        w+=sz*station_font_width[c-32]/32.f;
    }
    return w;
}
static void text(float x,float y,const char *s,int size,u32 color,int maximum) {
    float h=size*4.f;
    int n=0;
    if(!font_offset)return;
    tiny3d_SetTexture(0,font_offset,FONT_W,FONT_H,FONT_W*4,
                      TINY3D_TEX_FORMAT_A8R8G8B8,TEXTURE_LINEAR);
    while(*s && n++<maximum) {
        unsigned char c=(unsigned char)*s++;
        int glyph;
        float u,v,w=h*24.f/32.f;
        if(c<32 || c>127)c='?';
        glyph=c-32;
        u=(glyph%16)*24.f/FONT_W;
        v=(glyph/16)*32.f/FONT_H;
        tiny3d_SetPolygon(TINY3D_QUADS);
        tiny3d_VertexPos(x,y,65531.f);tiny3d_VertexColor(color);tiny3d_VertexTexture(u,v);
        tiny3d_VertexPos(x+w,y,65531.f);tiny3d_VertexTexture(u+24.f/FONT_W,v);
        tiny3d_VertexPos(x+w,y+h,65531.f);tiny3d_VertexTexture(u+24.f/FONT_W,v+32.f/FONT_H);
        tiny3d_VertexPos(x,y+h,65531.f);tiny3d_VertexTexture(u,v+32.f/FONT_H);
        tiny3d_End();
        x+=h*station_font_width[glyph]/32.f;
    }
}
/* All visual effects are Tiny3D geometry: no browser, shaders, oversized
 * textures or startup PNG decoding. Optimised for the PS3's 848x512 frame.
 * Inspired by the user-approved NOVA reference: dark cinematic hero,
 * orange-lit celestial horizon, slim sidebar, portrait-cover shelves. */
static void nova_backdrop(void) {
    int y,i;
    for(y=0;y<512;y+=10) {
        int v=5+(y*3)/512;
        rect(0,y,848,10,RGBA(v+1,v+2,v+6,255));
    }
    /* Featured sci-fi sky: warm light on horizon, ink-dark starfield. */
    for(y=0;y<107;y+=3) {
        int w=y<70?y/12:6;
        rect(176,y,672,3,RGBA(8+w*2,12+w,19+w,255));
    }
    for(i=0;i<54;i++){
        unsigned hash=(unsigned)(i*2654435761u+0x45bf41u);
        int xx=182+(int)(hash%651u);
        int yy=4+(int)((hash>>12)%96u);
        int glow=(hash>>26)&3;
        rect((float)xx,(float)yy,glow==3?2:1,1,
             glow==3?RGBA(255,186,115,255):RGBA(80+glow*30,95+glow*26,127+glow*21,255));
    }
    /* Celestial crescent clipped into hero right edge, no bitmap cost. */
    for(i=0;i<86;i+=2){
        float xx=(float)(721+i);
        float dx=(xx-769.f)/78.f;
        float inside=1.f-dx*dx;
        if(inside>0.f) {
            float h=79.f*sqrtf(inside);
            float top=41.f-h, bottom=41.f+h;
            int warm=(int)(44.f*(1.f-inside));
            if(top<0.f)top=0.f;
            if(bottom>104.f)bottom=104.f;
            if(bottom>top) {
                rect(xx,top,2.f,bottom-top,
                     RGBA(45+warm,40+warm/3,44+warm/4,255));
                rect(xx,top,2.f,2.f,RGBA(232,104,44,255));
            }
        }
    }
    /* Land silhouettes and cinematic orange horizon. */
    for(i=0;i<92;i++){
        int xx=180+i*8;
        int ridge=77+((i*13+i*i*7)%24);
        if(i>53&&i<76)ridge-=((i-53)*(76-i))/18;
        rect((float)xx,(float)ridge,9.f,(float)(107-ridge),
             RGBA(7+(i%4),9+(i%3),15+(i%3),255));
    }
    for(i=0;i<14;i++)
        rect(176,105+i/2,672,(float)(i%2?1:2),
             RGBA(47+i*3,22+i/2,17+i/3,255));
}
static void nova_sidebar(void) {
    int i;
    rect(0,0,174,497,RGBA(7,8,13,255));
    rect(172,0,2,497,RGBA(45,42,47,255));
    /* Brand mark: angular orange N with a subtle double line. */
    rect(22,21,7,40,RGBA(255,87,25,255));
    for(i=0;i<29;i++) {
        rect((float)(28+i),(float)(22+i),7,4,RGBA(255,120+i*3,31,255));
    }
    rect(60,21,7,40,RGBA(243,57,22,255));
    text(75,23,"NOVA",7,C_TEXT,4);
    text(77,55,"CINEMA LIBRARY",2,C_MUTED,18);
    rect(14,94,147,1,RGBA(48,43,45,255));
    /* The highlighted sections correspond to genuinely usable controls. */
    rect(0,115,4,37,C_ACCENT);
    rect(8,115,155,37,RGBA(37,24,23,255));
    text(24,125,"[+]  ALL GAMES",3,C_ACCENT,20);
    rect(15,167,140,1,RGBA(30,31,39,255));
    text(24,184,"L1  PLAYSTATION 3",3,
         tab==1?C_TEXT:C_MUTED,23);
    text(24,221,"R1  PS2 CLASSICS",3,
         tab==0?C_TEXT:C_MUTED,21);
    rect(15,266,140,1,RGBA(30,31,39,255));
    text(24,282,"[ ]  SEARCH",3,C_MUTED,19);
    text(24,320,"[+]  REFRESH",3,C_MUTED,19);
    rect(15,411,140,1,RGBA(42,35,40,255));
    text(24,431,"WEBMAN + HEN",3,C_MUTED,20);
    text(24,458,"PRESS X TO PLAY",3,C_ACCENT,23);
}
static int suffix(const char *s,const char *extension) {
    size_t n=strlen(s),e=strlen(extension);
    return n>=e && !strcasecmp(s+n-e,extension);
}
static int folder_exists(const char *p) {
    int fd;
    if(sysLv2FsOpenDir(p,&fd)==0) {sysLv2FsCloseDir(fd);return 1;}
    return 0;
}
static int cover_exists(const char *p) {
    FILE *f=fopen(p,"rb");
    if(!f)return 0;
    fclose(f);
    return 1;
}
static void find_cover(Entry *e,const char *dir,const char *name,int folder) {
    char base[256],c[620];
    size_t n;
    if(folder)return;
    snprintf(base,sizeof(base),"%s",name);
    n=strlen(base);
    if(suffix(base,".BIN.ENC")&&n>8)base[n-8]=0;
    else if(suffix(base,".ISO")&&n>4)base[n-4]=0;
    /* webMAN MOD accepts either full filename.PNG or basename.PNG. */
    if(cover_exists(e->cover))return;
    snprintf(c,sizeof(c),"%s/%s.PNG",dir,base);
    if(cover_exists(c)){snprintf(e->cover,sizeof(e->cover),"%s",c);return;}
    snprintf(c,sizeof(c),"%s/%s.png",dir,name);
    if(cover_exists(c)){snprintf(e->cover,sizeof(e->cover),"%s",c);return;}
    snprintf(c,sizeof(c),"%s/%s.png",dir,base);
    if(cover_exists(c)){snprintf(e->cover,sizeof(e->cover),"%s",c);return;}
    snprintf(c,sizeof(c),"/dev_hdd0/tmp/wmtmp/%s.PNG",base);
    if(cover_exists(c)){snprintf(e->cover,sizeof(e->cover),"%s",c);return;}
    snprintf(c,sizeof(c),"/dev_hdd0/tmp/wmtmp/%s.png",base);
    if(cover_exists(c)){snprintf(e->cover,sizeof(e->cover),"%s",c);return;}
}
static void push_game(int platform,const char *dir,const char *name,int isfolder) {
    Entry *e;
    size_t n,i;
    if(count[platform]>=MAX_GAMES || name[0]=='.') return;
    e=&games[platform][count[platform]++];
    memset(e,0,sizeof(*e));
    snprintf(e->path,sizeof(e->path),"%s/%s",dir,name);
    snprintf(e->title,sizeof(e->title),"%s",name);
    if(isfolder)
        snprintf(e->cover,sizeof(e->cover),"%s/%s/PS3_GAME/ICON0.PNG",dir,name);
    else
        snprintf(e->cover,sizeof(e->cover),"%s/%s.PNG",dir,name);

    find_cover(e,dir,name,isfolder);
    n=strlen(e->title);
    if(suffix(e->title,".BIN.ENC") && n>=8) e->title[n-8]=0;
    else if(suffix(e->title,".ISO") && n>=4) e->title[n-4]=0;
    for(i=0;e->title[i];i++) if(e->title[i]=='_') e->title[i]=' ';
}
static void scan_directory(int platform,const char *folder,int isfolder) {
    int fd;
    u64 read=0;
    sysFSDirent d;
    if(sysLv2FsOpenDir(folder,&fd)) return;
    for(;;) {
        char nested[620];
        memset(&d,0,sizeof(d));
        read=sizeof(d);
        if(sysLv2FsReadDir(fd,&d,&read)!=0 || read==0) break;
        if(d.d_name[0]=='.') continue;
        if(isfolder) {
            snprintf(nested,sizeof(nested),"%s/%s/PS3_GAME",folder,d.d_name);
            if(folder_exists(nested)) push_game(platform,folder,d.d_name,1);
        } else if((platform==0 && suffix(d.d_name,".BIN.ENC")) ||
                  (platform==1 && suffix(d.d_name,".ISO"))) {
            push_game(platform,folder,d.d_name,0);
        }
    }
    sysLv2FsCloseDir(fd);
}
static int cmp_games(const void *aa,const void *bb) {
    const Entry *a=(const Entry *)aa,*b=(const Entry *)bb;
    return strcasecmp(a->title,b->title);
}

static int contains_ci(const char *hay,const char *needle) {
    size_t n=strlen(needle),i,j;
    if(!n)return 1;
    for(i=0;hay[i];i++){
        for(j=0;j<n && hay[i+j];j++)
            if(toupper((unsigned char)hay[i+j])!=toupper((unsigned char)needle[j]))break;
        if(j==n)return 1;
    }
    return 0;
}
static void update_filter(void) {
    int p,i;
    for(p=0;p<2;p++){
        matches[p]=0;
        for(i=0;i<count[p];i++)if(contains_ci(games[p][i].title,query))
            matching[p][matches[p]++]=i;
        chosen[p]=0;offset[p]=0;
    }
    loaded_off[0]=loaded_off[1]=-1;
    focus_row=matches[1]?0:2;
    tab=matches[1]?1:0;
}
static void refresh(void) {
    count[0]=count[1]=0;
    scan_directory(0,"/dev_hdd0/PS2ISO",0);
    scan_directory(1,"/dev_hdd0/PS3ISO",0);
    scan_directory(1,"/dev_hdd0/GAMES",1);
    qsort(games[0],count[0],sizeof(Entry),cmp_games);
    qsort(games[1],count[1],sizeof(Entry),cmp_games);
    update_filter();show_details=0;
}
/* No more synchronous decode of 32 full PNGs at startup. */
static int safe_cover_mode=0;
static int stream_slot=0;
static int stream_delay=0;
static unsigned stream_bytes=0;
static void unload_covers(void) {
    int i;
    for(i=0;i<2*PER_PAGE;i++) {
        cover_offset[i]=0;
        memset(&cover_data[i],0,sizeof(pngData));
    }
}
static void begin_cover_stream(void) {
    unload_covers();
    stream_slot=0;stream_delay=8;stream_bytes=0;
    loaded_off[0]=offset[0];loaded_off[1]=offset[1];
}
static int valid_cover(const char *path) {
    FILE *f=fopen(path,"rb");
    unsigned char h[24];
    long sz;
    unsigned w,hgt;
    if(!f)return 0;
    fseek(f,0,SEEK_END);sz=ftell(f);rewind(f);
    if(sz<24||sz>1024*1024){fclose(f);return 0;}
    if(fread(h,1,sizeof(h),f)!=sizeof(h)){fclose(f);return 0;}
    fclose(f);
    if(memcmp(h,"\x89PNG\r\n\x1a\n",8))return 0;
    w=((unsigned)h[16]<<24)|((unsigned)h[17]<<16)|((unsigned)h[18]<<8)|h[19];
    hgt=((unsigned)h[20]<<24)|((unsigned)h[21]<<16)|((unsigned)h[22]<<8)|h[23];
    return w>=8 && hgt>=8 && w<=1024 && hgt<=1024;
}
static void stream_one_cover(void) {
    int slot,p,i,idx;
    unsigned n;
    pngData tmp;
    u32 *write;
    if(!texture_mem || stream_slot>=2*PER_PAGE)return;
    if(stream_delay>0){stream_delay--;return;}
    /* Load visible PS3 shelf first, then the PS2 shelf. */
    slot=(stream_slot<PER_PAGE)?PER_PAGE+stream_slot:stream_slot-PER_PAGE;
    stream_slot++;
    p=slot/PER_PAGE;i=slot%PER_PAGE;idx=offset[p]+i;
    if(idx>=matches[p])return;
    if(stream_bytes>3u*1024u*1024u)return;
    if(!valid_cover(games[p][matching[p][idx]].cover))return;
    memset(&tmp,0,sizeof(tmp));
    pngLoadFromFile(games[p][matching[p][idx]].cover,&tmp);
    if(!tmp.bmp_out)return;
    if(tmp.width>1024 || tmp.height>1024 || !tmp.pitch){
        free(tmp.bmp_out);return;
    }
    if(tmp.width>160 || tmp.height>160){
        unsigned mx=tmp.width>tmp.height?tmp.width:tmp.height;
        unsigned dw=tmp.width*160u/mx,dh=tmp.height*160u/mx;
        unsigned dpitch,x,y;
        unsigned char *small;
        if(dw==0||dh==0){free(tmp.bmp_out);return;}
        dpitch=(dw*4u+63u)&~63u;
        small=(unsigned char *)calloc((size_t)dpitch*dh,1);
        if(!small){free(tmp.bmp_out);return;}
        for(y=0;y<dh;y++)for(x=0;x<dw;x++){
            unsigned sx=x*tmp.width/dw,sy=y*tmp.height/dh;
            memcpy(small+y*dpitch+x*4u,
                (unsigned char *)tmp.bmp_out+sy*tmp.pitch+sx*4u,4);
        }
        free(tmp.bmp_out);
        tmp.bmp_out=small;tmp.width=dw;tmp.height=dh;tmp.pitch=dpitch;
    }
    n=(unsigned)tmp.pitch*tmp.height;
    if(n<16 || n>160u*256u*4u || stream_bytes+n+16>3u*1024u*1024u) {
        free(tmp.bmp_out);return;
    }
    write=texture_mem+stream_bytes/4;
    RSX_MEMCPY(write,tmp.bmp_out,n);
    cover_offset[slot]=tiny3d_TextureOffset(write);
    free(tmp.bmp_out);tmp.bmp_out=0;
    cover_data[slot]=tmp;
    stream_bytes+=(n+15u)&~15u;
}
static void move_selection(int p,int change) {
    int next;
    if(matches[p]<=0)return;
    next=chosen[p]+change;
    if(next<0)next=0;
    if(next>=matches[p])next=matches[p]-1;
    chosen[p]=next;
    offset[p]=(next/PER_PAGE)*PER_PAGE;
    focus_row=(p==1?0:2)+(next-offset[p])/COLUMNS;
}
static void navigate_vertical(int direction) {
    int old_p=focus_row<2?1:0;
    int column=(chosen[old_p]-offset[old_p])%COLUMNS;
    int row=focus_row+direction;
    while(row>=0 && row<4) {
        int p=row<2?1:0;
        int begin=offset[p]+(row%2)*COLUMNS;
        if(begin<matches[p]) {
            int target=begin+column;
            if(target>=matches[p])target=matches[p]-1;
            chosen[p]=target;
            tab=p;
            focus_row=row;
            return;
        }
        row+=direction;
    }
}
static void search_action(int pos) {
    size_t len=strlen(query);
    if(pos<36){
        if(len<sizeof(query)-2){query[len]=keyboard[pos];query[len+1]=0;update_filter();}
    }else if(pos==36){
        if(len<sizeof(query)-2){query[len]=' ';query[len+1]=0;update_filter();}
    }else if(pos==37){
        if(len){query[len-1]=0;update_filter();}
    }else if(pos==38){
        search_mode=0;
    }else if(pos==40){
        query[0]=0;update_filter();
    }
}
static void image(float x,float y,float w,float h,int slot) {
    const pngData *p=&cover_data[slot];
    float ratio,box;
    if(!cover_offset[slot] || !p->width || !p->height)return;
    ratio=(float)p->width/(float)p->height;
    box=w/h;
    if(ratio>box) {
        float nh=w/ratio;
        y+=(h-nh)*.5f;h=nh;
    } else {
        float nw=h*ratio;
        x+=(w-nw)*.5f;w=nw;
    }
    tiny3d_SetTexture(0,cover_offset[slot],p->width,p->height,p->pitch,
                      TINY3D_TEX_FORMAT_A8R8G8B8,TEXTURE_LINEAR);
    tiny3d_SetPolygon(TINY3D_QUADS);
    tiny3d_VertexPos(x,y,65534.f);tiny3d_VertexColor(0xffffffff);
    tiny3d_VertexTexture(0.f,0.f);
    tiny3d_VertexPos(x+w,y,65534.f);tiny3d_VertexTexture(.99f,0.f);
    tiny3d_VertexPos(x+w,y+h,65534.f);tiny3d_VertexTexture(.99f,.99f);
    tiny3d_VertexPos(x,y+h,65534.f);tiny3d_VertexTexture(0.f,.99f);
    tiny3d_End();
}


static void title_label(char *dest,size_t capacity,const char *src) {
    size_t i=0;
    if(!capacity)return;
    dest[0]=0;
    while(src[i] && i+1<capacity){
        dest[i]=src[i];
        dest[i+1]=0;
        if(line_width(dest,8.f,(int)i+1)>67.f)break;
        i++;
    }
    if(src[i]) {
        while(i>0 && line_width(dest,8.f,(int)i)>52.f)dest[--i]=0;
        if(i+4<capacity)strcat(dest,"...");
    }
}
static void draw_shelf_row(int p,int row,int y) {
    int i;
    const int active_row=(p==1?0:2)+row;
    for(i=0;i<COLUMNS;i++){
        int at=offset[p]+row*COLUMNS+i;
        int slot=p*PER_PAGE+row*COLUMNS+i;
        float x=185.f+i*82.f,top=(float)y;
        int selected=focus_row==active_row && chosen[p]==at;
        char label[42];
        if(at>=matches[p])break;
        /* Multi-layer illuminated focus + subtle drop shadow. */
        if(selected){
            rect(x-5,top-5,80,87,RGBA(83,41,26,255));
            rect(x-4,top-4,78,85,RGBA(252,95,30,255));
            rect(x-2,top-2,74,81,RGBA(255,185,64,255));
        } else {
            rect(x+2,top+3,71,81,RGBA(3,3,6,255));
            rect(x-1,top-1,72,79,RGBA(63,57,61,255));
        }
        rect(x,top,70,77,RGBA(25,27,36,255));
        if(cover_offset[slot]) image(x+1,top+1,68,75,slot);
        else {
            rect(x+11,top+5,48,67,RGBA(36,39,49,255));
            rect(x+11,top+5,3,67,p==1?C_ACCENT:RGBA(95,143,185,255));
            text(x+20,top+24,p==1?"PS3":"PS2",5,C_TEXT,3);
        }
        /* Integrated title strip, as on the user-supplied reference. */
        rect(x,top+64,70,13,RGBA(8,8,12,242));
        title_label(label,sizeof(label),games[p][matching[p][at]].title);
        text(x+3,top+65,label,2,selected?RGBA(255,211,165,255):C_TEXT,23);
    }
}
static void draw_platform(int p,int header){
    char counter[40];
    rect(184,header+1,642,1,RGBA(55,53,59,255));
    rect(186,header+2,3,12,p==1?C_ACCENT:RGBA(123,143,202,255));
    text(197,header+1,p==1?"PLAYSTATION 3":"PS2 CLASSICS",4,C_TEXT,19);
    snprintf(counter,sizeof(counter),"%d TITLES",matches[p]);
    text(745,header+3,counter,2,C_MUTED,12);
    draw_shelf_row(p,0,header+20);
    draw_shelf_row(p,1,header+105);
    if(matches[p]==0)text(195,header+51,"NO GAMES IN THIS LIBRARY",3,C_MUTED,40);
}
static void draw_search(void) {
    int i;
    char hint[64];
    rect(51,49,746,417,RGBA(6,7,11,255));
    rect(51,49,746,4,C_ACCENT);
    rect(51,462,746,4,C_ACCENT);
    text(85,67,"SEARCH YOUR LIBRARY",7,C_TEXT,28);
    text(86,104,"FIND A GAME BY NAME",4,C_MUTED,32);
    rect(84,139,679,48,RGBA(31,32,40,255));
    text(100,146,query[0]?query:"TYPE USING YOUR CONTROLLER",6,query[0]?C_TEXT:C_MUTED,24);
    for(i=0;i<36;i++){
        float x=95+(i%9)*74.f,y=203+(i/9)*46.f;
        char v[2]={keyboard[i],0};
        if(keypos==i)rect(x-2,y-2,61,41,C_ACCENT);
        rect(x,y,57,37,keypos==i?RGBA(84,18,33,255):RGBA(37,38,47,255));
        text(x+20,y+7,v,6,C_TEXT,2);
    }
    {
      const char *actions[5]={"SPACE","DELETE","DONE","ONLINE","CLEAR"};
      for(i=0;i<5;i++){
          float x=95+i*133.f,y=394.f;
          if(keypos==36+i)rect(x-2,y-2,127,44,C_ACCENT);
          rect(x,y,123,40,keypos==36+i?RGBA(84,18,33,255):RGBA(37,38,47,255));
          text(x+9,y+11,actions[i],4,C_TEXT,9);
      }
    }
    snprintf(hint,sizeof(hint),"%d PS3 / %d PS2 RESULTS",matches[1],matches[0]);
    text(95,448,hint,3,C_MUTED,34);
}
static void draw_message(void){
    rect(188,175,642,147,RGBA(5,6,10,255));
    rect(194,179,630,139,RGBA(24,25,34,255));
    rect(194,179,630,4,show_error?RGBA(222,71,51,255):C_ACCENT);
    text(215,195,show_error?"COULD NOT COMPLETE":"LAUNCH HANDOFF",6,C_TEXT,33);
    text(215,242,message,4,C_TEXT,85);
    text(216,294,exit_frames?
         "IF NEEDED, SELECT GAME ICON FROM XMB":
         "CIRCLE TO RETURN TO LIBRARY",3,C_MUTED,80);
}
static void render(void) {
    char selected_title[72],count_label[32];
    const Entry *active=NULL;
    tiny3d_Clear(0xFF05070Bu,TINY3D_CLEAR_ALL);
    tiny3d_AlphaTest(1,0x10,TINY3D_ALPHA_FUNC_GEQUAL);
    tiny3d_BlendFunc(1,TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
        TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
        TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
    tiny3d_Project2D();
    nova_backdrop();
    nova_sidebar();
    if(matches[tab] && chosen[tab]<matches[tab])
        active=&games[tab][matching[tab][chosen[tab]]];
    if(active)snprintf(selected_title,sizeof(selected_title),"%.34s",active->title);
    else snprintf(selected_title,sizeof(selected_title),"DISCOVER YOUR COLLECTION");
    rect(190,12,4,48,C_ACCENT);
    text(209,13,"NOW SELECTED",3,RGBA(239,126,75,255),16);
    text(209,34,selected_title,6,C_TEXT,34);
    text(210,68,tab==1?"PS3  /  WEBMAN":"PS2  /  CLASSICS",3,C_MUTED,24);
    text(390,69,"X  READY TO LAUNCH",3,C_ACCENT,30);
    snprintf(count_label,sizeof(count_label),"%d GAMES",count[0]+count[1]);
    rect(749,9,84,20,RGBA(25,23,31,255));
    text(758,11,count_label,2,C_TEXT,14);
    if(safe_cover_mode)text(652,88,"ART SAFE MODE",2,C_ACCENT,18);
    draw_platform(1,105);
    draw_platform(0,302);
    /* Compact console legend; intentionally not interactive decoration. */
    rect(0,497,848,15,RGBA(8,10,16,255));
    rect(174,497,674,1,RGBA(61,43,36,255));
    text(185,498,"X  PLAY",2,RGBA(85,189,243,255),12);
    text(282,498,"R2+X MOUNT ONLY",2,C_MUTED,23);
    text(462,498,"SQUARE SEARCH",2,RGBA(223,104,192,255),18);
    text(610,498,"TRIANGLE RESCAN",2,RGBA(75,206,151,255),20);
    text(758,498,"O EXIT",2,C_MUTED,8);
    if(search_mode)draw_search();
    if(show_details)draw_message();
    tiny3d_Flip();
}
/* webMAN runs on the console's localhost interface. We never touch the
 * PS2 Classics encrypted files or rewrite system game folders ourselves.
 * Responses are checked before reporting a successful hand-off. */
static int webman_http(const char *endpoint) {
    struct sockaddr_in server;
    struct timeval t={3,0};
    int fd;
    int sent,total,received;
    char req[1950],response[256];
    size_t len;
    if(!network_ready) network_ready=(netInitialize()>=0);
    if(!network_ready)return 0;
    fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd<0)return 0;
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&t,sizeof(t));
    setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t));
    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(0x7f000001);
    server.sin_port=htons(80);
    if(connect(fd,(struct sockaddr *)&server,sizeof(server))<0){close(fd);return 0;}
    snprintf(req,sizeof(req),"GET %s HTTP/1.0\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n",endpoint);
    len=strlen(req);
    total=0;
    while(total<(int)len) {
        sent=send(fd,req+total,len-total,0);
        if(sent<=0){close(fd);return 0;}
        total+=sent;
    }
    received=recv(fd,response,sizeof(response)-1,0);
    close(fd);
    if(received<=0)return 0;
    response[received]=0;
    return strstr(response," 200 ") || strstr(response," 202 ") || strstr(response," 302 ");
}
/* Queue a two-command webMAN request on the webMAN server thread.
 * This HTTP request is intentionally NOT awaited: its first command waits
 * until VSH returns to XMB, which cannot happen while NOVA is still open.
 *
 * Sequence is document-supported: /wait.ps3?xmb;/play.ps3[?TITLEID]
 * The socket is sent in full and closed; no onxmb.bat or startup plugin.
 * HTTP accepted is not proof the game started; on failure users can use
 * the already-mounted game from XMB.
 */
static int webman_queue_xmb_launch(const char *endpoint) {
    struct sockaddr_in server;
    struct timeval t={2,0};
    char req[512];
    size_t len;
    int fd,total=0,n;
    if(!network_ready)return 0;
    fd=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd<0)return 0;
    setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&t,sizeof(t));
    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=htonl(0x7f000001);
    server.sin_port=htons(80);
    if(connect(fd,(struct sockaddr *)&server,sizeof(server))<0) {
        close(fd);return 0;
    }
    snprintf(req,sizeof(req),
        "GET %s HTTP/1.0\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n",endpoint);
    len=strlen(req);
    while(total<(int)len) {
        n=send(fd,req+total,len-total,0);
        if(n<=0){close(fd);return 0;}
        total+=n;
    }
    /* webMAN has received a complete request. Deliberately do not recv()
     * because /wait.ps3?xmb must execute AFTER this process ends. */
    shutdown(fd,SHUT_WR);
    close(fd);
    return 1;
}
static int sfo_has_words(const char *path,const char *words) {
    unsigned char b[8192];
    size_t n,i,j,k=strlen(words);
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    n=fread(b,1,sizeof(b),f);
    fclose(f);
    if(n<k)return 0;
    for(i=0;i+k<=n;i++){
        for(j=0;j<k;j++)
            if(toupper(b[i+j])!=toupper((unsigned char)words[j]))break;
        if(j==k)return 1;
    }
    return 0;
}
static void find_ps2_classics_id(char *out,size_t cap) {
    int fd;
    u64 read;
    sysFSDirent e;
    snprintf(out,cap,"PS2U10000"); /* Official Classics Placeholder title ID */
    /* If the standard location exists, do not waste time scanning. */
    if(folder_exists("/dev_hdd0/game/PS2U10000"))return;
    if(sysLv2FsOpenDir("/dev_hdd0/game",&fd)!=0)return;
    for(;;) {
        char sfo[560];
        int j,valid=1;
        memset(&e,0,sizeof(e));read=sizeof(e);
        if(sysLv2FsReadDir(fd,&e,&read)!=0 || read==0)break;
        if(strlen(e.d_name)!=9)continue;
        for(j=0;j<9;j++)if(!isalnum((unsigned char)e.d_name[j]))valid=0;
        if(!valid)continue;
        snprintf(sfo,sizeof(sfo),"/dev_hdd0/game/%s/PARAM.SFO",e.d_name);
        if(sfo_has_words(sfo,"PS2 Classics Launcher") ||
           sfo_has_words(sfo,"PS2 Classics Placeholder")) {
            snprintf(out,cap,"%s",e.d_name);
            break;
        }
    }
    sysLv2FsCloseDir(fd);
}
/* NOVA 1.5 request-file handoff.
 *
 * Upstream webMAN-MOD include/poll/poll.h polls WM_REQUEST_FILE only when
 * not in a game. include/www/www_client.h reads the file as an HTTP
 * request, explicitly retries /play.ps3 while still in-game, then
 * unlinks WM_REQUEST_FILE before executing the command.
 *
 * This is a real plugin-side deferred queue, unlike the earlier NOVA
 * fire-and-forget HTTP requests. No background NOVA process, new plugin,
 * EBOOT syscall or persistent onxmb.bat is installed.
 *
 * NEVER truncate an existing wm_request belonging to webMAN/another app.
 * O_EXCL creates one request or does nothing. Deliberately don't remove
 * request from NOVA; webMAN owns its consumption and deletion.
 */
#define NOVA_WM_REQUEST_FILE "/dev_hdd0/tmp/wm_request"
static int schedule_webman_xmb_play(const char *command) {
    int fd;
    size_t n;
    ssize_t sent;
    if(!command || strncmp(command,"/play.ps3",9)!=0)return 0;
    /* IMPORTANT upstream parser detail: webMAN www_client.h accepts a
     * bare /play.ps3 request and PREPENDS the GET verb automatically.
     * It also rewrites embedded spaces to '+' before parsing. A complete
     * HTTP/1.0 request in wm_request could mistakenly become part of the
     * URL, so write ONLY the documented bare endpoint here. */
    n=strlen(command);
    if(n<=5 || n>=80)return 0;
    fd=open(NOVA_WM_REQUEST_FILE,O_WRONLY|O_CREAT|O_EXCL,0644);
    if(fd<0)return 0;
    sent=write(fd,command,n);
    if(sent!=(ssize_t)n) {
        close(fd);
        unlink(NOVA_WM_REQUEST_FILE); /* ours: O_EXCL succeeded */
        return 0;
    }
    if(close(fd)!=0) {
        unlink(NOVA_WM_REQUEST_FILE);
        return 0;
    }
    return 1;
}
static int encode_game_path(const char *path,char *encoded,size_t capacity) {
    static const char hex[]="0123456789ABCDEF";
    size_t o=0,i;
    if(strncmp(path,"/dev_hdd0/",10))return 0;
    for(i=0;path[i];i++) {
        unsigned char c=(unsigned char)path[i];
        if(o+4>=capacity)return 0;
        if(isalnum(c)||c=='/'||c=='-'||c=='_'||c=='.'||c=='('||c==')'||c=='['||c==']')encoded[o++]=c;
        else {
            encoded[o++]='%';
            encoded[o++]=hex[c>>4];
            encoded[o++]=hex[c&15];
        }
    }
    encoded[o]=0;
    return 1;
}

/*
 * Direct-launch experiment based on PSL1GHT's sysProcessExitSpawn2 API,
 * as used by PS3LoadX. The PS3 can execute only one foreground game;
 * a successful syscall replaces NOVA with the target process.
 *
 * Important: commercial PS3 disc titles may require special XMB launch
 * context which direct EBOOT spawning might not reproduce. Failed calls
 * return a code and leave NOVA running, with manual XMB fallback.
 */
static int has_boot_file(const char *path) {
    FILE *f=fopen(path,"rb");
    if(!f)return 0;
    fclose(f);
    return 1;
}
static void log_launch_stage(const char *stage,const Entry *game,const char *target) {
    FILE *f=fopen("/dev_hdd0/tmp/station_nova_launch.log","wb");
    if(!f)return;
    fprintf(f,"STATION NOVA 1.5\nplatform=%s\nstage=%s\n",
            tab==1?"PS3":"PS2",stage);
    fprintf(f,"game=%.119s\n",game?game->title:"<none>");
    fprintf(f,"source=%.479s\n",game?game->path:"<none>");
    fprintf(f,"target=%s\n",target?target:"<none>");
    fclose(f);
}
/* STATION NOVA CINEMA 1.1: verified two-stage handoff.
 * Always mount through webMAN's in-game-safe /mount_ps3 endpoint.
 * Then return to XMB. PS3 starts from mounted DISC icon.
 * PS2 Classics starts from installed Classics Placeholder icon.
 * No EBOOT spawning, no onxmb scripts, no launcher-path guesses.
 */
static void clear_legacy_pending_requests(void) {
    /* Files created solely by NOVA 1.0; avoid stale automatic XMB launches.
     * No game images, settings, saves or other plugins are modified. */
    unlink("/dev_hdd0/tmp/nova_xmb_ps3.once");
    unlink("/dev_hdd0/tmp/nova_xmb_ps2.once");
}
/* One press: mount known-good route, hand an XMB wait+play request
 * to webMAN, exit normally. Hold R2+X to mount only as verified fallback. */
/* Mount exactly like console-verified NOVA 1.2, then ask webMAN
 * to run one temporary script that waits for XMB and selects the mounted
 * PS3 disc icon or installed PS2 Classics Launcher. R2+X stays mount-only. */
/* NOVA 1.4: preserve original proven mount first.
 *
 * webMAN source: include/cmd/pad_combo_play.h maps
 * /play.ps3/<game-path> to the native force-autoplay path (ap_param=2).
 * include/www/www_client.h then calls auto_play(param, 1).
 * include/mount/mount.h auto_play waits for IS_INGAME to clear (15s max)
 * and uses XMB's disc icon / PS2 Classics Launcher as appropriate.
 *
 * A second webMAN autoplay request is not the same as the proven
 * /mount_ps3 call; it may invoke webMAN's mount logic again. We keep
 * the first mount unchanged and give webMAN the full selected path.
 *
 * No direct EBOOT process spawn and no scripts or startup hooks.
 * Hold R2+X to skip this experimental action, retaining mount-only.
 */
/* NOVA 1.5: original known-good mount first, then XMB-only file queue.
 * PS3: webMAN starts mounted disc.
 * PS2: webMAN starts the installed PS2 Classics Launcher (title ID from
 * scanning, not an EBOOT path). R2+X remains mount-only fallback.
 */
static void launch_selected(void) {
    char encoded[1500],command[1600],play_cmd[80],titleid[16]={0};
    Entry *game;
    int queued;
    if(matches[tab]<=0)return;
    game=&games[tab][matching[tab][chosen[tab]]];
    if(!encode_game_path(game->path,encoded,sizeof(encoded))) {
        snprintf(message,sizeof(message),"INVALID GAME PATH");
        show_error=show_details=1;return;
    }

    /* webMAN owns its global request queue. Never overwrite, remount over,
     * or race a pending command left by another app or an old attempt. */
    if(!launch_mode_manual) {
        FILE *pending=fopen(NOVA_WM_REQUEST_FILE,"rb");
        if(pending) {
            fclose(pending);
            snprintf(message,sizeof(message),
                "WEBMAN ALREADY HAS A PENDING XMB COMMAND");
            show_error=show_details=1;
            log_launch_stage("wm_request_pending_collision",game,
                             NOVA_WM_REQUEST_FILE);
            return;
        }
    }

    /* DO NOT MODIFY: user-confirmed webMAN mount from NOVA 1.1-1.4. */
    snprintf(command,sizeof(command),"/mount_ps3%s",encoded);
    snprintf(message,sizeof(message),"WEBMAN MOUNTING YOUR GAME...");
    show_details=1;show_error=0;exit_frames=0;
    log_launch_stage("mount_request",game,NULL);
    render();
    if(!webman_http(command)) {
        snprintf(message,sizeof(message),"WEBMAN MOUNT FAILED - CHECK HEN");
        show_error=show_details=1;
        log_launch_stage("mount_http_failure",game,NULL);
        return;
    }

    if(launch_mode_manual) {
        snprintf(message,sizeof(message),tab==1?
            "PS3 MOUNTED - OPEN DISC IN XMB":
            "PS2 MOUNTED - OPEN CLASSICS IN XMB");
        show_details=1;show_error=0;exit_frames=65;
        log_launch_stage("mount_only_user_override",game,NULL);
        return;
    }

    if(tab==1) {
        snprintf(play_cmd,sizeof(play_cmd),"/play.ps3");
    } else {
        /* Use webMAN's installed-application XMB launch path, not the
         * raw PS2 emulator EBOOT or the encrypted image itself. */
        find_ps2_classics_id(titleid,sizeof(titleid));
        snprintf(play_cmd,sizeof(play_cmd),"/play.ps3?%s",titleid);
    }

    /* Queue request AFTER the successful mount and BEFORE normal exit.
     * Plugin's poll thread picks it up on return to XMB, and deletes it
     * on dispatch. File collision = safe manual fallback, not overwrite. */
    log_launch_stage("wm_request_create_attempt",game,play_cmd);
    queued=schedule_webman_xmb_play(play_cmd);
    if(queued) {
        snprintf(message,sizeof(message),tab==1?
            "PS3: XMB AUTO-PLAY REQUEST PENDING":
            "PS2: CLASSICS PLAY REQUEST PENDING");
        log_launch_stage("wm_request_written_not_yet_executed",game,play_cmd);
    } else {
        snprintf(message,sizeof(message),tab==1?
            "PS3 MOUNTED - USE DISC ICON":
            "PS2 MOUNTED - USE CLASSICS ICON");
        log_launch_stage("wm_request_exists_or_write_failed",game,play_cmd);
    }
    show_details=1;show_error=0;
    /* Return XMB fast; the webMAN polling thread, not NOVA, launches. */
    exit_frames=8;
}
static void online_lookup(void) {
    char path[640];
    size_t j,len;
    const char *base="/browser.ps3?https://www.mobygames.com/search/?q=";
    snprintf(path,sizeof(path),"%s",base);
    len=strlen(path);
    for(j=0;query[j] && len+4<sizeof(path);j++){
        unsigned char c=(unsigned char)query[j];
        if(isalnum(c))path[len++]=c;
        else if(c==' ')path[len++]='+';
    }
    path[len]=0;
    search_mode=0;
    snprintf(message,sizeof(message),"OPENING ONLINE GAME INFORMATION...");
    show_details=1;show_error=0;render();
    if(webman_http(path)){exit_frames=90;return;}
    snprintf(message,sizeof(message),"ONLINE SEARCH NEEDS WEBMAN AND PS3 BROWSER");
    show_error=1;show_details=1;
}
static void callback(u64 status,u64 param,void *userdata) {
    (void)param;(void)userdata;
    if(status==SYSUTIL_EXIT_GAME)keep_running=0;
}

static void search_keys(unsigned pressed) {
    int row=keypos/9;
    if(keypos>=36)row=4;
    if(pressed&PAD_LEFT){
        if(keypos>=36 && keypos>36)keypos--;
        else if(keypos%9>0)keypos--;
    }
    if(pressed&PAD_RIGHT){
        if(keypos>=36 && keypos<40)keypos++;
        else if(keypos%9<8)keypos++;
    }
    if(pressed&PAD_UP){
        if(keypos>=36)keypos=27+(keypos-36)*2;
        else if(row>0)keypos-=9;
    }
    if(pressed&PAD_DOWN){
        if(keypos<27)keypos+=9;
        else if(keypos<36)keypos=36+(keypos%9)/2;
    }
    if(pressed&PAD_CROSS){
        if(keypos==39)online_lookup();
        else search_action(keypos);
    }
    if(pressed&PAD_CIRCLE || pressed&PAD_SQUARE)search_mode=0;
}
int main(void) {
    padInfo info;
    padData pd;
    unsigned previous=0;
    sysModuleLoad(SYSMODULE_FS);
    sysModuleLoad(SYSMODULE_PNGDEC);
    tiny3d_Init(1024*1024);
    texture_mem=tiny3d_AllocTexture(TEX_BUDGET);
    font_setup();
    ioPadInit(7);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0,callback,NULL);
    clear_legacy_pending_requests();
    unlink("/dev_hdd0/tmp/nova_13_play.bat"); /* NOVA-owned legacy script only */
    refresh();
    while(keep_running) {
        unsigned buttons=0,pressed;
        sysUtilCheckCallback();
        ioPadGetInfo(&info);
        if(info.status[0]){
            ioPadGetData(0,&pd);
            buttons=((unsigned)pd.button[2]<<8)|pd.button[3];
        }
        pressed=buttons & ~previous;
        previous=buttons;
        /* Emergency safe mode: hold L2 when opening the application to
         * avoid all artwork decodes if a particular cover is malformed. */
        if(buttons & PAD_L2)safe_cover_mode=1;
        if(exit_frames>0){
            --exit_frames;
            if(exit_frames==0)keep_running=0;
        }else if(search_mode){
            search_keys(pressed);
        }else if(show_details){
            if(pressed&PAD_CIRCLE) {show_details=show_error=0;}
        }else{
            if(pressed&PAD_CIRCLE)keep_running=0;
            if(pressed&PAD_L1 && matches[1]>0){tab=1;focus_row=(chosen[1]-offset[1])/COLUMNS;}
            if(pressed&PAD_R1 && matches[0]>0){tab=0;focus_row=2+(chosen[0]-offset[0])/COLUMNS;}
            if(pressed&PAD_UP)navigate_vertical(-1);
            if(pressed&PAD_DOWN)navigate_vertical(1);
            if(pressed&PAD_LEFT)move_selection(tab,-1);
            if(pressed&PAD_RIGHT)move_selection(tab,1);
            if(pressed&PAD_SQUARE || pressed&PAD_SELECT){search_mode=1;keypos=0;}
            if(pressed&PAD_TRIANGLE)refresh();
            if(pressed&PAD_CROSS) {
                launch_mode_manual=(buttons&PAD_R2)!=0;
                launch_selected();
            }
        }
        if(loaded_off[0]!=offset[0] || loaded_off[1]!=offset[1])begin_cover_stream();
        render();
        if(!safe_cover_mode && !show_details && !search_mode)stream_one_cover();
        usleep(16000);
    }
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    ioPadEnd();
    if(network_ready)netDeinitialize();
    sysModuleUnload(SYSMODULE_PNGDEC);
    sysModuleUnload(SYSMODULE_FS);
    return 0;
}
