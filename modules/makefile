# Install modules and 3d models
include ../libs.linux

$(KICAD_MODULES):
	mkdir -p $(KICAD_MODULES)

install: $(KICAD_MODULES)
	cp *.brd *.equ *.mdc *.mod $(KICAD_MODULES)
	cp -R packages3d $(KICAD_MODULES)

.PHONY: $(TARGETS)
