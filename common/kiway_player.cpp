

#include <kiway_player.h>
#include <kiway_express.h>
#include <typeinfo>


BEGIN_EVENT_TABLE( KIWAY_PLAYER, EDA_BASE_FRAME )
    /* have not been able to get this to work yet:
    EVT_KIWAY_EXPRESS( KIWAY_PLAYER::kiway_express )
    Use Connect() in constructor until this can be sorted out.

    OK the problem is KIWAY_PLAYER::wxEVENT_ID not being unique accross all link images.
    */
END_EVENT_TABLE()



KIWAY_PLAYER::KIWAY_PLAYER( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString& aWdoName ) :
    EDA_BASE_FRAME( aParent, aFrameType, aTitle, aPos, aSize, aStyle, aWdoName ),
    KIWAY_HOLDER( aKiway )
{
    DBG( printf("KIWAY_EXPRESS::wxEVENT_ID:%d\n", KIWAY_EXPRESS::wxEVENT_ID );)
    Connect( KIWAY_EXPRESS::wxEVENT_ID, wxKiwayExressHandler( KIWAY_PLAYER::kiway_express ) );
}


KIWAY_PLAYER::KIWAY_PLAYER( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
        const wxPoint& aPos, const wxSize& aSize, long aStyle,
        const wxString& aWdoName ) :
    EDA_BASE_FRAME( aParent, (FRAME_T) aId, aTitle, aPos, aSize, aStyle, aWdoName ),
    KIWAY_HOLDER( 0 )
{
    DBG( printf("KIWAY_EXPRESS::wxEVENT_ID:%d\n", KIWAY_EXPRESS::wxEVENT_ID );)
    Connect( KIWAY_EXPRESS::wxEVENT_ID, wxKiwayExressHandler( KIWAY_PLAYER::kiway_express ) );
}


void KIWAY_PLAYER::kiway_express( KIWAY_EXPRESS& aEvent )
{
    // logging support
#if defined(DEBUG)
    const char* class_name = typeid(this).name();

    printf( "%s:  cmd:%d  pay:'%s'\n", class_name,
        aEvent.GetEventType(), aEvent.GetPayload().c_str() );
#endif

    KiwayMailIn( aEvent );     // call the virtual, overload in derived.
}


void KIWAY_PLAYER::KiwayMailIn( KIWAY_EXPRESS& aEvent )
{
    // overload this.
}
