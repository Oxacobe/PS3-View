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
#define PER_PAGE 5
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
static const char keyboard[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int show_details=0, keep_running=1, loaded_tab=-1, loaded_offset=-1;
static int loaded_off[2]={-1,-1};
static u32 *texture_mem=0;
static u32 cover_offset[2*PER_PAGE];
static pngData cover_data[2*PER_PAGE];
static u32 font_offset=0;
static char message[128]="SELECT A GAME TO PLAY";
static int show_error=0;
static int network_ready=0;

static const u32 C_BG=RGBA(4,4,6,255);
static const u32 C_CARD=RGBA(22,22,24,255);
static const u32 C_EDGE=RGBA(52,52,57,255);
static const u32 C_BLUE=RGBA(245,28,54,255);
static const u32 C_TEXT=RGBA(246,247,250,255);
static const u32 C_MUTED=RGBA(202,203,207,255);


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
    /* Flat black / red accent, intentionally never blue; unmistakable design. */
    rect(0,0,848,512,RGBA(5,5,8,255));
    rect(0,72,848,2,RGBA(54,17,23,255));
    rect(0,472,848,2,RGBA(54,17,23,255));
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
static void unload_covers(void) {
    int i;
    for(i=0;i<2*PER_PAGE;i++){
        cover_offset[i]=0;
        memset(&cover_data[i],0,sizeof(pngData));
    }
}
static void covers_for_page(void) {
    int p,i;
    unsigned used=0;
    u32 *write=texture_mem;
    unload_covers();
    loaded_off[0]=offset[0];loaded_off[1]=offset[1];
    if(!texture_mem)return;
    for(p=0;p<2;p++)for(i=0;i<PER_PAGE;i++){
        int idx=offset[p]+i,slot=p*PER_PAGE+i;
        unsigned n;
        pngData tmp;
        if(idx>=matches[p])break;
        memset(&tmp,0,sizeof(tmp));
        pngLoadFromFile(games[p][matching[p][idx]].cover,&tmp);
        if(!tmp.bmp_out)continue;
        n=(unsigned)(tmp.pitch*tmp.height);
        if(n<16||n+15u>TEX_BUDGET-used||tmp.width>1024||tmp.height>1024){
            free(tmp.bmp_out);continue;
        }
        RSX_MEMCPY(write,tmp.bmp_out,n);
        n=(n+15u)&~15u;
        free(tmp.bmp_out);
        tmp.bmp_out=0;
        cover_offset[slot]=tiny3d_TextureOffset(write);
        cover_data[slot]=tmp;
        write+=n/4;
        used+=n;
    }
}
static void move_selection(int p,int change) {
    int next;
    if(!matches[p])return;
    next=chosen[p]+change;
    if(next<0)next=0;
    if(next>=matches[p])next=matches[p]-1;
    chosen[p]=next;
    if(chosen[p]<offset[p])offset[p]=chosen[p];
    if(chosen[p]>=offset[p]+PER_PAGE)offset[p]=chosen[p]-(PER_PAGE-1);
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
    size_t n=strlen(src);
    if(n>18)snprintf(dest,capacity,"%.15s...",src);
    else snprintf(dest,capacity,"%s",src);
}
static void draw_shelf(int p,int y) {
    int i;
    char stats[100],label[32];
    float base=(float)y;
    u32 title_col=(p==tab)?C_TEXT:C_MUTED;
    if(p==tab) rect(24,base+4,5,20,C_BLUE);
    text(39,base,p==1?"PLAYSTATION 3":"PLAYSTATION 2",5,title_col,22);
    snprintf(stats,sizeof(stats),"%d GAMES",matches[p]);
    text(686,base+3,stats,4,C_MUTED,18);
    for(i=0;i<PER_PAGE;i++){
        int shown=offset[p]+i,slot=p*PER_PAGE+i;
        float x=31+i*160.f,cover_y=base+28;
        int active=p==tab && shown==chosen[p];
        if(shown>=matches[p])break;
        if(active){
            rect(x-3,cover_y-3,147,138,C_BLUE);
            rect(x,cover_y,141,132,RGBA(38,39,45,255));
        }else{
            rect(x,cover_y,141,132,RGBA(36,37,42,255));
        }
        rect(x+3,cover_y+3,135,126,RGBA(15,16,21,255));
        if(cover_offset[slot])image(x+4,cover_y+3,133,125,slot);
        else{
            rect(x+34,cover_y+17,73,92,RGBA(43,45,53,255));
            rect(x+34,cover_y+17,4,92,C_BLUE);
            text(x+46,cover_y+49,p==1?"PS3":"PS2",6,C_TEXT,3);
        }
        title_label(label,sizeof(label),games[p][matching[p][shown]].title);
        text(x+3,cover_y+139,label,4,active?C_TEXT:C_MUTED,19);
    }
    if(matches[p]==0){
        rect(31,base+30,785,132,RGBA(27,28,35,255));
        text(52,base+70,query[0]?"NO MATCHING GAMES":"NO GAMES FOUND",6,C_MUTED,29);
    }
    if(matches[p]>PER_PAGE){
        snprintf(stats,sizeof(stats),"%d / %d",chosen[p]+1,matches[p]);
        text(720,base+172,stats,3,C_MUTED,15);
    }
}
static void draw_search(void) {
    int i;
    char hint[64];
    rect(51,49,746,417,RGBA(6,7,11,255));
    rect(51,49,746,4,C_BLUE);
    rect(51,462,746,4,C_BLUE);
    text(85,67,"SEARCH YOUR LIBRARY",7,C_TEXT,28);
    text(86,104,"FIND A GAME BY NAME",4,C_MUTED,32);
    rect(84,139,679,48,RGBA(31,32,40,255));
    text(100,146,query[0]?query:"TYPE USING YOUR CONTROLLER",6,query[0]?C_TEXT:C_MUTED,24);
    for(i=0;i<36;i++){
        float x=95+(i%9)*74.f,y=203+(i/9)*46.f;
        char v[2]={keyboard[i],0};
        if(keypos==i)rect(x-2,y-2,61,41,C_BLUE);
        rect(x,y,57,37,keypos==i?RGBA(84,18,33,255):RGBA(37,38,47,255));
        text(x+20,y+7,v,6,C_TEXT,2);
    }
    {
      const char *actions[5]={"SPACE","DELETE","DONE","ONLINE","CLEAR"};
      for(i=0;i<5;i++){
          float x=95+i*133.f,y=394.f;
          if(keypos==36+i)rect(x-2,y-2,127,44,C_BLUE);
          rect(x,y,123,40,keypos==36+i?RGBA(84,18,33,255):RGBA(37,38,47,255));
          text(x+9,y+11,actions[i],4,C_TEXT,9);
      }
    }
    snprintf(hint,sizeof(hint),"%d PS3 / %d PS2 RESULTS",matches[1],matches[0]);
    text(95,448,hint,3,C_MUTED,34);
}
static void draw_message(void) {
    rect(102,160,643,179,RGBA(8,9,15,255));
    rect(102,160,643,4,show_error?RGBA(248,170,59,255):C_BLUE);
    text(129,185,show_error?"CANNOT MOUNT GAME":"GAME READY",7,C_TEXT,26);
    text(130,233,message,5,C_TEXT,58);
    text(130,292,exit_frames?"IF GAME DOES NOT START, SELECT MOUNTED ICON IN XMB":"PRESS O TO CLOSE",4,C_MUTED,70);
}
static void render(void) {
    char status[110];
    tiny3d_Clear(C_BG,TINY3D_CLEAR_ALL);
    tiny3d_AlphaTest(1,0x10,TINY3D_ALPHA_FUNC_GEQUAL);
    tiny3d_BlendFunc(1,TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
        TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
        TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
    tiny3d_Project2D();
    gradient();
    rect(0,0,848,70,RGBA(8,9,13,255));
    rect(0,0,6,70,C_BLUE);
    text(31,13,"STATION NOVA",9,C_TEXT,18);
    text(335,27,"THE GAME COLLECTION",4,C_MUTED,23);
    if(query[0])snprintf(status,sizeof(status),"SEARCH: %.21s",query);
    else snprintf(status,sizeof(status),"%d TITLES  /  SELECT TO SEARCH",count[0]+count[1]);
    text(618,27,"V0.4 - NOVA",3,RGBA(254,105,115,255),25);
    draw_shelf(1,81);
    rect(31,270,785,1,RGBA(51,53,62,255));
    draw_shelf(0,276);
    rect(0,474,848,38,RGBA(8,9,13,255));
    rect(0,474,848,1,C_EDGE);
    text(24,484,"X PLAY   /   SQUARE SEARCH   /   TRIANGLE RESCAN",3,C_TEXT,70);
    text(683,484,"O EXIT",3,C_MUTED,12);
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
    int play_sent=0;
    if(matches[tab]<=0)return;
    game=&games[tab][matching[tab][chosen[tab]]];
    if(!encode_game_path(game->path,encoded,sizeof(encoded))) {
        snprintf(message,sizeof(message),"INVALID GAME PATH");
        show_error=show_details=1;return;
    }
    if(tab==0 && !folder_exists("/dev_hdd0/game/PS2U10000")) {
        snprintf(message,sizeof(message),"PS2 CLASSICS LAUNCHER NOT FOUND");
        show_error=show_details=1;return;
    }
    /* First mount while still in a game/homebrew context. webMAN's
     * /mount_ps3 endpoint explicitly bypasses its in-game mount guard.
     * Then ask the plugin to queue/execute launch on the XMB. If the
     * PS3 VSH declines, the game remains mounted for manual launching. */
    snprintf(command,sizeof(command),"/mount_ps3%s",encoded);
    snprintf(message,sizeof(message),"MOUNTING SELECTED GAME...");
    show_details=1;show_error=0;render();
    if(!webman_http(command)) {
        snprintf(message,sizeof(message),"MOUNT FAILED - CHECK HEN / WEBMAN");
        show_error=show_details=1;return;
    }
    usleep(280000);
    /* A successful HTTP response means webMAN accepted the command,
     * not that a PS3 game actually started: the real-console test is vital. */
    play_sent=webman_http(tab==1?"/play.ps3":"/play.ps3?PS2U10000");
    if(play_sent) {
        snprintf(message,sizeof(message),"WEBMAN LAUNCH REQUEST SENT");
    } else {
        snprintf(message,sizeof(message),
            tab==1?"MOUNTED - USE PS3 DISC ICON":"MOUNTED - USE PS2 CLASSICS ICON");
    }
    show_error=0;show_details=1;exit_frames=42;
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
        if(exit_frames>0){
            --exit_frames;
            if(exit_frames==0)keep_running=0;
        }else if(search_mode){
            search_keys(pressed);
        }else if(show_details){
            if(pressed&PAD_CIRCLE) {show_details=show_error=0;}
        }else{
            if(pressed&PAD_CIRCLE)keep_running=0;
            if(pressed&PAD_UP)tab=1;
            if(pressed&PAD_DOWN)tab=0;
            if(pressed&PAD_LEFT)move_selection(tab,-1);
            if(pressed&PAD_RIGHT)move_selection(tab,1);
            if(pressed&PAD_SQUARE || pressed&PAD_SELECT){search_mode=1;keypos=0;}
            if(pressed&PAD_TRIANGLE)refresh();
            if(pressed&PAD_CROSS)launch_selected();
        }
        if(loaded_off[0]!=offset[0] || loaded_off[1]!=offset[1])covers_for_page();
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
