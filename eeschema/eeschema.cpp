/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see change_log.txt for contributors.
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

#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <gestfich.h>
#include <eda_dde.h>
#include <eeschema_settings.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <transform.h>
#include <symbol_lib_table.h>
#include <dialogs/dialog_global_sym_lib_table_config.h>
#include <dialogs/panel_sym_lib_table.h>
#include <kiway.h>
#include <sim/sim_plot_frame.h>
#include <settings/settings_manager.h>
#include <sexpr/sexpr.h>
#include <sexpr/sexpr_parser.h>
#include <kiface_ids.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <wx/ffile.h>
#include <wildcards_and_files_ext.h>

#include <schematic.h>
#include <connection_graph.h>

// The main sheet of the project
SCH_SHEET*  g_RootSheet = NULL;

// a transform matrix, to display components in lib editor
TRANSFORM DefaultTransform = TRANSFORM( 1, 0, 0, -1 );


namespace SCH {


static std::unique_ptr<SCHEMATIC> readSchematicFromFile( const std::string& aFilename )
{
    auto pi = SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD );
    std::unique_ptr<SCHEMATIC> schematic = std::make_unique<SCHEMATIC>( nullptr );

    auto &manager = Pgm().GetSettingsManager();

    manager.LoadProject( "" );
    schematic->Reset();
    schematic->SetProject( &manager.Prj() );
    schematic->SetRoot( pi->Load( aFilename, schematic.get() ) );
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_SCREENS screens( schematic->Root() );

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        screen->UpdateLocalLibSymbolLinks();

    SCH_SHEET_LIST sheets = schematic->GetSheets();

    // Restore all of the loaded symbol instances from the root sheet screen.
    sheets.UpdateSymbolInstances( schematic->RootScreen()->GetSymbolInstances() );

    sheets.AnnotatePowerSymbols();

    // NOTE: This is required for multi-unit symbols to be correct
    // Normally called from SCH_EDIT_FRAME::FixupJunctions() but could be refactored
    for( SCH_SHEET_PATH& sheet : sheets )
        sheet.UpdateAllScreenReferences();

    // NOTE: SchematicCleanUp is not called; QA schematics must already be clean or else
    // SchematicCleanUp must be freed from its UI dependencies.

    schematic->ConnectionGraph()->Recalculate( sheets, true );

    return schematic;
}


bool generateSchematicNetlist( const wxString& aFilename, wxString& aNetlist )
{
    std::unique_ptr<SCHEMATIC> schematic = readSchematicFromFile( aFilename.ToStdString() );
    NETLIST_EXPORTER_KICAD exporter( schematic.get() );
    STRING_FORMATTER formatter;

    exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD );
    aNetlist = formatter.GetString();

    return true;
}


static struct IFACE : public KIFACE_I
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_I( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_SCH:
        {
            SCH_EDIT_FRAME* frame = new SCH_EDIT_FRAME( aKiway, aParent );

            if( Kiface().IsSingle() )
            {
                // only run this under single_top, not under a project manager.
                frame->CreateServer( KICAD_SCH_PORT_SERVICE_NUMBER );
            }

            return frame;
        }

        case FRAME_SCH_SYMBOL_EDITOR:
        {
            SYMBOL_EDIT_FRAME* frame = new SYMBOL_EDIT_FRAME( aKiway, aParent );
            return frame;
        }

#ifdef KICAD_SPICE
        case FRAME_SIMULATOR:
        {
            SIM_PLOT_FRAME* frame = new SIM_PLOT_FRAME( aKiway, aParent );
            return frame;
        }
#endif
        case FRAME_SCH_VIEWER:
        case FRAME_SCH_VIEWER_MODAL:
        {
            SYMBOL_VIEWER_FRAME* frame = new SYMBOL_VIEWER_FRAME( aKiway, aParent,
                                                                  FRAME_T( aClassId ) );
            return frame;
        }

        case DIALOG_SCH_LIBRARY_TABLE:
            InvokeSchEditSymbolLibTable( aKiway, aParent );
            // Dialog has completed; nothing to return.
            return nullptr;

        default:
            return NULL;
        }
    }

    /**
     * Function IfaceOrAddress
     * return a pointer to the requested object.  The safest way to use this is to retrieve
     * a pointer to a static instance of an interface, similar to how the KIFACE interface
     * is exported.  But if you know what you are doing use it to retrieve anything you want.
     * @param aDataId identifies which object you want the address of.
     * @return void* - and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        switch( aDataId )
        {
            case KIFACE_NETLIST_SCHEMATIC:
                return (void*) generateSchematicNetlist;
        }
        return NULL;
    }

    /**
     * Function SaveFileAs
     * Saving a file under a different name is delegated to the various KIFACEs because
     * the project doesn't know the internal format of the various files (which may have
     * paths in them that need updating).
     */
    void SaveFileAs( const wxString& aProjectBasePath, const wxString& aProjectName,
                     const wxString& aNewProjectBasePath, const wxString& aNewProjectName,
                     const wxString& aSrcFilePath, wxString& aErrors ) override;

} kiface( "eeschema", KIWAY::FACE_SCH );

} // namespace

using namespace SCH;

static PGM_BASE* process;


KIFACE_I& Kiface() { return kiface; }


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

// Similar to PGM_BASE& Pgm(), but return nullptr when a *.ki_face is run from
// a python script or something else.
PGM_BASE* PgmOrNull()
{
    return process;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    // This is process-level-initialization, not project-level-initialization of the DSO.
    // Do nothing in here pertinent to a project!
    InitSettings( new EESCHEMA_SETTINGS );
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );

    start_common( aCtlBits );

    wxFileName fn = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG fpDialog( NULL );

        fpDialog.ShowModal();
    }
    else
    {
        try
        {
            // The global table is not related to a specific project.  All projects
            // will use the same global table.  So the KIFACE::OnKifaceStart() contract
            // of avoiding anything project specific is not violated here.
            if( !SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE::GetGlobalLibTable() ) )
                return false;
        }
        catch( const IO_ERROR& ioe )
        {
            // if we are here, a incorrect global symbol library table was found.
            // Incorrect global symbol library table is not a fatal error:
            // the user just has to edit the (partially) loaded table.
            wxString msg = _(
                "An error occurred attempting to load the global symbol library table.\n"
                "Please edit this global symbol library table in Preferences menu."
                );

            DisplayErrorMessage( NULL, msg, ioe.What() );
        }
    }

    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}

static void traverseSEXPR( SEXPR::SEXPR* aNode,
                           const std::function<void( SEXPR::SEXPR* )>& aVisitor )
{
    aVisitor( aNode );

    if( aNode->IsList() )
    {
        for( unsigned i = 0; i < aNode->GetNumberOfChildren(); i++ )
            traverseSEXPR( aNode->GetChild( i ), aVisitor );
    }
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
        destPath.Replace( aProjectBasePath, aNewProjectBasePath, false );

    destFile.SetPath( destPath );

    if( ext == LegacySchematicFileExtension || ext == LegacySchematicFileExtension + BackupFileSuffix ||
        ext == KiCadSchematicFileExtension || ext == KiCadSchematicFileExtension + BackupFileSuffix )
    {
        if( destFile.GetName() == aProjectName )
            destFile.SetName( aNewProjectName  );

        // Sheet paths when auto-generated are relative to the root, so those will stay
        // pointing to whatever they were pointing at.
        // The author can create their own absolute and relative sheet paths.  Absolute
        // sheet paths aren't an issue, and relative ones will continue to work as long
        // as the author didn't include any '..'s.  If they did, it's still not clear
        // whether they should be adjusted or not (as the author may be duplicating an
        // entire tree with several projects within it), so we leave this as an exercise
        // to the author.

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == SchematicSymbolFileExtension )
    {
        // Symbols are not project-specific.  Keep their source names.
        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == LegacySymbolLibFileExtension || ext == LegacySymbolDocumentFileExtension ||
             ext == KiCadSymbolLibFileExtension )
    {
        if( destFile.GetName() == aProjectName + "-cache" )
            destFile.SetName( aNewProjectName + "-cache"  );

        KiCopyFile( aSrcFilePath, destFile.GetFullPath(), aErrors );
    }
    else if( ext == NetlistFileExtension )
    {
        bool success = false;

        if( destFile.GetName() == aProjectName )
            destFile.SetName( aNewProjectName  );

        try
        {
            SEXPR::PARSER parser;
            std::unique_ptr<SEXPR::SEXPR> sexpr( parser.ParseFromFile( TO_UTF8( aSrcFilePath ) ) );

            traverseSEXPR( sexpr.get(), [&]( SEXPR::SEXPR* node )
                {
                    if( node->IsList() && node->GetNumberOfChildren() > 1
                            && node->GetChild( 0 )->IsSymbol()
                            && node->GetChild( 0 )->GetSymbol() == "source" )
                    {
                        auto pathNode = dynamic_cast<SEXPR::SEXPR_STRING*>( node->GetChild( 1 ) );
                        auto symNode = dynamic_cast<SEXPR::SEXPR_SYMBOL*>( node->GetChild( 1 ) );
                        wxString path;

                        if( pathNode )
                            path = pathNode->m_value;
                        else if( symNode )
                            path = symNode->m_value;

                        if( path == aProjectName + ".sch" )
                            path = aNewProjectName + ".sch";
                        else if( path == aProjectBasePath + "/" + aProjectName + ".sch" )
                            path = aNewProjectBasePath + "/" + aNewProjectName + ".sch";
                        else if( path.StartsWith( aProjectBasePath ) )
                            path.Replace( aProjectBasePath, aNewProjectBasePath, false );

                        if( pathNode )
                            pathNode->m_value = path;
                        else if( symNode )
                            symNode->m_value = path;
                    }
                } );

            wxFFile destNetList( destFile.GetFullPath(), "wb" );

            if( destNetList.IsOpened() )
                success = destNetList.Write( sexpr->AsString( 0 ) );

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

            msg.Printf( _( "Cannot copy file \"%s\"." ), destFile.GetFullPath() );
            aErrors += msg;
        }
    }
    else if( destFile.GetName() == "sym-lib-table" )
    {
        SYMBOL_LIB_TABLE symbolLibTable;
        symbolLibTable.Load( aSrcFilePath );

        for( unsigned i = 0; i < symbolLibTable.GetCount(); i++ )
        {
            LIB_TABLE_ROW& row = symbolLibTable.At( i );
            wxString       uri = row.GetFullURI();

            uri.Replace( "/" + aProjectName + "-cache.lib", "/" + aNewProjectName + "-cache.lib" );
            uri.Replace( "/" + aProjectName + "-rescue.lib", "/" + aNewProjectName + "-rescue.lib" );
            uri.Replace( "/" + aProjectName + ".lib", "/" + aNewProjectName + ".lib" );

            row.SetFullURI( uri );
        }

        try
        {
            symbolLibTable.Save( destFile.GetFullPath() );
        }
        catch( ... )
        {
            wxString msg;

            if( !aErrors.empty() )
                aErrors += "\n";

            msg.Printf( _( "Cannot copy file \"%s\"." ), destFile.GetFullPath() );
            aErrors += msg;
        }
    }
    else
    {
        wxFAIL_MSG( "Unexpected filetype for Eeschema::SaveFileAs()" );
    }
}

