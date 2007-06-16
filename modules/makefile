# Install modules and 3d models
include ../libs.linux

TARGETS=*.brd *.equ *.mdc *.mod packages3d

install: $(TARGETS)

$(KICAD_MODULES):
	mkdir -p $(KICAD_MODULES)

$(TARGETS): $(KICAD_MODULES)
	cp -R $@ $(KICAD_MODULES)

.PHONY: $(TARGETS)
