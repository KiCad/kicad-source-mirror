/*****************************************************************/
/*  Functions to handle component library files : read functions */
/*****************************************************************/

#include "fctsys.h"
#include "confirm.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "general.h"
#include "class_library.h"

#include "dialog_load_error.h"


/**
 * Function LoadLibraries
 *
 * Clear all already loaded libraries and load all of the project libraries.
 */
void WinEDA_SchematicFrame::LoadLibraries( void )
{
    size_t         ii;
    wxFileName     fn;
    wxString       msg, tmp, errMsg;
    wxString       libraries_not_found;
    wxArrayString  sortOrder;

    CMP_LIBRARY_LIST::iterator i = CMP_LIBRARY::GetLibraryList().begin();

    /* Free the unwanted libraries but keep the cache library. */
    while ( i < CMP_LIBRARY::GetLibraryList().end() )
    {
        if( i->IsCache() )
        {
            i++;
            continue;
        }

        if( m_ComponentLibFiles.Index( i->GetName(), false ) == wxNOT_FOUND )
            i = CMP_LIBRARY::GetLibraryList().erase( i );
        else
            i++;
    }

    /* Load missing libraries. */
    for( ii = 0; ii < m_ComponentLibFiles.GetCount(); ii++ )
    {
        fn = m_ComponentLibFiles[ii];
        fn.SetExt( CompLibFileExtension );

        /* Skip if the file name is not valid.. */
        if( !fn.IsOk() )
            continue;

        if( !fn.FileExists() )
        {
            tmp = wxGetApp().FindLibraryPath( fn );
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
        msg = _( "Library " ) + tmp;
        fn = tmp;

        if( CMP_LIBRARY::AddLibrary( fn, errMsg ) )
        {
            msg += _( " loaded" );
            sortOrder.Add( fn.GetName() );
        }
        else
        {
            wxString prompt;

            prompt.Printf( _( "Component library <%s> failed to load.\nError: %s" ),
                           GetChars( fn.GetFullPath() ),
                           GetChars( errMsg ) );
            DisplayError( this, prompt );
            msg += _( " error!" );
        }

        PrintMsg( msg );
    }

    /* Print the libraries not found */
    if( !libraries_not_found.IsEmpty() )
    {
        DIALOG_LOAD_ERROR dialog( this );
        dialog.MessageSet( _( "The following libraries could not be found:" ) );
        dialog.ListSet( libraries_not_found );
        libraries_not_found.empty();
        dialog.ShowModal();
    }

    /* Put the libraries in the correct order. */
    CMP_LIBRARY::SetSortOrder( sortOrder );
    CMP_LIBRARY::GetLibraryList().sort();

#if 0   // #ifdef __WXDEBUG__
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
