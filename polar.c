#include <bitbox.h>
#include <math.h> // sqrtf

#include <lib/blitter/blitter.h>
#include <lib/chiptune/player.h>

#include "tmap.h"
	
#define SCREEN_X 40
#define SCREEN_Y 30

uint8_t vram[SCREEN_Y][SCREEN_X];
extern char sprite_spr[];
extern const struct ChipSong polar_chipsong;

object *bg, *sprite;

// game state
// ----

int level; // level 0 : intro, level 1 : game menu, next levels : games
float x,y, vx,vy;
int polarity; // current polarity tmap_ID, or zero.
int start_time;
int pause; 

// constants
// ----

#define START_LEVEL 0
#define REAL_LEVEL 3
#define GRAVITY .08f
#define DAMPING .97f;
#define ELECT 700.f

// code
// ----

void enter_level(int l)
{
	if (level==REAL_LEVEL-1) 
		start_time=vga_frame; // begin chronometer here

	level = l;

	// non jolie transition vram
	tmap_blit(bg,0,0, tmap_header, tmap_tmap[level]);

	vx=vy=0;
	x=200.f;y=10000.f;sprite->y=10000;
	polarity=0;
	pause = 30; // half second pause.
	
	if (level>=2) // dont bother for first levels
	{
		// player stop
		chip_play(0);

		// search start tmap & position sprite
		for (int j=0;j<SCREEN_Y;j++)
			for (int i=0;i<SCREEN_Y;i++)  // loop on all tmaps. could be faster to keep only pos and neg index ..
				if (vram[j][i] == tmap_neutral) {
					x = i*16; y=j*16; 
					vram[j][i]=1; 
					break;
				}
	}

}


void physics( void ) 
{
	// uses global polarity, vx,vy x,y
	vy += GRAVITY;

	if (!polarity) return; // only gravity if neutral
	
	int inv_polarity = polarity==tmap_plus ? tmap_minus:tmap_plus;

	for (int j=0;j<SCREEN_Y;j++)
		for (int i=0;i<SCREEN_X;i++) { // loop on all tmaps. could be faster to keep only pos and neg index ..
			if (vram[j][i]== polarity || vram[j][i]== inv_polarity) {

				float dx = x-(i*16+8); // keep sign since it is oriented
				float dy = y-(j*16+8); 
				float d = sqrtf(dx*dx + dy*dy); // dist to charge

				// too small distance does not count
				// message("%d %d : d %f +force %f %f\n",i,j,d, ELECT * dx/d/d/d, ELECT * dy/d/d/d);
				if (d<16) d=16;

				// fx = cos.f = cos.k/d^2 = k.cos.d/d3 =k.dx/d3, a=f/m, mcte, dvx = Kdx/d3
				if (vram[j][i] == inv_polarity) {
					vx -= ELECT * dx/d/d/d; 
					vy -= ELECT * dy/d/d/d; 
				} else {
					vx += ELECT * dx/d/d/d; 
					vy += ELECT * dy/d/d/d; 		
				}
			}
			// other tmaps don't do anything
		}
		// message("force %f %f\n", vx, vy);
}


int collide (int x, int y)
// return the touched tile at this position (0 : nothing, 1: block, 2:pike, 3:goal )
{

	int touched = vram[y/16][x/16]; 

	// vertical touch, x and y can be inverted
	switch (touched) {
		case tmap_block : 
		case tmap_block2 : 
			return 1;
			break;

		case tmap_pike : 
		case tmap_pike2 : 
		case tmap_pike3 : 
			return 2;
			break;

		case tmap_goal : 
			return 3;
			break;

		default : 
			return 0; 
			break;
	}
}

int max(int a, int b, int c, int d)
{
	int m=a;
	m = b>m ? b : m;
	m = c>m ? c : m;
	m = d>m ? d : m;
	return m;
}

int touch_square(int x, int y)
{
	return max(
		collide(x   ,y),
		collide(x+15,y),
		collide(x   ,y+15),
		collide(x+15,y+15)
	);
}


void game_init( void ) 
{
	blitter_init();
	bg = tilemap_new(tmap_tset,0,0,TMAP_HEADER(SCREEN_X,SCREEN_Y,TSET_16, TMAP_U8), vram); 
	sprite = sprite_new(sprite_spr,0,0,0);
	enter_level(START_LEVEL);
	chip_play(&polar_chipsong);
}

void game_frame( void ) {

	// little pause
	if (pause) {
		pause--;
		return; 
	} 
	
	if (y >= 512.f ) { // sprite not visible? menu level. 

		// animate tmaps : cycle between tmap_anim and tmap_anim_end
		if (vga_frame % 8==0)
			for (int j=0;j<SCREEN_Y;j++) 
				for (int i=0;i<SCREEN_X;i++) { 
					if (vram[j][i]>=tmap_anim && vram[j][i]<tmap_anim_end)
						vram[j][i]++;
					else if (vram[j][i] == tmap_anim_end)
						vram[j][i] = tmap_anim;
				}

		// just wait for keypress - wait a bit ?
		if (GAMEPAD_PRESSED(0,start) || GAMEPAD_PRESSED(0,A)) 
			enter_level(level+1);

	} else {
		// show time since beginning
		int t=(vga_frame-start_time)/60;
		vram[0][ 8] = tmap_zero + (t/100)%10;
		vram[0][ 9] = tmap_zero + (t/10)%10;
		vram[0][10] = tmap_zero + t%10;

		// input handling
		if (GAMEPAD_PRESSED(0,L) || GAMEPAD_PRESSED(0,A) || GAMEPAD_PRESSED(0,X)) { // plus
			polarity=tmap_plus;
			sprite->fr=2; 
		} else if (GAMEPAD_PRESSED(0,R) || GAMEPAD_PRESSED(0,B) || GAMEPAD_PRESSED(0,Y)) { // minus
			polarity=tmap_minus; 
			sprite->fr=0; 
		} else if (GAMEPAD_PRESSED(0,select)) { // select : restart
			enter_level(level); 
			sprite->fr=1; 
		} else {
			polarity=0;
			sprite->fr=1; 
		}

		physics(); // computes vx,vy

		// hit vertical wall after moving ?
		switch(touch_square(x+vx,y)) {
			case 0 : x += vx; break; // OK can move
			case 1 : vx =0.f; vy *= DAMPING;break; // dampen
			case 2 : enter_level(level);break;
			case 3 : enter_level(level+1); break;
		}

		// hit horizontal wall after moving ?
		switch(touch_square(x,y+vy)) {
			case 0 : y += vy; break; // OK can move
			case 1 : vy=0.f; vx *= DAMPING; break;// dampen
			case 2 : enter_level(level);break;
			case 3 : enter_level(level+1); break;
		}

		// message("%d %d tch %d\n",(int)(x/16)+(vx>0?1:0),(int)(y/16), touched);

		// update sprite
		sprite->x=x;
		sprite->y=y;

	}
}