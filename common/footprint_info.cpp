/**
 * @file footprint_info.cpp
 */


/*
 * Functions to read footprint libraries and fill m_footprints by available footprints names
 * and their documentation (comments and keywords)
 */
#include <fctsys.h>
#include <wxstruct.h>
#include <common.h>
#include <kicad_string.h>
#include <macros.h>
#include <appl_wxstruct.h>

#include <pcbcommon.h>
#include <pcbstruct.h>
#include <footprint_info.h>
#include <io_mgr.h>

#include <class_pad.h>
#include <class_module.h>
#include <wildcards_and_files_ext.h>


/* Read the list of libraries (*.mod files)
 * for each module are stored
 *      the module name
 *      documentation string
 *      associated keywords
 *      lib name
 * Module description format:
 *   $MODULE c64acmd                    First line of module description
 *   Li c64acmd DIN connector           Library reference
 *   Cd Europe 96 AC male vertical      documentation string
 *   Kw PAD_CONN DIN                    associated keywords
 *   ...... other data (pads, outlines ..)
 *   $Endmodule
 */
bool FOOTPRINT_LIST::ReadFootprintFiles( wxArrayString& aFootprintsLibNames )
{
    // Clear data before reading files
    m_filesNotFound.Empty();
    m_filesInvalid.Empty();
    m_List.clear();

    // try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        // Parse Libraries Listed
        for( unsigned ii = 0; ii < aFootprintsLibNames.GetCount(); ii++ )
        {
            wxFileName filename = aFootprintsLibNames[ii];

            filename.SetExt( FootprintLibFileExtension );

            wxString libPath = wxGetApp().FindLibraryPath( filename );

            if( !libPath )
            {
                m_filesNotFound << filename.GetFullName() << wxT("\n");
                continue;
            }

            try
            {
                wxArrayString fpnames = pi->FootprintEnumerate( libPath );

                for( unsigned i=0; i<fpnames.GetCount();  ++i )
                {
                    auto_ptr<MODULE> m( pi->FootprintLoad( libPath, fpnames[i] ) );

                    // we're loading what we enumerated, all must be there.
                    wxASSERT( m.get() );

                    FOOTPRINT_INFO* fpinfo = new FOOTPRINT_INFO();

                    fpinfo->m_Module   = fpnames[i];
                    fpinfo->m_LibName  = libPath;
                    fpinfo->m_padCount = m->GetPadCount();
                    fpinfo->m_KeyWord  = m->GetKeywords();
                    fpinfo->m_Doc      = m->GetDescription();

                    AddItem( fpinfo );
                }
            }
            catch( IO_ERROR ioe )
            {
                m_filesInvalid << ioe.errorText << wxT("\n");
            }
        }
    }

    /*  caller should catch this, UI seems not wanted here.
    catch( IO_ERROR ioe )
    {
        DisplayError( NULL, ioe.errorText );
        return false;
    }
    */

    m_List.sort();

    return true;
}
