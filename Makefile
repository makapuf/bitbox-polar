USE_ENGINE = 1
USE_CHIPTUNE = 1

NAME=polar

GAME_C_FILES = $(NAME).c song.c
GAME_BINARY_FILES = tmap.tset tmap.tmap build/sprite.spr

include $(BITBOX)/lib/bitbox.mk

build/$(NAME).o : build/tmap.h

# sprite
build/sprite.spr: spr_minus.png spr_zero.png spr_plus.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/couples_encode.py $@ $^ 

# tileset 
build/%.h build/%.tmap build/%.tset: %.tmx
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/tmx.py -o $(dir $@) $^ > $@

# utiliser le linker direct ?
%_dat.o: %
	@mkdir -p $(dir $@)
	cd $(dir $<); xxd -i $(notdir $<) | sed "s/unsigned/const unsigned/" > $(*F).c
	$(CC) $(ALL_CFLAGS) $(AUTODEPENDENCY_CFLAGS) -c $*.c -o $@

