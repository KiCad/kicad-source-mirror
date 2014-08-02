/**
 * @file eelibs_read_libraryfiles.cpp
 * @brief Functions to handle reading component library files.
 */

#include <fctsys.h>
#include <kiway.h>
#include <confirm.h>
#include <macros.h>
#include <pgm_base.h>
#include <wxEeschemaStruct.h>

#include <general.h>
#include <class_library.h>
#include <wildcards_and_files_ext.h>

#include <html_messagebox.h>


void SCH_EDIT_FRAME::LoadLibraries()
{
    size_t          ii;
    wxFileName      fn;
    wxString        msg, tmp, errMsg;
    wxString        libraries_not_found;
    wxArrayString   sortOrder;
    SEARCH_STACK&   lib_search = Prj().SchSearchS();

#if defined(DEBUG) && 1
    lib_search.Show( __func__ );
#endif

    CMP_LIBRARY_LIST::iterator i = CMP_LIBRARY::GetLibraryList().begin();

    // Free the unwanted libraries but keep the cache library.
    while( i < CMP_LIBRARY::GetLibraryList().end() )
    {
        if( i->IsCache() )
        {
            i++;
            continue;
        }

        DBG(printf( "ll:%s\n", TO_UTF8( i->GetName() ) );)

        if( m_componentLibFiles.Index( i->GetName(), false ) == wxNOT_FOUND )
            i = CMP_LIBRARY::GetLibraryList().erase( i );
        else
            i++;
    }

    // Load missing libraries.
    for( ii = 0; ii < m_componentLibFiles.GetCount(); ii++ )
    {
        fn.Clear();
        fn.SetName( m_componentLibFiles[ii] );
        fn.SetExt( SchematicLibraryFileExtension );

        // Skip if the file name is not valid..
        if( !fn.IsOk() )
            continue;

        if( !fn.FileExists() )
        {
            tmp = lib_search.FindValidPath( fn.GetFullPath() );

            if( !tmp )
            {
                libraries_not_found += fn.GetName() + _( "\n" );
                continue;
            }
        }
        else
        {
            tmp = fn.GetFullPath();
        }

        // Loaded library statusbar message
        fn = tmp;

        if( CMP_LIBRARY::AddLibrary( fn, errMsg ) )
        {
            msg.Printf( _( "Library '%s' loaded" ), GetChars( tmp ) );
            sortOrder.Add( fn.GetName() );
        }
        else
        {
            wxString prompt;

            prompt.Printf( _( "Component library '%s' failed to load.\nError: %s" ),
                           GetChars( fn.GetFullPath() ),
                           GetChars( errMsg ) );
            DisplayError( this, prompt );
            msg.Printf( _( "Library '%s' error!" ), GetChars( tmp ) );
        }

        PrintMsg( msg );
    }

    // Print the libraries not found
    if( !libraries_not_found.IsEmpty() )
    {
        // parent of this dialog cannot be NULL since that breaks the Kiway() chain.
        HTML_MESSAGE_BOX dialog( this, _("Files not found") );

        dialog.MessageSet( _( "The following libraries could not be found:" ) );
        dialog.ListSet( libraries_not_found );
        libraries_not_found.empty();
        dialog.ShowModal();
    }

    // Put the libraries in the correct order.
    CMP_LIBRARY::SetSortOrder( sortOrder );
    CMP_LIBRARY::GetLibraryList().sort();

#if 0 && defined(__WXDEBUG__)
    wxLogDebug( wxT( "LoadLibraries() requested component library sort order:" ) );

    for( size_t i = 0; i < sortOrder.GetCount(); i++ )
         wxLogDebug( wxT( "    " ) + sortOrder[i] );

    wxLogDebug( wxT( "Real component library sort order:" ) );

    for ( i = CMP_LIBRARY::GetLibraryList().begin();
          i < CMP_LIBRARY::GetLibraryList().end(); i++ )
        wxLogDebug( wxT( "    " ) + i->GetName() );

    wxLogDebug( wxT( "end LoadLibraries ()" ) );
#endif
}

