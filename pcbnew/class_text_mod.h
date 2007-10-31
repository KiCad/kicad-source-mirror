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
    int      m_Width;
    wxPoint  m_Pos;         // Real coord
    wxPoint  m_Pos0;        // coord du debut du texte /ancre, orient 0
    char     m_Unused;      // unused (reserved for future extensions)
    char     m_Miroir;      // vue normale / miroir
    char     m_NoShow;      // 0: visible 1: invisible  (bool)
    char     m_Type;        // 0: ref,1: val, autre = 2..255
    int      m_Orient;      // orientation en 1/10 degre
    wxSize   m_Size;        // dimensions (en X et Y) du texte
    wxString m_Text;

public:
    TEXTE_MODULE( MODULE* parent, int text_type = TEXT_is_DIVERS );
    ~TEXTE_MODULE();

    /* supprime du chainage la structure Struct */
    void    UnLink();

    void    Copy( TEXTE_MODULE* source ); // copy structure

    /* Gestion du texte */
    void    SetWidth( int new_width );
    int     GetLength();          /* text length */
    int     Pitch();              /* retourne le pas entre 2 caracteres */
    int     GetDrawRotation();    // Return text rotation for drawings and plotting

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
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, wxPoint offset, int draw_mode );

    
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

