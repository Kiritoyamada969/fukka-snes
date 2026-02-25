
#include <snes.h>
#include "fukka_gfx.h"

/*
  Fukka Doritos Blaster (SNES demo)
  - Move: LEFT/RIGHT
  - Shoot: B
  - Power: Y (gives triple shot for a few seconds)
  - Boss spawns after score threshold

  Graphics are embedded in fukka_gfx.h (4bpp tiles + 16-color palette).
  NOTE: This project targets PVSnesLib (devkitPro toolchain).
*/

#define MAX_SHOTS 8
#define MAX_FISH  8

typedef struct { s16 x,y; s16 vx,vy; u8 alive; } Shot;
typedef struct { s16 x,y; s16 vx; u8 alive; } Fish;

static Shot shots[MAX_SHOTS];
static Fish fishs[MAX_FISH];

static s16 px = 120;
static s16 py = 176;

static u16 pad = 0;
static u16 pad_old = 0;

static u16 score = 0;
static u8 lives = 3;

static u8 animFrame = 0;
static u8 animTick = 0;

static s16 bossX = 120;
static s16 bossY = 64;
static s8  bossVX = 1;
static s8  bossHP = 18;
static u8  bossOn = 0;

static u8 tripleT = 0;

static void sfx_shoot(void){
  // Simple SPC-less “beep” using the built-in sound driver is more work.
  // Leaving as silent on real hardware. (Still playable.)
}

static void spawnShot(s16 x, s16 y, s16 vx){
  for(u8 i=0;i<MAX_SHOTS;i++){
    if(!shots[i].alive){
      shots[i].alive=1;
      shots[i].x=x;
      shots[i].y=y;
      shots[i].vx=vx;
      shots[i].vy=-6; // up
      return;
    }
  }
}

static void shoot(void){
  // Cooldown via animTick low bits
  if((animTick & 0x07) != 0) return;

  sfx_shoot();

  if(tripleT){
    spawnShot(px+12, py-10, -1);
    spawnShot(px+16, py-10,  0);
    spawnShot(px+20, py-10,  1);
  } else {
    spawnShot(px+16, py-10, 0);
  }
}

static void spawnFish(void){
  for(u8 i=0;i<MAX_FISH;i++){
    if(!fishs[i].alive){
      fishs[i].alive=1;
      if((score + i) & 1){
        fishs[i].x = -16;
        fishs[i].vx = 2;
      } else {
        fishs[i].x = 256;
        fishs[i].vx = -2;
      }
      fishs[i].y = 96 + (i*10);
      return;
    }
  }
}

static u8 aabb(s16 ax,s16 ay,u8 aw,u8 ah, s16 bx,s16 by,u8 bw,u8 bh){
  return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
}

static void resetGame(void){
  score=0; lives=3;
  bossOn=0; bossHP=18; bossX=120; bossVX=1;
  tripleT=0;

  for(u8 i=0;i<MAX_SHOTS;i++) shots[i].alive=0;
  for(u8 i=0;i<MAX_FISH;i++) fishs[i].alive=0;

  px=120; py=176;
}

static void drawTextHud(void){
  // Using console on BG for HUD text
  consoleSetCursor(2,2);
  consoleDrawText(2,2,"Fukka Doritos Blaster");
  consoleSetCursor(2,4);
  consoleDrawText(2,4,"SCORE:");
  consoleDrawText(9,4, "%u", score);
  consoleDrawText(2,5,"LIVES:");
  consoleDrawText(9,5, "%u", lives);

  consoleDrawText(2,7,"B:Shoot  Y:Triple");
  if(bossOn){
    consoleDrawText(2,8,"BOSS HP:");
    consoleDrawText(10,8,"%u", (u8)bossHP);
  } else {
    consoleDrawText(2,8,"BOSS @ 60+");
  }
}

int main(void){
  // --- init ---
  consoleInit();
  setMode(BG_MODE1, 0);

  // OBJ init
  oamInit();

  // Load palette (OBJ palette 0)
  // PVSnesLib expects palettes in CGRAM format (u16 BGR555).
  setPalette((u8*)fukka_pal, 16, 0);

  // Load tiles into VRAM for OBJ
  // We copy our tiles to VRAM starting at tile index 0.
  // Each 4bpp tile = 32 bytes.
  // The helper dmaCopyVram copies bytes to VRAM address.
  // VRAM address for OBJ tiles is 0x0000 in tile units; use address = tileIndex*32.
  dmaCopyVram((u8*)fukka_player_tiles, 0x0000, sizeof(fukka_player_tiles));
  dmaCopyVram((u8*)fukka_bullet_tiles, 0x0000 + sizeof(fukka_player_tiles), sizeof(fukka_bullet_tiles));
  dmaCopyVram((u8*)fukka_boss_tiles,   0x0000 + sizeof(fukka_player_tiles) + sizeof(fukka_bullet_tiles), sizeof(fukka_boss_tiles));
  dmaCopyVram((u8*)fukka_hud_tiles,    0x0000 + sizeof(fukka_player_tiles) + sizeof(fukka_bullet_tiles) + sizeof(fukka_boss_tiles), sizeof(fukka_hud_tiles));

  // Show screen
  setBrightness(0x0F);

  resetGame();

  u16 spawnT=0;

  while(1){
    pad_old = pad;
    pad = padsCurrent(0);

    // Controls
    if(pad & KEY_LEFT)  px -= 2;
    if(pad & KEY_RIGHT) px += 2;
    if(px < 8) px = 8;
    if(px > 232) px = 232;

    // Triple power (Y)
    if((pad & KEY_Y) && !(pad_old & KEY_Y)) tripleT = 180; // ~3 seconds
    if(tripleT) tripleT--;

    // Shoot (B held)
    if(pad & KEY_B) shoot();

    // Anim
    animTick++;
    if((pad & (KEY_LEFT|KEY_RIGHT)) && (animTick & 0x07)==0){
      animFrame = (animFrame + 1) % 3;
    }
    if(!(pad & (KEY_LEFT|KEY_RIGHT))) animFrame=0;

    // Spawn fish periodically
    spawnT++;
    if((spawnT & 0x1F)==0) spawnFish();

    // Update shots
    for(u8 i=0;i<MAX_SHOTS;i++){
      if(!shots[i].alive) continue;
      shots[i].x += shots[i].vx;
      shots[i].y += shots[i].vy;
      if(shots[i].y < -16) shots[i].alive=0;
    }

    // Update fish
    for(u8 i=0;i<MAX_FISH;i++){
      if(!fishs[i].alive) continue;
      fishs[i].x += fishs[i].vx;
      if(fishs[i].x < -32 || fishs[i].x > 272){
        fishs[i].alive=0;
        score += 1;
      }
      // Player hit
      if(aabb(px,py,32,32, fishs[i].x,fishs[i].y,16,16)){
        if(lives) lives--;
        fishs[i].alive=0;
      }
    }

    // Boss spawn
    if(!bossOn && score >= 60){
      bossOn=1;
      bossHP=18;
      bossX=120;
      bossVX=1;
    }

    // Boss update + collisions
    if(bossOn){
      bossX += bossVX;
      if(bossX < 48){ bossX=48; bossVX=1; }
      if(bossX > 192){ bossX=192; bossVX=-1; }

      // shots hit boss
      for(u8 i=0;i<MAX_SHOTS;i++){
        if(!shots[i].alive) continue;
        if(aabb(shots[i].x, shots[i].y, 16,16, bossX-32, bossY-32, 64,64)){
          shots[i].alive=0;
          if(bossHP) bossHP--;
          score += 2;
        }
      }

      // win
      if(bossHP==0){
        bossOn=0;
        // “You win” by reaching 120 points
      }
    }

    // Game over
    if(lives==0){
      // reset for endless play
      resetGame();
    }

    // --- Draw ---
    // OAM entry 0: Player (32x32 OBJ)
    // Each frame is 16 tiles (32x32 in 8x8 tiles)
    u16 playerTile = animFrame * 16;

    // size parameter depends on OBJ size settings; PVSnesLib uses default.
    // We use tile index in 8x8 units.
    oamSet(0, px, py, 0, 0, 0, playerTile, 0);

    // Shots (16x16) tile base after player tiles: 48 tiles total for player
    // bullet tiles start at 48
    u16 bulletBase = 48;
    u8 o=1;
    for(u8 i=0;i<MAX_SHOTS;i++){
      if(!shots[i].alive) continue;
      oamSet(o++, shots[i].x, shots[i].y, 0, 0, 0, bulletBase, 0);
      if(o>=32) break;
    }

    // Fish (use HUD tile as placeholder fish) start tile after boss+... but ok:
    // We'll reuse bullet tile for fish too (simple)
    for(u8 i=0;i<MAX_FISH && o<64;i++){
      if(!fishs[i].alive) continue;
      oamSet(o++, fishs[i].x, fishs[i].y, 0, 0, 0, bulletBase, 0);
    }

    // Boss (64x64): uses 64 tiles after bullet tiles.
    // boss tiles start at 52 (48+4)
    if(bossOn && o<120){
      u16 bossBase = 52;
      // A 64x64 sprite may require OBJ size 64; as fallback we draw 4x 32x32 sprites.
      // Here we draw 4 x 32x32 blocks, each block is 16 tiles.
      // bossBase layout assumes a 64x64 image encoded row-major in 8x8 tiles.
      // Block offsets: (0,0)=0, (32,0)=16, (0,32)=32, (32,32)=48
      oamSet(o++, bossX-32, bossY-32, 0,0,0, bossBase + 0, 0);
      oamSet(o++, bossX,     bossY-32, 0,0,0, bossBase + 16,0);
      oamSet(o++, bossX-32,  bossY,    0,0,0, bossBase + 32,0);
      oamSet(o++, bossX,     bossY,    0,0,0, bossBase + 48,0);
    }

    // clear remaining sprites
    for(; o<128; o++){
      oamSet(o, 0, 240, 0,0,0, 0, 0); // off-screen
    }

    oamUpdate();

    drawTextHud();

    WaitForVBlank();
  }

  return 0;
}
