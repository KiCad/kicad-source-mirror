

#include <wx/config.h>
#include <bin_mod.h>
#include <online_help.h>


BIN_MOD::BIN_MOD( const char* aName ) :
    m_name( aName ),
    m_config( 0 )
{
}


void BIN_MOD::Init()
{
    // do an OS specific wxConfig instantiation, using the bin_mod (EXE/DLL/DSO) name.
    m_config = new wxConfig( wxString::FromUTF8( m_name ) );

    m_history.Load( *m_config );

    // Prepare On Line Help. Use only lower case for help file names, in order to
    // avoid problems with upper/lower case file names under windows and unix.
#if defined ONLINE_HELP_FILES_FORMAT_IS_HTML
    m_help_file = wxString::FromUTF8( m_name ) + wxT( ".html" );
#elif defined ONLINE_HELP_FILES_FORMAT_IS_PDF
    m_help_file = wxString::FromUTF8( m_name ) + wxT( ".pdf" );
#else
    #error Help files format not defined
#endif
}


void BIN_MOD::End()
{
    if( m_config )
    {
        m_history.Save( *m_config );

        // Deleting a wxConfigBase writes its contents to disk if changed.
        // Might be NULL if called twice, in which case nothing happens.
        delete m_config;
        m_config = 0;
    }
}


BIN_MOD::~BIN_MOD()
{
    End();
}

