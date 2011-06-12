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
#include "cvpcb_mainframe.h"
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
wxString GetQuotedText( wxString & text )
{
    int i = text.Find( QUOTE );
    if( wxNOT_FOUND == i ) return wxT( "" );
    wxString shrt = text.Mid( i + 1 );
    i = shrt.Find( QUOTE );
    if( wxNOT_FOUND == i ) return wxT( "" );
    text = shrt.Mid( i + 1 );
    return shrt.Mid( 0, i );
}


/*
 * Called by the automatic association button
 * Read *.equ files to try to find corresponding footprint
 * for each component that is not already linked to a footprint ( a "free"
 * component )
 * format of a line:
 * 'cmp_ref' 'footprint_name'
 */
void CVPCB_MAINFRAME::AssocieModule( wxCommandEvent& event )
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
            wxString value, footprint, wtext = FROM_UTF8( Line );

            value = GetQuotedText( wtext );

            if( text == NULL || ( *text == 0 ) || value.IsEmpty() )
                continue;

            footprint = GetQuotedText( wtext );

            if( footprint.IsEmpty() )
                continue;

            value.Replace( wxT( " " ), wxT( "_" ) );

            alias = new FOOTPRINT_ALIAS();
            alias->m_Name = value;
            alias->m_FootprintName = footprint;
            aliases.push_back( alias );
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
            bool found = false;
            if( alias.m_Name.CmpNoCase( component.m_Value ) != 0 )
                continue;

            /* filter alias so one can use multiple aliases (for polar and nonpolar caps for example) */
            FOOTPRINT_INFO *module = m_footprints.GetModuleInfo( alias.m_FootprintName );

            if( module )
            {
                size_t filtercount = component.m_FootprintFilter.GetCount();
                found = ( 0 == filtercount ); // if no entries, do not filter

                for( size_t jj = 0; jj < filtercount && !found; jj++ )
                {
                    found = module->m_Module.Matches( component.m_FootprintFilter[jj] );
                }
            }
            else
            {
                msg.Printf( _( "Component %s: footprint %s not found in \
any of the project footprint libraries." ),
                            GetChars( component.m_Reference ),
                            GetChars( alias.m_FootprintName ) );
                wxMessageBox( msg, _( "CVPcb Error" ), wxOK | wxICON_ERROR,
                              this );
            }
            if( found )
            {
                SetNewPkg( alias.m_FootprintName );
                break;
            }

        }
    }
}
