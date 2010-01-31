/*************************/
/** gerberview_config.h **/
/*************************/

#include "param_config.h"
#include "colors_selection.h"

#define GROUP wxT("/gerbview")
#define GROUPLIB wxT("libraries")

#define INSETUP TRUE

/* Useful macro : */
#define LOC_COLOR(numlayer) &g_ColorsSettings.m_LayersColors[numlayer]

/* Config parameters list */

static PARAM_CFG_WXSTRING PhotoExtBufCfg
(
    wxT("PhoExt"),
    &g_PhotoFilenameExt
);

static PARAM_CFG_WXSTRING PenExtBufCfg
(
    wxT("PenExt"),
    &g_PenFilenameExt
);

static PARAM_CFG_WXSTRING DrillExtBufCfg
(
    wxT("DrilExt"),
    &g_DrillFilenameExt
);

static PARAM_CFG_INT UnitCfg    // Units; 0 inches, 1 mm
(
    wxT("Unite"),
    &g_UnitMetric,
    FALSE
);

static PARAM_CFG_INT GerberScaleCfg // default scale; 0 2.3, 1 3.4
(
    wxT("Def_fmt"),
    &g_Default_GERBER_Format,
    23,
    23, 66
);

static PARAM_CFG_BOOL SegmFillCfg
(
    INSETUP,
    wxT("SegFill"),
    &DisplayOpt.DisplayPcbTrackFill,
    TRUE
);


static PARAM_CFG_INT PadFillCfg
(
    INSETUP,
    wxT("PadFill"),
    (int*)&DisplayOpt.DisplayPadFill,
    TRUE
);

static PARAM_CFG_INT ViaFillCfg
(
    INSETUP,
    wxT("ViaFill"),
    (int*)&DisplayOpt.DisplayViaFill,
    TRUE
);

static PARAM_CFG_BOOL PadShowNumCfg  // Show DCodes
(
    INSETUP,
    wxT("PadSNum"),
    &DisplayOpt.DisplayPadNum,
    TRUE
);

static PARAM_CFG_SETCOLOR ColorLayer0Cfg
(
    INSETUP,
    wxT("ColLay0"),
    LOC_COLOR(0),
    GREEN
);

static PARAM_CFG_SETCOLOR ColorLayer1Cfg
(
    INSETUP,
    wxT("ColLay1"),
    LOC_COLOR(1),
    BLUE
);

static PARAM_CFG_SETCOLOR ColorLayer2Cfg
(
    INSETUP,
    wxT("ColLay2"),
    LOC_COLOR(2),
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer3Cfg
(
    INSETUP,
    wxT("ColLay3"),
    LOC_COLOR(3),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer4Cfg
(
    INSETUP,
    wxT("ColLay4"),
    LOC_COLOR(4),
    4
);

static PARAM_CFG_SETCOLOR ColorLayer5Cfg
(
    INSETUP,
    wxT("ColLay5"),
    LOC_COLOR(5),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer6Cfg
(
    INSETUP,
    wxT("ColLay6"),
    LOC_COLOR(6),
    6
);

static PARAM_CFG_SETCOLOR ColorLayer7Cfg
(
    INSETUP,
    wxT("ColLay7"),
    LOC_COLOR(7),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer8Cfg
(
    INSETUP,
    wxT("ColLay8"),
    LOC_COLOR(8),
    7
);

static PARAM_CFG_SETCOLOR ColorLayer9Cfg
(
    INSETUP,
    wxT("ColLay9"),
    LOC_COLOR(9),
    1
);

static PARAM_CFG_SETCOLOR ColorLayer10Cfg
(
    INSETUP,
    wxT("ColLayA"),
    LOC_COLOR(10),
    2
);

static PARAM_CFG_SETCOLOR ColorLayer11Cfg
(
    INSETUP,
    wxT("ColLayB"),
    LOC_COLOR(11),
    3
);

static PARAM_CFG_SETCOLOR ColorLayer12Cfg
(
    INSETUP,
    wxT("ColLayC"),
    LOC_COLOR(12),
    12
);

static PARAM_CFG_SETCOLOR ColorLayer13Cfg
(
    INSETUP,
    wxT("ColLayD"),
    LOC_COLOR(13),
    13
);

static PARAM_CFG_SETCOLOR ColorLayer14Cfg
(
    INSETUP,
    wxT("ColLayE"),
    LOC_COLOR(14),
    14
);

static PARAM_CFG_SETCOLOR ColorLayer15Cfg
(
    INSETUP,
    wxT("ColLayF"),
    LOC_COLOR(15),
    RED
);

static PARAM_CFG_SETCOLOR ColorLayer16Cfg
(
    INSETUP,
    wxT("ColLayG"),
    LOC_COLOR(16),
    1
);

static PARAM_CFG_SETCOLOR ColorLayer17Cfg
(
    INSETUP,
    wxT("ColLayH"),
    LOC_COLOR(17),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer18Cfg
(
    INSETUP,
    wxT("ColLayI"),
    LOC_COLOR(18),
    11
);

static PARAM_CFG_SETCOLOR ColorLayer19Cfg
(
    INSETUP,
    wxT("ColLayJ"),
    LOC_COLOR(19),
    4
);

static PARAM_CFG_SETCOLOR ColorLayer20Cfg
(
    INSETUP,
    wxT("ColLayK"),
    LOC_COLOR(20),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer21Cfg
(
    INSETUP,
    wxT("ColLayL"),
    LOC_COLOR(21),
    3
);

static PARAM_CFG_SETCOLOR ColorLayer22Cfg
(
    INSETUP,
    wxT("ColLayM"),
    LOC_COLOR(22),
    6
);

static PARAM_CFG_SETCOLOR ColorLayer23Cfg
(
    INSETUP,
    wxT("ColLayN"),
    LOC_COLOR(23),
    5
);

static PARAM_CFG_SETCOLOR ColorLayer24Cfg
(
    INSETUP,
    wxT("ColLayO"),
    LOC_COLOR(24),
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer25Cfg
(
    INSETUP,
    wxT("ColLayP"),
    LOC_COLOR(25),
    1
);

static PARAM_CFG_SETCOLOR ColorLayer26Cfg
(
    INSETUP,
    wxT("ColLayQ"),
    LOC_COLOR(26),
    2
);

static PARAM_CFG_SETCOLOR ColorLayer27Cfg
(
    INSETUP,
    wxT("ColLayR"),
    LOC_COLOR(27),
    14
);

static PARAM_CFG_SETCOLOR ColorLayer28Cfg
(
    INSETUP,
    wxT("ColLayS"),
    LOC_COLOR(28),
    YELLOW
);

static PARAM_CFG_SETCOLOR ColorLayer29Cfg
(
    INSETUP,
    wxT("ColLayT"),
    LOC_COLOR(29),
    13
);

static PARAM_CFG_SETCOLOR ColorLayer30Cfg
(
    INSETUP,
    wxT("ColLayU"),
    LOC_COLOR(30),
    14
);

static PARAM_CFG_SETCOLOR ColorLayer31Cfg
(
    INSETUP,
    wxT("ColLayV"),
    LOC_COLOR(31),
    7
);


static PARAM_CFG_SETCOLOR ColorpcbGrilleCfg
(
    INSETUP,
    wxT("CoPcbGr"),
    &g_GridColor,
    DARKGRAY
);

static PARAM_CFG_SETCOLOR ColorDCodesCfg
(
    INSETUP,
    wxT("CoDCode"),
    &g_ColorsSettings.m_ItemsColors[DCODES_VISIBLE],
    WHITE
);

static PARAM_CFG_INT GERBERSpotMiniCfg
(
    wxT("GERBmin"),
    &g_Plot_Spot_Mini,
    15,
    2,0xFFFF
);

static PARAM_CFG_INT TimeOutCfg
(
    wxT("TimeOut"),
    &g_TimeOut,
    600,
    0,60000
);

static PARAM_CFG_BOOL DisplPolairCfg
(
    INSETUP,
    wxT("DPolair"),
    &DisplayOpt.DisplayPolarCood,
    FALSE
);

PARAM_CFG_BASE * ParamCfgList[] =
{
    & PhotoExtBufCfg,
    & PenExtBufCfg,
    & DrillExtBufCfg,
    & UnitCfg,
    & GerberScaleCfg,
    & SegmFillCfg,
    & PadFillCfg,
    & ViaFillCfg,  //TODO: Will adding this line break tha pcbnew file compatibility?
    & PadShowNumCfg,
    & ColorLayer0Cfg,
    & ColorLayer1Cfg,
    & ColorLayer2Cfg,
    & ColorLayer3Cfg,
    & ColorLayer4Cfg,
    & ColorLayer5Cfg,
    & ColorLayer6Cfg,
    & ColorLayer7Cfg,
    & ColorLayer8Cfg,
    & ColorLayer9Cfg,
    & ColorLayer10Cfg,
    & ColorLayer11Cfg,
    & ColorLayer12Cfg,
    & ColorLayer13Cfg,
    & ColorLayer14Cfg,
    & ColorLayer15Cfg,
    & ColorLayer16Cfg,
    & ColorLayer17Cfg,
    & ColorLayer18Cfg,
    & ColorLayer19Cfg,
    & ColorLayer20Cfg,
    & ColorLayer21Cfg,
    & ColorLayer22Cfg,
    & ColorLayer23Cfg,
    & ColorLayer24Cfg,
    & ColorLayer25Cfg,
    & ColorLayer26Cfg,
    & ColorLayer27Cfg,
    & ColorLayer28Cfg,
    & ColorLayer29Cfg,
    & ColorLayer30Cfg,
    & ColorLayer31Cfg,
    & ColorpcbGrilleCfg,
    & ColorDCodesCfg,
    & GERBERSpotMiniCfg,
    & TimeOutCfg,
    & DisplPolairCfg,
    NULL
};
