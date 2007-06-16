# Install component libraries
include ../libs.linux

$(KICAD_LIBRARY):
	mkdir -p $(KICAD_LIBRARY)

install: $(KICAD_LIBRARY)
	cp *.dcm *.lib *.sym $(KICAD_LIBRARY)

.PHONY: $(TARGETS)
