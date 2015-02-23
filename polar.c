#include <bitbox.h>
#include <blitter.h>
#include <math.h> // sqrtf
#include <chiptune_player.h>

#include "song.h"

#include "build/tmap.h"
	
#define SCREEN_X 40
#define SCREEN_Y 30

uint8_t vram[SCREEN_Y][64];
extern char build_sprite_spr[];
extern const unsigned char songdata[];

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

#define REAL_LEVEL 3
#define GRAVITY .1f
#define DAMPING .97f;
#define ELECT 500.f

// code
// ----

void enter_level(int l)
{
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
		ply_init(0,0);

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
				if (vram[j][i] == polarity) {
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

void game_init( void ) 
{
	blitter_init();
	bg = tilemap_new(tmap_tset,0,0,TMAP_HEADER(64,SCREEN_Y,TSET_16, TMAP_U8), vram); 
	sprite = sprite_new(build_sprite_spr,0,0,0);
	enter_level(0);
	ply_init(SONGLEN,songdata);
}

void touch (int touched,float *y, float *vx, float *vy )
{
	// vertical touch, x and y can be inverted
	switch (touched) {
		case tmap_block : 
		case tmap_block2 : 
			// rewind move
			*y-=*vy;
			*vy=0.f; 
			*vx *= DAMPING;
			break;

		case tmap_pike : 
		case tmap_pike2 : 
		case tmap_pike3 : 
			enter_level(level);
			break;

		case tmap_goal : 
			// if we leave the first non start level, reset
			if (level==REAL_LEVEL-1) 
				start_time=vga_frame;
			enter_level(level+1);
			break;

		default : 
			// do nothing, dont collide
			break;
	}
}

void game_frame( void ) {

	ply_update();

	// little pause
	if (pause) {
		pause--;
		return; 
	} 
	
	kbd_emulate_gamepad();

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

		// input handling
		if (GAMEPAD_PRESSED(0,L) || GAMEPAD_PRESSED(0,A) || GAMEPAD_PRESSED(0,X)) { // plus
			polarity=tmap_plus;
			sprite->fr=0; 
		} else if (GAMEPAD_PRESSED(0,R) || GAMEPAD_PRESSED(0,B) || GAMEPAD_PRESSED(0,Y)) { // minus
			polarity=tmap_minus; 
			sprite->fr=2; 
		} else {
			polarity=0;
			sprite->fr=1; 
		}

		physics();
		x += vx; y+= vy;
		// update sprite
		sprite->x=x;
		sprite->y=y;
		
		// show time since beginning
		int t=(vga_frame-start_time)/60;
		vram[0][ 8] = tmap_zero + (t/100)%10;
		vram[0][ 9] = tmap_zero + (t/10)%10;
		vram[0][10] = tmap_zero + t%10;

		// vertical hit ?
		int touched = vram[(int)(y/16)+(vy>0?1:0)][(int)(x/16)]; // ??? 
		touch(touched,&y,&vx,&vy);

		// horizontal hit ? same inverting x and y
		touched = vram[(int)(y/16)+1][(int)(x/16)+(vx>0?1:0)]; // ??? 
		// message("%d %d tch %d\n",(int)(x/16)+(vx>0?1:0),(int)(y/16), touched);

		touch(touched,&x,&vy,&vx);
	}
}