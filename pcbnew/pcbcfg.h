/**********************************************************/
/** pcbcfg.h : configuration: definition des structures  **/
/**********************************************************/

#include "param_config.h"

#define GROUP       wxT( "/pcbnew" )
#define GROUPLIB    wxT( "/pcbnew/libraries" )
#define GROUPCOMMON wxT( "/common" )

// Flag for member .m_Setup
// .m_Setup = TRUE: write info in user config
//		(i.e. for all project, in registry base or equivalent)
// .m_Setup = FALSE: write info in project config (i.e. only for this project, in .pro file)
#define INSETUP TRUE


/* Liste des parametres */

static PARAM_CFG_WXSTRING UserLibDirBufCfg
(
    wxT( "LibDir" ),            /* Keyword */
    &g_UserLibDirBuffer,        /* Parameter address */
    GROUPLIB
);

static PARAM_CFG_LIBNAME_LIST LibNameBufCfg
(
    wxT( "LibName" ),           /* Keyword */
    &g_LibName_List,            /* Parameter address */
    GROUPLIB
);

static PARAM_CFG_INT PadDrillCfg
(
    wxT( "PadDrlX" ),                   /* Keyword */
    &g_Pad_Master.m_Drill.x,            /* Parameter address */
    320,                                /* Default value */
    0, 0x7FFF                           /* Min and max values*/
);

static PARAM_CFG_INT PadDimHCfg     //Pad Diameter / H Size
(
    wxT( "PadDimH" ),               /* Keyword */
    &g_Pad_Master.m_Size.x,         /* Parameter address */
    550,                            /* Default value */
    0, 0x7FFF                       /* Min and max values*/
);

static PARAM_CFG_INT PadDimVCfg
(
    wxT( "PadDimV" ),           /* Keyword */
    &g_Pad_Master.m_Size.y,     /* Parameter address */
    550,                        /* Default value */
    0, 0x7FFF                   /* Min and max values*/
);


static PARAM_CFG_INT ViaDiametreCfg
(
    wxT( "ViaDiam" ),                           /* Keyword */
    &g_DesignSettings.m_CurrentViaSize,         /* Parameter address */
    450,                                        /* Default value */
    0, 0xFFFF                                   /* Min and max values*/
);

static PARAM_CFG_INT ViaDrillCfg
(
    wxT( "ViaDril" ),                           /* Keyword */
    &g_DesignSettings.m_ViaDrill,               /* Parameter address */
    250,                                        /* Default value */
    0, 0xFFFF                                   /* Min and max values*/
);

static PARAM_CFG_INT ViaAltDrillCfg
(
    wxT( "ViaAltD" ),                           /* Keyword */
    &g_DesignSettings.m_ViaDrillCustomValue,    /* Parameter address */
    250,                                        /* Default value */
    0, 0xFFFF                                   /* Min and max values*/
);

static PARAM_CFG_INT MicroViaDiametreCfg
(
    wxT( "MViaDia" ),                               /* Keyword */
    &g_DesignSettings.m_CurrentMicroViaSize,        /* Parameter address */
    200,                                            /* Default value */
    0, 1000                                         /* Min and max values*/
);

static PARAM_CFG_INT MicroViaDrillCfg
(
    wxT( "MViaDrl" ),                           /* Keyword */
    &g_DesignSettings.m_MicroViaDrill,          /* Parameter address */
    80,                                         /* Default value */
    0, 800                                      /* Min and max values*/
);

static PARAM_CFG_INT ViaShowHoleCfg
(
    INSETUP,
    wxT( "ViaSHole" ),                  /* Keyword */
    &DisplayOpt.m_DisplayViaMode,       /* Parameter address */
    VIA_SPECIAL_HOLE_SHOW,              /* Default value */
    VIA_HOLE_NOT_SHOW,                  /* Min and max values*/
    OPT_VIA_HOLE_END - 1                /* Min and max values*/
);

static PARAM_CFG_INT ShowNetNamesModeCfg
(
    INSETUP,
    wxT( "ShowNetNamesMode" ),                  /* Keyword */
    &DisplayOpt.DisplayNetNamesMode,            /* Parameter address */
    3,                                          /* Default value */
    0,                                          /* Min and max values*/
    3                                           /* Min and max values*/
);

static PARAM_CFG_INT TrackClearenceCfg
(
    wxT( "Isol" ),                                  /* Keyword */
    &g_DesignSettings.m_TrackClearence,             /* Parameter address */
    120,                                            /* Default value */
    0, 0xFFFF                                       /* Min and max values*/
);

static PARAM_CFG_INT LayerCountCfg                      // Mask Working Layers
(
    wxT( "Countlayer" ),                                /* Keyword */
    &g_DesignSettings.m_CopperLayerCount,               /* Parameter address */
    2,                                                  /* Default value */
    1, NB_COPPER_LAYERS                                 /* Min and max values*/
);

static PARAM_CFG_INT TrackWidthCfg
(
    wxT( "Lpiste" ),                                /* Keyword */
    &g_DesignSettings.m_CurrentTrackWidth,          /* Parameter address */
    170,                                            /* Default value */
    2, 0xFFFF                                       /* Min and max values*/
);

static PARAM_CFG_INT RouteLayTopCfg     // First current working layer
(
    wxT( "RouteTo" ),                   /* Keyword */
    &Route_Layer_TOP,                   /* Parameter address */
    15,                                 /* Default value */
    0, 15                               /* Min and max values*/
);

static PARAM_CFG_INT RouteLayBotCfg     // second current working layer
(
    wxT( "RouteBo" ),                   /* Keyword */
    &Route_Layer_BOTTOM,                /* Parameter address */
    0,                                  /* Default value */
    0, 15                               /* Min and max values*/
);

static PARAM_CFG_INT TypeViaCfg
(
    wxT( "TypeVia" ),                               /* Keyword */
    &g_DesignSettings.m_CurrentViaType,             /* Parameter address */
    VIA_THROUGH,                                    /* Default value */
    0, 3                                            /* Min and max values*/
);

static PARAM_CFG_BOOL Segm45Cfg     // Segm Pistes a 0, 45, 90 degres uniquement
(
    wxT( "Segm45" ),                /* Keyword */
    &Track_45_Only,                 /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_BOOL Raccord45Cfg  // Generation automatique des Raccords a 45 degres
(
    wxT( "Racc45" ),                /* Keyword */
    &g_Raccord_45_Auto,             /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_INT UnitCfg    // Units: 0 inch, 1 mm
(
    INSETUP,
    wxT( "Unite" ),             /* Keyword */
    &g_UnitMetric,              /* Parameter address */
    FALSE                       /* Default value */
);

static PARAM_CFG_BOOL SegmFillCfg
(
    INSETUP,
    wxT( "SegFill" ),                               /* Keyword */
    &DisplayOpt.DisplayPcbTrackFill,                /* Parameter address */
    TRUE                                            /* Default value */
);

static PARAM_CFG_BOOL NewTrackAfficheGardeCfg
(
    INSETUP,
    wxT( "NewAffG" ),               /* Keyword */
    &g_ShowIsolDuringCreateTrack,   /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_BOOL TrackAfficheGardeCfg
(
    INSETUP,
    wxT( "SegAffG" ),               /* Keyword */
    &DisplayOpt.DisplayTrackIsol,   /* Parameter address */
    FALSE                           /* Default value */
);

static PARAM_CFG_BOOL PadFillCfg
(
    INSETUP,
    wxT( "PadFill" ),               /* Keyword */
    &DisplayOpt.DisplayPadFill,     /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_BOOL PadAfficheGardeCfg
(
    INSETUP,
    wxT( "PadAffG" ),               /* Keyword */
    &DisplayOpt.DisplayPadIsol,     /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_BOOL PadShowNumCfg
(
    INSETUP,
    wxT( "PadSNum" ),               /* Keyword */
    &DisplayOpt.DisplayPadNum,      /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_INT AfficheContourModuleCfg    // Module Edges: fill/line/sketch
(
    INSETUP,
    wxT( "ModAffC" ),                           /* Keyword */
    &DisplayOpt.DisplayModEdge,                 /* Parameter address */
    FILLED,                                     /* Default value */
    0, 2                                        /* Min and max values*/
);

static PARAM_CFG_INT AfficheTexteModuleCfg  // Module Texts: fill/line/sketch
(
    INSETUP,
    wxT( "ModAffT" ),                       /* Keyword */
    &DisplayOpt.DisplayModText,             /* Parameter address */
    FILLED,                                 /* Default value */
    0, 2                                    /* Min and max values*/
);

static PARAM_CFG_INT AfficheTextePcbCfg     // PCB Texts: fill/line/sketch
(
    INSETUP,
    wxT( "PcbAffT" ),                       /* Keyword */
    &DisplayOpt.DisplayDrawItems,           /* Parameter address */
    FILLED,                                 /* Default value */
    0, 2                                    /* Min and max values*/
);

static PARAM_CFG_BOOL SegmPcb45Cfg  // Force 45 degrees for segments
(
    wxT( "SgPcb45" ),               /* Keyword */
    &Segments_45_Only,              /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_INT TextePcbDimVCfg
(
    wxT( "TxtPcbV" ),                                                       /* Keyword */
    &g_DesignSettings.m_PcbTextSize.y,                                      /* Parameter address */
    600,                                                                    /* Default value */
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE                                          /* Min and max values*/
);

static PARAM_CFG_INT TextePcbDimHCfg
(
    wxT( "TxtPcbH" ),                                                       /* Keyword */
    &g_DesignSettings.m_PcbTextSize.x,                                      /* Parameter address */
    600,                                                                    /* Default value */
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE                                          /* Min and max values*/
);

static PARAM_CFG_SETCOLOR ColorLayer0Cfg   // CU Layer Color
(
    INSETUP,
    wxT( "ColLay0" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[0],          /* Parameter address */
    GREEN                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer1Cfg
(
    INSETUP,
    wxT( "ColLay1" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[1],          /* Parameter address */
    BLUE                                        /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer2Cfg
(
    INSETUP,
    wxT( "ColLay2" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[2],          /* Parameter address */
    LIGHTGRAY                                   /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer3Cfg
(
    INSETUP,
    wxT( "ColLay3" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[3],          /* Parameter address */
    5                                           /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer4Cfg
(
    INSETUP,
    wxT( "ColLay4" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[4],          /* Parameter address */
    4                                           /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer5Cfg
(
    INSETUP,
    wxT( "ColLay5" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[5],          /* Parameter address */
    5                                           /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer6Cfg
(
    INSETUP,
    wxT( "ColLay6" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[6],          /* Parameter address */
    6                                           /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer7Cfg
(
    INSETUP,
    wxT( "ColLay7" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[7],          /* Parameter address */
    5                                           /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer8Cfg
(
    INSETUP,
    wxT( "ColLay8" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[8],          /* Parameter address */
    7                                           /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer9Cfg
(
    INSETUP,
    wxT( "ColLay9" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[9],      /* Parameter address */
    1                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer10Cfg
(
    INSETUP,
    wxT( "ColLayA" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[10],     /* Parameter address */
    2                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer11Cfg
(
    INSETUP,
    wxT( "ColLayB" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[11],     /* Parameter address */
    3                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer12Cfg
(
    INSETUP,
    wxT( "ColLayC" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[12],     /* Parameter address */
    12                                      /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer13Cfg
(
    INSETUP,
    wxT( "ColLayD" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[13],     /* Parameter address */
    13                                      /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer14Cfg
(
    INSETUP,
    wxT( "ColLayE" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[14],     /* Parameter address */
    14                                      /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer15Cfg  // CMP Layer Color
(
    INSETUP,
    wxT( "ColLayF" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[15],     /* Parameter address */
    RED                                     /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer16Cfg  // Adhesive CU Layer Color
(
    INSETUP,
    wxT( "ColLayG" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[16],     /* Parameter address */
    1                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer17Cfg  // Adhesive CMP Layer Color
(
    INSETUP,
    wxT( "ColLayH" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[17],     /* Parameter address */
    5                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer18Cfg  // Solder Mask CU Layer Color
(
    INSETUP,
    wxT( "ColLayI" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[18],     /* Parameter address */
    11                                      /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer19Cfg  // Solder Mask CMP Layer Color
(
    INSETUP,
    wxT( "ColLayJ" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[19],     /* Parameter address */
    4                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer20Cfg  // Silk Screen CU Layer Color
(
    INSETUP,
    wxT( "ColLayK" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[20],     /* Parameter address */
    5                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer21Cfg  // Silk Screen CMP Layer Color
(
    INSETUP,
    wxT( "ColLayL" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[21],     /* Parameter address */
    3                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer22Cfg  // Mask CU Layer Color
(
    INSETUP,
    wxT( "ColLayM" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[22],     /* Parameter address */
    6                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer23Cfg  // Mask CMP Layer Color
(
    INSETUP,
    wxT( "ColLayN" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[23],     /* Parameter address */
    5                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer24Cfg  // DRAW Layer Color
(
    INSETUP,
    wxT( "ColLayO" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[24],         /* Parameter address */
    LIGHTGRAY                                   /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer25Cfg  // Comment Layer Color
(
    INSETUP,
    wxT( "ColLayP" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[25],     /* Parameter address */
    1                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer26Cfg  // ECO1 Layer Color
(
    INSETUP,
    wxT( "ColLayQ" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[26],     /* Parameter address */
    2                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer27Cfg  //ECO2 Layer Color
(
    INSETUP,
    wxT( "ColLayR" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[27],         /* Parameter address */
    14                                          /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer28Cfg  // EDGES Layer Color
(
    INSETUP,
    wxT( "ColLayS" ),                           /* Keyword */
    &g_DesignSettings.m_LayerColor[28],         /* Parameter address */
    YELLOW                                      /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer29Cfg
(
    INSETUP,
    wxT( "ColLayT" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[29],     /* Parameter address */
    13                                      /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer30Cfg
(
    INSETUP,
    wxT( "ColLayU" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[30],     /* Parameter address */
    14                                      /* Default value */
);

static PARAM_CFG_SETCOLOR ColorLayer31Cfg
(
    INSETUP,
    wxT( "ColLayV" ),                       /* Keyword */
    &g_DesignSettings.m_LayerColor[31],     /* Parameter address */
    7                                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorTxtModCmpCfg
(
    INSETUP,
    wxT( "CTxtMoC" ),       /* Keyword */
    &g_ModuleTextCMPColor,  /* Parameter address */
    LIGHTGRAY               /* Default value */
);

static PARAM_CFG_SETCOLOR ColorTxtModCuCfg
(
    INSETUP,
    wxT( "CTxtMoS" ),       /* Keyword */
    &g_ModuleTextCUColor,   /* Parameter address */
    1                       /* Default value */
);

static PARAM_CFG_SETCOLOR VisibleTxtModCfg
(
    INSETUP,
    wxT( "CTxtVis" ),           /* Keyword */
    &g_ModuleTextNOVColor,      /* Parameter address */
    DARKGRAY                    /* Default value */
);

static PARAM_CFG_INT TexteModDimVCfg
(
    wxT( "TxtModV" ),                               /* Keyword */
    &ModuleTextSize.y,                              /* Parameter address */
    500,                                            /* Default value */
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE                  /* Min and max values*/
);

static PARAM_CFG_INT TexteModDimHCfg
(
    wxT( "TxtModH" ),               /* Keyword */
    &ModuleTextSize.x,              /* Parameter address */
    500,                            /* Default value */
    TEXTS_MIN_SIZE, TEXTS_MAX_SIZE  /* Min and max values*/
);

static PARAM_CFG_INT TexteModWidthCfg
(
    wxT( "TxtModW" ),               /* Keyword */
    &ModuleTextWidth,               /* Parameter address */
    100,                            /* Default value */
    1, TEXTS_MAX_WIDTH              /* Min and max values*/
);

static PARAM_CFG_SETCOLOR ColorAncreModCfg
(
    INSETUP,
    wxT( "CAncreM" ),       /* Keyword */
    &g_AnchorColor,         /* Parameter address */
    BLUE                    /* Default value */
);

static PARAM_CFG_SETCOLOR ColorPadCuCfg
(
    INSETUP,
    wxT( "CoPadCu" ),           /* Keyword */
    &g_PadCUColor,              /* Parameter address */
    GREEN                       /* Default value */
);

static PARAM_CFG_SETCOLOR ColorPadCmpCfg
(
    INSETUP,
    wxT( "CoPadCm" ),           /* Keyword */
    &g_PadCMPColor,             /* Parameter address */
    RED                         /* Default value */
);

static PARAM_CFG_SETCOLOR ColorViaThroughCfg
(
    INSETUP,
    wxT( "CoViaTh" ),                           /* Keyword */
    &g_DesignSettings.m_ViaColor[VIA_THROUGH],  /* Parameter address */
    LIGHTGRAY                                   /* Default value */
);

static PARAM_CFG_SETCOLOR ColorViaBlindBuriedCfg
(
    INSETUP,
    wxT( "CoViaBu" ),                                   /* Keyword */
    &g_DesignSettings.m_ViaColor[VIA_BLIND_BURIED],     /* Parameter address */
    BROWN                                               /* Default value */
);

static PARAM_CFG_SETCOLOR ColorViaMicroViaCfg  // Buried Via Color
(
    INSETUP,
    wxT( "CoViaMi" ),                                   /* Keyword */
    &g_DesignSettings.m_ViaColor[VIA_MICROVIA],         /* Parameter address */
    CYAN                                                /* Default value */
);

static PARAM_CFG_SETCOLOR ColorpcbGrilleCfg
(
    INSETUP,
    wxT( "CoPcbGr" ),           /* Keyword */
    &g_GridColor,               /* Parameter address */
    DARKGRAY                    /* Default value */
);

static PARAM_CFG_SETCOLOR ColorCheveluCfg
(
    INSETUP,
    wxT( "CoRatsN" ),                           /* Keyword */
    &g_DesignSettings.m_RatsnestColor,          /* Parameter address */
    WHITE                                       /* Default value */
);

static PARAM_CFG_INT HPGLpenNumCfg
(
    wxT( "HPGLnum" ),               /* Keyword */
    &g_HPGL_Pen_Num,                /* Parameter address */
    1,                              /* Default value */
    1, 16                           /* Min and max values*/
);

static PARAM_CFG_INT HPGLdiamCfg    // HPGL pen size (mils)
(
    wxT( "HPGdiam" ),               /* Keyword */
    &g_HPGL_Pen_Diam,               /* Parameter address */
    15,                             /* Default value */
    0, 100                          /* Min and max values*/
);

static PARAM_CFG_INT HPGLspeedCfg   //HPGL pen speed (cm/s)
(
    wxT( "HPGLSpd" ),               /* Keyword */
    &g_HPGL_Pen_Speed,              /* Parameter address */
    20,                             /* Default value */
    0, 1000                         /* Min and max values*/
);

static PARAM_CFG_INT HPGLrecouvrementCfg
(
    wxT( "HPGLrec" ),           /* Keyword */
    &g_HPGL_Pen_Recouvrement,   /* Parameter address */
    2,                          /* Default value */
    0, 0x100                    /* Min and max values*/
);

static PARAM_CFG_BOOL HPGLcenterCfg     //HPGL Org Coord ( 0 normal, 1 Centre)
(
    wxT( "HPGLorg" ),                   /* Keyword */
    &HPGL_Org_Centre,                   /* Parameter address */
    FALSE                               /* Default value */
);

static PARAM_CFG_INT VernisEpargneGardeCfg
(
    wxT( "VEgarde" ),                       /* Keyword */
    &g_DesignSettings.m_MaskMargin,         /* Parameter address */
    100,                                    /* Default value */
    0, 0xFFFF                               /* Min and max values*/
);

static PARAM_CFG_INT DrawSegmLargeurCfg
(
    wxT( "DrawLar" ),                               /* Keyword */
    &g_DesignSettings.m_DrawSegmentWidth,           /* Parameter address */
    120,                                            /* Default value */
    0, 0xFFFF                                       /* Min and max values*/
);

static PARAM_CFG_INT EdgeSegmLargeurCfg
(
    wxT( "EdgeLar" ),                               /* Keyword */
    &g_DesignSettings.m_EdgeSegmentWidth,           /* Parameter address */
    120,                                            /* Default value */
    0, 0xFFFF                                       /* Min and max values*/
);

static PARAM_CFG_INT TexteSegmLargeurCfg
(
    wxT( "TxtLar" ),                            /* Keyword */
    &g_DesignSettings.m_PcbTextWidth,           /* Parameter address */
    120,                                        /* Default value */
    0, 0xFFFF                                   /* Min and max values*/
);

static PARAM_CFG_INT ModuleSegmWidthCfg
(
    wxT( "MSegLar" ),               /* Keyword */
    &ModuleSegmentWidth,            /* Parameter address */
    120,                            /* Default value */
    0, 0xFFFF                       /* Min and max values*/
);

static PARAM_CFG_INT WTraitSerigraphiePlotCfg
(
    wxT( "WpenSer" ),           /* Keyword */
    &g_PlotLine_Width,          /* Parameter address */
    10,                         /* Default value */
    1, 10000                    /* Min and max values*/
);

static PARAM_CFG_INT TimeOutCfg     //Duree entre Sauvegardes auto en secondes
(
    INSETUP,
    wxT( "TimeOut" ),               /* Keyword */
    &g_TimeOut,                     /* Parameter address */
    600,                            /* Default value */
    0, 60000                        /* Min and max values*/
);

static PARAM_CFG_BOOL DisplPolairCfg
(
    INSETUP,
    wxT( "DPolair" ),                   /* Keyword */
    &DisplayOpt.DisplayPolarCood,       /* Parameter address */
    FALSE                               /* Default value */
);

static PARAM_CFG_INT CursorShapeCfg
(
    INSETUP,
    wxT( "CuShape" ),               /* Keyword */
    &g_CursorShape,                 /* Parameter address */
    0,                              /* Default value */
    0, 1                            /* Min and max values*/
);

static PARAM_CFG_INT PrmMaxLinksShowed
(
    INSETUP,
    wxT( "MaxLnkS" ),               /* Keyword */
    &g_MaxLinksShowed,              /* Parameter address */
    3,                              /* Default value */
    0, 15                           /* Min and max values*/
);

static PARAM_CFG_BOOL ShowRatsnestCfg
(
    INSETUP,
    wxT( "ShowRat" ),               /* Keyword */
    &g_Show_Ratsnest,               /* Parameter address */
    FALSE                           /* Default value */
);

static PARAM_CFG_BOOL ShowModuleRatsnestCfg
(
    INSETUP,
    wxT( "ShowMRa" ),               /* Keyword */
    &g_Show_Module_Ratsnest,        /* Parameter address */
    TRUE                            /* Default value */
);

static PARAM_CFG_BOOL TwoSegmentTrackBuildCfg
(
    INSETUP,
    wxT( "TwoSegT" ),               /* Keyword */
    &g_TwoSegmentTrackBuild,        /* Parameter address */
    TRUE                            /* Default value */
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
    &ViaDiametreCfg,
    &ViaDrillCfg,
    &ViaAltDrillCfg,
    &MicroViaDiametreCfg,
    &MicroViaDrillCfg,
    &ViaShowHoleCfg,
    &ShowNetNamesModeCfg,
    &TrackClearenceCfg,
    &LayerCountCfg,
    &TrackWidthCfg,
    &RouteLayTopCfg,
    &RouteLayBotCfg,
    &TypeViaCfg,
    &Segm45Cfg,
    &Raccord45Cfg,
    &UnitCfg,
    &SegmFillCfg,
    &TrackAfficheGardeCfg,
    &NewTrackAfficheGardeCfg,
    &PadFillCfg,
    &PadAfficheGardeCfg,
    &PadShowNumCfg,
    &AfficheContourModuleCfg,
    &AfficheTexteModuleCfg,
    &AfficheTextePcbCfg,
    &SegmPcb45Cfg,
    &TextePcbDimVCfg,
    &TextePcbDimHCfg,
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
    &HPGLcenterCfg,
    &VernisEpargneGardeCfg,
    &DrawSegmLargeurCfg,
    &EdgeSegmLargeurCfg,
    &TexteSegmLargeurCfg,
    &ModuleSegmWidthCfg,
    &WTraitSerigraphiePlotCfg,
    &TimeOutCfg,
    &DisplPolairCfg,
    &CursorShapeCfg,
    &PrmMaxLinksShowed,
    &ShowRatsnestCfg,
    &ShowModuleRatsnestCfg,
    &TwoSegmentTrackBuildCfg,

    NULL
};
