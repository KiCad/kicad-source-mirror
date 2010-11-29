/**********************/
/* CVPCB: autosel.cpp  */
/**********************/

/* Routines for automatic selection of modules. */

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "appl_wxstruct.h"
#include "kicad_string.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


#define QUOTE '\''

class FOOTPRINT_ALIAS
{
public:
    int         m_Type;
    wxString    m_Name;
    wxString    m_FootprintName;

    FOOTPRINT_ALIAS() { m_Type = 0; }
};

typedef boost::ptr_vector< FOOTPRINT_ALIAS > FOOTPRINT_ALIAS_LIST;


/*
 * read the string between quotes and put it in aTarget
 * put text in aTarget
 * return a pointer to the last read char (the second quote if Ok)
 */
char * ReadQuotedText(wxString & aTarget, char * aText)
{
    // search the first quote:
    for( ; *aText != 0; aText++ )
    {
        if( *aText == QUOTE )
            break;
    }

    if ( *aText == 0 )
        return NULL;

    aText++;
    for(; *aText != 0; aText++ )
    {
        if( *aText == QUOTE )
            break;
        aTarget.Append(*aText);
    }

    return aText;
}


/*
 * Called by the automatic association button
 * Read *.equ files to try to find corresponding footprint
 * for each component that is not already linked to a footprint ( a "free"
 * component )
 * format of a line:
 * 'cmp_ref' 'footprint_name'
 */
void WinEDA_CvpcbFrame::AssocieModule( wxCommandEvent& event )
{
    FOOTPRINT_ALIAS_LIST aliases;
    FOOTPRINT_ALIAS*     alias;
    wxFileName           fn;
    wxString             msg, tmp;
    char                 Line[1024];
    FILE*                file;
    size_t               ii;

    if( m_components.empty() )
        return;

    /* Find equivalents in all available files. */
    for( ii = 0; ii < m_AliasLibNames.GetCount(); ii++ )
    {
        fn = m_AliasLibNames[ii];
        fn.SetExt( FootprintAliasFileExtension );
        tmp = wxGetApp().FindLibraryPath( fn );

        if( !tmp )
        {
            msg.Printf( _( "Footprint alias library file <%s> could not be \
found in the default search paths." ),
                        GetChars( fn.GetFullName() ) );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Error opening alias library <%s>." ), GetChars( tmp ) );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        while( GetLine( file, Line, NULL, sizeof(Line) ) != NULL )
        {
            char* text = Line;
            wxString value, footprint;

            text = ReadQuotedText( value, text );

            if( text == NULL || ( *text == 0 ) || value.IsEmpty() )
                continue;

            text++;
            text = ReadQuotedText( footprint, text );

            if( footprint.IsEmpty() )
                continue;

            alias = new FOOTPRINT_ALIAS();
            alias->m_Name = value;
            alias->m_FootprintName = footprint;
            aliases.push_back( alias );
            text++;
        }

        fclose( file );
    }

    /* Display the number of footprint aliases.  */
    msg.Printf( _( "%d footprint aliases found." ), aliases.size() );
    SetStatusText( msg, 0 );

    ii = 0;

    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        m_ListCmp->SetSelection( ii++, true );

        if( !component.m_Module.IsEmpty() )
            continue;

        BOOST_FOREACH( FOOTPRINT_ALIAS& alias, aliases )
        {
            if( alias.m_Name.CmpNoCase( component.m_Value ) != 0 )
                continue;

            BOOST_FOREACH( FOOTPRINT& footprint, m_footprints )
            {
                if( alias.m_FootprintName.CmpNoCase( footprint.m_Module ) == 0 )
                {
                    SetNewPkg( footprint.m_Module );
                    break;
                }
            }

            if( component.m_Module.IsEmpty() )
            {
                msg.Printf( _( "Component %s: footprint %s not found in \
any of the project footprint libraries." ),
                            GetChars( component.m_Reference ),
                            GetChars( alias.m_FootprintName ) );
                wxMessageBox( msg, _( "CVPcb Error" ), wxOK | wxICON_ERROR,
                              this );
            }
        }
    }
}
