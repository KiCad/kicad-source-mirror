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
#include <richio.h>
#include <filter_reader.h>
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
    wxFileName  filename;
    wxString    libname;

    // Clear data before reading files
    m_filesNotFound.Empty();
    m_filesInvalid.Empty();
    m_List.clear();

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

    // Parse Libraries Listed
    for( unsigned ii = 0; ii < aFootprintsLibNames.GetCount(); ii++ )
    {
        filename = aFootprintsLibNames[ii];
        filename.SetExt( FootprintLibFileExtension );

        libname = wxGetApp().FindLibraryPath( filename );

        if( libname.IsEmpty() )
        {
            m_filesNotFound << filename.GetFullName() << wxT("\n");
            continue;
        }

        try
        {
            wxArrayString fpnames = pi->FootprintEnumerate( libname );

            for( unsigned i=0; i<fpnames.GetCount();  ++i )
            {
                std::auto_ptr<MODULE> m( pi->FootprintLoad( libname, fpnames[i] ) );

                FOOTPRINT_INFO* fpinfo = new FOOTPRINT_INFO();

                fpinfo->m_Module   = fpnames[i];
                fpinfo->m_LibName  = libname;
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

    m_List.sort();

    return true;
}

