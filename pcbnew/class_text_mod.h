/***************************************************/
/* class_text_module.h : texts module description  */
/***************************************************/


#ifndef TEXT_MODULE_H
#define TEXT_MODULE_H


/* Description des Textes sur Modules : */
#define TEXT_is_REFERENCE 0
#define TEXT_is_VALUE     1
#define TEXT_is_DIVERS    2


class TEXTE_MODULE : public BOARD_ITEM
{
public:
    wxPoint  m_Pos;         // Real (physical)coord
    int      m_Width;
    wxPoint  m_Pos0;        // text coordinates relatives to the footprint ancre, orient 0
                            // Text coordinate ref point is the text centre
    char     m_Unused;      // unused (reserved for future extensions)
    char     m_Miroir;      // Show normal / mirror
    char     m_NoShow;      // 0: visible 1: invisible  (bool)
    char     m_Type;        // 0: ref,1: val, others = 2..255
    int      m_Orient;      // orientation in 1/10 deg relative to the footprint
                            // Physical orient is m_Orient + m_Parent->m_Orient
    wxSize   m_Size;        // text size
    wxString m_Text;

public:
    TEXTE_MODULE( MODULE* parent, int text_type = TEXT_is_DIVERS );
    ~TEXTE_MODULE();

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


    /* supprime du chainage la structure Struct */
    void    UnLink();

    void    Copy( TEXTE_MODULE* source ); // copy structure

    /* Gestion du texte */
    void    SetWidth( int new_width );
    int     GetLength();          /* text length */
    int     Pitch();              /* retourne le pas entre 2 caracteres */
    int     GetDrawRotation();    // Return text rotation for drawings and plotting

    /** Function GetTextRect
     * @return an EDA_Rect which gives the position and size of the text area (for the 0 orient text and footprint)
     */
    EDA_Rect GetTextRect(void);

    /**
     * Function GetBoundingBox
     * returns the bounding box of this Text (according to text and footprint orientation)
     */
    EDA_Rect GetBoundingBox();

    void    SetDrawCoord();       // mise a jour des coordonn�s absolues de trac�
                                        // a partir des coord relatives

    void    SetLocalCoord();      // mise a jour des coordonn�s relatives

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;


    int     ReadDescr( FILE* File, int* LineNum = NULL );

    /* drawing functions */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& offset = ZeroOffset );


    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void    Display_Infos( WinEDA_DrawFrame* frame );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param posref A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& posref );

    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual so
     * objects like D_PAD, which reside on multiple layers can do their own
     * form of testing.
     * virtual inheritance from BOARD_ITEM.
     * @param aLayer The layer to test for.
     * @return bool - true if on given layer, else false.
     */
    bool IsOnLayer( int aLayer ) const;

    /**
     * Function IsOnOneOfTheseLayers
     * returns true if this object is on one of the given layers.  Is virtual so
     * objects like D_PAD, which reside on multiple layers, can do their own
     * form of testing.
     * virtual inheritance from BOARD_ITEM.
     * @param aLayerMask The bit-mapped set of layers to test for.
     * @return bool - true if on one of the given layers, else false.
    bool IsOnOneOfTheseLayers( int aLayerMask ) const;
     */




    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "MTEXT" );
    }

#if defined(DEBUG)
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

#endif // TEXT_MODULE_H

