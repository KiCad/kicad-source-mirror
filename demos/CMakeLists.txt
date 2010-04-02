install(DIRECTORY complex_hierarchy ecc83 electric flat_hierarchy interf_u microwave
                  pic_programmer pspice "sonde xilinx" test_xil_95108 video
        DESTINATION ${KICAD_DEMOS}
        COMPONENT resources
        PATTERN ".svn" EXCLUDE)
