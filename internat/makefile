# Install GUI translations
include ../libs.linux

TARGETS=cs de es fr hu it ko pl pt ru sl

install: $(TARGETS)

$(KICAD_INTERNAT):
	mkdir -p $(KICAD_INTERNAT)

$(TARGETS): $(KICAD_INTERNAT)
	cp -R $@ $(KICAD_INTERNAT)

.PHONY: $(TARGETS)
