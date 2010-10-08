/*********************/
/* lib_draw_item.cpp */
/*********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "plot_common.h"
#include "drawtxt.h"
#include "trigo.h"
#include "bezier_curves.h"
#include "confirm.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "lib_draw_item.h"

const int fill_tab[3] = { 'N', 'F', 'f' };

//#define DRAW_ARC_WITH_ANGLE       // Used to draw arcs


/* Base class (abstract) for components bodies items */
LIB_DRAW_ITEM::LIB_DRAW_ITEM( KICAD_T        aType,
                              LIB_COMPONENT* aComponent,
                              int            aUnit,
                              int            aConvert,
                              FILL_T         aFillType ) :
    EDA_BaseStruct( aType )
{
    m_Unit       = aUnit;
    m_Convert    = aConvert;
    m_Fill       = aFillType;
    m_Parent     = (EDA_BaseStruct*) aComponent;
    m_typeName   = _( "Undefined" );
    m_isFillable = false;
}


LIB_DRAW_ITEM::LIB_DRAW_ITEM( const LIB_DRAW_ITEM& aItem ) :
    EDA_BaseStruct( aItem )
{
    m_Unit = aItem.m_Unit;
    m_Convert = aItem.m_Convert;
    m_Fill = aItem.m_Fill;
    m_Parent = aItem.m_Parent;
    m_typeName = aItem.m_typeName;
    m_isFillable = aItem.m_isFillable;
}


/**
 * Update the message panel information with the drawing information.
 *
 * This base function is used to display the information common to the
 * all library items.  Call the base class from the derived class or the
 * common information will not be updated in the message panel.
 */
void LIB_DRAW_ITEM::DisplayInfo( WinEDA_DrawFrame* aFrame )
{
    wxString msg;

    aFrame->ClearMsgPanel();
    aFrame->AppendMsgPanel( _( "Type" ), m_typeName, CYAN );

    if( m_Unit == 0 )
        msg = _( "All" );
    else
        msg.Printf( wxT( "%d" ), m_Unit );
    aFrame->AppendMsgPanel( _( "Unit" ), msg, BROWN );

    if( m_Convert == 0 )
        msg = _( "All" );
    else if( m_Convert == 1 )
        msg = _( "no" );
    else if( m_Convert == 2 )
        msg = _( "yes" );
    else
        msg = wxT( "?" );
    aFrame->AppendMsgPanel( _( "Convert" ), msg, BROWN );
}


bool LIB_DRAW_ITEM::operator==( const LIB_DRAW_ITEM& aOther ) const
{
    return ( ( Type() == aOther.Type() )
             && ( m_Unit == aOther.m_Unit )
             && ( m_Convert == aOther.m_Convert )
             && DoCompare( aOther ) == 0 );
}


bool LIB_DRAW_ITEM::operator<( const LIB_DRAW_ITEM& aOther ) const
{
    int result = m_Convert - aOther.m_Convert;

    if( result != 0 )
        return result < 0;

    result = m_Unit - aOther.m_Unit;

    if( result != 0 )
        return result < 0;

    result = Type() - aOther.Type();

    if( result != 0 )
        return result < 0;

    return ( DoCompare( aOther ) < 0 );
}
