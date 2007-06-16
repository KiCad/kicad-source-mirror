# Install template
include ../libs.linux

TARGETS=kicad.pro

install: $(TARGETS)

$(KICAD_TEMPLATE):
	mkdir -p $(KICAD_TEMPLATE)

$(TARGETS): $(KICAD_TEMPLATE)
	cp -R $@ $(KICAD_TEMPLATE)

.PHONY: $(TARGETS)
