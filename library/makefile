# Install component libraries
include ../libs.linux

TARGETS=*.dcm *.lib *.sym

install: $(TARGETS)

$(KICAD_LIBRARY):
	mkdir -p $(KICAD_LIBRARY)

$(TARGETS): $(KICAD_LIBRARY)
	cp -R $@ $(KICAD_LIBRARY)

.PHONY: $(TARGETS)
