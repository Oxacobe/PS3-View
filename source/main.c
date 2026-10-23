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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/socket.h>
#include <net/net.h>
#include "ui_font_generated.h"

#define MAX_GAMES 600
#define PER_PAGE 8
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
static u32 font_offset=0;
static char message[128]="SELECT A GAME TO PLAY";
static int show_error=0;
static int network_ready=0;

static const u32 C_BG=RGBA(10,15,27,255);
static const u32 C_CARD=RGBA(23,33,55,255);
static const u32 C_EDGE=RGBA(40,55,80,255);
static const u32 C_BLUE=RGBA(35,119,213,255);
static const u32 C_TEXT=RGBA(246,247,250,255);
static const u32 C_MUTED=RGBA(140,161,190,255);


/* Smooth anti-aliased font baked in CI from Pillow's freely distributable
 * bundled default font, rendered as a single hardware texture atlas. */
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
        pixels[ty*FONT_W+tx]=RGBA(255,255,255,a);
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
static void gradient(void) {
    int i;
    for(i=0;i<64;i++) {
        int r=8+(i*7/64),g=13+(i*10/64),b=27+(i*20/64);
        rect(0,i*8,848,9,RGBA(r,g,b,255));
    }
}
static void frame(float x,float y,float w,float h,int active) {
    rect(x,y,w,h,active?RGBA(69,151,255,255):RGBA(44,59,79,255));
    rect(x+2,y+2,w-4,h-4,RGBA(20,31,52,255));
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

static void render(void) {
    int i;
    char stats[90];
    const int ix=chosen[tab];
    gradient();
    rect(0,0,848,79,RGBA(10,17,29,255));
    rect(29,20,5,39,C_BLUE);
    text(45,16,"STATION",10,C_TEXT,14);
    text(47,51,"YOUR GAMES. YOUR CONSOLE.",3,C_MUTED,26);
    rect(647,27,160,26,RGBA(27,45,70,255));
    text(662,32,"BETA 0.2  /  PS3",3,RGBA(123,184,255,255),22);

    /* Navigation rail */
    text(28,102,"LIBRARY",4,C_MUTED,12);
    rect(19,143,154,60,tab==0?RGBA(36,93,157,255):C_CARD);
    rect(19,211,154,60,tab==1?RGBA(36,93,157,255):C_CARD);
    rect(21,145,3,56,tab==0?RGBA(81,177,255,255):C_EDGE);
    rect(21,213,3,56,tab==1?RGBA(81,177,255,255):C_EDGE);
    text(38,151,"PS2",7,C_TEXT,4);
    text(38,179,"CLASSICS",3,C_MUTED,10);
    text(38,219,"PS3",7,C_TEXT,4);
    text(38,247,"GAMES",3,C_MUTED,10);
    rect(175,90,1,352,C_EDGE);
    text(27,355,"POWERED BY",3,C_MUTED,16);
    text(27,372,"WEBMAN MOD",4,RGBA(125,191,255,255),16);
    text(27,409,"L1 / R1",3,C_MUTED,14);
    text(27,425,"SWITCH SYSTEM",3,C_MUTED,18);

    /* Collection header */
    text(198,89,tab==0?"PLAYSTATION 2":"PLAYSTATION 3",6,C_TEXT,20);
    snprintf(stats,sizeof(stats),"%d GAMES   /   PAGE %d OF %d",
             count[tab],offset[tab]/PER_PAGE+1,(count[tab]+PER_PAGE-1)/PER_PAGE>0?(count[tab]+PER_PAGE-1)/PER_PAGE:1);
    text(199,121,stats,3,C_MUTED,52);
    for(i=0;i<PER_PAGE;i++) {
        const int idx=offset[tab]+i;
        int col=i%GRID_COLS,row=i/GRID_COLS;
        float x=196+col*158.f,y=150+row*155.f;
        int selected=idx==chosen[tab];
        if(idx>=count[tab])break;
        if(selected) rect(x-3,y-3,151,150,RGBA(39,120,221,255));
        frame(x,y,145,144,selected);
        rect(x+10,y+9,125,106,RGBA(11,21,35,255));
        if(cover_offset[i])image(x+10,y+9,125,106,i);
        else {
            text(x+46,y+38,tab==0?"PS2":"PS3",6,RGBA(88,133,184,255),4);
            text(x+38,y+71,"NO ART",3,C_MUTED,12);
        }
        text(x+10,y+122,games[tab][idx].title,3,C_TEXT,18);
    }
    if(count[tab]==0) {
        rect(205,176,593,184,C_CARD);
        text(242,218,"NO GAMES FOUND",8,C_TEXT,30);
        text(244,277,"PRESS TRIANGLE TO SCAN AGAIN",4,C_MUTED,35);
    }
    /* Context overlay: never obscure selection before confirmation */
    if(show_details && count[tab]) {
        rect(193,335,635,125,RGBA(16,38,66,255));
        rect(193,335,635,3,C_BLUE);
        text(208,349,show_error?"LAUNCH ERROR":"SELECTED GAME",4,show_error?RGBA(255,139,139,255):C_MUTED,26);
        text(208,372,show_error?message:games[tab][ix].title,5,C_TEXT,46);
        text(208,413,show_error?"O TO CLOSE   /   CHECK WEBMAN":"X TO PLAY   /   O TO CLOSE",3,C_MUTED,50);
    }
    rect(0,474,848,38,RGBA(8,16,29,255));
    rect(0,473,848,1,C_EDGE);
    text(25,483,"X  PLAY   /   SQUARE  INFO   /   TRIANGLE  RESCAN",3,C_MUTED,63);
    text(656,483,"O  EXIT  /  L1 R1",3,C_MUTED,26);
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
static void launch_selected(void) {
    char encoded[1500],command[1600];
    Entry *game;
    if(!count[tab])return;
    game=&games[tab][chosen[tab]];
    if(!encode_game_path(game->path,encoded,sizeof(encoded))) {
        snprintf(message,sizeof(message),"INVALID GAME PATH");
        show_error=show_details=1;
        return;
    }
    if(tab==0 && !folder_exists("/dev_hdd0/game/PS2U10000/USRDIR")) {
        snprintf(message,sizeof(message),"INSTALL PS2 CLASSICS LAUNCHER FIRST");
        show_error=show_details=1;
        return;
    }
    snprintf(command,sizeof(command),"/%s%s",tab==0?"mount.ps3":"play.ps3",encoded);
    if(!webman_http(command)) {
        snprintf(message,sizeof(message),"WEBMAN NOT RESPONDING - ENABLE HEN");
        show_error=show_details=1;
        return;
    }
    if(tab==0) {
        /* Mount encrypted BIN.ENC via webMAN, then request its installed
         * PS2 Classics Launcher by the established PS2U10000 title ID. */
        usleep(900000);
        if(!webman_http("/play.ps3?PS2U10000")) {
            snprintf(message,sizeof(message),"PS2 MOUNTED; START CLASSICS IN XMB");
            show_error=show_details=1;
            return;
        }
    }
    keep_running=0; /* hand control to webMAN/XMB */
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
    font_setup();
    ioPadInit(7);
    network_ready=(netInitialize()>=0);
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
        if(pressed & PAD_CIRCLE) {if(show_details){show_details=0;show_error=0;}else keep_running=0;}
        if(pressed & PAD_L1) {tab=0;show_details=0;}
        if(pressed & PAD_R1) {tab=1;show_details=0;}
        if(pressed & PAD_TRIANGLE) refresh();
        if(pressed & PAD_LEFT && chosen[tab]>0) {chosen[tab]--;show_details=0;}
        if(pressed & PAD_RIGHT && chosen[tab]<count[tab]-1) {chosen[tab]++;show_details=0;}
        if(pressed & PAD_UP && chosen[tab]>=GRID_COLS) {chosen[tab]-=GRID_COLS;show_details=0;}
        if(pressed & PAD_DOWN && chosen[tab]+GRID_COLS<count[tab]) {chosen[tab]+=GRID_COLS;show_details=0;}
        if(pressed & PAD_SQUARE && count[tab]) {show_error=0;show_details=!show_details;}
        if(pressed & PAD_CROSS && count[tab]) launch_selected();
        offset[tab]=(chosen[tab]/PER_PAGE)*PER_PAGE;
        if(loaded_tab!=tab || loaded_offset!=offset[tab])covers_for_page();
        render();
        usleep(16000);
    }
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    ioPadEnd();
    if(network_ready)netDeinitialize();
    sysModuleUnload(SYSMODULE_PNGDEC);
    sysModuleUnload(SYSMODULE_FS);
    return 0;
}
