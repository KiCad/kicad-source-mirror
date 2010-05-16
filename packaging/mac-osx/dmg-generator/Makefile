###########
# KiCad OSX DMG packaging
# Create a bin directory and copy the .app files in
# Create a build directory
# Then execute the makefile
#
# Original credits by Adium developers !
# http://www.adium.im
###########
VERSION=`date +%Y%m%d`

###########
# Variables
###########
BUILD_DIR=build
APP_DIR=release
KICAD_DIR=$(BUILD_DIR)
RELEASE_NAME=kicad-$(VERSION)

###########
# Targets
###########
all:
	echo "Copying application bundles"
	mkdir release
	cp -R ../../../*/*.app release
	echo "Building kicad-${VERSION}.dmg"
	rm -rfv build
	mkdir build	

	cp -r $(APP_DIR)/* $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/.background
	cp Artwork/dmgBackground.png $(BUILD_DIR)/.background
	./mkalias -r /Applications $(BUILD_DIR)/Applications

	mkdir -p $(BUILD_DIR)/.background

	./make-diskimage.sh $(BUILD_DIR)/$(RELEASE_NAME).dmg $(KICAD_DIR) "KiCad" dmg_kicad.scpt $(ART_DIR)
	
	@echo "Build finished. `md5 $(BUILD_DIR)/$(RELEASE_NAME).dmg`"
