Q2_CC ?= clang
Q2_LD ?= ld.lld
CFLAGS ?= -O2 -Wall -Wextra -Werror
Q2_CFLAGS ?= -Oz -Wall -Wextra -Werror
Q2_FLAGS = --target=mipsel-linux-gnu -march=mips32r2 -mabi=32 -mfp64 -mnan=2008 -mabs=2008 \
	-mno-abicalls -fno-pic -G0 -ffreestanding -fno-builtin -fno-stack-protector \
	-fno-unwind-tables -fno-asynchronous-unwind-tables
SOURCES = src/game.c src/input.c src/screen.c
OBJECTS = $(patsubst src/%.c,build/%.o,$(SOURCES) src/platform_q2.c)
CARD = dist/.2048
LICENSES = $(CARD)/LICENSE-2048.txt $(CARD)/LICENSE-BUDDY.txt $(CARD)/LICENSE-FONT.txt

all: dist/q2-2048.zip

build/%.o: src/%.c src/game.h src/screen.h src/font.h
	mkdir -p $(@D)
	$(Q2_CC) $(Q2_FLAGS) $(Q2_CFLAGS) -std=c11 -c $< -o $@

$(CARD)/q2-2048: $(OBJECTS)
	mkdir -p $(@D)
	$(Q2_LD) -m elf32ltsmip -static -e __start -s -z noexecstack -o $@ $^

$(CARD)/q2app: q2app
	mkdir -p $(@D)
	cp $< $@

$(CARD)/LICENSE-2048.txt: LICENSE.txt
	mkdir -p $(@D)
	cp $< $@

$(CARD)/LICENSE-%.txt: LICENSE-%.txt
	mkdir -p $(@D)
	cp $< $@

dist/q2-2048.zip: $(CARD)/q2-2048 $(CARD)/q2app $(LICENSES)
	cd dist && zip -q q2-2048.zip $(patsubst dist/%,%,$^)

build/check: tests/check.c $(SOURCES) src/game.h src/screen.h src/font.h
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -std=c11 -Isrc -o $@ tests/check.c $(SOURCES)

check: build/check
	./build/check

clean:
	rm -rf build dist

.PHONY: all check clean
