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
    FRAME_SCH_VIEWER_MODAL,

    FRAME_PCB,
    FRAME_PCB_MODULE_EDITOR,
    FRAME_PCB_MODULE_VIEWER,
    FRAME_PCB_MODULE_VIEWER_MODAL,
    FRAME_PCB_FOOTPRINT_WIZARD_MODAL,
    FRAME_PCB_DISPLAY3D,

    FRAME_CVPCB,
    FRAME_CVPCB_DISPLAY,

    FRAME_GERBER,

    FRAME_PL_EDITOR,

    FRAME_BM2CMP,

    FRAME_CALC,

    KIWAY_PLAYER_COUNT,         // counts subset of FRAME_T's which are KIWAY_PLAYER derivatives

    // C++ project manager is not a KIWAY_PLAYER
    KICAD_MAIN_FRAME_T = KIWAY_PLAYER_COUNT,

    FRAME_T_COUNT
};

    //TEXT_EDITOR_FRAME_T,


#endif  // FRAME_T_H_
