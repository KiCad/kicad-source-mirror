

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
    // Help files are now using html format.
    // Old help files used pdf format.
    // so when searching a help file, the .html file will be searched,
    // and if not found, the .pdf file  will be searched.
    m_help_file = wxString::FromUTF8( m_name );     // no ext given. can be .html or .pdf
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

