/**
 * @file class_module.h
 * @brief Module description (excepted pads)
 */


#ifndef _MODULE_H_
#define _MODULE_H_


class Pcb3D_GLCanvas;
class S3D_MASTER;

#include "richio.h"


/************************************/
/* Modules (footprints) description */
/* pad are in class_pad.xx          */
/************************************/


enum Mod_Attribut       /* Attributes used for modules */
{
    MOD_DEFAULT = 0,    /* Type default */
    MOD_CMS = 1,        /* Set for modules listed in the automatic insertion list
                         *  (usually SMD footprints) */
    MOD_VIRTUAL = 2     /* Virtual component: when created by copper shapes on
                         * board (Like edge card connectors, mounting hole...) */
};


/* flags for autoplace and autoroute (.m_ModuleStatus member) */
#define MODULE_is_LOCKED 0x01  /* module LOCKED: no autoplace allowed */
#define MODULE_is_PLACED 0x02  /* In autoplace: module automatically placed */
#define MODULE_to_PLACE  0x04  /* In autoplace: module waiting for autoplace */

class MODULE : public BOARD_ITEM
{
public:
    wxPoint           m_Pos;           // Real coord on board
    DLIST<D_PAD>      m_Pads;          /* Pad list (linked list) */
    DLIST<BOARD_ITEM> m_Drawings;      /* Graphic items list (linked list) */
    DLIST<S3D_MASTER> m_3D_Drawings;   /* First item of the 3D shapes (linked
                                        * list)*/
    TEXTE_MODULE*     m_Reference;     // Component reference (U34, R18..)
    TEXTE_MODULE*     m_Value;         // Component value (74LS00, 22K..)
    wxString          m_LibRef;        /* Name of the module in library (and
                                        * the default value when loading a
                                        *module from the library) */
    wxString          m_AlternateReference;  /* Used when m_Reference cannot
                                              * be used to identify the
                                              * footprint ( after a full
                                              * reannotation of the schematic */

    int           m_Attributs;          /* Flag bits ( see Mod_Attribut ) */
    int           m_Orient;             /* orientation in 0.1 degrees */
    int           flag;                 /* Use to trace ratsnest and auto
                                         * routing. */
    int           m_ModuleStatus;       /* For autoplace: flags (LOCKED,
                                         * AUTOPLACED) */
    EDA_RECT      m_BoundaryBox;        /* Bounding box : coordinates on board,
                                         * real orientation */
    int           m_PadNum;             // Pad count
    int           m_AltPadNum;          /* Pad with netcode > 0 (active pads)
                                         * count */

    int           m_CntRot90;           /* Automatic placement : cost ( 0..10 )
                                         * for 90 degrees rotation
                                         * (Horiz<->Vertical) */
    int           m_CntRot180;          /* Automatic placement : cost ( 0..10 )
                                         * for 180 degrees rotation
                                         * (UP <->Down) */
    wxSize        m_Ext;                /* Automatic placement margin around
                                         * the module */
    double         m_Surface;            // Bounding box area

    unsigned long m_Link;               /* Temporary variable ( used in
                                         * editions, ...) */
    long          m_LastEdit_Time;
    wxString      m_Path;

    wxString      m_Doc;                // Module Description (info for users)
    wxString      m_KeyWord;            // Keywords to select the module in lib

    // Local clearance. When null, the netclasses values are used. Usually
    // the local clearance is null
    int           m_LocalClearance;

    // Local mask margins: when NULL, the global design values are used
    int           m_LocalSolderMaskMargin;         // Solder mask margin
    int           m_LocalSolderPasteMargin;        /* Solder paste margin
                                                    * absolute value */
    double        m_LocalSolderPasteMarginRatio;   /* Solder mask margin ratio
                                                    * value of pad size */
    // The final margin is the sum of these 2 values

public:
    MODULE( BOARD* parent );
    MODULE( MODULE* module );
    ~MODULE();

    MODULE* Next() const { return (MODULE*) Pnext; }
    MODULE* Back() const { return (MODULE*) Pback; }

    void Copy( MODULE* Module );        // Copy structure


    /*
     * Function Add
     * adds the given item to this MODULE and takes ownership of its memory.
     * @param aBoardItem The item to add to this board.
     * @param doInsert If true, then insert, else append
     *  void    Add( BOARD_ITEM* aBoardItem, bool doInsert = true );
     */

    /**
     * Function CalculateBoundingBox
     * calculates the bounding box in board coordinates.
     */
    void CalculateBoundingBox();

    /**
     * Function GetFootPrintRect()
     * Returns the area of the module footprint excluding any text.
     * @return EDA_RECT - The rectangle containing the footprint.
     */
    EDA_RECT GetFootPrintRect() const;

    /**
     * Function GetBoundingBox
     * returns the bounding box of this
     * footprint.  Mainly used to redraw the screen area occupied by
     * the footprint.
     * @return EDA_RECT - The rectangle containing the footprint and texts.
     */
    EDA_RECT GetBoundingBox() const;

    /**
     * Function GetPosition
     * returns the position of this object.
     * Required by pure virtual BOARD_ITEM::GetPosition()
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Pos;
    }

    // Moves
    void SetPosition( const wxPoint& newpos );

    void SetOrientation( int newangle );

    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move( const wxPoint& aMoveVector );

    /**
     * Function Rotate
     * Rotate this object.
     * @param aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate( const wxPoint& aRotCentre, int aAngle );

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre );

    /**
     * Function IsLocked
     * (virtual from BOARD_ITEM )
     * @return bool - true if the MODULE is locked, else false
     */
    bool IsLocked() const
    {
        return (m_ModuleStatus & MODULE_is_LOCKED) != 0;
    }


    /**
     * Function SetLocked
     * sets the MODULE_is_LOCKED bit in the m_ModuleStatus
     * @param setLocked When true means turn on locked status, else unlock
     */
    void SetLocked( bool setLocked )
    {
        if( setLocked )
            m_ModuleStatus |= MODULE_is_LOCKED;
        else
            m_ModuleStatus &= ~MODULE_is_LOCKED;
    }


    /* Reading and writing data on files */

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    int Write_3D_Descr( FILE* File ) const;

    int ReadDescr( LINE_READER* aReader );

    /**
     * Function Read_GPCB_Descr
     * reads a footprint description in GPCB format (newlib version)
     * @param CmpFullFileName = Full file name (there is one footprint per file.
     * this is also the footprint name
     * @return bool - true if success reading else false.
     */
    bool Read_GPCB_Descr( const wxString& CmpFullFileName );

    int Read_3D_Descr( LINE_READER* aReader );

    /* drawing functions */

    /**
     * Function Draw
     * Draw the text according to the footprint pos and orient
     * @param aPanel = draw panel, Used to know the clip box
     * @param aDC = Current Device Context
     * @param aDrawMode = GR_OR, GR_XOR..
     * @param aOffset = draw offset (usually wxPoint(0,0)
     */
    void Draw( EDA_DRAW_PANEL* aPanel,
               wxDC*           aDC,
               int             aDrawMode,
               const wxPoint&  aOffset = ZeroOffset );

    void Draw3D( Pcb3D_GLCanvas* glcanvas );

    void DrawEdgesOnly( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset, int draw_mode );

    void DrawAncre( EDA_DRAW_PANEL* panel, wxDC* DC,
                    const wxPoint& offset, int dim_ancre, int draw_mode );

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos is a wxPoint to test.
     * @return bool - true if a hit, else false.
     */
    bool HitTest( const wxPoint& aRefPos );

    /**
     * Function HitTest (overlaid)
     * tests if the given EDA_RECT intersect the bounds of this object.
     * @param aRefArea is the given EDA_RECT.
     * @return bool - true if a hit, else false.
     */
    bool HitTest( EDA_RECT& aRefArea );

    /**
     * Function GetReference
     * @return const wxString& - the reference designator text.
     */
    const wxString& GetReference() const
    {
        return m_Reference->m_Text;
    }


    /**
     * Function GetValue
     * @return const wxString& - the value text.
     */
    const wxString& GetValue()
    {
        return m_Value->m_Text;
    }


    /**
     * Function FindPadByName
     * returns a D_PAD* with a matching name.  Note that names may not be
     * unique, depending on how the foot print was created.
     * @param aPadName the pad name to find
     * @return D_PAD* - The first matching name is returned, or NULL if not
     *                  found.
     */
    D_PAD* FindPadByName( const wxString& aPadName ) const;

    /**
     * Function GetPad
     * get a pad at \a aPosition on \a aLayer in the footprint.
     *
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found otherwise NULL.
     */
    D_PAD* GetPad( const wxPoint& aPosition, int aLayerMask = ALL_LAYERS );

    /**
     * Function Visit
     * should be re-implemented for each derived class in order to handle
     * all the types given by its member data.  Implementations should call
     * inspector->Inspect() on types in scanTypes[], and may use IterateForward()
     * to do so on lists of such data.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order
     *                  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *                         else SCAN_CONTINUE;
     */
    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                         const KICAD_T scanTypes[] );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "MODULE" );
    }

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  module_xpm; }

 #if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *                  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );

#endif
};


#endif     // _MODULE_H_
