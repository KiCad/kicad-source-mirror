/**********************************************************/
/** cfg.h : configuration: definition des structures  **/
/**********************************************************/

#include "param_config.h"

#define GROUP wxT("/gerbview")
#define GROUPLIB wxT("libraries")

#define INSETUP TRUE

/* Liste des parametres */

static PARAM_CFG_WXSTRING PhotoExtBufCfg
(
    wxT("PhoExt"),          /* identification */
    &g_PhotoFilenameExt     /* Adresse du parametre */
);

static PARAM_CFG_WXSTRING PenExtBufCfg
(
    wxT("PenExt"),          /* identification */
    &g_PenFilenameExt       /* Adresse du parametre */
);

static PARAM_CFG_WXSTRING DrillExtBufCfg
(
    wxT("DrilExt"),       /* identification */
    &g_DrillFilenameExt  /* Adresse du parametre */
);

static PARAM_CFG_INT UnitCfg    // Unites; 0 inche, 1 mm
(
    wxT("Unite"),           /* identification */
    &g_UnitMetric,      /* Adresse du parametre */
    FALSE               /* Valeur par defaut */
);

static PARAM_CFG_INT GerberScaleCfg // default scale; 0 2.3, 1 3.4
(
    wxT("Def_fmt"),         /* identification */
    &g_Default_GERBER_Format,           /* Adresse du parametre */
    23,                 /* Valeur par defaut */
    23, 66              /* Valeurs extremes */
);

static PARAM_CFG_BOOL SegmFillCfg
(
    INSETUP,
    wxT("SegFill"),             /* identification */
    &DisplayOpt.DisplayPcbTrackFill,            /* Adresse du parametre */
    TRUE                    /* Valeur par defaut */
);


static PARAM_CFG_INT PadFillCfg
(
    INSETUP,
    wxT("PadFill"),             /* identification */
    (int*)&DisplayOpt.DisplayPadFill, /* Adresse du parametre */
    TRUE                    /* Valeur par defaut */
);

static PARAM_CFG_INT ViaFillCfg
(
    INSETUP,
    wxT("ViaFill"),             /* identification */
    (int*)&DisplayOpt.DisplayViaFill, /* Adresse du parametre */
    TRUE                    /* Valeur par defaut */
);

static PARAM_CFG_BOOL PadShowNumCfg // Affiche DCodes
(
    INSETUP,
    wxT("PadSNum"),                 /* identification */
    &DisplayOpt.DisplayPadNum,  /* Adresse du parametre */
    TRUE                        /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer0Cfg
(
    INSETUP,
    wxT("ColLay0"),             /* identification */
    &g_DesignSettings.m_LayerColor[0],      /* Adresse du parametre */
    GREEN                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer1Cfg
(
    INSETUP,
    wxT("ColLay1"),             /* identification */
    &g_DesignSettings.m_LayerColor[1],      /* Adresse du parametre */
    BLUE                    /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer2Cfg
(
    INSETUP,
    wxT("ColLay2"),             /* identification */
    &g_DesignSettings.m_LayerColor[2],      /* Adresse du parametre */
    LIGHTGRAY               /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer3Cfg
(
    INSETUP,
    wxT("ColLay3"),             /* identification */
    &g_DesignSettings.m_LayerColor[3],      /* Adresse du parametre */
    5                       /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer4Cfg
(
    INSETUP,
    wxT("ColLay4"),             /* identification */
    &g_DesignSettings.m_LayerColor[4],      /* Adresse du parametre */
    4                       /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer5Cfg
(
    INSETUP,
    wxT("ColLay5"),             /* identification */
    &g_DesignSettings.m_LayerColor[5],      /* Adresse du parametre */
    5                       /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer6Cfg
(
    INSETUP,
    wxT("ColLay6"),             /* identification */
    &g_DesignSettings.m_LayerColor[6],      /* Adresse du parametre */
    6                       /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer7Cfg
(
    INSETUP,
    wxT("ColLay7"),             /* identification */
    &g_DesignSettings.m_LayerColor[7],      /* Adresse du parametre */
    5                       /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer8Cfg
(
    INSETUP,
    wxT("ColLay8"),             /* identification */
    &g_DesignSettings.m_LayerColor[8],      /* Adresse du parametre */
    7                       /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer9Cfg
(
    INSETUP,
    wxT("ColLay9"),         /* identification */
    &g_DesignSettings.m_LayerColor[9],  /* Adresse du parametre */
    1                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer10Cfg
(
    INSETUP,
    wxT("ColLayA"),         /* identification */
    &g_DesignSettings.m_LayerColor[10], /* Adresse du parametre */
    2                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer11Cfg
(
    INSETUP,
    wxT("ColLayB"),         /* identification */
    &g_DesignSettings.m_LayerColor[11], /* Adresse du parametre */
    3                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer12Cfg
(
    INSETUP,
    wxT("ColLayC"),         /* identification */
    &g_DesignSettings.m_LayerColor[12], /* Adresse du parametre */
    12                  /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer13Cfg
(
    INSETUP,
    wxT("ColLayD"),         /* identification */
    &g_DesignSettings.m_LayerColor[13], /* Adresse du parametre */
    13                  /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer14Cfg
(
    INSETUP,
    wxT("ColLayE"),         /* identification */
    &g_DesignSettings.m_LayerColor[14], /* Adresse du parametre */
    14                  /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer15Cfg
(
    INSETUP,
    wxT("ColLayF"),         /* identification */
    &g_DesignSettings.m_LayerColor[15], /* Adresse du parametre */
    RED             /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer16Cfg
(
    INSETUP,
    wxT("ColLayG"),         /* identification */
    &g_DesignSettings.m_LayerColor[16], /* Adresse du parametre */
    1                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer17Cfg
(
    INSETUP,
    wxT("ColLayH"),         /* identification */
    &g_DesignSettings.m_LayerColor[17], /* Adresse du parametre */
    5                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer18Cfg
(
    INSETUP,
    wxT("ColLayI"),         /* identification */
    &g_DesignSettings.m_LayerColor[18], /* Adresse du parametre */
    11                  /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer19Cfg
(
    INSETUP,
    wxT("ColLayJ"),         /* identification */
    &g_DesignSettings.m_LayerColor[19], /* Adresse du parametre */
    4                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer20Cfg
(
    INSETUP,
    wxT("ColLayK"),         /* identification */
    &g_DesignSettings.m_LayerColor[20], /* Adresse du parametre */
    5                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer21Cfg
(
    INSETUP,
    wxT("ColLayL"),         /* identification */
    &g_DesignSettings.m_LayerColor[21], /* Adresse du parametre */
    3                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer22Cfg
(
    INSETUP,
    wxT("ColLayM"),         /* identification */
    &g_DesignSettings.m_LayerColor[22], /* Adresse du parametre */
    6                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer23Cfg
(
    INSETUP,
    wxT("ColLayN"),         /* identification */
    &g_DesignSettings.m_LayerColor[23], /* Adresse du parametre */
    5                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer24Cfg
(
    INSETUP,
    wxT("ColLayO"),             /* identification */
    &g_DesignSettings.m_LayerColor[24],     /* Adresse du parametre */
    LIGHTGRAY               /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer25Cfg
(
    INSETUP,
    wxT("ColLayP"),         /* identification */
    &g_DesignSettings.m_LayerColor[25], /* Adresse du parametre */
    1                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer26Cfg
(
    INSETUP,
    wxT("ColLayQ"),         /* identification */
    &g_DesignSettings.m_LayerColor[26], /* Adresse du parametre */
    2                   /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer27Cfg
(
    INSETUP,
    wxT("ColLayR"),             /* identification */
    &g_DesignSettings.m_LayerColor[27],     /* Adresse du parametre */
    14                      /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer28Cfg
(
    INSETUP,
    wxT("ColLayS"),             /* identification */
    &g_DesignSettings.m_LayerColor[28],     /* Adresse du parametre */
    YELLOW                  /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer29Cfg
(
    INSETUP,
    wxT("ColLayT"),         /* identification */
    &g_DesignSettings.m_LayerColor[29], /* Adresse du parametre */
    13                  /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer30Cfg
(
    INSETUP,
    wxT("ColLayU"),         /* identification */
    &g_DesignSettings.m_LayerColor[30], /* Adresse du parametre */
    14                  /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorLayer31Cfg
(
    INSETUP,
    wxT("ColLayV"),         /* identification */
    &g_DesignSettings.m_LayerColor[31], /* Adresse du parametre */
    7                   /* Valeur par defaut */
);


static PARAM_CFG_SETCOLOR ColorpcbGrilleCfg
(
    INSETUP,
    wxT("CoPcbGr"),             /* identification */
//@@IMB: Wrong object    &g_DesignSettings.m_PcbGridColor, /* Adresse du parametre */
    &g_GridColor,       //@@IMB: This is the real variable.
    DARKGRAY                /* Valeur par defaut */
);

static PARAM_CFG_SETCOLOR ColorDCodesCfg
(
    INSETUP,
    wxT("CoDCode"),             /* identification */
    &g_DCodesColor, /* Adresse du parametre */
    WHITE               /* Valeur par defaut */
);

static PARAM_CFG_INT GERBERSpotMiniCfg
(
    wxT("GERBmin"),         /* identification */
    &g_Plot_Spot_Mini,          /* Adresse du parametre */
    15,                 /* Valeur par defaut */
    2,0xFFFF            /* Valeurs extremes */
);

static PARAM_CFG_INT DrawSegmLargeurCfg
(
    wxT("DrawLar"),         /* identification */
    &g_DesignSettings.m_DrawSegmentWidth,       /* Adresse du parametre */
    120,                /* Valeur par defaut */
    0,10000             /* Valeurs extremes */
);

static PARAM_CFG_INT EdgeSegmLargeurCfg
(
    wxT("EdgeLar"),         /* identification */
    &g_DesignSettings.m_EdgeSegmentWidth,       /* Adresse du parametre */
    120,                /* Valeur par defaut */
    0,10000             /* Valeurs extremes */
);

static PARAM_CFG_INT TimeOutCfg
(
    wxT("TimeOut"),         /* identification */
    &g_TimeOut,             /* Adresse du parametre */
    600,                    /* Valeur par defaut */
    0,60000                 /* Valeurs extremes */
);

static PARAM_CFG_BOOL DisplPolairCfg
(
    INSETUP,
    wxT("DPolair"),             /* identification */
    &DisplayOpt.DisplayPolarCood,   /* Adresse du parametre */
    FALSE                       /* Valeur par defaut */
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
