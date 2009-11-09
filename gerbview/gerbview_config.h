/*************************/
/** gerberview_config.h **/
/*************************/

#include "param_config.h"

#define GROUP wxT("/gerbview")
#define GROUPLIB wxT("libraries")

#define INSETUP TRUE

/* Liste des parametres */

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
    &g_DesignSettings.m_LayerColor[0],
    GREEN
);

static PARAM_CFG_SETCOLOR ColorLayer1Cfg
(
    INSETUP,
    wxT("ColLay1"),
    &g_DesignSettings.m_LayerColor[1],
    BLUE
);

static PARAM_CFG_SETCOLOR ColorLayer2Cfg
(
    INSETUP,
    wxT("ColLay2"),
    &g_DesignSettings.m_LayerColor[2],
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer3Cfg
(
    INSETUP,
    wxT("ColLay3"),
    &g_DesignSettings.m_LayerColor[3],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer4Cfg
(
    INSETUP,
    wxT("ColLay4"),
    &g_DesignSettings.m_LayerColor[4],
    4
);

static PARAM_CFG_SETCOLOR ColorLayer5Cfg
(
    INSETUP,
    wxT("ColLay5"),
    &g_DesignSettings.m_LayerColor[5],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer6Cfg
(
    INSETUP,
    wxT("ColLay6"),
    &g_DesignSettings.m_LayerColor[6],
    6
);

static PARAM_CFG_SETCOLOR ColorLayer7Cfg
(
    INSETUP,
    wxT("ColLay7"),
    &g_DesignSettings.m_LayerColor[7],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer8Cfg
(
    INSETUP,
    wxT("ColLay8"),
    &g_DesignSettings.m_LayerColor[8],
    7
);

static PARAM_CFG_SETCOLOR ColorLayer9Cfg
(
    INSETUP,
    wxT("ColLay9"),
    &g_DesignSettings.m_LayerColor[9],
    1
);

static PARAM_CFG_SETCOLOR ColorLayer10Cfg
(
    INSETUP,
    wxT("ColLayA"),
    &g_DesignSettings.m_LayerColor[10],
    2
);

static PARAM_CFG_SETCOLOR ColorLayer11Cfg
(
    INSETUP,
    wxT("ColLayB"),
    &g_DesignSettings.m_LayerColor[11],
    3
);

static PARAM_CFG_SETCOLOR ColorLayer12Cfg
(
    INSETUP,
    wxT("ColLayC"),
    &g_DesignSettings.m_LayerColor[12],
    12
);

static PARAM_CFG_SETCOLOR ColorLayer13Cfg
(
    INSETUP,
    wxT("ColLayD"),
    &g_DesignSettings.m_LayerColor[13],
    13
);

static PARAM_CFG_SETCOLOR ColorLayer14Cfg
(
    INSETUP,
    wxT("ColLayE"),
    &g_DesignSettings.m_LayerColor[14],
    14
);

static PARAM_CFG_SETCOLOR ColorLayer15Cfg
(
    INSETUP,
    wxT("ColLayF"),
    &g_DesignSettings.m_LayerColor[15],
    RED
);

static PARAM_CFG_SETCOLOR ColorLayer16Cfg
(
    INSETUP,
    wxT("ColLayG"),
    &g_DesignSettings.m_LayerColor[16],
    1
);

static PARAM_CFG_SETCOLOR ColorLayer17Cfg
(
    INSETUP,
    wxT("ColLayH"),
    &g_DesignSettings.m_LayerColor[17],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer18Cfg
(
    INSETUP,
    wxT("ColLayI"),
    &g_DesignSettings.m_LayerColor[18],
    11
);

static PARAM_CFG_SETCOLOR ColorLayer19Cfg
(
    INSETUP,
    wxT("ColLayJ"),
    &g_DesignSettings.m_LayerColor[19],
    4
);

static PARAM_CFG_SETCOLOR ColorLayer20Cfg
(
    INSETUP,
    wxT("ColLayK"),
    &g_DesignSettings.m_LayerColor[20],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer21Cfg
(
    INSETUP,
    wxT("ColLayL"),
    &g_DesignSettings.m_LayerColor[21],
    3
);

static PARAM_CFG_SETCOLOR ColorLayer22Cfg
(
    INSETUP,
    wxT("ColLayM"),
    &g_DesignSettings.m_LayerColor[22],
    6
);

static PARAM_CFG_SETCOLOR ColorLayer23Cfg
(
    INSETUP,
    wxT("ColLayN"),
    &g_DesignSettings.m_LayerColor[23],
    5
);

static PARAM_CFG_SETCOLOR ColorLayer24Cfg
(
    INSETUP,
    wxT("ColLayO"),
    &g_DesignSettings.m_LayerColor[24],
    LIGHTGRAY
);

static PARAM_CFG_SETCOLOR ColorLayer25Cfg
(
    INSETUP,
    wxT("ColLayP"),
    &g_DesignSettings.m_LayerColor[25],
    1
);

static PARAM_CFG_SETCOLOR ColorLayer26Cfg
(
    INSETUP,
    wxT("ColLayQ"),
    &g_DesignSettings.m_LayerColor[26],
    2
);

static PARAM_CFG_SETCOLOR ColorLayer27Cfg
(
    INSETUP,
    wxT("ColLayR"),
    &g_DesignSettings.m_LayerColor[27],
    14
);

static PARAM_CFG_SETCOLOR ColorLayer28Cfg
(
    INSETUP,
    wxT("ColLayS"),
    &g_DesignSettings.m_LayerColor[28],
    YELLOW
);

static PARAM_CFG_SETCOLOR ColorLayer29Cfg
(
    INSETUP,
    wxT("ColLayT"),
    &g_DesignSettings.m_LayerColor[29],
    13
);

static PARAM_CFG_SETCOLOR ColorLayer30Cfg
(
    INSETUP,
    wxT("ColLayU"),
    &g_DesignSettings.m_LayerColor[30],
    14
);

static PARAM_CFG_SETCOLOR ColorLayer31Cfg
(
    INSETUP,
    wxT("ColLayV"),
    &g_DesignSettings.m_LayerColor[31],
    7
);


static PARAM_CFG_SETCOLOR ColorpcbGrilleCfg
(
    INSETUP,
    wxT("CoPcbGr"),
//@@IMB: Wrong object    &g_DesignSettings.m_PcbGridColor, /* Adresse du parametre */
    &g_GridColor,       //@@IMB: This is the real variable.
    DARKGRAY
);

static PARAM_CFG_SETCOLOR ColorDCodesCfg
(
    INSETUP,
    wxT("CoDCode"),
    &g_DCodesColor,
    WHITE
);

static PARAM_CFG_INT GERBERSpotMiniCfg
(
    wxT("GERBmin"),
    &g_Plot_Spot_Mini,
    15,
    2,0xFFFF
);

static PARAM_CFG_INT DrawSegmLargeurCfg
(
    wxT("DrawLar"),
    &g_DesignSettings.m_DrawSegmentWidth,
    120,
    0,10000
);

static PARAM_CFG_INT EdgeSegmLargeurCfg
(
    wxT("EdgeLar"),
    &g_DesignSettings.m_EdgeSegmentWidth,
    120,
    0,10000
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
    & DrawSegmLargeurCfg,
    & EdgeSegmLargeurCfg,
    & TimeOutCfg,
    & DisplPolairCfg,
    NULL
};
