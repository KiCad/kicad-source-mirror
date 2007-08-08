/**********************************/
/* class_pad.h : Pads description */
/**********************************/

class Pcb3D_GLCanvas;

/* forme des pastilles : (parametre .forme) */
#define CIRCLE      1
#define RECT        2
#define OVALE       3
#define TRAPEZE     4       // trapeze: traversante ou surfacique
#define SPECIAL_PAD 5       // description libre

/* Attributs des PADS */
#define STANDARD    0       // pad classique
#define SMD         1       // surfacique, generation d'un masque d'empatement
#define CONN        2       // surfacique, peut etre dore
#define P_HOLE      3       // trou simple, utile sur pad stack
#define MECA        4       // PAD "mecanique" (fixation, zone cuivre...)
#define PAD_STACK   0x80    // bit 7 de .attrib  (flag)

/* Definition type Structure d'un pad */
class D_PAD : public EDA_BaseStruct
{
public:
    union
    {
        unsigned long m_NumPadName;
        char          m_Padname[4]; /* nom (numero) de la pastille (assimilable a un long)*/
    };
    
    wxString m_Netname;             /* Net Name */
    
    int      m_Masque_Layer;        // (Bit a Bit :1= cuivre, 15= cmp,
                                    // 2..14 = interne
                                    // 16 .. 31 = couches non cuivre
                                
    int      m_PadShape;            // forme CERCLE, RECT, OVALE, TRAPEZE ou libre
    int      m_DrillShape;          // forme CERCLE, OVAL
    
    wxPoint  m_Pos;                 // Position de reference du pad
    
    wxSize   m_Drill;               // Drill diam (drill shape = CIRCLE) or drill size(shape = OVAL)
                                    // for drill shape = CIRCLE, drill diam = m_Drill.x
                                    
    wxSize   m_Offset;              // Offset de la forme (pastilles excentrees)
    wxSize   m_Size;                // Dimensions X et Y ( si orient 0 x = axe X
                                    // y = axe Y
                                    
    wxSize   m_DeltaSize;           // delta sur formes rectangle -> trapezes
    
    wxPoint  m_Pos0;                // Coord relatives a l'ancre du pad en orientation 0
    
    int      m_Rayon;               // rayon du cercle exinscrit du pad
    int      m_Attribut;            // NORMAL, SMD, CONN, Bit 7 = STACK
    int      m_Orient;              // en 1/10 degres

    int      m_NetCode;             /* Numero de net pour comparaisons rapides */
    int      m_logical_connexion;   // variable utilisee lors du calcul du chevelu:
                                    // contient de numero de block pour une connexion type ratsnet
                                    
    int      m_physical_connexion;  // variable utilisee lors du calcul de la connexit�
                                    // contient de numero de block pour une connexion type piste

public:
    D_PAD( MODULE* parent );
    D_PAD( D_PAD* pad );
    ~D_PAD( void );

    void            Copy( D_PAD* source );

    D_PAD*          Next( void ) { return (D_PAD*) Pnext; }

    /* supprime du chainage la structure Struct */
    void            UnLink( void );

    /* Reading and writing data on files */
    int             ReadDescr( FILE* File, int* LineNum = NULL );
    int             WriteDescr( FILE* File );

    /* drawing functions */
    void            Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode );
    void            Draw3D( Pcb3D_GLCanvas* glcanvas );

    // autres
    void            SetPadName( const wxString& name );     // Change pad name
    wxString        ReturnStringPadName( void );            // Return pad name as string in a wxString
    void            ReturnStringPadName( wxString& text );  // Return pad name as string in a buffer
    void            ComputeRayon( void );                   // met a jour m_Rayon, rayon du cercle exinscrit
    const wxPoint   ReturnShapePos( void );                 // retourne la position

    // de la forme (pastilles excentrees)
    void            Display_Infos( WinEDA_BasePcbFrame* frame );
    
    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool            HitTest( const wxPoint& refPos );
    
#if defined(DEBUG)
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
