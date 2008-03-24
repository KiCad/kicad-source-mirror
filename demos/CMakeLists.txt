install(DIRECTORY electric microwave pspice test_xil_95108 ecc83 interf_u
    pic_programmer "sonde xilinx" video
    DESTINATION ${KICAD_DEMOS} COMPONENT resources
    PATTERN ".svn" EXCLUDE)
