/***************************************************/
/* class_text_module.h : texts module description  */
/***************************************************/

/* Description des Textes sur Modules : */
#define TEXT_is_REFERENCE 0
#define TEXT_is_VALUE     1
#define TEXT_is_DIVERS    2


class TEXTE_MODULE : public EDA_BaseStruct
{
public:
    int      m_Layer;       // layer number
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
    ~TEXTE_MODULE( void );

    /* supprime du chainage la structure Struct */
    void    UnLink( void );

    void    Copy( TEXTE_MODULE* source ); // copy structure

    /* Gestion du texte */
    void    SetWidth( int new_width );
    int     GetLength( void );          /* text length */
    int     Pitch( void );              /* retourne le pas entre 2 caracteres */
    int     GetDrawRotation( void );    // Return text rotation for drawings and plotting

    void    SetDrawCoord( void );       // mise a jour des coordonn�s absolues de trac�			
                                        // a partir des coord relatives

    void    SetLocalCoord( void );      // mise a jour des coordonn�s relatives

    // a partir des coord absolues de trac�
    /* Reading and writing data on files */
    int     WriteDescr( FILE* File );
    int     ReadDescr( FILE* File, int* LineNum = NULL );

    /* drawing functions */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, wxPoint offset, int draw_mode );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param posref A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& posref );
    
#if defined(DEBUG)
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "MTEXT" );
    }

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
