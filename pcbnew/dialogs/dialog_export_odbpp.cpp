/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialogs/dialog_export_odbpp.h"
#include "dialogs/panel_fab_export_content.h"

#include <wx/filedlg.h>
#include <kiplatform/ui.h>

#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project.h>
#include <string_utils.h>
#include <widgets/std_bitmap_button.h>
#include <jobs/job_export_pcb_odb.h>
#include <board.h>
#include <reporter.h>
#include <widgets/wx_progress_reporters.h>
#include <widgets/wx_html_report_panel.h>
#include <pcb_io/odbpp/pcb_io_odbpp.h>
#include <tools/zone_filler_tool.h>
#include <tool/tool_manager.h>
#include <grid_tricks.h>
#include <widgets/wx_grid.h>
#include <pcb_io/odbpp/odb_util.h>

#include <algorithm>


namespace
{
enum LAYER_GRID_COL
{
    COL_INCLUDE,
    COL_LAYER,
    COL_NAME,
    COL_TYPE,
    COL_CONTEXT,
    COL_SPAN
};


wxArrayString overridableTypeNames( PCB_LAYER_ID aLayer )
{
    wxArrayString names;

    for( ODB_TYPE type : ODB::OverridableLayerTypes( aLayer ) )
        names.Add( wxString::FromUTF8( ODB::Enum2String( type ) ) );

    return names;
}

JOB_EXPORT_PCB_ODB::DATA_SET toDataSet( IPC2581::MODE aMode )
{
    switch( aMode )
    {
    case IPC2581::MODE::FABRICATION: return JOB_EXPORT_PCB_ODB::DATA_SET::FABRICATION;
    case IPC2581::MODE::ASSEMBLY: return JOB_EXPORT_PCB_ODB::DATA_SET::ASSEMBLY;
    case IPC2581::MODE::TEST: return JOB_EXPORT_PCB_ODB::DATA_SET::TEST;
    case IPC2581::MODE::STACKUP: return JOB_EXPORT_PCB_ODB::DATA_SET::STACKUP;
    default: return JOB_EXPORT_PCB_ODB::DATA_SET::ALL;
    }
}


IPC2581::MODE fromDataSet( JOB_EXPORT_PCB_ODB::DATA_SET aDataSet )
{
    switch( aDataSet )
    {
    case JOB_EXPORT_PCB_ODB::DATA_SET::FABRICATION: return IPC2581::MODE::FABRICATION;
    case JOB_EXPORT_PCB_ODB::DATA_SET::ASSEMBLY: return IPC2581::MODE::ASSEMBLY;
    case JOB_EXPORT_PCB_ODB::DATA_SET::TEST: return IPC2581::MODE::TEST;
    case JOB_EXPORT_PCB_ODB::DATA_SET::STACKUP: return IPC2581::MODE::STACKUP;
    default: return IPC2581::MODE::USERDEF;
    }
}
} // namespace


DIALOG_EXPORT_ODBPP::DIALOG_EXPORT_ODBPP( PCB_EDIT_FRAME* aParent ) :
        DIALOG_EXPORT_ODBPP_BASE( aParent ),
        m_parent( aParent ),
        m_job( nullptr )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    SetupStandardButtons( { { wxID_OK, _( "Export" ) }, { wxID_CANCEL, _( "Close" ) } } );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    setupControls();
    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXPORT_ODBPP::DIALOG_EXPORT_ODBPP( JOB_EXPORT_PCB_ODB* aJob, PCB_EDIT_FRAME* aEditFrame,
                                          wxWindow* aParent ) :
        DIALOG_EXPORT_ODBPP_BASE( aParent ),
        m_parent( aEditFrame ),
        m_job( aJob )
{
    m_browseButton->Hide();
    m_messagesPanel->Hide();

    SetupStandardButtons();

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    setupControls();
    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_EXPORT_ODBPP::setupControls()
{
    m_choiceUnits->Bind( wxEVT_CHOICE,
                         [this]( wxCommandEvent& )
                         {
                             updatePrecisionRange();
                         } );
    m_contentPanel->Configure( FAB_CONTENT_FORMAT::ODBPP, m_parent->GetBoard() );
    m_contentPanel->SetContentChanged(
            [this]
            {
                updateVariantFilename();
                refreshLayerRows();
            } );

    m_layers->PushEventHandler( new GRID_TRICKS( m_layers ) );

    wxGridCellAttr* includeAttr = new wxGridCellAttr;
    includeAttr->SetRenderer( new wxGridCellBoolRenderer );
    includeAttr->SetEditor( new wxGridCellBoolEditor );
    includeAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_layers->SetColAttr( COL_INCLUDE, includeAttr );

    for( int col : { COL_LAYER, COL_CONTEXT, COL_SPAN } )
    {
        wxGridCellAttr* readOnlyAttr = new wxGridCellAttr;
        readOnlyAttr->SetReadOnly();
        m_layers->SetColAttr( col, readOnlyAttr );
    }
}


DIALOG_EXPORT_ODBPP::~DIALOG_EXPORT_ODBPP()
{
    m_layers->PopEventHandler( true );
}


void DIALOG_EXPORT_ODBPP::updatePrecisionRange()
{
    m_precision->SetRange( PCB_IO_ODBPP::MinPrecision( m_choiceUnits->GetSelection() == 1 ),
                           PCB_IO_ODBPP::MaxPrecision() );
}


bool DIALOG_EXPORT_ODBPP::TransferDataToWindow()
{
    JOB_EXPORT_PCB_ODB        defaults;
    const JOB_EXPORT_PCB_ODB& settings = m_job ? *m_job : defaults;

    m_choiceUnits->SetSelection( settings.m_units == JOB_EXPORT_PCB_FAB::UNITS::MM ? 0 : 1 );
    updatePrecisionRange();
    m_precision->SetValue( settings.m_precision );
    m_choiceCompress->SetSelection( static_cast<int>( settings.m_compressionMode ) );
    m_choiceOrigin->SetSelection( static_cast<int>( settings.m_origin ) );
    m_productName->SetValue( settings.m_productName );
    m_refillZones->SetValue( m_job ? settings.m_checkZonesBeforeExport : true );
    m_contentPanel->SetDataSet( fromDataSet( settings.m_dataSet ) );
    m_contentPanel->SetSectionKey( settings.m_sections.IsEmpty() ? std::optional<wxString>()
                                                                 : std::optional<wxString>( settings.m_sections ) );
    m_contentPanel->SetNetNamePolicy( settings.m_netNamePolicy );
    m_contentPanel->SetVariantNames( settings.m_variantNames );
    m_contentPanel->SetCombinedVariantOutput( settings.m_variantPackaging
                                              == JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING::COMBINED );
    IPC2581_BOM_FIELDS fields;
    fields.m_mfgPn = settings.m_colMfgPn;
    m_contentPanel->SetBomFields( fields );
    m_layerOverrides = settings.m_layerOverrides;

    if( !m_job )
    {
        if( m_outputFileName->GetValue().IsEmpty() )
        {
            wxFileName brdFile( m_parent->GetBoard()->GetFileName() );
            wxFileName odbFile( brdFile.GetPath(), wxString::Format( wxS( "%s-odb" ), brdFile.GetName() ),
                                FILEEXT::ArchiveFileExtension );

            m_outputFileName->SetValue( odbFile.GetFullPath() );
            OnFmtChoiceOptionChanged();
        }
    }
    else
    {
        SetTitle( m_job->GetSettingsDialogTitle() );
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    }

    updateVariantFilename();
    refreshLayerRows();
    return true;
}


void DIALOG_EXPORT_ODBPP::populateJob( JOB_EXPORT_PCB_ODB& aJob ) const
{
    aJob.SetConfiguredOutputPath( m_outputFileName->GetValue() );

    if( &aJob != m_job )
        aJob.m_filename = m_parent->GetBoard()->GetFileName();

    aJob.m_precision = m_precision->GetValue();
    aJob.m_units = m_choiceUnits->GetSelection() == 0 ? JOB_EXPORT_PCB_FAB::UNITS::MM : JOB_EXPORT_PCB_FAB::UNITS::INCH;
    aJob.m_compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( m_choiceCompress->GetSelection() );
    aJob.m_origin = static_cast<JOB_EXPORT_PCB_ODB::ORIGIN>( m_choiceOrigin->GetSelection() );
    aJob.m_productName = m_productName->GetValue();
    aJob.m_checkZonesBeforeExport = m_refillZones->GetValue();
    aJob.m_dataSet = toDataSet( m_contentPanel->GetDataSet() );
    aJob.m_sections = m_contentPanel->GetSectionKey().value_or( wxEmptyString );
    aJob.m_netNamePolicy = m_contentPanel->GetNetNamePolicy();
    aJob.m_variantNames = m_contentPanel->GetVariantNames();
    aJob.m_variantPackaging = m_contentPanel->IsCombinedVariantOutput()
                                      ? JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING::COMBINED
                                      : JOB_EXPORT_PCB_ODB::VARIANT_PACKAGING::SEPARATE;
    aJob.m_colMfgPn = m_contentPanel->GetBomFields().m_mfgPn;
    aJob.m_layerOverrides = m_layerOverrides;
}


void DIALOG_EXPORT_ODBPP::refreshLayerRows()
{
    JOB_EXPORT_PCB_ODB previewJob;
    populateJob( previewJob );
    previewJob.m_layerOverrides.clear();
    harvestLayerOverrides();
    m_previewRows = PreviewOdbMatrix( m_parent->GetBoard(), previewJob );
    m_layers->ClearRows();
    m_layers->AppendRows( static_cast<int>( m_previewRows.size() ) );
    int gridRow = 0;

    for( const ODB_MATRIX_PREVIEW_ROW& row : m_previewRows )
    {
        auto it = std::find_if( m_layerOverrides.begin(), m_layerOverrides.end(),
                                [&]( const ODB_LAYER_OVERRIDE& aCandidate )
                                {
                                    return row.m_editable && aCandidate.m_layer == row.m_boardLayer;
                                } );
        const ODB_LAYER_OVERRIDE* override = it == m_layerOverrides.end() ? nullptr : &*it;

        wxString name = override && !override->m_odbName.IsEmpty() ? override->m_odbName : row.m_matrix.m_name;
        wxString type = override && !override->m_odbType.IsEmpty() ? override->m_odbType : row.m_matrix.m_type;
        wxString span = row.m_matrix.m_startName;

        if( !row.m_matrix.m_endName.IsEmpty() )
            span += wxS( " – " ) + row.m_matrix.m_endName;

        // The preview omits overrides, and the exporter only references copper from mask, paste and silk
        bool keepsRef = type == wxS( "SOLDER_MASK" ) || type == wxS( "SOLDER_PASTE" ) || type == wxS( "SILK_SCREEN" );

        if( !row.m_matrix.m_ref.IsEmpty() && keepsRef )
        {
            span += wxS( " REF " ) + OdbPreviewReferenceName( row.m_matrix.m_ref, m_previewRows );
        }

        bool include = !override || override->m_include;
        m_layers->SetCellValue( gridRow, COL_INCLUDE, include ? wxString( wxS( "1" ) ) : wxString() );
        m_layers->SetCellValue( gridRow, COL_LAYER, row.m_displayLayer );
        m_layers->SetCellValue( gridRow, COL_NAME, name );
        m_layers->SetCellValue( gridRow, COL_TYPE, type );
        m_layers->SetCellValue( gridRow, COL_CONTEXT, row.m_matrix.m_context );
        m_layers->SetCellValue( gridRow, COL_SPAN, span );

        if( row.m_editable )
        {
            m_layers->SetCellEditor( gridRow, COL_TYPE,
                                     new wxGridCellChoiceEditor( overridableTypeNames( row.m_boardLayer ) ) );
        }
        else
        {
            for( int col : { COL_INCLUDE, COL_NAME, COL_TYPE } )
                m_layers->SetReadOnly( gridRow, col );
        }

        ++gridRow;
    }
}


void DIALOG_EXPORT_ODBPP::harvestLayerOverrides()
{
    m_layers->CommitPendingChanges( true );

    for( size_t index = 0; index < m_previewRows.size() && index < static_cast<size_t>( m_layers->GetNumberRows() );
         ++index )
    {
        const ODB_MATRIX_PREVIEW_ROW& row = m_previewRows[index];

        if( !row.m_editable )
            continue;

        int      gridRow = static_cast<int>( index );
        wxString name = m_layers->GetCellValue( gridRow, COL_NAME );
        wxString type = m_layers->GetCellValue( gridRow, COL_TYPE );

        // Pasted text bypasses the choice editor
        if( overridableTypeNames( row.m_boardLayer ).Index( type ) == wxNOT_FOUND )
            type = row.m_matrix.m_type;

        ODB_LAYER_OVERRIDE override;
        override.m_layer = row.m_boardLayer;
        override.m_include = !m_layers->GetCellValue( gridRow, COL_INCLUDE ).IsEmpty();
        override.m_odbName = name == row.m_matrix.m_name ? wxString() : name;
        override.m_odbType = type == row.m_matrix.m_type ? wxString() : type;

        std::erase_if( m_layerOverrides,
                       [&]( const ODB_LAYER_OVERRIDE& aCandidate )
                       {
                           return aCandidate.m_layer == row.m_boardLayer;
                       } );

        if( !override.m_include || !override.m_odbName.IsEmpty() || !override.m_odbType.IsEmpty() )
            m_layerOverrides.push_back( override );
    }
}


void DIALOG_EXPORT_ODBPP::updateVariantFilename()
{
    bool separate = m_contentPanel->GetVariantNames().size() > 1 && !m_contentPanel->IsCombinedVariantOutput();
    bool directory = m_choiceCompress->GetSelection() == static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE );
    wxString path =
            UpdateOdbVariantOutputPath( m_outputFileName->GetValue(), directory, separate, m_autoVariantSuffix );
    m_autoVariantSuffix = separate;

    if( !path.IsEmpty() )
        m_outputFileName->SetValue( path );
}


void DIALOG_EXPORT_ODBPP::onBrowseClicked( wxCommandEvent& event )
{
    // clang-format off
    wxString filter = _( "zip files" )
                      + AddFileExtListToFilter( { FILEEXT::ArchiveFileExtension } ) + "|"
                      + _( "tgz files" )
                      + AddFileExtListToFilter( { "tgz" } );
    // clang-format on

    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString   path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    wxFileName fn( Prj().AbsolutePath( path ) );

    wxFileName brdFile( m_parent->GetBoard()->GetFileName() );

    wxString fileDialogName( wxString::Format( wxS( "%s-odb" ), brdFile.GetName() ) );

    wxFileDialog dlg( this, _( "Export ODB++ File" ), fn.GetPath(), fileDialogName, filter, wxFD_SAVE );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    path = dlg.GetPath();

    fn = wxFileName( path );

    if( fn.GetExt().Lower() == "zip" )
    {
        m_choiceCompress->SetSelection( static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP ) );
    }
    else if( fn.GetExt().Lower() == "tgz" )
    {
        m_choiceCompress->SetSelection( static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ ) );
    }
    else if( path.EndsWith( "/" ) || path.EndsWith( "\\" ) )
    {
        m_choiceCompress->SetSelection( static_cast<int>( JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE ) );
    }
    else
    {
        DisplayErrorMessage( this, _( "The selected output file name is not a supported archive format." ) );
        return;
    }

    m_outputFileName->SetValue( path );
}


void DIALOG_EXPORT_ODBPP::onFormatChoice( wxCommandEvent& event )
{
    OnFmtChoiceOptionChanged();
}


void DIALOG_EXPORT_ODBPP::OnFmtChoiceOptionChanged()
{
    wxString fn = m_outputFileName->GetValue();

    wxFileName fileName( fn );

    auto compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( m_choiceCompress->GetSelection() );

    int sepIdx = std::max( fn.Find( '/', true ), fn.Find( '\\', true ) );
    int dotIdx = fn.Find( '.', true );

    if( fileName.IsDir() )
        fn = fn.Mid( 0, sepIdx );
    else if( sepIdx < dotIdx )
        fn = fn.Mid( 0, dotIdx );

    switch( compressionMode )
    {
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP:
        fn = fn + '.' + FILEEXT::ArchiveFileExtension;
        break;
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ:
        fn += ".tgz";
        break;
    case JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE:
        fn = wxFileName( fn, "" ).GetFullPath();
        break;
    default:
        break;
    };

    m_outputFileName->SetValue( fn );
    updateVariantFilename();
}


void DIALOG_EXPORT_ODBPP::onOKClick( wxCommandEvent& event )
{
    if( !m_job )
    {
        wxString fn = m_outputFileName->GetValue();

        if( fn.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "Output file name cannot be empty." ) );
            return;
        }

        auto compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( m_choiceCompress->GetSelection() );

        wxFileName fileName( fn );
        bool       isDirectory = fileName.IsDir();
        wxString   extension = fileName.GetExt();

        if( ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE && !isDirectory )
            || ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::ZIP && extension != "zip" )
            || ( compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ && extension != "tgz" ) )
        {
            DisplayErrorMessage( this, _( "The output file name conflicts with the selected compression format." ) );
            return;
        }
    }

    if( m_job )
    {
        event.Skip();
        return;
    }

    harvestLayerOverrides();
    JOB_EXPORT_PCB_ODB job;
    populateJob( job );
    m_messagesPanel->Clear();
    REPORTER& reporter = m_messagesPanel->Reporter();

    if( job.m_checkZonesBeforeExport )
        m_parent->GetToolManager()->GetTool<ZONE_FILLER_TOOL>()->CheckAllZones( this );

    WX_PROGRESS_REPORTER progress( this, _( "Generate ODB++ Files" ), 3, PR_CAN_ABORT );
    ODB_EXPORT_RESULT    result = GenerateODBPPFiles( job, m_parent->GetBoard(), m_parent, &progress, &reporter );

    for( const wxString& output : result.m_outputs )
        reporter.Report( wxString::Format( _( "Generated %s" ), output ), RPT_SEVERITY_ACTION );
}


bool DIALOG_EXPORT_ODBPP::TransferDataFromWindow()
{
    harvestLayerOverrides();

    if( m_job )
        populateJob( *m_job );

    return true;
}
