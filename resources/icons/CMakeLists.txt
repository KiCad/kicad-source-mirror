#
# Install the PNG icons
#

file(GLOB png_icons "*.png")

install (FILES ${png_icons}
         DESTINATION ${KICAD_PROGRAM_ICONS}
         COMPONENT resources)
