/*
 * STATION for PS3 — safe native proof of concept.
 * Detects game files in known internal HDD locations and previews local covers.
 * Deliberately has NO mount/write/delete/rename/launch operations.
 * GPL-3.0-or-later app code; Tiny3D and PSL1GHT retain upstream licences.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/file.h>
#include <sysmodule/sysmodule.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>
#include <pngdec/pngdec.h>
#include <tiny3d.h>

#define MAX_GAMES 600
#define PER_PAGE 10
#define TEX_BUDGET (20u * 1024u * 1024u)
#define RGBA(r,g,b,a) ((((u32)(r))<<24)|(((u32)(g))<<16)|(((u32)(b))<<8)|(u32)(a))

#define PAD_LEFT    0x8000
#define PAD_DOWN    0x4000
#define PAD_RIGHT   0x2000
#define PAD_UP      0x1000
#define PAD_CROSS   0x0040
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
static int count[2]={0,0}, tab=0, chosen[2]={0,0}, offset[2]={0,0};
static int show_details=0, keep_running=1, loaded_tab=-1, loaded_offset=-1;
static u32 *texture_mem=0;
static u32 cover_offset[PER_PAGE];
static pngData cover_data[PER_PAGE];

static const u32 C_BG=RGBA(10,15,27,255);
static const u32 C_CARD=RGBA(23,33,55,255);
static const u32 C_EDGE=RGBA(40,55,80,255);
static const u32 C_BLUE=RGBA(35,119,213,255);
static const u32 C_TEXT=RGBA(246,247,250,255);
static const u32 C_MUTED=RGBA(140,161,190,255);

/* Compact readable glyphs, avoiding external font/asset dependencies. */
typedef struct { char ch; unsigned char row[5]; } Glyph;
static const Glyph glyphs[]={
 {'A',{2,5,7,5,5}}, {'B',{6,5,6,5,6}}, {'C',{3,4,4,4,3}},
 {'D',{6,5,5,5,6}}, {'E',{7,4,6,4,7}}, {'F',{7,4,6,4,4}},
 {'G',{3,4,5,5,3}}, {'H',{5,5,7,5,5}}, {'I',{7,2,2,2,7}},
 {'J',{1,1,1,5,2}}, {'K',{5,5,6,5,5}}, {'L',{4,4,4,4,7}},
 {'M',{5,7,7,5,5}}, {'N',{5,7,7,7,5}}, {'O',{2,5,5,5,2}},
 {'P',{6,5,6,4,4}}, {'Q',{2,5,5,7,3}}, {'R',{6,5,6,5,5}},
 {'S',{3,4,2,1,6}}, {'T',{7,2,2,2,2}}, {'U',{5,5,5,5,7}},
 {'V',{5,5,5,5,2}}, {'W',{5,5,7,7,5}}, {'X',{5,5,2,5,5}},
 {'Y',{5,5,2,2,2}}, {'Z',{7,1,2,4,7}},
 {'0',{7,5,5,5,7}}, {'1',{2,6,2,2,7}}, {'2',{6,1,7,4,7}},
 {'3',{6,1,3,1,6}}, {'4',{5,5,7,1,1}}, {'5',{7,4,6,1,6}},
 {'6',{3,4,7,5,7}}, {'7',{7,1,2,2,2}}, {'8',{7,5,7,5,7}},
 {'9',{7,5,7,1,6}},
 {' ',{0,0,0,0,0}}, {'.',{0,0,0,0,2}}, {'-',{0,0,7,0,0}},
 {'/',{1,1,2,4,4}}, {':',{0,2,0,2,0}}, {'?',{6,1,2,0,2}},
 {'&',{2,5,2,5,3}}, {'(',{2,4,4,4,2}}, {')',{2,1,1,1,2}},
 {'_',{0,0,0,0,7}}, {'+', {0,2,7,2,0}}, {'\'',{2,2,0,0,0}}
};

static void rect(float x,float y,float w,float h,u32 color) {
    tiny3d_SetPolygon(TINY3D_QUADS);
    tiny3d_VertexPos(x,y,65535.f); tiny3d_VertexColor(color);
    tiny3d_VertexPos(x+w,y,65535.f);
    tiny3d_VertexPos(x+w,y+h,65535.f);
    tiny3d_VertexPos(x,y+h,65535.f);
    tiny3d_End();
}
static void text(float x,float y,const char *s,int size,u32 color,int maximum) {
    int chars=0;
    while(*s && chars++<maximum) {
        unsigned char ch=(unsigned char)toupper((unsigned char)*s++);
        int i,j,k;
        const Glyph *g=0;
        for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));i++)
            if(glyphs[i].ch==ch) {g=&glyphs[i];break;}
        if(!g) ch='?';
        if(!g) for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));i++)
            if(glyphs[i].ch==ch) {g=&glyphs[i];break;}
        if(g) for(j=0;j<5;j++) for(k=0;k<3;k++)
            if(g->row[j] & (1 << (2-k)))
                rect(x+(k*size),y+(j*size),(float)size-0.3f,(float)size-0.3f,color);
        x+=size*4;
    }
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
static void refresh(void) {
    count[0]=count[1]=0;
    scan_directory(0,"/dev_hdd0/PS2ISO",0);
    scan_directory(1,"/dev_hdd0/PS3ISO",0);
    scan_directory(1,"/dev_hdd0/GAMES",1);
    qsort(games[0],count[0],sizeof(Entry),cmp_games);
    qsort(games[1],count[1],sizeof(Entry),cmp_games);
    chosen[0]=chosen[1]=offset[0]=offset[1]=0;
    loaded_tab=-1;loaded_offset=-1;show_details=0;
}
static void unload_covers(void) {
    int i;
    for(i=0;i<PER_PAGE;i++) {
        cover_offset[i]=0;
        memset(&cover_data[i],0,sizeof(pngData));
    }
}
static void covers_for_page(void) {
    int i;
    unsigned used=0;
    u32 *write=texture_mem;
    unload_covers();
    loaded_tab=tab;loaded_offset=offset[tab];
    if(!texture_mem)return;
    for(i=0;i<PER_PAGE;i++) {
        int idx=offset[tab]+i;
        unsigned n;
        pngData tmp;
        if(idx>=count[tab])break;
        memset(&tmp,0,sizeof(tmp));
        pngLoadFromFile(games[tab][idx].cover,&tmp);
        if(!tmp.bmp_out)continue;
        n=(unsigned)(tmp.pitch*tmp.height);
        if(n<16 || n+15u>TEX_BUDGET-used || tmp.width>1024 || tmp.height>1024) {
            free(tmp.bmp_out);continue;
        }
        RSX_MEMCPY(write,tmp.bmp_out,n);
        n=(n+15u)&~15u;
        free(tmp.bmp_out);
        tmp.bmp_out=0;
        cover_offset[i]=tiny3d_TextureOffset(write);
        cover_data[i]=tmp;
        write+=n/4;
        used+=n;
    }
}
static void image(float x,float y,float w,float h,int slot) {
    const pngData *p=&cover_data[slot];
    if(!cover_offset[slot])return;
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
static void render(void) {
    int i;
    char stats[64];
    tiny3d_Clear(C_BG,TINY3D_CLEAR_ALL);
    tiny3d_AlphaTest(1,0x10,TINY3D_ALPHA_FUNC_GEQUAL);
    tiny3d_BlendFunc(1,TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
        TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
        TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
    tiny3d_Project2D();
    rect(0,0,848,512,C_BG);
    rect(0,0,848,4,C_BLUE);
    text(45,22,"STATION",8,C_TEXT,10);
    text(47,70,"YOUR PLAYSTATION LIBRARY",3,C_MUTED,28);
    rect(42,103,140,38,tab==0?C_BLUE:C_CARD);
    rect(191,103,140,38,tab==1?C_BLUE:C_CARD);
    text(88,112,"PS2",5,C_TEXT,3);
    text(236,112,"PS3",5,C_TEXT,3);
    snprintf(stats,sizeof(stats),"%d GAMES  |  PAGE %d",count[tab],offset[tab]/PER_PAGE+1);
    text(574,115,stats,3,C_MUTED,30);
    for(i=0;i<PER_PAGE;i++) {
        int idx=offset[tab]+i;
        int column=i%5,row=i/5;
        float x=46+(column*157),y=156+(row*155);
        unsigned selected=idx==chosen[tab];
        if(idx>=count[tab])break;
        rect(x-3,y-3,143,147,selected?RGBA(65,148,237,255):C_EDGE);
        rect(x,y,137,141,C_CARD);
        rect(x+16,y+6,105,103,selected?RGBA(34,75,121,255):RGBA(30,46,73,255));
        if(cover_offset[i]) image(x+16,y+6,105,103,i);
        else {
            text(x+47,y+42,tab==0?"PS2":"PS3",5,C_MUTED,3);
            text(x+34,y+70,"NO COVER",2,C_MUTED,8);
        }
        text(x+10,y+118,games[tab][idx].title,2,C_TEXT,15);
    }
    if(!count[tab]) {
        rect(43,184,756,170,C_CARD);
        text(93,232,"NO GAMES FOUND ON THIS DRIVE",5,C_TEXT,32);
        text(120,286,"TRIANGLE TO RESCAN",3,C_MUTED,25);
    }
    if(show_details && count[tab]) {
        rect(38,366,772,99,RGBA(24,49,83,255));
        text(56,379,"LAUNCH DISABLED IN SAFE TEST BUILD",3,C_TEXT,39);
        text(56,402,games[tab][chosen[tab]].title,3,C_TEXT,48);
        text(56,429,games[tab][chosen[tab]].path,2,C_MUTED,83);
    }
    rect(0,475,848,37,RGBA(13,24,42,255));
    text(26,486,"L1 R1  SYSTEM   D PAD  BROWSE   X  INFO   O  EXIT   TRIANGLE  RESCAN",2,C_MUTED,105);
    tiny3d_Flip();
}
static void callback(u64 status,u64 param,void *userdata) {
    (void)param;(void)userdata;
    if(status==SYSUTIL_EXIT_GAME)keep_running=0;
}
int main(void) {
    padInfo info;
    padData pd;
    unsigned previous=0;
    sysModuleLoad(SYSMODULE_FS);
    sysModuleLoad(SYSMODULE_PNGDEC);
    tiny3d_Init(1024*1024);
    texture_mem=tiny3d_AllocTexture(TEX_BUDGET);
    ioPadInit(7);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0,callback,NULL);
    refresh();
    while(keep_running) {
        unsigned buttons=0,pressed;
        sysUtilCheckCallback();
        ioPadGetInfo(&info);
        if(info.status[0]) {
            ioPadGetData(0,&pd);
            buttons=((unsigned)pd.button[2]<<8)|pd.button[3];
        }
        pressed=buttons & ~previous;
        previous=buttons;
        if(pressed & PAD_CIRCLE) keep_running=0;
        if(pressed & PAD_L1) {tab=0;show_details=0;}
        if(pressed & PAD_R1) {tab=1;show_details=0;}
        if(pressed & PAD_TRIANGLE) refresh();
        if(pressed & PAD_LEFT && chosen[tab]>0) {chosen[tab]--;show_details=0;}
        if(pressed & PAD_RIGHT && chosen[tab]<count[tab]-1) {chosen[tab]++;show_details=0;}
        if(pressed & PAD_UP && chosen[tab]>=5) {chosen[tab]-=5;show_details=0;}
        if(pressed & PAD_DOWN && chosen[tab]+5<count[tab]) {chosen[tab]+=5;show_details=0;}
        if(pressed & PAD_CROSS && count[tab]) show_details=!show_details;
        offset[tab]=(chosen[tab]/PER_PAGE)*PER_PAGE;
        if(loaded_tab!=tab || loaded_offset!=offset[tab])covers_for_page();
        render();
        usleep(16000);
    }
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    ioPadEnd();
    sysModuleUnload(SYSMODULE_PNGDEC);
    sysModuleUnload(SYSMODULE_FS);
    return 0;
}
