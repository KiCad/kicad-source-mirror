/**************************************************************/
/*	pcbstruct.h :  definition des structures de donnees type PCB */
/**************************************************************/

#ifndef PCBSTRUCT_H
#define PCBSTRUCT_H

#include "base_struct.h"

// Definitions relatives aux libariries
#define ENTETE_LIBRAIRIE "PCBNEW-LibModule-V1"
#define ENTETE_LIBDOC    "PCBNEW-LibDoc----V1"
#define L_ENTETE_LIB     18
#define EXT_CMP          wxT( ".emp" )
#define EXT_CMP_MASK     wxT( "*.emp" )
#define EXT_DOC          wxT( ".mdc" )


/* Bits indicateurs du membre .Status, pour pistes, modules... */
#define FLAG1       0x2000  /* flag libre pour calculs locaux */
#define FLAG0       0x1000  /* flag libre pour calculs locaux */
#define BEGIN_ONPAD 0x800   /* flag indiquant un debut de segment sur pad */
#define END_ONPAD   0x400   /* flag indiquant une fin de segment sur pad */
#define BUSY        0x0200  /* flag indiquant que la structure a deja
                             *  ete examinee, dans certaines routines */
#define DELETED     0x0100  /* Bit flag de Status pour structures effacee
                             *  et mises en chaine "DELETED" */
#define NO_TRACE    0x80    /* l'element ne doit pas etre affiche */
#define SURBRILL    0x20    /* element en surbrillance */
#define DRAG        0x10    /* segment en mode drag */
#define EDIT        0x8     /* element en cours d'edition */
#define SEGM_FIXE   0x04    /* segment FIXE ( pas d'effacement global ) */
#define SEGM_AR     0x02    /* segment Auto_Route */
#define CHAIN       0x01    /* segment marque  */


/* Layer identification (layer number) */
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
#define NB_COPPER_LAYERS      (CMP_N + 1)

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
#define CUIVRE_LAYER            (1<<COPPER_LAYER_N)     ///< bit mask for copper layer 
#define LAYER_2                 (1<<LAYER_N_2)          ///< bit mask for layer 2
#define LAYER_3                 (1<<LAYER_N_3)          ///< bit mask for layer 3
#define LAYER_4                 (1<<LAYER_N_4)          ///< bit mask for layer 4
#define LAYER_5                 (1<<LAYER_N_5)          ///< bit mask for layer 5
#define LAYER_6                 (1<<LAYER_N_6)          ///< bit mask for layer 6
#define LAYER_7                 (1<<LAYER_N_7)          ///< bit mask for layer 7
#define LAYER_8                 (1<<LAYER_N_8)          ///< bit mask for layer 8
#define LAYER_9                 (1<<LAYER_N_9)          ///< bit mask for layer 9
#define LAYER_10                (1<<LAYER_N_10)         ///< bit mask for layer 10
#define LAYER_11                (1<<LAYER_N_11)         ///< bit mask for layer 11
#define LAYER_12                (1<<LAYER_N_12)         ///< bit mask for layer 12
#define LAYER_13                (1<<LAYER_N_13)         ///< bit mask for layer 13
#define LAYER_14                (1<<LAYER_N_14)         ///< bit mask for layer 14
#define LAYER_15                (1<<LAYER_N_15)         ///< bit mask for layer 15
#define CMP_LAYER               (1<<LAYER_CMP_N)        ///< bit mask for component layer
#define ADHESIVE_LAYER_CU       (1<<ADHESIVE_N_CU)
#define ADHESIVE_LAYER_CMP      (1<<ADHESIVE_N_CMP)
#define SOLDERPASTE_LAYER_CU    (1<<SOLDERPASTE_N_CU)
#define SOLDERPASTE_LAYER_CMP   (1<<SOLDERPASTE_N_CMP)
#define SILKSCREEN_LAYER_CU     (1<<SILKSCREEN_N_CU)
#define SILKSCREEN_LAYER_CMP    (1<<SILKSCREEN_N_CMP)
#define SOLDERMASK_LAYER_CU     (1<<SOLDERMASK_N_CU)
#define SOLDERMASK_LAYER_CMP    (1<<SOLDERMASK_N_CMP)
#define DRAW_LAYER              (1<<DRAW_N)
#define COMMENT_LAYER           (1<<COMMENT_N)
#define ECO1_LAYER              (1<<ECO1_N)
#define ECO2_LAYER              (1<<ECO2_N)
#define EDGE_LAYER              (1<<EDGE_N)
//      extra bits              0xE0000000
/* masques generaux : */
#define ALL_LAYERS              0x1FFFFFFF
#define ALL_NO_CU_LAYERS        0x1FFF0000
#define ALL_CU_LAYERS           0x0000FFFF
#define INTERNAL_LAYERS         0x00007FFE      /* Bits layers internes */
#define EXTERNAL_LAYERS         0x00008001

/* Forme des segments (pistes, contours ..) ( parametre .shape ) */
enum Track_Shapes {
    S_SEGMENT = 0,      /* segment rectiligne */
    S_RECT,             /* segment forme rect (i.e. bouts non arrondis) */
    S_ARC,              /* segment en arc de cercle (bouts arrondis)*/
    S_CIRCLE,           /* segment en cercle (anneau)*/
    S_ARC_RECT,         /* segment en arc de cercle (bouts droits) (GERBER)*/
    S_SPOT_OVALE,       /* spot ovale (for GERBER)*/
    S_SPOT_CIRCLE,      /* spot rond (for GERBER)*/
    S_SPOT_RECT,        /* spot rect (for GERBER)*/
    S_POLYGON           /* polygon shape */
};


/* Forward declaration */
class MODULE;
class EQUIPOT;
class MARQUEUR;
class TRACK;
class D_PAD;
struct CHEVELU;
class Ki_PageDescr;
class DrawBlockStruct;

// Class for handle current printed board design settings
#define HIST0RY_NUMBER 8
class EDA_BoardDesignSettings
{
public:
    int    m_CopperLayerCount;                  // Number of copper layers for this design
    int    m_ViaDrill;                          // via drill (for the entire board)
    int    m_CurrentViaSize;                    // Current via size
    int    m_ViaSizeHistory[HIST0RY_NUMBER];    // Last HIST0RY_NUMBER used via sizes
    int    m_CurrentViaType;                    // via type (BLIND, TROUGHT ...), bits 1 and 2 (not 0 and 1)
    int    m_CurrentTrackWidth;                 // current track width
    int    m_TrackWidhtHistory[HIST0RY_NUMBER]; // Last HIST0RY_NUMBER used track widths
    int    m_DrawSegmentWidth;                  // current graphic line width (not EDGE layer)
    int    m_EdgeSegmentWidth;                  // current graphic line width (EDGE layer only)
    int    m_PcbTextWidth;                      // current Pcb (not module) Text width
    wxSize m_PcbTextSize;                       // current Pcb (not module) Text size
    int    m_TrackClearence;                    // track to track and track to pads clearance
    int    m_ZoneClearence;                     // zone to track and zone to pads clearance
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


class BOARD : public BOARD_ITEM
{
public:
    WinEDA_BasePcbFrame*    m_PcbFrame;         // Window de visualisation
    EDA_Rect                m_BoundaryBox;      // Limites d'encadrement du PCB
    int                     m_Unused;
    int                     m_Status_Pcb;       // Mot d'etat: Bit 1 = Chevelu calcule
    EDA_BoardDesignSettings* m_BoardSettings;   // Link to current design settings
    int             m_NbNets;                   // Nombre de nets (equipotentielles)
    int             m_NbNodes;                  // nombre de pads connectes
    int             m_NbLinks;                  // nombre de chevelus
    int             m_NbLoclinks;               // nombre de chevelus dans Local ratsnest
                                                // minimal de pistes a tracer
    int             m_NbNoconnect;              // nombre de chevelus actifs
    int             m_NbSegmTrack;              // nombre d'elements de type segments de piste
    int             m_NbSegmZone;               // nombre d'elements de type segments de zone

    BOARD_ITEM*     m_Drawings;                 // linked list of lines & texts
    MODULE*         m_Modules;                  // linked list of MODULEs
    EQUIPOT*        m_Equipots;                 // linked list of nets
    TRACK*          m_Track;                    // linked list of TRACKs and SEGVIAs
    SEGZONE*        m_Zone;                     // linked list of SEGZONEs
    D_PAD**         m_Pads;                     // pointeur liste d'acces aux pads
    int             m_NbPads;                   // nombre total de pads
    CHEVELU*        m_Ratsnest;                 // pointeur liste des chevelus
    CHEVELU*        m_LocalRatsnest;            // pointeur liste des chevelus d'un module

    EDGE_ZONE*      m_CurrentLimitZone;         /* pointeur sur la liste des segments
                                                 * de delimitation de la zone en cours de trace */

    BOARD( EDA_BaseStruct* StructFather, WinEDA_BasePcbFrame* frame );
    ~BOARD();

    /* supprime du chainage la structure Struct */
    void    UnLink();

    /* Routines de calcul des nombres de segments pistes et zones */
    int     GetNumSegmTrack();
    int     GetNumSegmZone();
    int     GetNumNoconnect();    // retourne le nombre de connexions manquantes
    int     GetNumRatsnests();    // retourne le nombre de chevelus
    int     GetNumNodes();        // retourne le nombre de pads a netcode > 0

    // Calcul du rectangle d'encadrement:
    bool    ComputeBoundaryBox();

    
    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */ 
    void    Display_Infos( WinEDA_DrawFrame* frame );
    
    
    /**
     * Function Visit
     * may be re-implemented for each derived class in order to handle
     * all the types given by its member data.  Implementations should call
     * inspector->Inspect() on types in scanTypes[], and may use IterateForward()
     * to do so on lists of such data.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order 
     *  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *  else SCAN_CONTINUE, and determined by the inspector.
     */
    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData, 
        const KICAD_T scanTypes[] );


    /**
     * Function FindPadOrModule
     * searches for either a pad or module, giving precedence to pads.
     * Any Pad or Module on the desired layer that HitTest()s true will be
     * returned, otherwise any visible Pad or Module on any other layer.
     * The provided layer must be visible.
     * @param refPos The wxPoint to hit-test.
     * @return BOARD_ITEM* - if a direct hit, else NULL.
     */
//  BOARD_ITEM* FindPadOrModule( const wxPoint& refPos, int layer );
    

    /**
     * Function FindNet
     * searches for a net with the given netcode.
     * @param aNetcode A netcode to search for.
     * @return EQUIPOT* - the net or NULL if not found.
     */
    EQUIPOT* FindNet( int aNetcode ) const;
    
    
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "BOARD" );
    }

#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level 
     *  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );
    
#endif
};


/* Description d'un ecran */
class PCB_SCREEN : public BASE_SCREEN
{
public:
    int m_Active_Layer;             /* ref couche active */
    int m_Route_Layer_TOP;          /* ref couches actives */
    int m_Route_Layer_BOTTOM;       /* pour placement vias et routage 2 couches */

public:
    PCB_SCREEN( int idscreen );
    ~PCB_SCREEN();
    
    PCB_SCREEN* Next() { return (PCB_SCREEN*) Pnext; }
    void        Init();
    void        SetNextZoom();
    void        SetPreviousZoom();
    void        SetLastZoom();
    
    /**
     * Function GetCurItem
     * returns the currently selected BOARD_ITEM, overriding BASE_SCREEN::GetCurItem().
     * @return BOARD_ITEM* - the one selected, or NULL.
     */
    BOARD_ITEM* GetCurItem() const {  return (BOARD_ITEM*) BASE_SCREEN::GetCurItem(); }
};

/***************************/
/* Description des Modules */
/***************************/

#include "class_pad.h"       /* Description des Pastilles :*/
#include "class_edge_mod.h"
#include "class_text_mod.h"
#include "class_module.h"
#include "class_equipot.h"


/***********************************/
/* Description des elements du PCB */
/***********************************/

class DRAWSEGMENT : public BOARD_ITEM
{
public:
    int     m_Width;            // 0 = line. if > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point
    
    int m_Shape;                // Shape: line, Circle, Arc
    int m_Type;                 // Used in complex associations ( Dimensions.. )
    int m_Angle;                // Used only for Arcs: Arc angle in 1/10 deg

public:
    DRAWSEGMENT( BOARD_ITEM* StructFather, KICAD_T idtype = TYPEDRAWSEGMENT );
    ~DRAWSEGMENT();

    // Read/write data
    bool    WriteDrawSegmentDescr( FILE* File );
    bool    ReadDrawSegmentDescr( FILE* File, int* LineNum );

    /* remove this from the linked list */
    void    UnLink();

    void    Copy( DRAWSEGMENT* source );


    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_BasePcbFrame in which to print status information.
     */ 
    void    Display_Infos( WinEDA_DrawFrame* frame );
    
    
    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );
    
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT("DRAWSEGMENT");
    }
};


#include "class_pcb_text.h"
#include "class_cotation.h"
#include "class_mire.h"
#include "class_track.h"

/*******************/
/* class EDGE_ZONE */
/*******************/

class EDGE_ZONE : public DRAWSEGMENT
{
public:
    EDGE_ZONE( BOARD_ITEM* StructFather );
    EDGE_ZONE( const EDGE_ZONE& edgezone );
    ~EDGE_ZONE();
};


/***************************************/
/* Markers: used to show a drc problem */
/***************************************/

class MARQUEUR : public BOARD_ITEM
{
public:
    wxPoint  m_Pos;
    char*    m_Bitmap;              /* Shape (bitmap) */
    int      m_Type;
    int      m_Color;               /* color */
    wxString m_Diag;                /* Associated text (comment) */

public:
    MARQUEUR( BOARD_ITEM* StructFather );
    ~MARQUEUR();
    void    UnLink();
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int DrawMode );
    
    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */ 
    void    Display_Infos( WinEDA_DrawFrame* frame );

    
    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );
};


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
    bool DisplayZones;
    bool Show_Modules_Cmp;
    bool Show_Modules_Cu;

    int  DisplayDrawItems;
    bool ContrastModeDisplay;

public:
    DISPLAY_OPTIONS();
};


#endif /* PCBSTRUCT_H */
