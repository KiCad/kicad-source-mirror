/***************/
/* genstf()  */
/***************/

/* genere le fichier STF type 'ref' 'nom_empreinte' pour DRAFT */

#include "fctsys.h"
#include "wxstruct.h"
#include "confirm.h"
#include "gestfich.h"
#include "macros.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


void WinEDA_CvpcbFrame::WriteStuffList( wxCommandEvent& event )
{
    COMPONENT_LIST::iterator i;
    FILE*      FileEquiv;
    COMPONENT* Cmp;
    wxString   Line;
    wxFileName fn = m_NetlistFileName;

    if( m_components.GetCount() <= 0 )
        return;

    /* calcul du nom du fichier */
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

    /* Generation de la liste */
    for( i = m_components.begin(); i != m_components.end(); ++i )
    {
        Cmp = *i;

        /* génération du composant si son empreinte est définie */
        if( Cmp->m_Module.IsEmpty() )
            continue;
        fprintf( FileEquiv, "comp = \"%s\" module = \"%s\"\n",
                 CONV_TO_UTF8( Cmp->m_Reference ),
                 CONV_TO_UTF8( Cmp->m_Module ) );
    }

    fclose( FileEquiv );
}
