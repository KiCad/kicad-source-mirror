/*****************************************************/
/** pcbcfg.h : configuration parameters for PCBNew  **/
/*****************************************************/

#include "param_config.h"
#include "colors_selection.h"

#define GROUP       wxT( "/pcbnew" )
#define GROUPLIB    wxT( "/pcbnew/libraries" )
#define GROUPCOMMON wxT( "/common" )

// Flag for member .m_Setup
// .m_Setup = TRUE: write info in user config
//      (i.e. for all project, in registry base or equivalent)
// .m_Setup = FALSE: write info in project config (i.e. only for this
//  project, in .pro file)
#define INSETUP TRUE

/* Useful macro : */
#define LOC_COLOR(layer) &g_ColorsSettings.m_LayersColors[layer]
#define ITEM_COLOR(item_visible) &g_ColorsSettings.m_ItemsColors[item_visible]

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
    LOC_COLOR(0),
    GREEN
);

static PARAM_CFG_SETCOLOR ColorLayer1Cfg
(
    INSETUP,
    wxT( "ColLay1" ),
    LOC_COLOR(1),
    BLUE
);

static PARAM_CFG_SETCOLOR ColorLayer2Cfg
(
    INSETUP,
    wxT( "ColLay2" ),
    LOC_COLOR(2),
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer3Cfg
(
    INSETUP,
    wxT( "ColLay3" ),
    LOC_COLOR(3),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer4Cfg
(
    INSETUP,
    wxT( "ColLay4" ),
    LOC_COLOR(4),
    4
);

static PARAM_CFG_SETCOLOR ColorLayer5Cfg
(
    INSETUP,
    wxT( "ColLay5" ),
    LOC_COLOR(5),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer6Cfg
(
    INSETUP,
    wxT( "ColLay6" ),
    LOC_COLOR(6),
    6
);

static PARAM_CFG_SETCOLOR ColorLayer7Cfg
(
    INSETUP,
    wxT( "ColLay7" ),
    LOC_COLOR(7),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer8Cfg
(
    INSETUP,
    wxT( "ColLay8" ),
    LOC_COLOR(8),
    7
);

static PARAM_CFG_SETCOLOR ColorLayer9Cfg
(
    INSETUP,
    wxT( "ColLay9" ),
    LOC_COLOR(9),
    1
);

static PARAM_CFG_SETCOLOR ColorLayer10Cfg
(
    INSETUP,
    wxT( "ColLayA" ),
    LOC_COLOR(10),
    2
);

static PARAM_CFG_SETCOLOR ColorLayer11Cfg
(
    INSETUP,
    wxT( "ColLayB" ),
    LOC_COLOR(11),
    3
);

static PARAM_CFG_SETCOLOR ColorLayer12Cfg
(
    INSETUP,
    wxT( "ColLayC" ),
    LOC_COLOR(12),
    12
);

static PARAM_CFG_SETCOLOR ColorLayer13Cfg
(
    INSETUP,
    wxT( "ColLayD" ),
    LOC_COLOR(13),
    13
);

static PARAM_CFG_SETCOLOR ColorLayer14Cfg
(
    INSETUP,
    wxT( "ColLayE" ),
    LOC_COLOR(14),
    14
);

static PARAM_CFG_SETCOLOR ColorLayer15Cfg  // CMP Layer Color
(
    INSETUP,
    wxT( "ColLayF" ),
    LOC_COLOR(15),
    RED
);

static PARAM_CFG_SETCOLOR ColorLayer16Cfg  // Adhesive CU Layer Color
(
    INSETUP,
    wxT( "ColLayG" ),
    LOC_COLOR(16),
    1
);

static PARAM_CFG_SETCOLOR ColorLayer17Cfg  // Adhesive CMP Layer Color
(
    INSETUP,
    wxT( "ColLayH" ),
    LOC_COLOR(17),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer18Cfg  // Solder Mask CU Layer Color
(
    INSETUP,
    wxT( "ColLayI" ),
    LOC_COLOR(18),
    11
);

static PARAM_CFG_SETCOLOR ColorLayer19Cfg  // Solder Mask CMP Layer Color
(
    INSETUP,
    wxT( "ColLayJ" ),
    LOC_COLOR(19),
    4
);

static PARAM_CFG_SETCOLOR ColorLayer20Cfg  // Silk Screen CU Layer Color
(
    INSETUP,
    wxT( "ColLayK" ),
    LOC_COLOR(20),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer21Cfg  // Silk Screen CMP Layer Color
(
    INSETUP,
    wxT( "ColLayL" ),
    LOC_COLOR(21),
    3
);

static PARAM_CFG_SETCOLOR ColorLayer22Cfg  // Mask CU Layer Color
(
    INSETUP,
    wxT( "ColLayM" ),
    LOC_COLOR(22),
    6
);

static PARAM_CFG_SETCOLOR ColorLayer23Cfg  // Mask CMP Layer Color
(
    INSETUP,
    wxT( "ColLayN" ),
    LOC_COLOR(23),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer24Cfg  // DRAW Layer Color
(
    INSETUP,
    wxT( "ColLayO" ),
    LOC_COLOR(24),
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer25Cfg  // Comment Layer Color
(
    INSETUP,
    wxT( "ColLayP" ),
    LOC_COLOR(25),
    1
);

static PARAM_CFG_SETCOLOR ColorLayer26Cfg  // ECO1 Layer Color
(
    INSETUP,
    wxT( "ColLayQ" ),
    LOC_COLOR(26),
    2
);

static PARAM_CFG_SETCOLOR ColorLayer27Cfg  //ECO2 Layer Color
(
    INSETUP,
    wxT( "ColLayR" ),
    LOC_COLOR(27),
    14
);

static PARAM_CFG_SETCOLOR ColorLayer28Cfg  // EDGES Layer Color
(
    INSETUP,
    wxT( "ColLayS" ),
    LOC_COLOR(28),
    YELLOW
);

static PARAM_CFG_SETCOLOR ColorLayer29Cfg
(
    INSETUP,
    wxT( "ColLayT" ),
    LOC_COLOR(29),
    13
);

static PARAM_CFG_SETCOLOR ColorLayer30Cfg
(
    INSETUP,
    wxT( "ColLayU" ),
    LOC_COLOR(30),
    14
);

static PARAM_CFG_SETCOLOR ColorLayer31Cfg
(
    INSETUP,
    wxT( "ColLayV" ),
    LOC_COLOR(31),
    7
);

static PARAM_CFG_SETCOLOR ColorTxtModCmpCfg
(
    INSETUP,
    wxT( "CTxtMoC" ),
    ITEM_COLOR(MOD_TEXT_FR_VISIBLE),
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorTxtModCuCfg
(
    INSETUP,
    wxT( "CTxtMoS" ),
    ITEM_COLOR(MOD_TEXT_BK_VISIBLE),
    BLUE
);

static PARAM_CFG_SETCOLOR VisibleTxtModCfg
(
    INSETUP,
    wxT( "CTxtVis" ),
    ITEM_COLOR(MOD_TEXT_INVISIBLE),
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
    ITEM_COLOR(ANCHOR_VISIBLE),
    BLUE
);

static PARAM_CFG_SETCOLOR ColorPadCuCfg
(
    INSETUP,
    wxT( "CoPadCu" ),
    ITEM_COLOR(PAD_BK_VISIBLE),
    GREEN
);

static PARAM_CFG_SETCOLOR ColorPadCmpCfg
(
    INSETUP,
    wxT( "CoPadCm" ),
    ITEM_COLOR(PAD_FR_VISIBLE),
    RED
);

static PARAM_CFG_SETCOLOR ColorViaThroughCfg
(
    INSETUP,
    wxT( "CoViaTh" ),
    ITEM_COLOR(VIA_THROUGH_VISIBLE),
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorViaBlindBuriedCfg
(
    INSETUP,
    wxT( "CoViaBu" ),
    ITEM_COLOR(VIA_BBLIND_VISIBLE),
    BROWN
);

static PARAM_CFG_SETCOLOR ColorViaMicroViaCfg  // Buried Via Color
(
    INSETUP,
    wxT( "CoViaMi" ),
    ITEM_COLOR(VIA_MICROVIA_VISIBLE),
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
    ITEM_COLOR(RATSNEST_VISIBLE),
    WHITE
);

static PARAM_CFG_INT HPGLpenNumCfg
(
    INSETUP,
    wxT( "HPGLnum" ),
    &g_pcb_plot_options.HPGL_Pen_Num,
    1,
    1, 16
);

static PARAM_CFG_INT HPGLdiamCfg    // HPGL pen size (mils)
(
    INSETUP,
    wxT( "HPGdiam" ),
    &g_pcb_plot_options.HPGL_Pen_Diam,
    15,
    0, 100
);

static PARAM_CFG_INT HPGLspeedCfg   //HPGL pen speed (cm/s)
(
    INSETUP,
    wxT( "HPGLSpd" ),
    &g_pcb_plot_options.HPGL_Pen_Speed,
    20,
    0, 1000
);

static PARAM_CFG_INT HPGLrecouvrementCfg
(
    INSETUP,
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
    &ShowModuleRatsnestCfg,
    &TwoSegmentTrackBuildCfg,

    NULL
};
