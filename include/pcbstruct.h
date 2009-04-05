/**************************************************************/
/*  pcbstruct.h :  definition des structures de donnees type PCB */
/**************************************************************/

#ifndef PCBSTRUCT_H
#define PCBSTRUCT_H

#include "base_struct.h"
#include "class_base_screen.h"
#include "board_item_struct.h"

// Definitions relatives aux libariries
#define ENTETE_LIBRAIRIE "PCBNEW-LibModule-V1"
#define ENTETE_LIBDOC    "PCBNEW-LibDoc----V1"
#define L_ENTETE_LIB     18
#define EXT_DOC          wxT( ".mdc" )


/* Bits indicateurs du membre .Status, pour pistes, modules... */

#define FLAG1       (1 << 13)   /* flag libre pour calculs locaux */
#define FLAG0       (1 << 12)   /* flag libre pour calculs locaux */
#define BEGIN_ONPAD (1 << 11)   /* flag indiquant un debut de segment sur pad */
#define END_ONPAD   (1 << 10)   /* flag indiquant une fin de segment sur pad */
#define BUSY        (1 << 9)    /* flag indiquant que la structure a deja
                                 *  ete examinee, dans certaines routines */
#define DELETED     (1 << 8)    /* Bit flag de Status pour structures effacee
                                 *  et mises en chaine "DELETED" */
#define NO_TRACE    (1 << 7)    /* l'element ne doit pas etre affiche */

#define SURBRILL    (1 << 5)    /* element en surbrillance */
#define DRAG        (1 << 4)    /* segment en mode drag */
#define EDIT        (1 << 3)    /* element en cours d'edition */
#define SEGM_FIXE   (1 << 2)    /* segment FIXE ( pas d'effacement global ) */
#define SEGM_AR     (1 << 1)    /* segment Auto_Route */
#define CHAIN       (1 << 0)    /* segment marque  */


/* Layer identification (layer number) */
#define FIRST_COPPER_LAYER    0
#define COPPER_LAYER_N        0
#define LAYER_N_2             1     /* Numero layer 2 */
#define LAYER_N_3             2     /* Numero layer 3 */
#define LAYER_N_4             3     /* Numero layer 4 */
#define LAYER_N_5             4     /* Numero layer 5 */
#define LAYER_N_6             5     /* Numero layer 6 */
#define LAYER_N_7             6     /* Numero layer 7 */
#define LAYER_N_8             7     /* Numero layer 8 */
#define LAYER_N_9             8     /* Numero layer 9 */
#define LAYER_N_10            9     /* Numero layer 10 */
#define LAYER_N_11            10    /* Numero layer 11 */
#define LAYER_N_12            11    /* Numero layer 12 */
#define LAYER_N_13            12    /* Numero layer 13 */
#define LAYER_N_14            13    /* Numero layer 14 */
#define LAYER_N_15            14    /* Numero layer 15 */
#define LAYER_CMP_N           15
#define CMP_N                 15
#define LAST_COPPER_LAYER     15
#define NB_COPPER_LAYERS      (LAST_COPPER_LAYER + 1)

#define FIRST_NO_COPPER_LAYER 16
#define ADHESIVE_N_CU         16
#define ADHESIVE_N_CMP        17
#define SOLDERPASTE_N_CU      18
#define SOLDERPASTE_N_CMP     19
#define SILKSCREEN_N_CU       20
#define SILKSCREEN_N_CMP      21
#define SOLDERMASK_N_CU       22
#define SOLDERMASK_N_CMP      23
#define DRAW_N                24
#define COMMENT_N             25
#define ECO1_N                26
#define ECO2_N                27
#define EDGE_N                28
#define LAST_NO_COPPER_LAYER  28
#define NB_LAYERS             (LAST_NO_COPPER_LAYER + 1)

#define LAYER_COUNT           32


/*************************************/
/* constantes de gestion des couches */
/*************************************/
#define CUIVRE_LAYER            (1 << COPPER_LAYER_N)       ///< bit mask for copper layer
#define LAYER_2                 (1 << LAYER_N_2)            ///< bit mask for layer 2
#define LAYER_3                 (1 << LAYER_N_3)            ///< bit mask for layer 3
#define LAYER_4                 (1 << LAYER_N_4)            ///< bit mask for layer 4
#define LAYER_5                 (1 << LAYER_N_5)            ///< bit mask for layer 5
#define LAYER_6                 (1 << LAYER_N_6)            ///< bit mask for layer 6
#define LAYER_7                 (1 << LAYER_N_7)            ///< bit mask for layer 7
#define LAYER_8                 (1 << LAYER_N_8)            ///< bit mask for layer 8
#define LAYER_9                 (1 << LAYER_N_9)            ///< bit mask for layer 9
#define LAYER_10                (1 << LAYER_N_10)           ///< bit mask for layer 10
#define LAYER_11                (1 << LAYER_N_11)           ///< bit mask for layer 11
#define LAYER_12                (1 << LAYER_N_12)           ///< bit mask for layer 12
#define LAYER_13                (1 << LAYER_N_13)           ///< bit mask for layer 13
#define LAYER_14                (1 << LAYER_N_14)           ///< bit mask for layer 14
#define LAYER_15                (1 << LAYER_N_15)           ///< bit mask for layer 15
#define CMP_LAYER               (1 << LAYER_CMP_N)          ///< bit mask for component layer
#define ADHESIVE_LAYER_CU       (1 << ADHESIVE_N_CU)
#define ADHESIVE_LAYER_CMP      (1 << ADHESIVE_N_CMP)
#define SOLDERPASTE_LAYER_CU    (1 << SOLDERPASTE_N_CU)
#define SOLDERPASTE_LAYER_CMP   (1 << SOLDERPASTE_N_CMP)
#define SILKSCREEN_LAYER_CU     (1 << SILKSCREEN_N_CU)
#define SILKSCREEN_LAYER_CMP    (1 << SILKSCREEN_N_CMP)
#define SOLDERMASK_LAYER_CU     (1 << SOLDERMASK_N_CU)
#define SOLDERMASK_LAYER_CMP    (1 << SOLDERMASK_N_CMP)
#define DRAW_LAYER              (1 << DRAW_N)
#define COMMENT_LAYER           (1 << COMMENT_N)
#define ECO1_LAYER              (1 << ECO1_N)
#define ECO2_LAYER              (1 << ECO2_N)
#define EDGE_LAYER              (1 << EDGE_N)

#define FIRST_NON_COPPER_LAYER  ADHESIVE_N_CU
#define LAST_NON_COPPER_LAYER  EDGE_N

//      extra bits              0xE0000000
/* masques generaux : */
#define ALL_LAYERS              0x1FFFFFFF
#define ALL_NO_CU_LAYERS        0x1FFF0000
#define ALL_CU_LAYERS           0x0000FFFF
#define INTERNAL_LAYERS         0x00007FFE      /* Bits layers internes */
#define EXTERNAL_LAYERS         0x00008001

/* Forward declaration */
class EQUIPOT;
class MARKER;
struct CHEVELU;

//class Ki_PageDescr;
//class DrawBlockStruct;


/* main window classes : */
#include "wxPcbStruct.h"

/* Class to handle a board */
#include "class_board.h"

// Class for handle current printed board design settings
#define HISTORY_NUMBER 8
class EDA_BoardDesignSettings
{
public:
    int    m_CopperLayerCount;                  // Number of copper layers for this design
    int    m_ViaDrill;                          // via drill (for the entire board)
    int    m_ViaDrillCustomValue;               // via drill for vias which must have a defined drill value
    int    m_MicroViaDrill;                     // micro via drill (for the entire board)
    int    m_CurrentViaSize;                    // Current via size
    int    m_CurrentMicroViaSize;               // Current micro via size
    bool   m_MicroViasAllowed;                  // true to allow micro vias
    int    m_ViaSizeHistory[HISTORY_NUMBER];    // Last HISTORY_NUMBER used via sizes
    int    m_CurrentViaType;                    // via type (VIA_BLIND_BURIED, VIA_TROUGHT VIA_MICROVIA)
    int    m_CurrentTrackWidth;                 // current track width
    bool   m_UseConnectedTrackWidth;            // if true, when creating a new track starting on an existing track, use this track width
    int    m_TrackWidthHistory[HISTORY_NUMBER]; // Last HISTORY_NUMBER used track widths
    int    m_DrawSegmentWidth;                  // current graphic line width (not EDGE layer)
    int    m_EdgeSegmentWidth;                  // current graphic line width (EDGE layer only)
    int    m_PcbTextWidth;                      // current Pcb (not module) Text width
    wxSize m_PcbTextSize;                       // current Pcb (not module) Text size
    int    m_TrackClearence;                    // track to track and track to pads clearance
    int    m_MaskMargin;                        // Solder mask margin

    // Color options for screen display of the Printed Board:
    int    m_PcbGridColor;                      // Grid color

    int    m_LayerColor[32];                    // Layer colors (tracks and graphic items)

    int    m_ViaColor[4];                       // Via color (depending on is type)
    int    m_ModuleTextCMPColor;                // Text module color for modules on the COMPONENT layer
    int    m_ModuleTextCUColor;                 // Text module color for modules on the COPPER layer
    int    m_ModuleTextNOVColor;                // Text module color for "invisible" texts (must be BLACK if really not displayed)
    int    m_AnchorColor;                       // Anchor color for modules and texts

    int    m_PadCUColor;                        // Pad color for the COPPER side of the pad
    int    m_PadCMPColor;                       // Pad color for the COMPONENT side of the pad
    // Pad color for the pads of both sides is m_PadCUColor OR m_PadCMPColor (in terms of colors)

    int    m_RatsnestColor;                     // Ratsnest color

public:
    EDA_BoardDesignSettings();

    /**
     * Function GetVisibleLayers
     * returns a bit-map of all the layers that are visible.
     * @return int - the visible layers in bit-mapped form.
     */
    int    GetVisibleLayers() const;
};


// Values for m_DisplayViaMode member:
enum DisplayViaMode {
    VIA_HOLE_NOT_SHOW = 0,
    VIA_SPECIAL_HOLE_SHOW,
    ALL_VIA_HOLE_SHOW,
    OPT_VIA_HOLE_END
};

/* Handle info to display a board */
class PCB_SCREEN : public BASE_SCREEN
{
public:
    int m_Active_Layer;             /* ref couche active */
    int m_Route_Layer_TOP;          /* ref couches actives */
    int m_Route_Layer_BOTTOM;       /* pour placement vias et routage 2 couches */

public:
    PCB_SCREEN();
    ~PCB_SCREEN();

    PCB_SCREEN* Next() { return (PCB_SCREEN*) Pnext; }
    void        Init();
    void        SetNextZoom();
    void        SetPreviousZoom();
    void        SetLastZoom();

    virtual int GetInternalUnits( void );

    /**
     * Function GetCurItem
     * returns the currently selected BOARD_ITEM, overriding BASE_SCREEN::GetCurItem().
     * @return BOARD_ITEM* - the one selected, or NULL.
     */
    BOARD_ITEM* GetCurItem() const {  return (BOARD_ITEM*) BASE_SCREEN::GetCurItem(); }

    /**
     * Function SetCurItem
     * sets the currently selected object, m_CurrentItem.
     * @param aItem Any object derived from BOARD_ITEM
     */
    void SetCurItem( BOARD_ITEM* aItem ) { BASE_SCREEN::SetCurItem( aItem ); }


    /* Return true if a microvia can be put on board
     * A microvia ia a small via restricted to 2 near neighbour layers
     * because its is hole is made by laser which can penetrate only one layer
     * It is mainly used to connect BGA to the first inner layer
     * And it is allowed from an external layer to the first inner layer
     */
    bool IsMicroViaAcceptable( void );
};

/**********************************/
/* Module (Footprint) description */
/**********************************/

#include "class_pad.h"          // class for pads
#include "class_edge_mod.h"     // Class for  footprint graphic elements
#include "class_text_mod.h"     // Class for  footprint fields
#include "class_module.h"       // Class for the footprint
#include "class_equipot.h"


/***********************************/
/* Description des elements du PCB */
/***********************************/

#include "class_drawsegment.h"
#include "class_pcb_text.h"
#include "class_cotation.h"
#include "class_mire.h"
#include "class_track.h"
#include "class_marker.h"
#include "class_zone.h"


class DISPLAY_OPTIONS
{
public:
    bool DisplayPadFill;
    bool DisplayPadNum;
    bool DisplayPadNoConn;
    bool DisplayPadIsol;

    int  DisplayModEdge;
    int  DisplayModText;
    bool DisplayPcbTrackFill;   /* FALSE = sketch , TRUE = filled */
    bool DisplayTrackIsol;

    int  m_DisplayViaMode;      /* 0 do not show via hole,
                                 * 1 show via hole for non default value
                                 * 2 show all via hole */

    bool DisplayPolarCood;
    int  DisplayZonesMode;
    int  DisplayNetNamesMode;   /* 0 do not show netnames,
                                 * 1 show netnames on pads
                                 * 2 show netnames on tracks
                                 * 3 show netnames on tracks and pads
                                */

    bool Show_Modules_Cmp;
    bool Show_Modules_Cu;

    int  DisplayDrawItems;
    bool ContrastModeDisplay;

public:
    DISPLAY_OPTIONS();
};


#endif /* PCBSTRUCT_H */
