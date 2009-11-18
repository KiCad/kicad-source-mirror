/*****************************************************/
/** pcbcfg.h : configuration parameters for PCBNew  **/
/*****************************************************/

#include "param_config.h"

#define GROUP       wxT( "/pcbnew" )
#define GROUPLIB    wxT( "/pcbnew/libraries" )
#define GROUPCOMMON wxT( "/common" )

// Flag for member .m_Setup
// .m_Setup = TRUE: write info in user config
//      (i.e. for all project, in registry base or equivalent)
// .m_Setup = FALSE: write info in project config (i.e. only for this
//  project, in .pro file)
#define INSETUP TRUE


/* Configuration parameters. */

static PARAM_CFG_WXSTRING UserLibDirBufCfg
(
    wxT( "LibDir" ),
    &g_UserLibDirBuffer,
    GROUPLIB
);

static PARAM_CFG_LIBNAME_LIST LibNameBufCfg
(
    wxT( "LibName" ),
    &g_LibName_List,
    GROUPLIB
);

static PARAM_CFG_INT PadDrillCfg
(
    wxT( "PadDrlX" ),
    &g_Pad_Master.m_Drill.x,
    320,
    0, 0x7FFF
);

static PARAM_CFG_INT PadDimHCfg     //Pad Diameter / H Size
(
    wxT( "PadDimH" ),
    &g_Pad_Master.m_Size.x,
    550,
    0, 0x7FFF
);

static PARAM_CFG_INT PadDimVCfg
(
    wxT( "PadDimV" ),
    &g_Pad_Master.m_Size.y,
    550,
    0, 0x7FFF
);


static PARAM_CFG_INT LayerThicknessCfg
(
    wxT( "LayerThickness" ),
    &g_DesignSettings.m_LayerThickness,
    630,
    0, 0xFFFF
);

static PARAM_CFG_INT ViaShowHoleCfg
(
    INSETUP,
    wxT( "ViaSHole" ),
    &DisplayOpt.m_DisplayViaMode,
    VIA_SPECIAL_HOLE_SHOW,
    VIA_HOLE_NOT_SHOW,
    OPT_VIA_HOLE_END - 1
);

static PARAM_CFG_INT ShowNetNamesModeCfg
(
    INSETUP,
    wxT( "ShowNetNamesMode" ),
    &DisplayOpt.DisplayNetNamesMode,
    3,
    0,
    3
);

static PARAM_CFG_INT RouteLayTopCfg     // First current working layer
(
    wxT( "RouteTo" ),
    &Route_Layer_TOP,
    15,
    0, 15
);

static PARAM_CFG_INT RouteLayBotCfg     // second current working layer
(
    wxT( "RouteBo" ),
    &Route_Layer_BOTTOM,
    0,
    0, 15
);

static PARAM_CFG_BOOL Segm45Cfg     // 0, 90, and 45 degrees are the only
(                                   // valid segment orientations.
    wxT( "Segm45" ),
    &Track_45_Only,
    TRUE
);

static PARAM_CFG_BOOL Raccord45Cfg  // Generate connections at 45 degrees
(                                   // only.
    wxT( "Racc45" ),
    &g_Raccord_45_Auto,
    TRUE
);

static PARAM_CFG_INT UnitCfg        // Units: 0 inch, 1 mm
(
    INSETUP,
    wxT( "Unite" ),
    &g_UnitMetric,
    FALSE
);

static PARAM_CFG_BOOL SegmFillCfg
(
    INSETUP,
    wxT( "SegFill" ),
    &DisplayOpt.DisplayPcbTrackFill,
    TRUE
);


static PARAM_CFG_INT TrackDisplayClearanceCfg
(
    INSETUP,
    wxT( "TrackDisplayClearance" ),
    &DisplayOpt.ShowTrackClearanceMode,
    SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS
);

static PARAM_CFG_BOOL PadFillCfg
(
    INSETUP,
    wxT( "PadFill" ),
    &DisplayOpt.DisplayPadFill,
    TRUE
);

static PARAM_CFG_BOOL ViaFillCfg
(
    INSETUP,
    wxT( "ViaFill" ),
    &DisplayOpt.DisplayViaFill,
    TRUE
);

static PARAM_CFG_BOOL PadAfficheGardeCfg
(
    INSETUP,
    wxT( "PadAffG" ),
    &DisplayOpt.DisplayPadIsol,
    TRUE
);

static PARAM_CFG_BOOL PadShowNumCfg
(
    INSETUP,
    wxT( "PadSNum" ),
    &DisplayOpt.DisplayPadNum,
    TRUE
);

static PARAM_CFG_INT AfficheContourModuleCfg    // Module Edges: fill/line/sketch
(
    INSETUP,
    wxT( "ModAffC" ),
    &DisplayOpt.DisplayModEdge,
    FILLED,
    0, 2
);

static PARAM_CFG_INT AfficheTexteModuleCfg  // Module Texts: fill/line/sketch
(
    INSETUP,
    wxT( "ModAffT" ),
    &DisplayOpt.DisplayModText,
    FILLED,
    0, 2
);

static PARAM_CFG_INT AffichePcbTextCfg     // PCB Texts: fill/line/sketch
(
    INSETUP,
    wxT( "PcbAffT" ),
    &DisplayOpt.DisplayDrawItems,
    FILLED,
    0, 2
);

static PARAM_CFG_BOOL SegmPcb45Cfg  // Force 45 degrees for segments
(
    wxT( "SgPcb45" ),
    &Segments_45_Only,
    TRUE
);

static PARAM_CFG_INT PcbTextDimVCfg
(
    wxT( "TxtPcbV" ),
    &g_DesignSettings.m_PcbTextSize.y,
    600,
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE
);

static PARAM_CFG_INT PcbTextDimHCfg
(
    wxT( "TxtPcbH" ),
    &g_DesignSettings.m_PcbTextSize.x,
    600,
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE
);

static PARAM_CFG_SETCOLOR ColorLayer0Cfg   // CU Layer Color
(
    INSETUP,
    wxT( "ColLay0" ),
    &g_DesignSettings.m_LayerColor[0],
    GREEN
);

static PARAM_CFG_SETCOLOR ColorLayer1Cfg
(
    INSETUP,
    wxT( "ColLay1" ),
    &g_DesignSettings.m_LayerColor[1],
    BLUE
);

static PARAM_CFG_SETCOLOR ColorLayer2Cfg
(
    INSETUP,
    wxT( "ColLay2" ),
    &g_DesignSettings.m_LayerColor[2],
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer3Cfg
(
    INSETUP,
    wxT( "ColLay3" ),
    &g_DesignSettings.m_LayerColor[3],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer4Cfg
(
    INSETUP,
    wxT( "ColLay4" ),
    &g_DesignSettings.m_LayerColor[4],
    4
);

static PARAM_CFG_SETCOLOR ColorLayer5Cfg
(
    INSETUP,
    wxT( "ColLay5" ),
    &g_DesignSettings.m_LayerColor[5],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer6Cfg
(
    INSETUP,
    wxT( "ColLay6" ),
    &g_DesignSettings.m_LayerColor[6],
    6
);

static PARAM_CFG_SETCOLOR ColorLayer7Cfg
(
    INSETUP,
    wxT( "ColLay7" ),
    &g_DesignSettings.m_LayerColor[7],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer8Cfg
(
    INSETUP,
    wxT( "ColLay8" ),
    &g_DesignSettings.m_LayerColor[8],
    7
);

static PARAM_CFG_SETCOLOR ColorLayer9Cfg
(
    INSETUP,
    wxT( "ColLay9" ),
    &g_DesignSettings.m_LayerColor[9],
    1
);

static PARAM_CFG_SETCOLOR ColorLayer10Cfg
(
    INSETUP,
    wxT( "ColLayA" ),
    &g_DesignSettings.m_LayerColor[10],
    2
);

static PARAM_CFG_SETCOLOR ColorLayer11Cfg
(
    INSETUP,
    wxT( "ColLayB" ),
    &g_DesignSettings.m_LayerColor[11],
    3
);

static PARAM_CFG_SETCOLOR ColorLayer12Cfg
(
    INSETUP,
    wxT( "ColLayC" ),
    &g_DesignSettings.m_LayerColor[12],
    12
);

static PARAM_CFG_SETCOLOR ColorLayer13Cfg
(
    INSETUP,
    wxT( "ColLayD" ),
    &g_DesignSettings.m_LayerColor[13],
    13
);

static PARAM_CFG_SETCOLOR ColorLayer14Cfg
(
    INSETUP,
    wxT( "ColLayE" ),
    &g_DesignSettings.m_LayerColor[14],
    14
);

static PARAM_CFG_SETCOLOR ColorLayer15Cfg  // CMP Layer Color
(
    INSETUP,
    wxT( "ColLayF" ),
    &g_DesignSettings.m_LayerColor[15],
    RED
);

static PARAM_CFG_SETCOLOR ColorLayer16Cfg  // Adhesive CU Layer Color
(
    INSETUP,
    wxT( "ColLayG" ),
    &g_DesignSettings.m_LayerColor[16],
    1
);

static PARAM_CFG_SETCOLOR ColorLayer17Cfg  // Adhesive CMP Layer Color
(
    INSETUP,
    wxT( "ColLayH" ),
    &g_DesignSettings.m_LayerColor[17],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer18Cfg  // Solder Mask CU Layer Color
(
    INSETUP,
    wxT( "ColLayI" ),
    &g_DesignSettings.m_LayerColor[18],
    11
);

static PARAM_CFG_SETCOLOR ColorLayer19Cfg  // Solder Mask CMP Layer Color
(
    INSETUP,
    wxT( "ColLayJ" ),
    &g_DesignSettings.m_LayerColor[19],
    4
);

static PARAM_CFG_SETCOLOR ColorLayer20Cfg  // Silk Screen CU Layer Color
(
    INSETUP,
    wxT( "ColLayK" ),
    &g_DesignSettings.m_LayerColor[20],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer21Cfg  // Silk Screen CMP Layer Color
(
    INSETUP,
    wxT( "ColLayL" ),
    &g_DesignSettings.m_LayerColor[21],
    3
);

static PARAM_CFG_SETCOLOR ColorLayer22Cfg  // Mask CU Layer Color
(
    INSETUP,
    wxT( "ColLayM" ),
    &g_DesignSettings.m_LayerColor[22],
    6
);

static PARAM_CFG_SETCOLOR ColorLayer23Cfg  // Mask CMP Layer Color
(
    INSETUP,
    wxT( "ColLayN" ),
    &g_DesignSettings.m_LayerColor[23],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer24Cfg  // DRAW Layer Color
(
    INSETUP,
    wxT( "ColLayO" ),
    &g_DesignSettings.m_LayerColor[24],
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer25Cfg  // Comment Layer Color
(
    INSETUP,
    wxT( "ColLayP" ),
    &g_DesignSettings.m_LayerColor[25],
    1
);

static PARAM_CFG_SETCOLOR ColorLayer26Cfg  // ECO1 Layer Color
(
    INSETUP,
    wxT( "ColLayQ" ),
    &g_DesignSettings.m_LayerColor[26],
    2
);

static PARAM_CFG_SETCOLOR ColorLayer27Cfg  //ECO2 Layer Color
(
    INSETUP,
    wxT( "ColLayR" ),
    &g_DesignSettings.m_LayerColor[27],
    14
);

static PARAM_CFG_SETCOLOR ColorLayer28Cfg  // EDGES Layer Color
(
    INSETUP,
    wxT( "ColLayS" ),
    &g_DesignSettings.m_LayerColor[28],
    YELLOW
);

static PARAM_CFG_SETCOLOR ColorLayer29Cfg
(
    INSETUP,
    wxT( "ColLayT" ),
    &g_DesignSettings.m_LayerColor[29],
    13
);

static PARAM_CFG_SETCOLOR ColorLayer30Cfg
(
    INSETUP,
    wxT( "ColLayU" ),
    &g_DesignSettings.m_LayerColor[30],
    14
);

static PARAM_CFG_SETCOLOR ColorLayer31Cfg
(
    INSETUP,
    wxT( "ColLayV" ),
    &g_DesignSettings.m_LayerColor[31],
    7
);

static PARAM_CFG_SETCOLOR ColorTxtModCmpCfg
(
    INSETUP,
    wxT( "CTxtMoC" ),
    &g_ModuleTextCMPColor,
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorTxtModCuCfg
(
    INSETUP,
    wxT( "CTxtMoS" ),
    &g_ModuleTextCUColor,
    1
);

static PARAM_CFG_SETCOLOR VisibleTxtModCfg
(
    INSETUP,
    wxT( "CTxtVis" ),
    &g_ModuleTextNOVColor,
    DARKGRAY
);

static PARAM_CFG_INT TexteModDimVCfg
(
    wxT( "TxtModV" ),
    &ModuleTextSize.y,
    500,
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE
);

static PARAM_CFG_INT TexteModDimHCfg
(
    wxT( "TxtModH" ),
    &ModuleTextSize.x,
    500,
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE
);

static PARAM_CFG_INT TexteModWidthCfg
(
    wxT( "TxtModW" ),
    &ModuleTextWidth,
    100,
    1, TEXTS_MAX_WIDTH
);

static PARAM_CFG_SETCOLOR ColorAncreModCfg
(
    INSETUP,
    wxT( "CAncreM" ),
    &g_AnchorColor,
    BLUE
);

static PARAM_CFG_SETCOLOR ColorPadCuCfg
(
    INSETUP,
    wxT( "CoPadCu" ),
    &g_PadCUColor,
    GREEN
);

static PARAM_CFG_SETCOLOR ColorPadCmpCfg
(
    INSETUP,
    wxT( "CoPadCm" ),
    &g_PadCMPColor,
    RED
);

static PARAM_CFG_SETCOLOR ColorViaThroughCfg
(
    INSETUP,
    wxT( "CoViaTh" ),
    &g_DesignSettings.m_ViaColor[VIA_THROUGH],
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorViaBlindBuriedCfg
(
    INSETUP,
    wxT( "CoViaBu" ),
    &g_DesignSettings.m_ViaColor[VIA_BLIND_BURIED],
    BROWN
);

static PARAM_CFG_SETCOLOR ColorViaMicroViaCfg  // Buried Via Color
(
    INSETUP,
    wxT( "CoViaMi" ),
    &g_DesignSettings.m_ViaColor[VIA_MICROVIA],
    CYAN
);

static PARAM_CFG_SETCOLOR ColorpcbGrilleCfg
(
    INSETUP,
    wxT( "CoPcbGr" ),
    &g_GridColor,
    DARKGRAY
);

static PARAM_CFG_SETCOLOR ColorCheveluCfg
(
    INSETUP,
    wxT( "CoRatsN" ),
    &g_DesignSettings.m_RatsnestColor,
    WHITE
);

static PARAM_CFG_INT HPGLpenNumCfg
(
    wxT( "HPGLnum" ),
    &g_pcb_plot_options.HPGL_Pen_Num,
    1,
    1, 16
);

static PARAM_CFG_INT HPGLdiamCfg    // HPGL pen size (mils)
(
    wxT( "HPGdiam" ),
    &g_pcb_plot_options.HPGL_Pen_Diam,
    15,
    0, 100
);

static PARAM_CFG_INT HPGLspeedCfg   //HPGL pen speed (cm/s)
(
    wxT( "HPGLSpd" ),
    &g_pcb_plot_options.HPGL_Pen_Speed,
    20,
    0, 1000
);

static PARAM_CFG_INT HPGLrecouvrementCfg
(
    wxT( "HPGLrec" ),
    &g_pcb_plot_options.HPGL_Pen_Recouvrement,
    2,
    0, 0x100
);

static PARAM_CFG_INT VernisEpargneGardeCfg
(
    wxT( "VEgarde" ),
    &g_DesignSettings.m_SolderMaskMargin,
    100,
    0, 10000
);

static PARAM_CFG_INT DrawSegmLargeurCfg
(
    wxT( "DrawLar" ),
    &g_DesignSettings.m_DrawSegmentWidth,
    120,
    0, 0xFFFF
);

static PARAM_CFG_INT EdgeSegmLargeurCfg
(
    wxT( "EdgeLar" ),
    &g_DesignSettings.m_EdgeSegmentWidth,
    120,
    0, 0xFFFF
);

static PARAM_CFG_INT TexteSegmLargeurCfg
(
    wxT( "TxtLar" ),
    &g_DesignSettings.m_PcbTextWidth,
    120,
    0, 0xFFFF
);

static PARAM_CFG_INT ModuleSegmWidthCfg
(
    wxT( "MSegLar" ),
    &ModuleSegmentWidth,
    120,
    0, 0xFFFF
);

static PARAM_CFG_INT WTraitSerigraphiePlotCfg
(
    wxT( "WpenSer" ),
    &g_pcb_plot_options.PlotLine_Width,
    10,
    1, 10000
);

static PARAM_CFG_INT TimeOutCfg     // Automatic backup duration time in
(                                   // seconds.
    INSETUP,
    wxT( "TimeOut" ),
    &g_TimeOut,
    600,
    0, 60000
);

static PARAM_CFG_BOOL DisplPolairCfg
(
    INSETUP,
    wxT( "DPolair" ),
    &DisplayOpt.DisplayPolarCood,
    FALSE
);

static PARAM_CFG_INT PrmMaxLinksShowed
(
    INSETUP,
    wxT( "MaxLnkS" ),
    &g_MaxLinksShowed,
    3,
    0, 15
);

static PARAM_CFG_BOOL ShowRatsnestCfg
(
    INSETUP,
    wxT( "ShowRat" ),
    &g_Show_Ratsnest,
    FALSE
);

static PARAM_CFG_BOOL ShowModuleRatsnestCfg
(
    INSETUP,
    wxT( "ShowMRa" ),
    &g_Show_Module_Ratsnest,
    TRUE
);

static PARAM_CFG_BOOL TwoSegmentTrackBuildCfg
(
    INSETUP,
    wxT( "TwoSegT" ),
    &g_TwoSegmentTrackBuild,
    TRUE
);


/* parameters in this list will be saved on request (when saving config).
 */
PARAM_CFG_BASE* ParamCfgList[] =
{
    &UserLibDirBufCfg,
    &LibNameBufCfg,
    &PadDrillCfg,
    &PadDimHCfg,
    &PadDimVCfg,
    &ViaShowHoleCfg,
    &ShowNetNamesModeCfg,
    &LayerThicknessCfg,
    &RouteLayTopCfg,
    &RouteLayBotCfg,
    &Segm45Cfg,
    &Raccord45Cfg,
    &UnitCfg,
    &SegmFillCfg,
    &TrackDisplayClearanceCfg,
    &PadFillCfg,
    &ViaFillCfg,
    &PadAfficheGardeCfg,
    &PadShowNumCfg,
    &AfficheContourModuleCfg,
    &AfficheTexteModuleCfg,
    &AffichePcbTextCfg,
    &SegmPcb45Cfg,
    &PcbTextDimVCfg,
    &PcbTextDimHCfg,
    &ColorLayer0Cfg,
    &ColorLayer1Cfg,
    &ColorLayer2Cfg,
    &ColorLayer3Cfg,
    &ColorLayer4Cfg,
    &ColorLayer5Cfg,
    &ColorLayer6Cfg,
    &ColorLayer7Cfg,
    &ColorLayer8Cfg,
    &ColorLayer9Cfg,
    &ColorLayer10Cfg,
    &ColorLayer11Cfg,
    &ColorLayer12Cfg,
    &ColorLayer13Cfg,
    &ColorLayer14Cfg,
    &ColorLayer15Cfg,
    &ColorLayer16Cfg,
    &ColorLayer17Cfg,
    &ColorLayer18Cfg,
    &ColorLayer19Cfg,
    &ColorLayer20Cfg,
    &ColorLayer21Cfg,
    &ColorLayer22Cfg,
    &ColorLayer23Cfg,
    &ColorLayer24Cfg,
    &ColorLayer25Cfg,
    &ColorLayer26Cfg,
    &ColorLayer27Cfg,
    &ColorLayer28Cfg,
    &ColorLayer29Cfg,
    &ColorLayer30Cfg,
    &ColorLayer31Cfg,
    &ColorTxtModCmpCfg,
    &ColorTxtModCuCfg,
    &VisibleTxtModCfg,
    &TexteModDimVCfg,
    &TexteModDimHCfg,
    &TexteModWidthCfg,
    &ColorAncreModCfg,
    &ColorPadCuCfg,
    &ColorPadCmpCfg,
    &ColorViaThroughCfg,
    &ColorViaBlindBuriedCfg,
    &ColorViaMicroViaCfg,
    &ColorpcbGrilleCfg,
    &ColorCheveluCfg,
    &HPGLpenNumCfg,
    &HPGLdiamCfg,
    &HPGLspeedCfg,
    &HPGLrecouvrementCfg,
    &VernisEpargneGardeCfg,
    &DrawSegmLargeurCfg,
    &EdgeSegmLargeurCfg,
    &TexteSegmLargeurCfg,
    &ModuleSegmWidthCfg,
    &WTraitSerigraphiePlotCfg,
    &TimeOutCfg,
    &DisplPolairCfg,
    &PrmMaxLinksShowed,
    &ShowRatsnestCfg,
    &ShowModuleRatsnestCfg,
    &TwoSegmentTrackBuildCfg,

    NULL
};
