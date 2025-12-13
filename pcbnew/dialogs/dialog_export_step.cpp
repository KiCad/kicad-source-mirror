/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_export_step.h"
#include "dialog_export_step_process.h"

#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/process.h>
#include <wx/string.h>
#include <wx/filedlg.h>

#include <pgm_base.h>
#include <board.h>
#include <confirm.h>
#include <kidialog.h>
#include <widgets/std_bitmap_button.h>
#include <footprint.h>
#include <kiface_base.h>
#include <locale_io.h>
#include <math/vector3.h>
#include <pcb_edit_frame.h>
#include <tools/board_editor_control.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <reporter.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <widgets/text_ctrl_eval.h>
#include <wildcards_and_files_ext.h>
#include <filename_resolver.h>
#include <core/map_helpers.h>
#include <settings/settings_manager.h>
#include <jobs/job_export_pcb_3d.h>


// Maps m_choiceFormat selection to extension (and kicad-cli command)
static const std::vector<wxString> c_formatCommand = { FILEEXT::StepFileExtension,
                                                       FILEEXT::GltfBinaryFileExtension,
                                                       FILEEXT::XaoFileExtension,
                                                       FILEEXT::BrepFileExtension,
                                                       FILEEXT::PlyFileExtension,
                                                       FILEEXT::StlFileExtension,
                                                       FILEEXT::StepZFileAbrvExtension,
                                                       FILEEXT::U3DFileExtension,
                                                       wxS( "3dpdf" ),
                                                    };

// Maps file extensions to m_choiceFormat selection
static const std::map<wxString, int> c_formatExtToChoice = { { FILEEXT::StepFileExtension, 0 },
                                                             { FILEEXT::StepFileAbrvExtension, 0 },
                                                             { FILEEXT::GltfBinaryFileExtension, 1 },
                                                             { FILEEXT::XaoFileExtension, 2 },
                                                             { FILEEXT::BrepFileExtension, 3 },
                                                             { FILEEXT::PlyFileExtension, 4 },
                                                             { FILEEXT::StlFileExtension, 5 },
                                                             { FILEEXT::StepZFileAbrvExtension, 6 },
                                                             { FILEEXT::U3DFileExtension, 7 },
                                                             { FILEEXT::PdfFileExtension, 8 }};


DIALOG_EXPORT_STEP::DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aEditFrame, const wxString& aBoardPath ) :
        DIALOG_EXPORT_STEP( aEditFrame, aEditFrame, aBoardPath )
{
}


DIALOG_EXPORT_STEP::DIALOG_EXPORT_STEP( PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent,
                                        const wxString& aBoardPath, JOB_EXPORT_PCB_3D* aJob ) :
        DIALOG_EXPORT_STEP_BASE( aEditFrame ),
        m_editFrame( aEditFrame ),
        m_job( aJob ),
        m_originX( aEditFrame, m_originXLabel, m_originXCtrl, m_originXUnits ),
        m_originY( aEditFrame, m_originYLabel, m_originYCtrl, m_originYUnits ),
        m_boardPath( aBoardPath )
{
    if( !m_job )
    {
        m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
        SetupStandardButtons( { { wxID_OK,     _( "Export" ) },
                                { wxID_CANCEL, _( "Close" )  } } );
    }
    else
    {
        SetTitle( m_job->GetSettingsDialogTitle() );

        m_browseButton->Hide();
        SetupStandardButtons();
    }

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    Layout();
    bSizerSTEPFile->Fit( this );

    SetFocus();

    wxString bad_scales;
    size_t   bad_count = 0;

    for( FOOTPRINT* fp : m_editFrame->GetBoard()->Footprints() )
    {
        for( const FP_3DMODEL& model : fp->Models() )
        {
            if( model.m_Scale.x != 1.0 || model.m_Scale.y != 1.0 || model.m_Scale.z != 1.0 )
            {
                bad_scales.Append( wxS("\n") );
                bad_scales.Append( model.m_Filename );
                bad_count++;
            }
        }

        if( bad_count >= 5 )
            break;
    }

    if( !bad_scales.empty() && !Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning )
    {
        wxString extendedMsg = _( "Non-unity scaled models:" ) + wxT( "\n" ) + bad_scales;

        KIDIALOG msgDlg( m_editFrame, _( "Scaled models detected.  Model scaling is not reliable for "
                                         "mechanical export." ),
                         _( "Model Scale Warning" ), wxOK | wxICON_WARNING );
        msgDlg.SetExtendedMessage( extendedMsg );
        msgDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        msgDlg.ShowModal();

        if( msgDlg.DoNotShowAgain() )
            Pgm().GetCommonSettings()->m_DoNotShowAgain.scaled_3d_models_warning = true;
    }

    OnFmtChoiceOptionChanged();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_EXPORT_STEP::TransferDataToWindow()
{
    if( !m_job )
    {
        if( m_outputFileName->GetValue().IsEmpty() )
        {
            wxFileName brdFile( m_editFrame->GetBoard()->GetFileName() );
            brdFile.SetExt( wxT( "step" ) );
            m_outputFileName->SetValue( brdFile.GetFullPath() );
        }
    }
    else
    {
        m_rbBoardCenterOrigin->SetValue( true );    // Default

        if( m_job->m_3dparams.m_UseDrillOrigin )
            m_rbDrillAndPlotOrigin->SetValue( true );
        else if( m_job->m_3dparams.m_UseGridOrigin )
            m_rbGridOrigin->SetValue( true );
        else if( m_job->m_3dparams.m_UseDefinedOrigin )
            m_rbUserDefinedOrigin->SetValue( true );
        else if( m_job->m_3dparams.m_UsePcbCenterOrigin )
            m_rbBoardCenterOrigin->SetValue( true );

        m_originX.SetValue( pcbIUScale.mmToIU( m_job->m_3dparams.m_Origin.x ) );
        m_originY.SetValue( pcbIUScale.mmToIU( m_job->m_3dparams.m_Origin.y ) );

        m_txtNetFilter->SetValue( m_job->m_3dparams.m_NetFilter );
        m_cbOptimizeStep->SetValue( m_job->m_3dparams.m_OptimizeStep );
        m_cbExportBody->SetValue( m_job->m_3dparams.m_ExportBoardBody );
        m_cbExportComponents->SetValue( m_job->m_3dparams.m_ExportComponents );
        m_cbExportTracks->SetValue( m_job->m_3dparams.m_ExportTracksVias );
        m_cbExportPads->SetValue( m_job->m_3dparams.m_ExportPads );
        m_cbExportZones->SetValue( m_job->m_3dparams.m_ExportZones );
        m_cbExportInnerCopper->SetValue( m_job->m_3dparams.m_ExportInnerCopper );
        m_cbExportSilkscreen->SetValue( m_job->m_3dparams.m_ExportSilkscreen );
        m_cbExportSoldermask->SetValue( m_job->m_3dparams.m_ExportSoldermask );
        m_cbFuseShapes->SetValue( m_job->m_3dparams.m_FuseShapes );
        m_cbCutViasInBody->SetValue( m_job->m_3dparams.m_CutViasInBody );
        m_cbFillAllVias->SetValue( m_job->m_3dparams.m_FillAllVias );
        m_cbRemoveUnspecified->SetValue( !m_job->m_3dparams.m_IncludeUnspecified );
        m_cbRemoveDNP->SetValue( !m_job->m_3dparams.m_IncludeDNP );
        m_cbSubstModels->SetValue( m_job->m_3dparams.m_SubstModels );
        m_cbOverwriteFile->SetValue( m_job->m_3dparams.m_Overwrite );

        if( m_job->m_3dparams.m_BoardOutlinesChainingEpsilon > 0.05 )
            m_choiceTolerance->SetSelection( 2 );
        else if( m_job->m_3dparams.m_BoardOutlinesChainingEpsilon < 0.005 )
            m_choiceTolerance->SetSelection( 0 );
        else
            m_choiceTolerance->SetSelection( 1 );

        m_txtComponentFilter->SetValue( m_job->m_3dparams.m_ComponentFilter );
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    }

    // Sync the enabled states
    wxCommandEvent dummy;
    DIALOG_EXPORT_STEP::onCbExportComponents( dummy );

    return true;
}


int BOARD_EDITOR_CONTROL::ExportSTEP( const TOOL_EVENT& aEvent )
{
    BOARD*     board = m_frame->GetBoard();
    wxFileName brdFile = board->GetFileName();

    DIALOG_EXPORT_STEP dlg( m_frame, brdFile.GetFullPath() );
    dlg.ShowModal();

    return 0;
}


void DIALOG_EXPORT_STEP::onUpdateXPos( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onUpdateYPos( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( m_rbUserDefinedOrigin->GetValue() );
}


void DIALOG_EXPORT_STEP::onBrowseClicked( wxCommandEvent& aEvent )
{
    // clang-format off
    wxString filter = _( "STEP files" )
                      + AddFileExtListToFilter( { FILEEXT::StepFileExtension, FILEEXT::StepFileAbrvExtension } ) + "|"
                      + _( "Binary glTF files" )
                      + AddFileExtListToFilter( { FILEEXT::GltfBinaryFileExtension } ) + "|"
                      + _( "XAO files" )
                      + AddFileExtListToFilter( { FILEEXT::XaoFileExtension} ) + "|"
                      + _( "BREP (OCCT) files" )
                      + AddFileExtListToFilter( { FILEEXT::BrepFileExtension } )  + "|"
                      + _( "PLY files" )
                      + AddFileExtListToFilter( { FILEEXT::PlyFileExtension} ) + "|"
                      + _( "STL files" )
                      + AddFileExtListToFilter( { FILEEXT::StlFileExtension} ) + "|"
                      + _( "Universal 3D files" )
                      + AddFileExtListToFilter( { FILEEXT::U3DFileExtension} )+ "|"
                      + _( "PDF files" )
                      + AddFileExtListToFilter( { FILEEXT::PdfFileExtension} );
    // clang-format on

    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString   path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    wxFileName fn( Prj().AbsolutePath( path ) );

    wxFileDialog dlg( this, _( "3D Model Output File" ), fn.GetPath(), fn.GetFullName(), filter, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    path = dlg.GetPath();
    m_outputFileName->SetValue( path );

    fn = wxFileName( path );

    if( auto formatChoice = get_opt( c_formatExtToChoice, fn.GetExt().Lower() ) )
        m_choiceFormat->SetSelection( *formatChoice );
}


void DIALOG_EXPORT_STEP::onFormatChoice( wxCommandEvent& event )
{
    OnFmtChoiceOptionChanged();
}


void DIALOG_EXPORT_STEP::OnFmtChoiceOptionChanged()
{
    wxString newExt;
    int idx = m_choiceFormat->GetSelection();

    for( auto& choices : c_formatExtToChoice )
    {
        if( choices.second == idx )
        {
            newExt = choices.first;
            break;
        }
    }

    wxString        path = m_outputFileName->GetValue();

    int sepIdx = std::max( path.Find( '/', true ), path.Find( '\\', true ) );
    int dotIdx = path.Find( '.', true );

    if( dotIdx == -1 || dotIdx < sepIdx )
        path << '.' << newExt;
    else
        path = path.Mid( 0, dotIdx ) << '.' << newExt;

    m_outputFileName->SetValue( path );
}


void DIALOG_EXPORT_STEP::onCbExportComponents( wxCommandEvent& event )
{
    bool enable = m_cbExportComponents->GetValue();

    m_rbAllComponents->Enable( enable );
    m_rbOnlySelected->Enable( enable );
    m_rbFilteredComponents->Enable( enable );
    m_txtComponentFilter->Enable( enable && m_rbFilteredComponents->GetValue() );
}


void DIALOG_EXPORT_STEP::OnComponentModeChange( wxCommandEvent& event )
{
    m_txtComponentFilter->Enable( m_rbFilteredComponents->GetValue() );
}


void DIALOG_EXPORT_STEP::onExportButton( wxCommandEvent& aEvent )
{
    wxString path = m_outputFileName->GetValue();
    double   tolerance;   // default value in mm

    switch( m_choiceTolerance->GetSelection() )
    {
    case 0:  tolerance = 0.001; break;
    default:
    case 1:  tolerance = 0.01;  break;
    case 2:  tolerance = 0.1;   break;
    }

    if( !m_job )
    {
        // Build the absolute path of current output directory to preselect it in the file browser.
        std::function<bool( wxString* )> textResolver =
                [&]( wxString* token ) -> bool
                {
                    return m_editFrame->GetBoard()->ResolveTextVar( token, 0 );
                };

        path = ExpandTextVars( path, &textResolver );
        path = ExpandEnvVarSubstitutions( path, &Prj() );
        path = Prj().AbsolutePath( path );

        if( path.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "No filename for output file" ) );
            return;
        }

        SHAPE_POLY_SET outline;
        wxString msg;

        // Check if the board outline is continuous
        // max dist from one endPt to next startPt to build a closed shape:
        int chainingEpsilon = pcbIUScale.mmToIU( tolerance );

        // Arc to segment approximation error (not critical here: we do not use the outline shape):
        int maxError = pcbIUScale.mmToIU( 0.05 );

        if( !BuildBoardPolygonOutlines( m_editFrame->GetBoard(), outline, maxError, chainingEpsilon, false ) )
        {
            DisplayErrorMessage( this, wxString::Format( _( "Board outline is missing or not closed using "
                                                            "%.3f mm tolerance.\n"
                                                            "Run DRC for a full analysis." ),
                                                         tolerance ) );
            return;
        }

        wxFileName fn( Prj().AbsolutePath( path ) );

        if( fn.FileExists() && !m_cbOverwriteFile->GetValue() )
        {
            msg.Printf( _( "File '%s' already exists. Do you want overwrite this file?" ), fn.GetFullPath() );

            if( wxMessageBox( msg, _( "STEP/GLTF Export" ), wxYES_NO | wxICON_QUESTION, this ) == wxNO )
                return;
        }

        wxFileName appK2S( wxStandardPaths::Get().GetExecutablePath() );
    #ifdef __WXMAC__
        // On macOS, we have standalone applications inside the main bundle, so we handle that here:
        if( appK2S.GetPath().Find( "/Contents/Applications/pcbnew.app/Contents/MacOS" ) != wxNOT_FOUND )
        {
            appK2S.AppendDir( wxT( ".." ) );
            appK2S.AppendDir( wxT( ".." ) );
            appK2S.AppendDir( wxT( ".." ) );
            appK2S.AppendDir( wxT( ".." ) );
            appK2S.AppendDir( wxT( "MacOS" ) );
        }
    #else
        if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
        {
            appK2S.RemoveLastDir();
            appK2S.AppendDir( "kicad" );
        }
    #endif

        appK2S.SetName( wxT( "kicad-cli" ) );
        appK2S.Normalize( FN_NORMALIZE_FLAGS );

        wxString cmdK2S = wxT( "\"" );
        cmdK2S.Append( appK2S.GetFullPath() );
        cmdK2S.Append( wxT( "\"" ) );

        cmdK2S.Append( wxT( " pcb" ) );
        cmdK2S.Append( wxT( " export" ) );

        cmdK2S.Append( wxT( " " ) );
        cmdK2S.Append( c_formatCommand[m_choiceFormat->GetSelection()] );

        if( m_cbRemoveUnspecified->GetValue() )
            cmdK2S.Append( wxT( " --no-unspecified" ) );

        if( m_cbRemoveDNP->GetValue() )
            cmdK2S.Append( wxT( " --no-dnp" ) );

        if( m_cbSubstModels->GetValue() )
            cmdK2S.Append( wxT( " --subst-models" ) );

        if( !m_cbOptimizeStep->GetValue() )
            cmdK2S.Append( wxT( " --no-optimize-step" ) );

        if( !m_cbExportBody->GetValue() )
            cmdK2S.Append( wxT( " --no-board-body" ) );

        if( !m_cbExportComponents->GetValue() )
            cmdK2S.Append( wxT( " --no-components" ) );

        if( m_cbExportTracks->GetValue() )
            cmdK2S.Append( wxT( " --include-tracks" ) );

        if( m_cbExportPads->GetValue() )
            cmdK2S.Append( wxT( " --include-pads" ) );

        if( m_cbExportZones->GetValue() )
            cmdK2S.Append( wxT( " --include-zones" ) );

        if( m_cbExportInnerCopper->GetValue() )
            cmdK2S.Append( wxT( " --include-inner-copper" ) );

        if( m_cbExportSilkscreen->GetValue() )
            cmdK2S.Append( wxT( " --include-silkscreen" ) );

        if( m_cbExportSoldermask->GetValue() )
            cmdK2S.Append( wxT( " --include-soldermask" ) );

        if( m_cbFuseShapes->GetValue() )
            cmdK2S.Append( wxT( " --fuse-shapes" ) );

        if( m_cbCutViasInBody->GetValue() )
            cmdK2S.Append( wxT( " --cut-vias-in-body" ) );

        if( m_cbFillAllVias->GetValue() )
            cmdK2S.Append( wxT( " --fill-all-vias" ) );

        // Note: for some reason, using \" to insert a quote in a format string, under MacOS
        // wxString::Format does not work. So use a %c format in string
        int quote = '\'';
        int dblquote = '"';

        if( !m_txtNetFilter->GetValue().empty() )
        {
            cmdK2S.Append( wxString::Format( wxT( " --net-filter %c%s%c" ),
                                             dblquote, m_txtNetFilter->GetValue(), dblquote ) );
        }

        if( m_rbOnlySelected->GetValue() )
        {
            wxArrayString components;
            SELECTION& selection = m_editFrame->GetCurrentSelection();

            std::for_each( selection.begin(), selection.end(),
                           [&components]( EDA_ITEM* item )
                           {
                               if( item->Type() == PCB_FOOTPRINT_T )
                                   components.push_back( static_cast<FOOTPRINT*>( item )->GetReference() );
                           } );

            cmdK2S.Append( wxString::Format( wxT( " --component-filter %c%s%c" ),
                                             dblquote, wxJoin( components, ',' ), dblquote ) );
        }
        else if( m_rbFilteredComponents->GetValue() )
        {
            cmdK2S.Append( wxString::Format( wxT( " --component-filter %c%s%c" ),
                                             dblquote, m_txtComponentFilter->GetValue(), dblquote ) );
        }

        if( m_rbDrillAndPlotOrigin->GetValue() )
        {
            cmdK2S.Append( wxT( " --drill-origin" ) );
        }
        else if( m_rbGridOrigin->GetValue() )
        {
            cmdK2S.Append( wxT( " --grid-origin" ) );
        }
        else if( m_rbUserDefinedOrigin->GetValue() )
        {
            double xOrg = pcbIUScale.IUTomm( m_originX.GetIntValue() );
            double yOrg = pcbIUScale.IUTomm( m_originY.GetIntValue() );

            LOCALE_IO dummy;
            cmdK2S.Append( wxString::Format( wxT( " --user-origin=%c%.6fx%.6fmm%c" ),
                                             quote, xOrg, yOrg, quote ) );
        }
        else if( m_rbBoardCenterOrigin->GetValue() )
        {
            BOX2I     bbox = m_editFrame->GetBoard()->ComputeBoundingBox( true );
            double    xOrg = pcbIUScale.IUTomm( bbox.GetCenter().x );
            double    yOrg = pcbIUScale.IUTomm( bbox.GetCenter().y );
            LOCALE_IO dummy;

            cmdK2S.Append( wxString::Format( wxT( " --user-origin=%c%.6fx%.6fmm%c" ),
                                             quote, xOrg, yOrg, quote ) );
        }
        else
        {
            wxFAIL_MSG( wxT( "Unsupported origin option: how did we get here?" ) );
        }

        {
            LOCALE_IO dummy;
            cmdK2S.Append( wxString::Format( wxT( " --min-distance=%c%.3fmm%c" ),
                                             quote, tolerance, quote ) );
        }

        // Output file path.
        cmdK2S.Append( wxString::Format( wxT( " -f -o %c%s%c" ),
                                         dblquote, fn.GetFullPath(), dblquote ) );


        // Input file path.
        cmdK2S.Append( wxString::Format( wxT( " %c%s%c" ), dblquote, m_boardPath, dblquote ) );

        wxLogTrace( traceKiCad2Step, wxT( "export step command: %s" ), cmdK2S );

        DIALOG_EXPORT_STEP_LOG* log = new DIALOG_EXPORT_STEP_LOG( this, cmdK2S );
        log->ShowModal();
    }
    else
    {
        m_job->SetConfiguredOutputPath( path );
        m_job->m_3dparams.m_NetFilter = m_txtNetFilter->GetValue();
        m_job->m_3dparams.m_ComponentFilter = m_txtComponentFilter->GetValue();
        m_job->m_3dparams.m_ExportBoardBody = m_cbExportBody->GetValue();
        m_job->m_3dparams.m_ExportComponents = m_cbExportComponents->GetValue();
        m_job->m_3dparams.m_ExportTracksVias = m_cbExportTracks->GetValue();
        m_job->m_3dparams.m_ExportPads = m_cbExportPads->GetValue();
        m_job->m_3dparams.m_ExportZones = m_cbExportZones->GetValue();
        m_job->m_3dparams.m_ExportInnerCopper = m_cbExportInnerCopper->GetValue();
        m_job->m_3dparams.m_ExportSilkscreen = m_cbExportSilkscreen->GetValue();
        m_job->m_3dparams.m_ExportSoldermask = m_cbExportSoldermask->GetValue();
        m_job->m_3dparams.m_FuseShapes = m_cbFuseShapes->GetValue();
        m_job->m_3dparams.m_CutViasInBody = m_cbCutViasInBody->GetValue();
        m_job->m_3dparams.m_FillAllVias = m_cbFillAllVias->GetValue();
        m_job->m_3dparams.m_OptimizeStep = m_cbOptimizeStep->GetValue();
        m_job->m_3dparams.m_Overwrite = m_cbOverwriteFile->GetValue();
        m_job->m_3dparams.m_IncludeUnspecified = !m_cbRemoveUnspecified->GetValue();
        m_job->m_3dparams.m_IncludeDNP = !m_cbRemoveDNP->GetValue();
        m_job->m_3dparams.m_SubstModels = m_cbSubstModels->GetValue();
        m_job->m_3dparams.m_BoardOutlinesChainingEpsilon = tolerance;

        m_job->SetStepFormat( static_cast<EXPORTER_STEP_PARAMS::FORMAT>( m_choiceFormat->GetSelection() ) );

        // ensure the main format on the job is populated
        switch( m_job->m_3dparams.m_Format )
        {
        case EXPORTER_STEP_PARAMS::FORMAT::STEP:  m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::STEP;  break;
        case EXPORTER_STEP_PARAMS::FORMAT::STEPZ: m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::STEPZ; break;
        case EXPORTER_STEP_PARAMS::FORMAT::GLB:   m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::GLB;   break;
        case EXPORTER_STEP_PARAMS::FORMAT::XAO:   m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::XAO;   break;
        case EXPORTER_STEP_PARAMS::FORMAT::BREP:  m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::BREP;  break;
        case EXPORTER_STEP_PARAMS::FORMAT::PLY:   m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::PLY;   break;
        case EXPORTER_STEP_PARAMS::FORMAT::STL:   m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::STL;   break;
        case EXPORTER_STEP_PARAMS::FORMAT::U3D:   m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::U3D;   break;
        case EXPORTER_STEP_PARAMS::FORMAT::PDF:   m_job->m_format = JOB_EXPORT_PCB_3D::FORMAT::PDF;   break;
        }

        m_job->m_3dparams.m_UseDrillOrigin = false;
        m_job->m_3dparams.m_UseGridOrigin = false;
        m_job->m_3dparams.m_UseDefinedOrigin = false;
        m_job->m_3dparams.m_UsePcbCenterOrigin = false;

        if( m_rbDrillAndPlotOrigin->GetValue() )
        {
            m_job->m_3dparams.m_UseDrillOrigin = true;
        }
        else if( m_rbGridOrigin->GetValue() )
        {
            m_job->m_3dparams.m_UseGridOrigin = true;
        }
        else if( m_rbUserDefinedOrigin->GetValue() )
        {
            double xOrg = pcbIUScale.IUTomm( m_originX.GetIntValue() );
            double yOrg = pcbIUScale.IUTomm( m_originY.GetIntValue() );

            m_job->m_3dparams.m_UseDefinedOrigin = true;
            m_job->m_3dparams.m_Origin = VECTOR2D( xOrg, yOrg );
        }
        else if( m_rbBoardCenterOrigin->GetValue() )
        {
            BOX2I     bbox = m_editFrame->GetBoard()->ComputeBoundingBox( true );
            double    xOrg = pcbIUScale.IUTomm( bbox.GetCenter().x );
            double    yOrg = pcbIUScale.IUTomm( bbox.GetCenter().y );
            LOCALE_IO dummy;

            m_job->m_3dparams.m_UsePcbCenterOrigin = true;
            m_job->m_3dparams.m_Origin = VECTOR2D( xOrg, yOrg );
        }

        EndModal( wxID_OK );
    }
}
