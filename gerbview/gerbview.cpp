/**
 * @file gerbview.cpp
 * @brief GERBVIEW main file.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_settings.h>
#include <gestfich.h>
#include <kiface_base.h>
#include <macros.h>
#include <nlohmann/json.hpp>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <wx/ffile.h>

using json = nlohmann::json;


namespace GERBV {

static struct IFACE : public KIFACE_BASE
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                            int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_GERBER:
            {
                GERBVIEW_FRAME* frame = new GERBVIEW_FRAME( aKiway, aParent );
                return frame;
            }
            break;

        default:
            ;
        }

        return nullptr;
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing use
     * it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the object requested and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return nullptr;
    }

    /**
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    void SaveFileAs( const wxString& aProjectBasePath, const wxString& aProjectName,
                     const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                     const wxString& aSrcFilePath, wxString& aErrors ) override;

} kiface( "gerbview", KIWAY::FACE_GERBVIEW );

} // namespace


using namespace GERBV;


static PGM_BASE* process;


KIFACE_BASE& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
MY_API( KIFACE* ) KIFACE_GETTER(  int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    process = aProgram;
    return &kiface;
}


PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}


// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from a python script.
PGM_BASE* PgmOrNull()
{
    return process;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    InitSettings( new GERBVIEW_SETTINGS );
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );
    start_common( aCtlBits );
    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}


void IFACE::SaveFileAs( const wxString& aProjectBasePath, const wxString& aProjectName,
                        const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                        const wxString& aSrcFilePath, wxString& aErrors )
{
    wxFileName destFile( aSrcFilePath );
    wxString   destPath = destFile.GetPathWithSep();
    wxUniChar  pathSep = wxFileName::GetPathSeparator();
    wxString   ext = destFile.GetExt();

    if( destPath.StartsWith( aProjectBasePath + pathSep ) )
    {
        destPath.Replace( aProjectBasePath, aNewProjectBasePath, false );
        destFile.SetPath( destPath );
    }

    if( ext == "gbr" || IsProtelExtension( ext ) )
    {
        wxString destFileName = destFile.GetName();

        if( destFileName.StartsWith( aProjectName + "-" ) )
        {
            destFileName.Replace( aProjectName, aNewProjectName, false );
            destFile.SetName( destFileName );
        }

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == "gbrjob" )
    {
        if( destFile.GetName() == aProjectName + "-job" )
            destFile.SetName( aNewProjectName + "-job"  );

         FILE_LINE_READER jobfileReader( aSrcFilePath );

         char*    line;
         wxString data;

         while( ( line = jobfileReader.ReadLine() ) )
            data << line << '\n';

        // detect the file format: old (deprecated) gerber format or official JSON format
        if( !data.Contains( "{" ) )
        {
            KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
            return;
        }

        bool success = false;

        try
        {
            // Will throw on parse error
            json js = json::parse( TO_UTF8( data ) );

            for( auto& entry : js["FilesAttributes"] )
            {
                wxString path = wxString( entry["Path"].get<std::string>() );

                if( path.StartsWith( aProjectName + "-" ) )
                {
                    path.Replace( aProjectName, aNewProjectName, false );
                    entry["Path"] = path.ToStdString();
                }
            }

            wxFFile destJobFile( destFile.GetFullPath(), "wb" );

            if( destJobFile.IsOpened() )
                success = destJobFile.Write( js.dump( 0 ) );

            // wxFFile dtor will close the file
        }
        catch( ... )
        {
            success = false;
        }

        if( !success )
        {
            wxString msg;

            if( !aErrors.empty() )
                aErrors += "\n";

            msg.Printf( _( "Cannot copy file '%s'." ), destFile.GetFullPath() );
            aErrors += msg;
        }
    }
    else if( ext == "drl" )
    {
        wxString destFileName = destFile.GetName();

        if( destFileName == aProjectName )
            destFileName = aNewProjectName;
        else if( destFileName.StartsWith( aProjectName + "-" ) )
            destFileName.Replace( aProjectName, aNewProjectName, false );

        destFile.SetName( destFileName );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else
    {
        wxFAIL_MSG( "Unexpected filetype for GerbView::SaveFileAs()" );
    }
}

