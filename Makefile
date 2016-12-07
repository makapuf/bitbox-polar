NAME=polar

GAME_C_FILES = $(NAME).c song.c lib/chiptune/chiptune.c lib/blitter/blitter.c lib/blitter/blitter_tmap.c lib/blitter/blitter_sprites.c
GAME_BINARY_FILES = sprite.spr tmap.tmap tmap.tset

include $(BITBOX)/kernel/bitbox.mk

$(NAME).c: tmap.h
song.c: polar.song
	python2 $(BITBOX)/scripts/song2C.py $^ > $@

# sprite
sprite.spr: spr_minus.png spr_zero.png spr_plus.png
	@mkdir -p $(dir $@)
	python2 $(BITBOX)/lib/blitter/scripts/couples_encode.py $@ $^

# tilemap / set
%.tmap %.tset %.h: %.tmx
	@mkdir -p $(dir $@)
	python2 $(BITBOX)/lib/blitter/scripts/tmx.py $^ > $*.h

clean::
	rm -rf *.spr *.tmap *.tset tmap.h