USE_ENGINE = 1
USE_CHIPTUNE = 1

NAME=polar

GAME_C_FILES = $(NAME).c song.c
GAME_BINARY_FILES = sprite.spr tmap.tmap tmap.tset

include $(BITBOX)/lib/bitbox.mk

$(NAME).c: tmap.h

# sprite
sprite.spr: spr_minus.png spr_zero.png spr_plus.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/couples_encode.py $@ $^

# tilemap / set
%.tmap %.tset %.h: %.tmx
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/tmx.py $^ > $*.h

clean::
	rm -rf *.spr *.tmap *.tset tmap.h