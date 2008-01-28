/**********************************/
/* class_pad.h : Pads description */
/**********************************/

class Pcb3D_GLCanvas;

#include "pad_shapes.h"


/* Definition type Structure d'un pad */
class D_PAD : public BOARD_ITEM
{
private:
    int m_NetCode;              // Net number for fast comparisons


public:
    wxPoint m_Pos;                  // pad Position on board

    union
    {
        unsigned long m_NumPadName;
        char          m_Padname[4]; /* Pad name (4 char) or a long identifier
                                     *  (used in pad name comparisons because  this is faster than string comparison)
                                     */
    };

    wxString m_Netname;             /* Net Name */

    int      m_Masque_Layer;        // Bitwise layer :1= copper layer, 15= cmp,
                                    // 2..14 = internal layers
                                    // 16 .. 31 = technical layers

    int     m_PadShape;             // Shape: PAD_CIRCLE, PAD_RECT, PAD_OVAL, PAD_TRAPEZOID
    int     m_DrillShape;           // Shape PAD_CIRCLE, PAD_OVAL

    wxSize  m_Drill;                // Drill diam (drill shape = PAD_CIRCLE) or drill size(shape = OVAL)
                                    // for drill shape = PAD_CIRCLE, drill diam = m_Drill.x

    wxSize  m_Offset;  /*This parameter is usefull only for oblong pads (it can be used for other
                         * shapes, but without any interest).
                         * this is the offset between the pad hole and the pad shape (you must
                         * understand here pad shape = copper area around the hole)
                         * Most of cases, the hole is the centre of the shape (m_Offset = 0).
                         * But some board designers use oblong pads with a hole moved to one of the
                         * oblong pad shape ends.
                         * In all cases the pad position is the pad hole.
                         * The physical shape position (used to draw it for instance) is pad
                         * position (m_Pos) + m_Offset.
                         * D_PAD::ReturnShapePos() returns the physical shape position according to
                         * the offset and the pad rotation.*/

    wxSize  m_Size;                 // X and Y size ( relative to orient 0)

    wxSize  m_DeltaSize;            // delta sur formes rectangle -> trapezes

    wxPoint m_Pos0;                 // Initial Pad position (i.e. pas position relative to the module anchor, orientation 0

    int     m_Rayon;                // rayon du cercle exinscrit du pad
    int     m_Attribut;             // NORMAL, PAD_SMD, PAD_CONN
    int     m_Orient;               // in 1/10 degrees

    int     m_logical_connexion;    // variable used in rastnest computations
                                    // handle block number in ratsnet connection

    int     m_physical_connexion;   // variable used in rastnest computations
                                    // handle block number in track connection

	int     m_zone_connexion;   	// variable used in rastnest computations
                                    // handle block number in zone connection

public:
    D_PAD( MODULE* parent );
    D_PAD( D_PAD* pad );
    ~D_PAD();

    void            Copy( D_PAD* source );

    D_PAD* Next() { return (D_PAD*) Pnext; }

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Pos;
    }


    void SetPosition( const wxPoint& aPos )
    {
        m_Pos = aPos;
    }


    /* remove from linked list */
    void            UnLink();

    /* Reading and writing data on files */
    int             ReadDescr( FILE* File, int* LineNum = NULL );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool            Save( FILE* aFile ) const;


    /* drawing functions */
    void            Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode );
    void            Draw3D( Pcb3D_GLCanvas* glcanvas );

    // others
    void            SetPadName( const wxString& name );     // Change pad name
    wxString        ReturnStringPadName();                  // Return pad name as string in a wxString
    void            ReturnStringPadName( wxString& text );  // Return pad name as string in a buffer
    void            ComputeRayon();                         // compute m_Rayon, rayon du cercle exinscrit
    const wxPoint   ReturnShapePos();                       // retourne la position


    /**
     * Function GetNet
     * @return int - the netcode
     */
    int GetNet() const { return m_NetCode; }
    void SetNet( int aNetCode ) { m_NetCode = aNetCode; }


    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void            Display_Infos( WinEDA_DrawFrame* frame );


    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual so
     * objects like D_PAD, which reside on multiple layers can do their own
     * form of testing.
     * @param aLayer The layer to test for.
     * @return bool - true if on given layer, else false.
     */
    bool            IsOnLayer( int aLayer ) const;


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool            HitTest( const wxPoint& refPos );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "PAD" );
    }


    /**
     * Function Compare
     * compares two pads and return 0 if they are equal.
     * @return int - <0 if left less than right, 0 if equal, >0 if left greater than right.
     */
    static int Compare( const D_PAD* padref, const D_PAD* padcmp );


#if defined (DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );

#endif
};

typedef class D_PAD * LISTE_PAD;
