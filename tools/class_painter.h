

#if defined(PCBNEW)

class BOARD;
class TRACK;
class ZONE_CONTAINER;

//:

#elif defined(EESCHEMA)

class SCH_SHEET;
//:

#endif


/**
 * Class PAINTER
 * contains all the knowledge about how to draw any graphical object onto
 * any particular output device.
 * This knowledge is held outside the individual graphical objects so that
 * alternative output devices may be used, and so that the graphical objects
 * themselves to not contain drawing routines.  Drawing routines in the objects
 * cause problems with usages of the objects as simple container objects in
 * DLL/DSOs.
 */
class PAINTER
{
public:


    /**
     * Constructor PAINTER( wxDC& )
     * initializes this object for painting on any of the polymorphic
     * wxDC derivatives.
     *
     * @param aDC is a reference to a polymorphic wx device context on which
     *  to draw. It can be any of the wxDC derivatives.
     *  No ownership is given to this PAINTER of aDC.
     */
    PAINTER( wxDC& aDC ) :
        m_dc( aDC ),
        m_highlight( false ),
        m_grayed( false )
    {
    }



#if defined(PCBNEW)

    void Draw( const BOARD_ITEM* );

#elif defined(EESCHEMA)

    void Draw( const SCH_ITEM* );

#endif


private:

    wxDC&   m_dc;

    // drawing state information.
    bool    m_highlite;
    bool    m_grayed;


#if defined(PCBNEW)

    void    draw( const TRACK* );
    void    draw( const MODULE* );
    void    draw( const EDGE_MODULE* );
    // :

#elif defined(EESCHEMA)
    void    draw( const SCH_WIRE* );
    // :

#endif
}


#if defined(PCBNEW)

void PAINTER::Draw( const BOARD_ITEM* aItem )
{
    // the "cast" applied in here clarifies which overloaded draw() is called

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
        draw( (MODULE*) aItem );
        break;

    case PCB_PAD_T:
        draw( (D_PAD*) aItem );
        break;

    case PCB_LINE_T:
        draw( (TEXTE_PCB*) aItem );
        break;

    case PCB_TEXT_T:
        draw( (TEXTE_PCB*) aItem );
        break;

    case PCB_MODULE_TEXT_T:
        draw( (TEXTE_PCB*) aItem );
        break;

    case PCB_MODULE_EDGE_T:
        draw( (EDGE_MODULE*) aItem );
        break;

    case PCB_TRACE_T:
        draw( (TRACKE*) aItem );
        break;

    case PCB_VIA_T:
        draw( (VIA*) aItem );
        break;

    case PCB_ZONE_T:
        draw( (SEGZONE*) aItem );
        break;

    case PCB_MARKER_T:
        draw( (MARKER_PCB*) aItem );
        break;

    case PCB_DIMENSION_T:
        draw( (DIMENSION*) aItem );
        break;

    case PCB_TARGET_T:
        draw( (TARGET*) aItem );
        break;


    case PCB_ZONE_AREA_T:
        draw( (ZONE_CONTAINER*) aItem );
        break;

    /* not used
    case PCB_ITEM_LIST_T:
        draw( (BOARD_ITEM_LIST*) aItem );
        break;
    */

    default:
        ; // nothing
    }
}

#elif defined(EESCHEMA)

void PAINTER::Draw( const SCH_ITEM* aItem )
{
    // the "cast" applied in here clarifies which overloaded draw() is called

    switch( aItem->Type() )
    {
        //:
    }
}


#endif