#ifndef FRAME_T_H_
#define FRAME_T_H_

/**
 * Enum FRAME_T
 * is the set of EDA_BASE_FRAME derivatives, typically stored in
 * EDA_BASE_FRAME::m_Ident.
 */
enum FRAME_T
{
    FRAME_SCH,
    FRAME_SCH_LIB_EDITOR,
    FRAME_SCH_VIEWER,
    FRAME_PCB,
    FRAME_PCB_MODULE_EDITOR,
    FRAME_PCB_MODULE_VIEWER,
    FRAME_PCB_FOOTPRINT_WIZARD,
    FRAME_PCB_DISPLAY3D,
    FRAME_CVPCB,
    FRAME_CVPCB_DISPLAY,
    FRAME_GERBER,

    KIWAY_PLAYER_COUNT,         // counts subset of FRAME_T's tracked in class KIWAY

    KICAD_MAIN_FRAME_T = KIWAY_PLAYER_COUNT,
    FRAME_PL_EDITOR,
    //TEXT_EDITOR_FRAME_T,

    FRAME_T_COUNT
};

#endif  // FRAME_T_H_
