/**
 * @file cvpcb/loadcmp.cpp
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <gr_basic.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <macros.h>
#include <appl_wxstruct.h>

#include <pcbstruct.h>
#include <class_module.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <class_DisplayFootprintsFrame.h>
#include <io_mgr.h>
#include <wildcards_and_files_ext.h>


/**
 * Read libraries to find a module.
 * If this module is found, copy it into memory
 *
 * @param CmpName - Module name
 * @return - a pointer to the loaded module or NULL.
 */
MODULE* DISPLAY_FOOTPRINTS_FRAME::Get_Module( const wxString& aFootprintName )
{
    CVPCB_MAINFRAME* parent = ( CVPCB_MAINFRAME* ) GetParent();

    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        for( unsigned i = 0; i < parent->m_ModuleLibNames.GetCount();  ++i )
        {
            wxFileName fn = parent->m_ModuleLibNames[i];

            fn.SetExt( FootprintLibFileExtension );

            wxString libPath = wxGetApp().FindLibraryPath( fn );

            if( !libPath )
            {
                wxString msg = wxString::Format(
                    _("PCB foot print library file <%s> could not be found in the default search paths." ),
                    fn.GetFullName().GetData() );

                // @todo we should not be using wxMessageBox directly.
                wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR, this );
                continue;
            }

            MODULE* footprint = pi->FootprintLoad( libPath, aFootprintName );

            if( footprint )
            {
                footprint->SetParent( GetBoard() );
                footprint->SetPosition( wxPoint( 0, 0 ) );
                return footprint;
            }
        }
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return NULL;
    }

    wxString msg = wxString::Format( _( "Footprint '%s' not found" ), aFootprintName.GetData() );
    DisplayError( this, msg );
    return NULL;
}

