
#include <kiway.h>
#include <kiway_player.h>

#if defined(DEBUG)
 #include <typeinfo>
#endif


PROJECT& KIWAY_HOLDER::Prj() const
{
    return Kiway().Prj();
}


// this is not speed critical, hide it out of line.
void KIWAY_HOLDER::SetKiway( wxWindow* aDest, KIWAY* aKiway )
{
#if defined(DEBUG)
    // offer a trap point for debugging most any window
    wxASSERT( aDest );
    if( !strcmp( typeid(aDest).name(), "DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB" ) )
    {
        int breakhere=1;
        (void) breakhere;
    }
#endif

    (void) aDest;

    m_kiway = aKiway;
}
