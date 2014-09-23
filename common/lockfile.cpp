

#include <wx/filename.h>
#include <wx/snglinst.h>


wxSingleInstanceChecker* LockFile( const wxString& aFileName )
{
    // first make absolute and normalize, to avoid that different lock files
    // for the same file can be created
    wxFileName fn( aFileName );

    fn.MakeAbsolute();

    wxString lockFileName = fn.GetFullPath() + wxT( ".lock" );

    lockFileName.Replace( wxT( "/" ), wxT( "_" ) );

    // We can have filenames coming from Windows, so also convert Windows separator
    lockFileName.Replace( wxT( "\\" ), wxT( "_" ) );

    wxSingleInstanceChecker* p = new wxSingleInstanceChecker( lockFileName );

    if( p->IsAnotherRunning() )
    {
        delete p;
        p = NULL;
    }

    return p;
}

