install(FILES kicad.pro
        DESTINATION ${KICAD_TEMPLATE}
        COMPONENT resources)

set( Templates raspberrypi-gpio stm32f100-discovery-shield )

foreach( Template ${Templates} )
	install( DIRECTORY ${Template}
		DESTINATION ${KICAD_TEMPLATE}
		COMPONENT ${Template} )
endforeach(Template)

