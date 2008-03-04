# Install help files & docs
include ../libs.linux

TARGETS=de en es fr it pt ru

install: $(TARGETS)

$(KICAD_DOCS):
	mkdir -p $(KICAD_DOCS)

$(TARGETS): $(KICAD_DOCS)
	cp -R $@ $(KICAD_DOCS)

.PHONY: $(TARGETS)
