install(FILES kicad.pro
        DESTINATION ${KICAD_TEMPLATE}
        COMPONENT resources)

# template_lst is a list of all templates.
set( template_lst
    raspberrypi-gpio
    stm32f100-discovery-shield
    ti-stellaris-boosterpack40
    ti-stellaris-boosterpack40_min
    )

foreach( tpl ${template_lst} )
    install( DIRECTORY ${tpl}
        DESTINATION ${KICAD_TEMPLATE}
        COMPONENT templates )
endforeach()

