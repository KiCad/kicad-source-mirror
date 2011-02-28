/*****************/
/* genequiv.cpp  */
/*****************/

#include "fctsys.h"
#include "wxstruct.h"
#include "confirm.h"
#include "gestfich.h"
#include "macros.h"

#include "cvpcb.h"
#include "cvpcb_mainframe.h"


void CVPCB_MAINFRAME::WriteStuffList( wxCommandEvent& event )
{
    FILE*      FileEquiv;
    wxString   Line;
    wxFileName fn = m_NetlistFileName;

    if( m_components.empty() )
        return;

    fn.SetExt( RetroFileExtension );

    wxFileDialog dlg( this, wxT( "Save Stuff File" ), fn.GetPath(),
                      fn.GetFullName(), RetroFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    FileEquiv = wxFopen( dlg.GetPath(), wxT( "wt" ) );

    if( FileEquiv == 0 )
    {
        Line = _( "Unable to create " ) + dlg.GetPath();
        DisplayError( this, Line, 30 );
        return;
    }

    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        if( component.m_Module.empty() )
            continue;
        fprintf( FileEquiv, "comp = \"%s\" module = \"%s\"\n",
                 TO_UTF8( component.m_Reference ),
                 TO_UTF8( component.m_Module ) );
    }

    fclose( FileEquiv );
}
