

#include <kiway_player.h>
#include <kiway_express.h>
#include <kiway.h>
#include <id.h>
#include <typeinfo>


BEGIN_EVENT_TABLE( KIWAY_PLAYER, EDA_BASE_FRAME )
    EVT_KIWAY_EXPRESS( KIWAY_PLAYER::kiway_express )
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END, KIWAY_PLAYER::language_change )
END_EVENT_TABLE()


KIWAY_PLAYER::KIWAY_PLAYER( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
        const wxString& aTitle, const wxPoint& aPos, const wxSize& aSize,
        long aStyle, const wxString& aWdoName ) :
    EDA_BASE_FRAME( aParent, aFrameType, aTitle, aPos, aSize, aStyle, aWdoName ),
    KIWAY_HOLDER( aKiway ),
    m_modal_dismissed( 0 )
{
    DBG( printf("KIWAY_EXPRESS::wxEVENT_ID:%d\n", KIWAY_EXPRESS::wxEVENT_ID );)
    //Connect( KIWAY_EXPRESS::wxEVENT_ID, wxKiwayExressHandler( KIWAY_PLAYER::kiway_express ) );
}


KIWAY_PLAYER::KIWAY_PLAYER( wxWindow* aParent, wxWindowID aId, const wxString& aTitle,
        const wxPoint& aPos, const wxSize& aSize, long aStyle,
        const wxString& aWdoName ) :
    EDA_BASE_FRAME( aParent, (FRAME_T) aId, aTitle, aPos, aSize, aStyle, aWdoName ),
    KIWAY_HOLDER( 0 ),
    m_modal_dismissed( 0 )
{
    DBG( printf("KIWAY_EXPRESS::wxEVENT_ID:%d\n", KIWAY_EXPRESS::wxEVENT_ID );)
    //Connect( KIWAY_EXPRESS::wxEVENT_ID, wxKiwayExressHandler( KIWAY_PLAYER::kiway_express ) );
}


KIWAY_PLAYER::~KIWAY_PLAYER(){}


void KIWAY_PLAYER::KiwayMailIn( KIWAY_EXPRESS& aEvent )
{
    // override this in derived classes.
}


static void makeModal( wxFrame* aFrame, bool IsModal )
{
    // disable or enable all other top level windows
#if wxCHECK_VERSION(2, 9, 4)
    wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();

    while( node )
    {
        wxWindow* win = node->GetData();

        if( win != aFrame )
            win->Enable( !IsModal );

        node = node->GetNext();
    }
#else
    // Deprecated since wxWidgets 2.9.4
    aFrame->MakeModal( IsModal );
#endif
}


/**
 * toggle global wxFrame enable/disable state, does the re-enable part even
 * if an exception is thrown.
 */
struct ENABLE_DISABLE
{
    wxFrame* m_frame;

    ENABLE_DISABLE( wxFrame* aFrame ) :
        m_frame( aFrame )
    {
        makeModal( aFrame, true );
    }

    ~ENABLE_DISABLE()
    {
        // Re-enable all frames, (oops, even if they were previously inactive).
        // This is probably why this function was deprecated in wx.
        makeModal( m_frame, false );
    }
};


bool KIWAY_PLAYER::ShowModal( wxString* aResult )
{
    /*
        This function has a nice interface but an unsightly implementation.
        Now it is encapsulated, making it easier to improve.  I am not sure
        a wxSemaphore was needed, especially since no blocking happens.  It seems
        like a volatile bool is sufficient.

        It works in tandem with DismissModal().  But only ShowModal() is in the
        vtable and therefore cross-module capable.
   */

    volatile bool dismissed = false;

    // disable all frames except the modal one, re-enable on exit, exception safe.
    ENABLE_DISABLE  toggle( this );

    m_modal_dismissed = &dismissed;

    Show( true );
    Raise();

    // Wait for the one and only active frame to call DismissModal() from
    // some concluding event.
    while( !dismissed )
    {
        wxYield();
        wxMilliSleep( 50 );
    }

    // no longer modal, not to mention that the pointer would be invalid outside this scope.
    m_modal_dismissed = NULL;

    if( aResult )
        *aResult = m_modal_string;

    return m_modal_ret_val;
}


bool KIWAY_PLAYER::IsDismissed()
{
    // if already dismissed, then m_modal_dismissed may be NULL, and if not,
    // it can still be dismissed if the bool is true.
    bool ret = !m_modal_dismissed || *m_modal_dismissed;

    return ret;
}


void KIWAY_PLAYER::DismissModal( bool aRetVal, const wxString& aResult )
{
    m_modal_ret_val = aRetVal;
    m_modal_string  = aResult;

    if( m_modal_dismissed )
        *m_modal_dismissed = true;
}


void KIWAY_PLAYER::kiway_express( KIWAY_EXPRESS& aEvent )
{
    // logging support
#if defined(DEBUG)
    const char* class_name = typeid( this ).name();

    printf( "%s: received cmd:%d  pay:'%s'\n", class_name,
        aEvent.Command(), aEvent.GetPayload().c_str() );
#endif

    KiwayMailIn( aEvent );     // call the virtual, override in derived.
}


void KIWAY_PLAYER::language_change( wxCommandEvent& event )
{
    int id = event.GetId();

    // tell all the KIWAY_PLAYERs about the language change.
    Kiway().SetLanguage( id );
}
