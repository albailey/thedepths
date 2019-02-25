/*
 *   the_depths.c
 *
 *  MAIN GAME FILE
 *
 * Meant to be 100 randomly generated levels
 * Minimal graphics.  
 * Try to get as deep as possible
 *
 *  Version in progress: 0.3
 */

#include "neslib.h"
#include "utils.h"


// ==================== CONSTANTS ============================================

//  GAME FEATURES
//#define DEV_MODE 1

#define FOG_MODE 1

//-------------------------------------- UNUSED
#define NTSC_FPS 60 
// ------------------------------------- Player movement speed
#define STEP 1            
// --------------------------------------UNUSED
#define INITIAL_HEALTH 5
#define INITIAL_POWER 1

#define PLAYER_SPRITE 0x40
#define PLAYER_ATTRIBUTE 0x00
#define PLAYER_WIDTH 6
#define PLAYER_HEIGHT 6


#define LEFT_LIMIT 16
#define RIGHT_LIMIT 232
#define TOP_LIMIT 16
#define BOTTOM_LIMIT 216

// Special LEVELS
#define SURFACE_LEVEL 1

// different game modes
#define GAME_MODE 0
#define INVENTORY_MODE 5

// level vars
#define TOP_BORDER 3
#define LEFT_BORDER 2

#define LEVEL_WIDTH  32
#define LEVEL_HEIGHT 30

// individual rooms (includes the wall)
#define ROOM_WIDTH  6
#define ROOM_HEIGHT 5
#define ROOMS_WIDE 5
#define ROOMS_HIGH 5

// Object types (these match to tiles in CHR)
// Only 8 types.  Upper 4 bits indicate tunnel values
#define EMPTY 0
#define STAIRS_DOWN 1
#define STAIRS_UP 2
#define STONE     3
#define TREASURE  4
#define RANDOM    5
#define NO_OBJ    6
// Room for lots more...
#define UNSET 0x0F

#define FOG_TILE 10

// Sound effects
#define LEVEL_SOUND 1
#define LEVEL_CHANNEL 2

#define TREASURE_SOUND 2
#define TREASURE_CHANNEL 2

#define REVEAL_ROOM_SOUND 3
#define REVEAL_ROOM_CHANNEL 2

#define ERROR_SOUND 4
#define ERROR_CHANNEL 2


// for displaying health, level and score
// We may show health and scores using different sprites (ie: hearts)
#define NUMBER_SPRITE_OFFSET 0x10
#define HEALTH_SPRITE_OFFSET 0x20
#define POWER_SPRITE_OFFSET 0x20
#define BLANK_SPRITE 0x3B
#define BLANK_HEALTH 0x20
#define BLANK_POWER 0x20

// BITMASKS for TUNNEL DIRECTIONS
#define TUNNEL_UP    0x10
#define TUNNEL_RIGHT 0x20
#define TUNNEL_DOWN  0x40
#define TUNNEL_LEFT  0x80


// =========================== MACROS =========================================
// NAMETABLE updates per frame
#define NTADR(x,y)      ((0x2000|((y)<<5)|x))
#define MSB(x)          (((x)>>8))
#define LSB(x)          (((x)&0xff))


// =========================== VARIABLES =========================================
//generic vars
static unsigned char spr;
static unsigned char i,j;
static unsigned char lastPad, pad;
static unsigned char lastRoomX, lastRoomY;
static unsigned char game_mode;

// player vars
static unsigned char player_x, player_y;
static unsigned char player_sprite, player_attribute;
int player_health;
int player_power;
int level;
int score;

// large map for the active level
unsigned char grid[LEVEL_WIDTH][LEVEL_HEIGHT];

// smaller map for the rooms
unsigned char room_grid[ROOMS_WIDE][ROOMS_HIGH];

// this data structure prevents stack overflow.
// basically we use un-bound iteration instead of recursion
static signed char crawlHead;
static signed char crawlTail;
unsigned char crawlStack[ROOMS_WIDE * ROOMS_HIGH * 2];

// This is a update-per-frame queue for VRAM
// It splits many updates during vblank over several
// frame updates
#define SIZEOF_QUEUE 120
unsigned char* vram_read_ptr;
unsigned char* vram_write_ptr;
unsigned char vram_queue[SIZEOF_QUEUE];



// =========================== CONST DATA =========================================
//palette for sprites
const unsigned char palSprites[16]={
	0x0f,0x17,0x27,0x37,
	0x0f,0x11,0x21,0x31,
	0x0f,0x15,0x25,0x35,
	0x0f,0x19,0x29,0x39
};

// Starts placing the 3 digit level at coordinates: 4,2  .. 6,2
// NTADR(4,2)
#define LEVEL_POSITION_X 4
#define LEVEL_POSITION_Y 2
#define NUM_LEVEL_SPRITES 3
static unsigned char level_oam[NUM_LEVEL_SPRITES];

// Starts placing the 5 digit score at coordinates: 8,2  .. 12,2
// 2 treasure bytes means a max score of 64k (which is 5 digits)
// If num treasure bytes increases, we need more digits for score
#define LAST_SCORE_SPRITE_POS  14
static unsigned char score_oam[5*3];
const unsigned char score_placement[5*3]={
        MSB(NTADR(8,2)),LSB(NTADR(8,2)),0,
        MSB(NTADR(9,2)),LSB(NTADR(9,2)),0,
        MSB(NTADR(10,2)),LSB(NTADR(10,2)),0,
        MSB(NTADR(11,2)),LSB(NTADR(11,2)),0,
        MSB(NTADR(12,2)),LSB(NTADR(12,2)),0,
};

// Starts placing the 2 digit health at coordinates: 14,2  .. 15,2
#define LAST_HEALTH_SPRITE_POS  5
static unsigned char health_oam[2*3];
const unsigned char health_placement[2*3]={
        MSB(NTADR(14,2)),LSB(NTADR(14,2)),0,
        MSB(NTADR(15,2)),LSB(NTADR(15,2)),0,
};

// Starts placing the 2 digit health at coordinates: 18,2  .. 19,2
#define LAST_POWER_SPRITE_POS  5
static unsigned char power_oam[2*3];
const unsigned char power_placement[2*3]={
        MSB(NTADR(18,2)),LSB(NTADR(18,2)),0,
        MSB(NTADR(19,2)),LSB(NTADR(19,2)),0,
};



// ======================= UNDER CONSTRUCTION =======================
// ------------ MOB variables, constants and utility methods -------
// THIS WILL BE MOVED AROUND LATER 
// referred to this page for help on struct in cc65
// http://kkfos.aspekt.fi/projects/nes/libraries/knes-library-for-cc65/

#define MOB_UNSET 0
#define MOB_DORMANT 1
#define MOB_ACTIVE 2
#define MOB_DEAD 3


#define MAX_MOBS 3
struct MOBS {
  unsigned char mobstate[MAX_MOBS];
  unsigned char mobtype[MAX_MOBS];
  unsigned char x[MAX_MOBS];
  unsigned char y[MAX_MOBS];
  unsigned char ai[MAX_MOBS];
};
struct MOBS mobs; // create the mobs memory

void clearMobs(){
 unsigned char m;
 for (m=0;m<MAX_MOBS;m++) {
    mobs.mobstate[m] = MOB_UNSET;
 }
}
// ---- END of mobs  code --------------------------------------------



void addCrawl(signed char gridx, signed char gridy) {
  crawlStack[crawlTail] = gridx;
  crawlTail++;
  crawlStack[crawlTail] = gridy;
  crawlTail++;
}

unsigned char canCreateRoom(signed char gridx, signed char gridy) {
  if((gridx < 0) || (gridx >= ROOMS_WIDE)){
     return FALSE;
  }
  if((gridy < 0) || (gridy >= ROOMS_HIGH)){
     return FALSE;
  }
  // Return true if the lower 4 bits are still UNSET 
  // the mask removes tunnel settings
  return ((room_grid[gridx][gridy] & 0x0F) == UNSET );
}

void createRoom(signed char gridx, signed char gridy) {
  unsigned char randDir = rand8() % 4;
  unsigned char loop = 3;


  lastRoomX = gridx;
  lastRoomY = gridy;


  while(loop > 0) {
   loop--;
   switch(randDir) {
    case 0: 
      // UP
      if(canCreateRoom(gridx, gridy-1)) { 
        room_grid[gridx][gridy]   |= TUNNEL_UP;
        room_grid[gridx][gridy-1] &= 0xF0;  // strip lower 4 bits
        room_grid[gridx][gridy-1] |= TUNNEL_DOWN;
        room_grid[gridx][gridy-1] |= RANDOM;
        addCrawl(gridx, gridy-1);
      }
      break;
    case 1: 
      // RIGHT
      if(canCreateRoom(gridx+1, gridy)) { 
        room_grid[gridx][gridy] |= TUNNEL_RIGHT;
        room_grid[gridx+1][gridy] &= 0xF0;  // strip lower 4 bits
        room_grid[gridx+1][gridy] |= TUNNEL_LEFT;
        room_grid[gridx+1][gridy] |= RANDOM;
        addCrawl(gridx+1, gridy);
      }
      break;
    case 2:
      // DOWN
      if(canCreateRoom(gridx, gridy+1)) { 
        room_grid[gridx][gridy] |= TUNNEL_DOWN;
        room_grid[gridx][gridy+1] &= 0xF0;  // strip lower 4 bits
        room_grid[gridx][gridy+1] |= TUNNEL_UP;
        room_grid[gridx][gridy+1] |= RANDOM;
        addCrawl(gridx, gridy+1);
      }
      break;
    case 3:
      // LEFT
      if(canCreateRoom(gridx-1, gridy)) { 
        room_grid[gridx][gridy] |= TUNNEL_LEFT;
        room_grid[gridx-1][gridy] &= 0xF0;  // strip lower 4 bits
        room_grid[gridx-1][gridy] |= TUNNEL_RIGHT;
        room_grid[gridx-1][gridy] |= RANDOM;
        addCrawl(gridx-1, gridy);
      }
      break;
   }
   randDir = (randDir + 1) % 4;
   }
}

void updateLevelSprites(){
    // 3 digit level. 
    int x = level;
    signed char pos = NUM_LEVEL_SPRITES-1;

    while(pos >=0 ) {
      if(x > 0) {
        level_oam[pos] = NUMBER_SPRITE_OFFSET + (x % 10);
        x = x / 10;
       } else {
         level_oam[pos] = BLANK_SPRITE;
       }
      --pos;
    }

}

void updateTreasureSprites(){
    // 5 digit health. 
    int x = score;
    signed char pos = LAST_SCORE_SPRITE_POS;
    // always draw first digit
    score_oam[pos] = NUMBER_SPRITE_OFFSET + (x % 10);
    x = x / 10;
    pos -=3;
    // conditionally draw the rest
    while(pos >=0 ) {
      if ( x > 0 ) {
         score_oam[pos] = NUMBER_SPRITE_OFFSET + (x % 10);
         x = x / 10;
      } else {
         score_oam[pos] = BLANK_SPRITE;
      }
      pos -=3;
    }
}



void generateLevel(unsigned char startObj, unsigned char endObj)
{
  unsigned char startX = rand8() % ROOMS_WIDE;
  unsigned char startY = rand8() % ROOMS_HIGH;

        // STEP 1: stone border
        for (j=0;j<LEVEL_HEIGHT;j++) {
            grid[0][j] = STONE;
            grid[LEVEL_WIDTH-1][j] = STONE;
        }
        for (i=0;i<LEVEL_WIDTH;i++) {
            grid[i][0] = STONE;
            grid[i][LEVEL_HEIGHT-1] = STONE;
        }

        // STEP 2: fog interior
#ifdef FOG_MODE
        for (j=1;j<LEVEL_HEIGHT-1;j++) {
            for (i=1;i<LEVEL_WIDTH-1;i++) {
                grid[i][j] = FOG_TILE;
            }
        }
#endif

        // Step 3: reset the room grid so the algorithm knows where to populate
        for (j=0;j<ROOMS_HIGH;j++) {
            for (i=0;i<ROOMS_WIDE;i++) {
                room_grid[i][j] = UNSET;
            }
        }

        // initialize start and end pointers for crawl array
        crawlHead = 0;
        crawlTail = 0;
        // STEP 3: generate starting point
        room_grid[startX][startY] = startObj;
        createRoom(startX, startY);
        while(crawlHead != crawlTail) {
            startX = crawlStack[crawlHead];
            crawlHead++;
            startY = crawlStack[crawlHead];
            crawlHead++; // this wraps 
            createRoom(startX, startY);  
        }
        // the last room we reach is the stairs down
        // Only set the lower portion of the grid
        room_grid[lastRoomX][lastRoomY] &= 0xF0;
        room_grid[lastRoomX][lastRoomY] |= endObj;

        // draw static text
        updateLevelSprites();
}


void setAndDraw(signed char u, signed char v, signed char val) {
    if(vram_write_ptr +3 == vram_read_ptr) {
      // queue is full.  Cannot add more to it
      // until we process some of what is queued up
      sfx_play(ERROR_SOUND, ERROR_CHANNEL);
      return;
    }
    grid[u][v] = val;
    *(vram_write_ptr++) = MSB(NTADR(u,v));
    *(vram_write_ptr++) = LSB(NTADR(u,v));
    *(vram_write_ptr++) = val;
    // If we reach the end, wrap around to the start.
    if(vram_write_ptr >= (vram_queue + SIZEOF_QUEUE)) {
      vram_write_ptr = vram_queue;
    }
}

unsigned char getRandomItem() {
 // this needs a lot of work
  unsigned char randItem = rand8() % 4;
  if (randItem == 0){
     return TREASURE;
  } else {
    return EMPTY;
  }

  
}

void fillRoom(signed char gridx, signed char gridy) {

  // set the bottom right corner of the room (not including wall)
  unsigned char sx = (gridx * ROOM_WIDTH) + LEFT_BORDER;
  unsigned char sy = (gridy * ROOM_HEIGHT) + TOP_BORDER;
  unsigned char ex = sx + ROOM_WIDTH;
  unsigned char ey = sy + ROOM_HEIGHT;

  unsigned char u,v,w;

  // do not use I or J

  // draw outer border of stone
  for(v=sy;v<ey;v++) {
      setAndDraw(sx,v,STONE);
      setAndDraw(ex-1,v,STONE);
  }
  for(u=sx+1;u<ex-1;u++) {
      setAndDraw(u,sy,STONE);
      setAndDraw(u,ey-1,STONE);
  }

  // clear room tiles
  for(v=sy+1;v<ey-1;v++) {
    for(u=sx+1;u<ex-1;u++) {
      setAndDraw(u,v,EMPTY);
    }
  }

  // add doors
  if ((room_grid[gridx][gridy] & TUNNEL_DOWN) == TUNNEL_DOWN) {
     // notch the floor
     setAndDraw(sx + (ROOM_WIDTH/2), ey-1, EMPTY);
  }
  if ((room_grid[gridx][gridy] & TUNNEL_UP) == TUNNEL_UP) {
     // notch the ceiling
     setAndDraw(sx + (ROOM_WIDTH/2), sy, EMPTY);
  }
  if ((room_grid[gridx][gridy] & TUNNEL_LEFT) == TUNNEL_LEFT) {
     // notch the left
     setAndDraw(sx, sy + (ROOM_HEIGHT/2), EMPTY);
  }
  if ((room_grid[gridx][gridy] & TUNNEL_RIGHT) == TUNNEL_RIGHT) {
     // notch the right
     setAndDraw(ex-1, sy + (ROOM_HEIGHT/2), EMPTY);
  }


  // last step we set the SPECIAL value to be in the middle of the room
  u = sx + (ROOM_WIDTH/2) - 1;
  v = sy + (ROOM_HEIGHT /2) - 1;
  w = room_grid[gridx][gridy] & 0x0F;
  if (w == RANDOM) {
    // clear the random value
    room_grid[gridx][gridy] = EMPTY;
    // calculate what the new item is
    setAndDraw(u,v,getRandomItem());
  } else {
    setAndDraw(u,v,w);
  }

}

// This method must be invoked while ppu display is OFF
void drawLevel(unsigned char target) {
  // EVERYTHING IS INITIALLY STONE BORDER AND FOG 
  for (j=0;j<LEVEL_HEIGHT;j++) {
    for (i=0;i<LEVEL_WIDTH;i++) {
      vram_adr(NTADR(i,j)); // left border
      vram_put(grid[i][j]);
    }
  }

  //  DRAW STARTING ROOM based on the passed in target.
  //  PUT the player there as well
  player_sprite = PLAYER_SPRITE;  
  player_attribute=PLAYER_ATTRIBUTE;

  for (j=0;j<ROOMS_HIGH;j++) {
     for (i=0;i<ROOMS_WIDE;i++) {
        if(( room_grid[i][j] & 0x0F) == target) {
           player_x = 8*(LEFT_BORDER + (i*ROOM_WIDTH) + (ROOM_WIDTH / 2)- 1);
           player_y = 8*(TOP_BORDER + (j*ROOM_HEIGHT) + (ROOM_HEIGHT / 2));
#ifdef FOG_MODE
           // this only draws one room. the rest are fogged
           fillRoom(i, j);
           break;
        }
#else
        }
        // this draws all rooms. fog disabled 
        fillRoom(i, j);
#endif
     }
  }
  // Draw text info:
  vram_adr(NTADR(LEVEL_POSITION_X, LEVEL_POSITION_Y));
  for(j=0; j<NUM_LEVEL_SPRITES; j++) {
      vram_put(level_oam[j]);
  }
    
}


// Usually a level starts with stairs up, and ends with stairs down
void loadLevel(unsigned char startObj, unsigned char endObj)
{
   // turn off display
   ppu_off();

   // clear any previous sprites
   oam_clear();

   // TO DO: if we have never seen this level before generate a random level
   generateLevel(startObj, endObj);
   drawLevel(startObj);

   // Update score sprites
   set_vram_update(5,score_oam);
   ppu_on_all();
   sfx_play(LEVEL_SOUND, LEVEL_CHANNEL);
}

void generateStartingLevel() {
  level=SURFACE_LEVEL;
  loadLevel(NO_OBJ, STAIRS_DOWN);
}

void generateNextLevel() {
  level+=1;
  loadLevel(STAIRS_UP, STAIRS_DOWN);
}
void generatePreviousLevel() {
  level-=1;
  if (level == SURFACE_LEVEL) {
     // cannot go beyond SURFACE 
     loadLevel(STAIRS_DOWN, NO_OBJ);
  } else {
     loadLevel(STAIRS_DOWN, STAIRS_UP);
  }
}

void gameOver() {
        // TO DO: show game over screen
	ppu_on_all();//enable rendering
	while(1) {
		ppu_waitnmi();//wait for next TV frame
                pad = pad_poll(0);
        	if (pad & PAD_START) {
                   // start was pressed
                   break;
		}
	}
	reset();
}

unsigned char clipCheck(unsigned char x, unsigned char y) {
  return grid[x>>3][y>>3];
}


void claimTreasure(unsigned char tx, unsigned char ty) {
  // random amount of treasure based on the level
  score += ((rand8() % level)+1);
  // clear the treasure from the screen
  setAndDraw(tx,ty,EMPTY);
  // update the overall amount of gold the player has
  updateTreasureSprites();
  // indicate using sound that we got some treasure
  sfx_play(TREASURE_SOUND, TREASURE_CHANNEL);
}

void revealRoom(unsigned char px, unsigned char py) {
  signed char roomX = ((px >> 3)-LEFT_BORDER) / ROOM_WIDTH; 
  signed char roomY = ((py >> 3)-TOP_BORDER) / ROOM_HEIGHT; 
  fillRoom(roomX,roomY);
  sfx_play(REVEAL_ROOM_SOUND, REVEAL_ROOM_CHANNEL);
}

void handleInteraction(unsigned char px, unsigned char py, unsigned char val) {
  switch(val) {
   case STAIRS_UP:
    generatePreviousLevel();
    break;
   case STAIRS_DOWN:
    generateNextLevel();
    break;
   case FOG_TILE:
     revealRoom(px,py);
    break;
   case TREASURE:
    claimTreasure(px>>3, py>>3);
    break;
  }
}


// CLIPPING should be based on the FEET (bottom CENTER) of the player
void moveLeft(void) 
{
  unsigned char val = clipCheck(player_x - STEP, player_y + PLAYER_HEIGHT);
  if (val != STONE) {
     player_x-=STEP;
     handleInteraction(player_x, player_y + PLAYER_HEIGHT, val);
  }
}

void moveRight(void) 
{
  unsigned char val = clipCheck(player_x + STEP + PLAYER_WIDTH, player_y + PLAYER_HEIGHT);
  if (val != STONE) {
     player_x+=STEP;
     handleInteraction(player_x + PLAYER_WIDTH, player_y + PLAYER_HEIGHT, val);
  }
}

void moveUp(void) 
{
  unsigned char val = clipCheck(player_x + (PLAYER_WIDTH/2), player_y + PLAYER_HEIGHT-STEP);
  if (val != STONE) {
     player_y-=STEP;
     handleInteraction(player_x + (PLAYER_WIDTH/2), player_y + PLAYER_HEIGHT, val);
  }
}

void moveDown(void) 
{
  unsigned char val = clipCheck(player_x + (PLAYER_WIDTH/2), player_y+STEP+PLAYER_HEIGHT);
  if (val != STONE) {
     player_y+=STEP;
     handleInteraction(player_x + (PLAYER_WIDTH/2), player_y + PLAYER_HEIGHT, val);
  }
}

void checkAttack(void) 
{
}

void processInventory(void) 
{
    // game is paused while the player deals with inventory issues.
    // START resumes game
    // other controls manipulate inventory
    if (pad & PAD_START) {
        game_mode = GAME_MODE;
    }
}
void processGame() {

 // Step 1: get player input


  if (pad & PAD_START) {
      // inventory mode.  same as pausing
      game_mode = INVENTORY_MODE;
      return;
  }

   if (pad & PAD_A) {
       // attack mode.  player cannot move while attacking
       checkAttack();
   }
   // movement
   if(pad & PAD_LEFT) {
        moveLeft();
   } else if(pad & PAD_RIGHT){
      moveRight();
   }

   if(pad & PAD_UP) {
    moveUp();
   } else if(pad & PAD_DOWN){
    moveDown();
   }

  // deal with player attack
  // deal with enemy attacks
  // deal with enemy movement

}

void updateHealthSprites(){
    // 2 digit health. 
    int x = player_health;
    signed char pos = LAST_HEALTH_SPRITE_POS;
    while( x >= 10 ) {
       health_oam[pos] = HEALTH_SPRITE_OFFSET + x % 10;
       x = x / 10;
       pos -=3;
    }
    while(pos >= 0) {
       health_oam[pos] = BLANK_HEALTH;
       pos -=3;
    }
}

void updatePowerSprites(){
    // 2 digit power. 
    int x = player_power;
    signed char pos = LAST_POWER_SPRITE_POS;
    while( x >= 10 ) {
       power_oam[pos] = POWER_SPRITE_OFFSET + x % 10;
       x = x / 10;
       pos -=3;
    }
    while(pos >= 0) {
       power_oam[pos] = BLANK_POWER;
       pos -=3;
    }
}


// This method updates chunks of vram
void showStatusSprites(){
 if(vram_read_ptr != vram_write_ptr){
   if(vram_read_ptr + 15 <= vram_write_ptr){
     // try 5 bytes
     set_vram_update(5,vram_read_ptr);
     vram_read_ptr += 15;
   } else if(vram_read_ptr + 9 < vram_write_ptr){
     // try 3 bytes
     set_vram_update(3,vram_read_ptr);
     vram_read_ptr += 9;
   } else {
     // do a single byte
     set_vram_update(1,vram_read_ptr);
     vram_read_ptr += 3;
   }
   // handle wrap around
   if(vram_read_ptr > vram_queue + SIZEOF_QUEUE) {
     vram_read_ptr = vram_queue;
   }
 } else {
   // show the score.  this code will change...
   set_vram_update(5,score_oam);
 }
}



void main(void)
{
	// initialize read and write pointers to start of queue
        vram_read_ptr = vram_queue;
        vram_write_ptr = vram_queue;

        // --------------------------------------------------------------------------------------
        // PHASE 1:  TITLE SCREEN
        // TO DO:  Add title screen
        // TO DO: populate random number seed based on time taken to begin game from title screen
        // --------------------------------------------------------------------------------------
	// ppu_on_all();//enable rendering


        
        // --------------------------------------------------------------------------------------
        // PHASE 2:  Start Game at level 0

        // setup starting attributes
        game_mode = GAME_MODE;

        level = SURFACE_LEVEL;
        player_health = INITIAL_HEALTH;
        player_power = INITIAL_POWER;
        score = 0;

        // score changes as we run around the level
        memcpy(score_oam,score_placement,sizeof(score_placement));

        // setup palette
        ppu_on_all();
	pal_spr(palSprites);
        pal_bg(palSprites);


        generateStartingLevel();

        // Main Game loop

        pad = 0;
	while(1)
	{
                // -- game loop --
		ppu_waitnmi();//wait for next TV frame
		spr=0;

		spr=oam_spr(player_x,player_y,player_sprite,player_attribute, spr);
                // perform VRAM queue processing
                showStatusSprites();

                // need to make sure we clear out dead enemies
		//iterate over active enemies and set a sprite for current enemy
		//spr=oam_spr(enemy_x[i],enemy_y[i],0xA0 + enemy_sprite[i],i&3,spr);// i&3 is palette


                // end game if player is DEAD
                if (player_health == 0) {
		    gameOver();
		    break;
                }

                // strobe joypad to get input
                // store last strobe and compare for changes

                lastPad = pad;
                pad = pad_poll(0);
                if (game_mode ==  INVENTORY_MODE) {
                  processInventory();
                } else {
                  processGame();
                }

#ifdef DEV_MODE
		showLine();
#endif
	}
}
