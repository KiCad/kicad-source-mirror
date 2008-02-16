/************************************************************/
/** eeconfig.h : configuration: definition des structures  **/
/************************************************************/

#ifndef eda_global
#define eda_global extern
#endif

#define GROUP       wxT( "/eeschema" )
#define GROUPCOMMON wxT( "/common" )
#define GROUPLIB    wxT( "libraries" )

#include "netlist.h" /* Definitions generales liees au calcul de netliste */

/* variables importees */
extern int g_PenMinWidth;

/* saving parameters option : */
#define INSETUP TRUE    // used when the parameter is saved in general config
                        // if not used, the parameter is saved in the loca config (project config)

/* Liste des parametres */
static PARAM_CFG_WXSTRING UserLibDirBufCfg
(
    wxT( "LibDir" ),                /* Ident String */
    &g_UserLibDirBuffer             /* Parameter address */
);


static PARAM_CFG_LIBNAME_LIST LibNameBufCfg
(
    wxT( "LibName" ),           /* Ident String */
    &g_LibName_List,            /* Parameter address */
    GROUPLIB                    /* Groupe */
);

static PARAM_CFG_INT NetFormatCfg
(
    wxT( "NetFmt" ),                            /* Ident String */
    &g_NetFormat,                               /* Parameter address */
    NET_TYPE_PCBNEW,                            /* Default  value */
    NET_TYPE_PCBNEW,                            /*  Min value for the parameter */
    NET_TYPE_CUSTOM_MAX                         /* Max value for the parameter */
);

static PARAM_CFG_INT UnitCfg
(
    INSETUP,
    wxT( "Unite" ),             /* Ident String */
    &g_UnitMetric,              /* Parameter address */
    0,                          /* Default  value */
    0, 1                        /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT CursorShapeCfg
(
    INSETUP,
    wxT( "CuShape" ),           /* Ident String */
    &g_CursorShape,             /* Parameter address */
    0,                          /* Default  value */
    0, 1                        /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT ShowGridCfg
(
    INSETUP,
    wxT( "ShGrid" ),            /* Ident String */
    &g_ShowGrid,                /* Parameter address */
    0, 1,                       /*  Min and Max values for the parameter */
    1                           /* Default  value */
);

static PARAM_CFG_SETCOLOR DrawBgColorCfg
(
    INSETUP,
    wxT( "BgColor" ),           /* Ident String */
    &g_DrawBgColor,             /* Parameter address */
    WHITE                       /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerWireCfg
(
    INSETUP,
    wxT( "ColWire" ),                           /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_WIRE],       /* Parameter address */
    GREEN                                       /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerBusCfg
(
    INSETUP,
    wxT( "ColorBus" ),                          /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_BUS],        /* Parameter address */
    BLUE                                        /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerJunctionCfg
(
    INSETUP,
    wxT( "ColorConn" ),                             /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_JUNCTION],       /* Parameter address */
    GREEN                                           /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerLLabelCfg
(
    INSETUP,
    wxT( "ColorLlab" ),                             /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_LOCLABEL],       /* Parameter address */
    BLACK                                           /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerHierarLabelCfg
(
    INSETUP,
    wxT( "ColorHlab" ),                             /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_HIERLABEL],      /* Parameter address */
    BROWN                                           /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerGLabelCfg
(
    INSETUP,
    wxT( "ColorGbllab" ),                             /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_GLOBLABEL],      /* Parameter address */
    RED                                             /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerPinFunCfg
(
    INSETUP,
    wxT( "ColorPinF" ),                         /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_PINFUN],     /* Parameter address */
    MAGENTA                                     /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerPinNumCfg
(
    INSETUP,
    wxT( "ColPinN" ),                           /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_PINNUM],     /* Parameter address */
    RED                                         /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerPinNamCfg
(
    INSETUP,
    wxT( "ColorPNam" ),                         /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_PINNAM],     /* Parameter address */
    CYAN                                        /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerFieldsCfg
(
    INSETUP,
    wxT( "ColorField" ),                        /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_FIELDS],     /* Parameter address */
    MAGENTA                                     /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerReferenceCfg
(
    INSETUP,
    wxT( "ColorRef" ),                              /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_REFERENCEPART],  /* Parameter address */
    CYAN                                            /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerValueCfg
(
    INSETUP,
    wxT( "ColorValue" ),                        /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_VALUEPART],  /* Parameter address */
    CYAN                                        /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerNotesCfg
(
    INSETUP,
    wxT( "ColorNote" ),                     /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_NOTES],  /* Parameter address */
    LIGHTBLUE                               /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerBodyCfg
(
    INSETUP,
    wxT( "ColorBody" ),                     /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_DEVICE], /* Parameter address */
    RED                                     /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerBodyBackgroundCfg
(
    INSETUP,
    wxT( "ColorBodyBg" ),                               /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_DEVICE_BACKGROUND],  /* Parameter address */
    LIGHTYELLOW                                         /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerNetNameCfg
(
    INSETUP,
    wxT( "ColorNetN" ),                     /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_NETNAM], /* Parameter address */
    DARKGRAY                                /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerPinCfg
(
    INSETUP,
    wxT( "ColorPin" ),                          /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_PIN],        /* Parameter address */
    RED                                         /* Default  value */
);


static PARAM_CFG_SETCOLOR ColorLayerSheetCfg
(
    INSETUP,
    wxT( "ColorSheet" ),                    /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_SHEET],  /* Parameter address */
    MAGENTA                                 /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerSheetFileNameCfg
(
    INSETUP,
    wxT( "ColorSheetFileName" ),                    /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_SHEETFILENAME],  /* Parameter address */
    BROWN                                           /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerSheetNameCfg
(
    INSETUP,
    wxT( "ColorSheetName" ),                    /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_SHEETNAME],  /* Parameter address */
    CYAN                                        /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerSheetLabelCfg
(
    INSETUP,
    wxT( "ColorSheetLab" ),                     /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_SHEETLABEL], /* Parameter address */
    BROWN                                       /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerNoConnectCfg
(
    INSETUP,
    wxT( "ColorNoCo" ),                         /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_NOCONNECT],  /* Parameter address */
    BLUE                                        /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerErcWarnCfg
(
    INSETUP,
    wxT( "ColorErcW" ),                         /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_ERC_WARN],   /* Parameter address */
    GREEN                                       /* Default  value */
);

static PARAM_CFG_SETCOLOR ColorLayerErcErrCfg
(
    INSETUP,
    wxT( "ColorErcE" ),                         /* Ident String */
    &g_LayerDescr.LayerColor[LAYER_ERC_ERR],    /* Parameter address */
    RED                                         /* Default  value */
);

static PARAM_CFG_INT PlotMarginCfg
(
    INSETUP,
    wxT( "Pltmarg" ),           /* Ident String */
    &g_PlotMargin,              /* Parameter address */
    300,                        /* Default  value */
    0, 10000                    /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT HPGLSpeed
(
    wxT( "HPGLSpd" ),                           /* Ident String */
    &g_HPGL_Pen_Descr.m_Pen_Speed,              /* Parameter address */
    20,                                         /* Default  value */
    2, 45                                       /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT HPGLDiam
(
    wxT( "HPGLDm" ),                            /* Ident String */
    &g_HPGL_Pen_Descr.m_Pen_Diam,               /* Parameter address */
    15,                                         /* Default  value */
    1, 150                                      /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT HPGLPenNum
(
    wxT( "HPGLNum" ),                           /* Ident String */
    &g_HPGL_Pen_Descr.m_Pen_Num,                /* Parameter address */
    1,                                          /* Default  value */
    1, 8                                        /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT PlotSheetOffsetX_A4
(
    wxT( "offX_A4" ),                   /* Ident String */
    &g_Sheet_A4.m_Offset.x              /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_A4
(
    wxT( "offY_A4" ),               /* Ident String */
    &g_Sheet_A4.m_Offset.y          /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetX_A3
(
    wxT( "offX_A3" ),               /* Ident String */
    &g_Sheet_A3.m_Offset.x          /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_A3
(
    wxT( "offY_A3" ),               /* Ident String */
    &g_Sheet_A3.m_Offset.y          /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetX_A2
(
    wxT( "offX_A2" ),               /* Ident String */
    &g_Sheet_A2.m_Offset.x          /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_A2
(
    wxT( "offY_A2" ),               /* Ident String */
    &g_Sheet_A2.m_Offset.y          /* Parameter address */
);


static PARAM_CFG_INT PlotSheetOffsetX_A1
(
    wxT( "offX_A1" ),               /* Ident String */
    &g_Sheet_A1.m_Offset.x          /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_A1
(
    wxT( "offY_A1" ),               /* Ident String */
    &g_Sheet_A1.m_Offset.y          /* Parameter address */
);


static PARAM_CFG_INT PlotSheetOffsetX_A0
(
    wxT( "offX_A0" ),               /* Ident String */
    &g_Sheet_A0.m_Offset.x          /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_A0
(
    wxT( "offY_A0" ),               /* Ident String */
    &g_Sheet_A0.m_Offset.y          /* Parameter address */
);


static PARAM_CFG_INT PlotSheetOffsetX_A
(
    wxT( "offX_A" ),                /* Ident String */
    &g_Sheet_A.m_Offset.x           /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_A
(
    wxT( "offY_A" ),                /* Ident String */
    &g_Sheet_A.m_Offset.y           /* Parameter address */
);


static PARAM_CFG_INT PlotSheetOffsetX_B
(
    wxT( "offX_B" ),                /* Ident String */
    &g_Sheet_B.m_Offset.x           /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_B
(
    wxT( "offY_B" ),                /* Ident String */
    &g_Sheet_B.m_Offset.y           /* Parameter address */
);


static PARAM_CFG_INT PlotSheetOffsetX_C
(
    wxT( "offX_C" ),                /* Ident String */
    &g_Sheet_C.m_Offset.x           /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_C
(
    wxT( "offY_C" ),                /* Ident String */
    &g_Sheet_C.m_Offset.y           /* Parameter address */
);


static PARAM_CFG_INT PlotSheetOffsetX_D
(
    wxT( "offX_D" ),                /* Ident String */
    &g_Sheet_D.m_Offset.x           /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_D
(
    wxT( "offY_D" ),                /* Ident String */
    &g_Sheet_D.m_Offset.y           /* Parameter address */
);


static PARAM_CFG_INT PlotSheetOffsetX_E
(
    wxT( "offX_E" ),                /* Ident String */
    &g_Sheet_E.m_Offset.x           /* Parameter address */
);

static PARAM_CFG_INT PlotSheetOffsetY_E
(
    wxT( "offY_E" ),                /* Ident String */
    &g_Sheet_E.m_Offset.y           /* Parameter address */
);

static PARAM_CFG_INT CfgRepeatDeltaX
(
    wxT( "RptD_X" ),                /* Ident String */
    &g_RepeatStep.x,                /* parameter address */
    0,                              /* Default  value */
    -1000, +1000                    /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT CfgRepeatDeltaY
(
    wxT( "RptD_Y" ),                /* Ident String */
    &g_RepeatStep.y,                /* Parameter address */
    100,                            /* Default  value */
    -1000, +1000                    /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT CfgRepeatDeltaLabel
(
    wxT( "RptLab" ),                /* Ident String */
    &g_RepeatDeltaLabel,            /* Parameter address */
    1,                              /* Default  value */
    -10, +10                        /*  Min and Max values for the parameter */
);

static PARAM_CFG_WXSTRING CfgSimulatorCommandLine
(
    wxT( "SimCmd" ),                /* Ident String */
    &g_SimulatorCommandLine         /* Parameter address */
);

static PARAM_CFG_INT OptNetListUseNamesCfg
(
    wxT( "UseNetN" ),               /* Ident String */
    &g_OptNetListUseNames,          /* Parameter address */
    0,                              /* Default  value */
    0, 1                            /*  Min and Max values for the parameter */
);

static PARAM_CFG_INT OptDefaultLabelSizeCfg
(
    wxT( "LabSize" ),               /* Ident String */
    &g_DefaultTextLabelSize,        /* Parameter address */
    DEFAULT_SIZE_TEXT,              /* Default  value */
    0, 1000                         /*  Min and Max values for the parameter */
);


PARAM_CFG_BASE* ParamCfgList[] =
{
    &UserLibDirBufCfg,
    &LibNameBufCfg,

    &NetFormatCfg,

    &UnitCfg,
    &CursorShapeCfg,
    &ShowGridCfg,
    &DrawBgColorCfg,
    &ColorLayerWireCfg,
    &ColorLayerBusCfg,
    &ColorLayerJunctionCfg,
    &ColorLayerLLabelCfg,
	&ColorLayerHierarLabelCfg,
    &ColorLayerGLabelCfg,
    &ColorLayerPinFunCfg,
    &ColorLayerPinNumCfg,
    &ColorLayerPinNamCfg,
    &ColorLayerFieldsCfg,
    &ColorLayerReferenceCfg,
    &ColorLayerValueCfg,
    &ColorLayerNotesCfg,
    &ColorLayerBodyCfg,
    &ColorLayerBodyBackgroundCfg,
    &ColorLayerNetNameCfg,
    &ColorLayerPinCfg,
    &ColorLayerSheetCfg,
    &ColorLayerSheetFileNameCfg,
    &ColorLayerSheetNameCfg,
    &ColorLayerSheetLabelCfg,
    &ColorLayerNoConnectCfg,
    &ColorLayerErcWarnCfg,
    &ColorLayerErcErrCfg,

    &PlotMarginCfg,
    &HPGLSpeed,
    &HPGLDiam,
    &HPGLPenNum,
    &PlotSheetOffsetX_A4,
    &PlotSheetOffsetY_A4,
    &PlotSheetOffsetX_A3,
    &PlotSheetOffsetY_A3,
    &PlotSheetOffsetX_A2,
    &PlotSheetOffsetY_A2,
    &PlotSheetOffsetX_A1,
    &PlotSheetOffsetY_A1,
    &PlotSheetOffsetX_A0,
    &PlotSheetOffsetY_A0,
    &PlotSheetOffsetX_A,
    &PlotSheetOffsetY_A,
    &PlotSheetOffsetX_B,
    &PlotSheetOffsetY_B,
    &PlotSheetOffsetX_C,
    &PlotSheetOffsetY_C,
    &PlotSheetOffsetX_D,
    &PlotSheetOffsetY_D,
    &PlotSheetOffsetX_E,
    &PlotSheetOffsetY_E,
    &CfgRepeatDeltaX,
    &CfgRepeatDeltaY,
    &CfgRepeatDeltaLabel,
    &CfgSimulatorCommandLine,
    &OptNetListUseNamesCfg,
    &OptDefaultLabelSizeCfg,
    NULL
};
