/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Mike Williams
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

#include "dialog_fields_table.h"
#include "fields_view_controls_grid_data_model.h"

#include <algorithm>
#include <functional>
#include <string>
#include <bitmaps.h>
#include <common.h>
#include <confirm.h>
#include <core/kicad_algo.h>
#include <eda_base_frame.h>
#include <eda_list_dialog.h>
#include <fields_table_data_model.h>
#include <grid_tricks.h>
#include <jobs/job_export_bom.h>
#include <kiplatform/ui.h>
#include <reporter.h>
#include <settings/app_settings.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <widgets/grid_checkbox.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/ui_common.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#ifdef __WXMAC__
#define COLUMN_MARGIN 4
#else
#define COLUMN_MARGIN 15
#endif


namespace
{
class VIEW_CONTROLS_GRID_TRICKS : public GRID_TRICKS
{
public:
    VIEW_CONTROLS_GRID_TRICKS( WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid )
    {
    }

protected:
    void doPopupSelection( wxCommandEvent& aEvent ) override
    {
        if( aEvent.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
            m_grid->PostSizeEvent();

        GRID_TRICKS::doPopupSelection( aEvent );
    }
};
} // namespace


DIALOG_FIELDS_TABLE::DIALOG_FIELDS_TABLE( wxWindow* aParent, FIELDS_TABLE_SETTINGS& aPanelSettings,
                                          FIELDS_TABLE_BOM_SETTINGS& aBomSettings,
                                          JOB_EXPORT_BOM* aJob ) :
        DIALOG_FIELDS_TABLE_BASE( aParent ),
        m_cfgDialogSettings( aPanelSettings ),
        m_cfgBomSettings( aBomSettings ),
        m_job( aJob )
{
    m_bRefresh->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_bMenu->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_bRefreshPreview->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    m_addFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_renameFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );

    m_addVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_renameVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_copyVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::copy ) );
    m_editVariantDescButton->SetBitmap( KiBitmapBundle( BITMAPS::text ) );

    m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );

    // Do not OptOut the notebook. That would also exclude its child controls such as the
    // scope selector from being persisted. The active page is forced by the opening tool.

    m_viewControlsDataModel = new VIEW_CONTROLS_GRID_DATA_MODEL( true );

    m_viewControlsGrid->UseNativeColHeader( true );
    m_viewControlsGrid->SetTable( m_viewControlsDataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_viewControlsGrid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_viewControlsGrid->PushEventHandler( new VIEW_CONTROLS_GRID_TRICKS( m_viewControlsGrid ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_viewControlsDataModel->SetColAttr( attr, DISPLAY_NAME_COLUMN );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, SHOW_FIELD_COLUMN );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, GROUP_BY_COLUMN );

    // Compress the view controls grid.  (We want it to look different from the fields grid.)
    m_viewControlsGrid->SetDefaultRowSize( m_viewControlsGrid->GetDefaultRowSize() - FromDIP( 4 ) );

    m_filter->SetDescriptiveText( _( "Filter" ) );

    m_grid->GetGridWindow()->Bind( wxEVT_MOUSEWHEEL, &DIALOG_FIELDS_TABLE::OnGridMouseWheel, this );
}


DIALOG_FIELDS_TABLE::~DIALOG_FIELDS_TABLE()
{
    m_grid->GetGridWindow()->Unbind( wxEVT_MOUSEWHEEL, &DIALOG_FIELDS_TABLE::OnGridMouseWheel, this );
    m_viewControlsGrid->PopEventHandler( true );
}


void DIALOG_FIELDS_TABLE::ShowEditTab()
{
    m_nbPages->SetSelection( 0 );
}


void DIALOG_FIELDS_TABLE::ShowExportTab()
{
    m_nbPages->SetSelection( 1 );
}


void DIALOG_FIELDS_TABLE::ShowHideColumn( int aCol, bool aShow )
{
    if( aShow )
        m_grid->ShowCol( aCol );
    else
        m_grid->HideCol( aCol );

    getDataModel()->SetShowColumn( aCol, aShow );

    if( getDataModel()->GetFilterScope() == BOM_FILTER_SCOPE::VISIBLE
        && !getDataModel()->GetFilter().IsEmpty() )
    {
        getDataModel()->RebuildRows();
    }

    syncBomPresetSelection();

    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
    else
        m_grid->ForceRefresh();

    OnModify();
}


void DIALOG_FIELDS_TABLE::AddField( const wxString& aFieldName, const wxString& aLabelValue, bool aShow, bool aGroupBy,
                                    bool aAddedByUser )
{
    // Users can add fields with variable names that match the special names in the grid,
    // e.g. ${QUANTITY} so make sure we don't add them twice
    for( int row = 0; row < m_viewControlsDataModel->GetNumberRows(); row++ )
    {
        if( FieldNamesAreDuplicates( m_viewControlsDataModel->GetCanonicalFieldName( row ), aFieldName ) )
        {
            return;
        }
    }

    getDataModel()->AddColumn( aFieldName, aLabelValue, aAddedByUser );

    wxGridTableMessage msg( getDataModel(), wxGRIDTABLE_NOTIFY_COLS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_viewControlsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_viewControlsDataModel->AppendRow( aFieldName, aLabelValue, aShow, aGroupBy );

                return { m_viewControlsDataModel->GetNumberRows() - 1, -1 };
            } );
}


wxString DIALOG_FIELDS_TABLE::GetDefaultBomFileName( const wxString& aInputFileName )
{
    if( aInputFileName.IsEmpty() )
        return wxEmptyString;

    wxFileName fn( aInputFileName );
    fn.SetExt( FILEEXT::CsvFileExtension );

    return fn.GetFullPath();
}


void DIALOG_FIELDS_TABLE::loadJobBomPreset( const JOB_EXPORT_BOM& aJob, BOM_PRESET& aPreset )
{
    aPreset.name = aJob.m_bomPresetName;
    aPreset.excludeDNP = aJob.m_excludeDNP;
    aPreset.filterString = aJob.m_filterString;
    aPreset.filterScope = aJob.m_filterScope;
    aPreset.sortAsc = aJob.m_sortAsc;
    aPreset.sortField = aJob.m_sortField;
    aPreset.groupSymbols = aJob.m_groupSymbols;

    aPreset.fieldsOrdered.clear();

    size_t i = 0;

    for( const wxString& fieldName : aJob.m_fieldsOrdered )
    {
        BOM_FIELD field;
        field.name = fieldName;
        field.show = !fieldName.StartsWith( wxT( "__" ), &field.name );
        field.groupBy = alg::contains( aJob.m_fieldsGroupBy, field.name );

        if( ( aJob.m_fieldsLabels.size() > i ) && !aJob.m_fieldsLabels[i].IsEmpty() )
            field.label = aJob.m_fieldsLabels[i];
        else if( IsGeneratedField( field.name ) )
            field.label = GetGeneratedFieldDisplayName( field.name );
        else
            field.label = field.name;

        aPreset.fieldsOrdered.emplace_back( field );
        i++;
    }
}


void DIALOG_FIELDS_TABLE::loadJobBomFmtPreset( const JOB_EXPORT_BOM& aJob, BOM_FMT_PRESET& aPreset )
{
    aPreset.name = aJob.m_bomFmtPresetName;
    aPreset.fieldDelimiter = aJob.m_fieldDelimiter;
    aPreset.keepLineBreaks = aJob.m_keepLineBreaks;
    aPreset.keepTabs = aJob.m_keepTabs;
    aPreset.includeByteOrderMark = aJob.m_includeByteOrderMark;
    aPreset.refDelimiter = aJob.m_refDelimiter;
    aPreset.refRangeDelimiter = aJob.m_refRangeDelimiter;
    aPreset.stringDelimiter = aJob.m_stringDelimiter;
}


void DIALOG_FIELDS_TABLE::saveJobSettings( JOB_EXPORT_BOM& aJob )
{
    aJob.SetConfiguredOutputPath( m_outputFileName->GetValue() );
    aJob.m_bomFmtPresetName = wxEmptyString;
    aJob.m_bomPresetName = wxEmptyString;

    BOM_FMT_PRESET fmtSettings = GetCurrentBomFmtSettings();
    aJob.m_fieldDelimiter = fmtSettings.fieldDelimiter;
    aJob.m_stringDelimiter = fmtSettings.stringDelimiter;
    aJob.m_refDelimiter = fmtSettings.refDelimiter;
    aJob.m_refRangeDelimiter = fmtSettings.refRangeDelimiter;
    aJob.m_keepTabs = fmtSettings.keepTabs;
    aJob.m_keepLineBreaks = fmtSettings.keepLineBreaks;
    aJob.m_includeByteOrderMark = fmtSettings.includeByteOrderMark;

    BOM_PRESET presetFields = getDataModel()->GetBomSettings();
    aJob.m_sortAsc = presetFields.sortAsc;
    aJob.m_excludeDNP = presetFields.excludeDNP;
    aJob.m_filterString = presetFields.filterString;
    aJob.m_filterScope = presetFields.filterScope;
    aJob.m_sortField = presetFields.sortField;
    aJob.m_groupSymbols = presetFields.groupSymbols;

    aJob.m_fieldsOrdered.clear();
    aJob.m_fieldsLabels.clear();
    aJob.m_fieldsGroupBy.clear();

    for( const BOM_FIELD& modelField : getDataModel()->GetFieldsOrdered() )
    {
        if( modelField.show )
            aJob.m_fieldsOrdered.emplace_back( modelField.name );
        else
            aJob.m_fieldsOrdered.emplace_back( wxT( "__" ) + modelField.name );

        aJob.m_fieldsLabels.emplace_back( modelField.label );

        if( modelField.groupBy )
            aJob.m_fieldsGroupBy.emplace_back( modelField.name );
    }

    aJob.SetSelectedVariant( getSelectedVariant() );
}


bool DIALOG_FIELDS_TABLE::savePresets( bool aSaveCurrentSettings )
{
    bool modified = false;

    std::vector<BOM_PRESET> presets = GetUserBomPresets();

    if( m_cfgBomSettings.m_BomPresets != presets )
    {
        modified = true;
        m_cfgBomSettings.m_BomPresets = presets;
    }

    BOM_PRESET bomSettings = getDataModel()->GetBomSettings();

    if( aSaveCurrentSettings && m_cfgBomSettings.m_BomSettings != bomSettings )
    {
        modified = true;
        m_cfgBomSettings.m_BomSettings = bomSettings;
    }

    std::vector<BOM_FMT_PRESET> fmts = GetUserBomFmtPresets();

    if( m_cfgBomSettings.m_BomFmtPresets != fmts )
    {
        modified = true;
        m_cfgBomSettings.m_BomFmtPresets = fmts;
    }

    BOM_FMT_PRESET fmtSettings = GetCurrentBomFmtSettings();

    if( aSaveCurrentSettings && m_cfgBomSettings.m_BomFmtSettings != fmtSettings )
    {
        modified = true;
        m_cfgBomSettings.m_BomFmtSettings = fmtSettings;
    }

    return modified;
}


wxSize DIALOG_FIELDS_TABLE::GetDefaultDialogSize() const
{
    return ConvertDialogToPixels( wxSize( 600, 300 ) );
}


void DIALOG_FIELDS_TABLE::EnableSelectionEvents()
{
    m_grid->Connect( wxEVT_GRID_RANGE_SELECTED,
                     wxGridRangeSelectEventHandler( DIALOG_FIELDS_TABLE::OnTableRangeSelected ), nullptr, this );
}


void DIALOG_FIELDS_TABLE::DisableSelectionEvents()
{
    m_grid->Disconnect( wxEVT_GRID_RANGE_SELECTED,
                        wxGridRangeSelectEventHandler( DIALOG_FIELDS_TABLE::OnTableRangeSelected ), nullptr, this );
}


void DIALOG_FIELDS_TABLE::RestorePanelLayout()
{
    bool sidebarCollapsed = m_cfgDialogSettings.sidebar_collapsed;
    int  sashPosition = m_cfgDialogSettings.sash_pos;
    int  variantSashPosition = m_cfgDialogSettings.variant_sash_pos;

    m_viewControlsGrid->ShowHideColumns( "0 1 2 3" );

    CallAfter(
            [this, sidebarCollapsed, sashPosition, variantSashPosition]()
            {
                if( sidebarCollapsed )
                    m_splitterMainWindow->Unsplit( m_leftPanel );
                else
                    m_splitterMainWindow->SetSashPosition( sashPosition );

                setSideBarButtonLook( sidebarCollapsed );

                m_splitter_left->SetSashPosition( variantSashPosition );
            } );
}


void DIALOG_FIELDS_TABLE::SavePanelLayout()
{
    if( !m_cfgDialogSettings.sidebar_collapsed )
        m_cfgDialogSettings.sash_pos = m_splitterMainWindow->GetSashPosition();

    m_cfgDialogSettings.variant_sash_pos = m_splitter_left->GetSashPosition();
}


void DIALOG_FIELDS_TABLE::SaveColumnWidths()
{
    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( getDataModel()->GetColFieldName( i ).ToUTF8() );
            m_cfgDialogSettings.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }
}


wxGridCellEditor* DIALOG_FIELDS_TABLE::createFootprintEditor()
{
    return new GRID_CELL_FPID_EDITOR( this, wxEmptyString );
}


void DIALOG_FIELDS_TABLE::onBomSettingsChanged()
{
    m_parentFrame->OnModify();
}


void DIALOG_FIELDS_TABLE::SetupColumnProperties( int aCol )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( false );

    // Set some column types to specific editors
    if( getDataModel()->ColIsItemIdentifier( aCol ) )
    {
        attr->SetReadOnly();
        attr->SetRenderer( new GRID_CELL_TEXT_RENDERER() );
        getDataModel()->SetColAttr( attr, aCol );
    }
    else if( getDataModel()->GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
    {
        attr->SetEditor( createFootprintEditor() );
        getDataModel()->SetColAttr( attr, aCol );
    }
    else if( getDataModel()->GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
    {
        // set datasheet column viewer button
        attr->SetEditor( createDatasheetEditor() );
        getDataModel()->SetColAttr( attr, aCol );
    }
    else if( getDataModel()->ColIsQuantity( aCol ) || getDataModel()->ColIsItemNumber( aCol ) )
    {
        attr->SetReadOnly();
        attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );
        attr->SetRenderer( new wxGridCellNumberRenderer() );
        getDataModel()->SetColAttr( attr, aCol );
    }
    else if( getDataModel()->ColIsCheck( aCol ) )
    {
        attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
        attr->SetRenderer( new GRID_CELL_CHECKBOX_RENDERER() );
        attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
        getDataModel()->SetColAttr( attr, aCol );
    }
    else if( IsGeneratedField( getDataModel()->GetColFieldName( aCol ) ) )
    {
        attr->SetReadOnly();
        getDataModel()->SetColAttr( attr, aCol );
    }
    else
    {
        attr->SetRenderer( new GRID_CELL_TEXT_RENDERER() );
        attr->SetEditor( m_grid->GetDefaultEditor() );
        getDataModel()->SetColAttr( attr, aCol );
    }
}


void DIALOG_FIELDS_TABLE::SetupAllColumnProperties()
{
    wxSize defaultDlgSize = GetDefaultDialogSize();

    // Restore column sorting order and widths
    m_grid->AutoSizeColumns( false );
    int  sortCol = 0;
    bool sortAscending = true;

    for( int col = 0; col < m_grid->GetNumberCols(); ++col )
    {
        SetupColumnProperties( col );

        if( col == getDataModel()->GetSortCol() )
        {
            sortCol = col;
            sortAscending = getDataModel()->GetSortAsc();
        }
    }

    // sync m_grid's column visibilities to Show checkboxes in m_viewControlsGrid
    for( int i = 0; i < m_viewControlsDataModel->GetNumberRows(); ++i )
    {
        int col = getDataModel()->GetFieldNameCol( m_viewControlsDataModel->GetCanonicalFieldName( i ) );

        if( col == -1 )
            continue;

        bool show = m_viewControlsDataModel->GetValueAsBool( i, SHOW_FIELD_COLUMN );
        getDataModel()->SetShowColumn( col, show );

        if( show )
        {
            m_grid->ShowCol( col );

            std::string key( getDataModel()->GetColFieldName( col ).ToUTF8() );

            if( m_cfgDialogSettings.field_widths.count( key ) && ( m_cfgDialogSettings.field_widths.at( key ) > 0 ) )
            {
                m_grid->SetColSize( col, m_cfgDialogSettings.field_widths.at( key ) );
            }
            else
            {
                int textWidth = getDataModel()->GetColDataWidth( col ) + COLUMN_MARGIN;
                int maxWidth = defaultDlgSize.x / 3;

                m_grid->SetColSize( col, std::clamp( textWidth, 100, maxWidth ) );
            }
        }
        else
        {
            m_grid->HideCol( col );
        }
    }

    getDataModel()->SetSorting( sortCol, sortAscending );
    m_grid->SetSortingColumn( sortCol, sortAscending );
}


void DIALOG_FIELDS_TABLE::setSideBarButtonLook( bool aIsLeftPanelCollapsed )
{
    // Set bitmap and tooltip according to left panel visibility

    if( aIsLeftPanelCollapsed )
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::right ) );
        m_sidebarButton->SetToolTip( _( "Expand left panel" ) );
    }
    else
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );
        m_sidebarButton->SetToolTip( _( "Collapse left panel" ) );
    }
}


void DIALOG_FIELDS_TABLE::OnSidebarToggle( wxCommandEvent& event )
{
    if( m_cfgDialogSettings.sidebar_collapsed )
    {
        m_cfgDialogSettings.sidebar_collapsed = false;
        m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, m_cfgDialogSettings.sash_pos );
    }
    else
    {
        m_cfgDialogSettings.sash_pos = m_splitterMainWindow->GetSashPosition();

        m_cfgDialogSettings.sidebar_collapsed = true;
        m_splitterMainWindow->Unsplit( m_leftPanel );
    }

    setSideBarButtonLook( m_cfgDialogSettings.sidebar_collapsed );
}


void DIALOG_FIELDS_TABLE::OnTableCellClick( wxGridEvent& event )
{
    if( getDataModel()->IsExpanderColumn( event.GetCol() ) )
    {
        m_grid->ClearSelection();

        getDataModel()->ExpandCollapseRow( event.GetRow() );
        m_grid->SetGridCursor( event.GetRow(), event.GetCol() );
    }
    else
    {
        event.Skip();
    }
}


void DIALOG_FIELDS_TABLE::OnTableValueChanged( wxGridEvent& aEvent )
{
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::OnTableColSize( wxGridSizeEvent& aEvent )
{
    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight(); // Presume buttons are square

    // TODO: restore cursor when mouse leaves the filter field (or is it a MSW bug?)
    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


void DIALOG_FIELDS_TABLE::OnSizeViewControlsGrid( wxSizeEvent& event )
{
    const wxString& showColLabel = m_viewControlsGrid->GetColLabelValue( SHOW_FIELD_COLUMN );
    const wxString& groupByColLabel = m_viewControlsGrid->GetColLabelValue( GROUP_BY_COLUMN );
    int             showColWidth = KIUI::GetTextSize( showColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             groupByColWidth = KIUI::GetTextSize( groupByColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             remainingWidth = m_viewControlsGrid->GetSize().GetX() - showColWidth - groupByColWidth;

    m_viewControlsGrid->SetColSize( showColWidth, SHOW_FIELD_COLUMN );
    m_viewControlsGrid->SetColSize( groupByColWidth, GROUP_BY_COLUMN );

    if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) && m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth / 2, 60 ) );
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth - ( remainingWidth / 2 ), 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth, 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth, 60 ) );
    }

    event.Skip();
}


void DIALOG_FIELDS_TABLE::OnViewControlsCellChanged( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    wxCHECK( row < m_viewControlsGrid->GetNumberRows(), /* void */ );

    switch( aEvent.GetCol() )
    {
    case LABEL_COLUMN:
    {
        wxString label = m_viewControlsDataModel->GetValue( row, LABEL_COLUMN );
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        int      dataCol = getDataModel()->GetFieldNameCol( fieldName );

        if( dataCol != -1 )
        {
            getDataModel()->SetColLabelValue( dataCol, label );
            m_grid->SetColLabelValue( dataCol, label );

            if( m_nbPages->GetSelection() == 1 )
                PreviewRefresh();
            else
                m_grid->ForceRefresh();

            syncBomPresetSelection();
            OnModify();
        }

        break;
    }

    case SHOW_FIELD_COLUMN:
    {
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        bool     value = m_viewControlsDataModel->GetValueAsBool( row, SHOW_FIELD_COLUMN );
        int      dataCol = getDataModel()->GetFieldNameCol( fieldName );

        if( dataCol != -1 )
            ShowHideColumn( dataCol, value );

        break;
    }

    case GROUP_BY_COLUMN:
    {
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        bool     value = m_viewControlsDataModel->GetValueAsBool( row, GROUP_BY_COLUMN );
        int      dataCol = getDataModel()->GetFieldNameCol( fieldName );

        if( getDataModel()->ColIsQuantity( dataCol ) && value )
        {
            DisplayError( this, _( "The Quantity column cannot be grouped by." ) );

            value = false;
            m_viewControlsDataModel->SetValueAsBool( row, GROUP_BY_COLUMN, value );
            m_viewControlsGrid->ForceRefresh();
            break;
        }

        if( getDataModel()->ColIsItemNumber( dataCol ) && value )
        {
            DisplayError( this, _( "The Item Number column cannot be grouped by." ) );

            value = false;
            m_viewControlsDataModel->SetValueAsBool( row, GROUP_BY_COLUMN, value );
            m_viewControlsGrid->ForceRefresh();
            break;
        }

        getDataModel()->SetGroupColumn( dataCol, value );
        getDataModel()->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();

        syncBomPresetSelection();
        OnModify();
        break;
    }

    default: break;
    }
}


void DIALOG_FIELDS_TABLE::OnAddField( wxCommandEvent& aEvent )
{
    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Add Field" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString fieldName = dlg.GetValue();

    if( fieldName.IsEmpty() )
    {
        DisplayError( this, _( "Field must have a name." ) );
        return;
    }

    for( int i = 0; i < getDataModel()->GetNumberCols(); ++i )
    {
        if( FieldNamesAreDuplicates( fieldName, getDataModel()->GetColFieldName( i ) ) )
        {
            DisplayError( this, wxString::Format( _( "Field name '%s' already in use." ), fieldName ) );
            return;
        }
    }

    AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false, true );

    SetupColumnProperties( getDataModel()->GetColsCount() - 1 );

    syncBomPresetSelection();
    OnModify();
}


void DIALOG_FIELDS_TABLE::OnRemoveField( wxCommandEvent& aEvent )
{
    m_viewControlsGrid->OnDeleteRows(
            [&]( int row )
            {
                for( FIELD_T id : MANDATORY_FIELDS )
                {
                    if( m_mandatoryFieldListIndexes[id] == row )
                    {
                        DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                              (int) m_mandatoryFieldListIndexes.size() ) );
                        return false;
                    }
                }

                return IsOK( this, wxString::Format( _( "Are you sure you want to remove the field '%s'?" ),
                                                     m_viewControlsDataModel->GetValue( row, DISPLAY_NAME_COLUMN ) ) );
            },
            [&]( int row )
            {
                wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
                int      col = getDataModel()->GetFieldNameCol( fieldName );

                if( col != -1 )
                    getDataModel()->RemoveColumn( col );

                m_viewControlsDataModel->DeleteRow( row );

                syncBomPresetSelection();
                OnModify();
            } );
}


void DIALOG_FIELDS_TABLE::OnRenameField( wxCommandEvent& aEvent )
{
    wxArrayInt selectedRows = m_viewControlsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_viewControlsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_viewControlsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    int row = selectedRows[0];

    for( FIELD_T id : MANDATORY_FIELDS )
    {
        if( m_mandatoryFieldListIndexes[id] == row )
        {
            DisplayError( this, wxString::Format( _( "The first %d fields are mandatory and names cannot be changed." ),
                                                  (int) m_mandatoryFieldListIndexes.size() ) );
            return;
        }
    }

    wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
    wxString label = m_viewControlsDataModel->GetValue( row, LABEL_COLUMN );
    bool     labelIsAutogenerated = label.IsSameAs( GetGeneratedFieldDisplayName( fieldName ) );

    int col = getDataModel()->GetFieldNameCol( fieldName );
    wxCHECK_RET( col != -1, wxS( "Existing field name missing from data model" ) );

    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Rename Field" ), fieldName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString newFieldName = dlg.GetValue();

    // No change, no-op
    if( newFieldName == fieldName )
        return;

    // New field name already exists
    if( getDataModel()->GetFieldNameCol( newFieldName ) != -1 )
    {
        wxString confirm_msg = wxString::Format( _( "Field name %s already exists." ), newFieldName );
        DisplayError( this, confirm_msg );
        return;
    }

    getDataModel()->RenameColumn( col, newFieldName );
    m_viewControlsDataModel->SetCanonicalFieldName( row, newFieldName );
    m_viewControlsDataModel->SetValue( row, DISPLAY_NAME_COLUMN, newFieldName );

    if( labelIsAutogenerated )
    {
        m_viewControlsDataModel->SetValue( row, LABEL_COLUMN, GetGeneratedFieldDisplayName( newFieldName ) );
        wxGridEvent evt( m_viewControlsGrid->GetId(), wxEVT_GRID_CELL_CHANGED, m_viewControlsGrid, row, LABEL_COLUMN );
        OnViewControlsCellChanged( evt );
    }

    syncBomPresetSelection();
    OnModify();
}


void DIALOG_FIELDS_TABLE::OnFilterText( wxCommandEvent& aEvent )
{
    getDataModel()->SetFilter( m_filter->GetValue() );
    getDataModel()->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_FIELDS_TABLE::OnFilterScope( wxCommandEvent& aEvent )
{
    getDataModel()->SetFilterScope( static_cast<BOM_FILTER_SCOPE>( m_filterScope->GetSelection() ) );
    getDataModel()->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_FIELDS_TABLE::OnGroupSymbolsToggled( wxCommandEvent& aEvent )
{
    getDataModel()->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );
    getDataModel()->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_FIELDS_TABLE::OnRegroupSymbols( wxCommandEvent& aEvent )
{
    getDataModel()->RebuildRows();
    m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::OnColSort( wxGridEvent& aEvent )
{
    int  sortCol = aEvent.GetCol();
    bool ascending;

    // Don't sort by item number, it is generated by the sort
    if( getDataModel()->ColIsItemNumber( sortCol ) )
    {
        aEvent.Veto();
        return;
    }

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the event, and
    // if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
    {
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    }
    else
    {
        // different column; start with ascending
        ascending = true;
    }

    getDataModel()->SetSorting( sortCol, ascending );
    getDataModel()->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_FIELDS_TABLE::OnColMove( wxGridEvent& aEvent )
{
    int origPos = aEvent.GetCol();

    // Save column widths since the setup function uses the saved config values
    SaveColumnWidths();

    CallAfter(
            [origPos, this]()
            {
                int newPos = m_grid->GetColPos( origPos );

#ifdef __WXMAC__
                if( newPos < origPos )
                    newPos += 1;
#endif

                getDataModel()->MoveColumn( origPos, newPos );

                // "Unmove" the column since we've moved the column internally
                m_grid->ResetColPos();

                // We need to reset all the column attr's to the correct column order
                SetupAllColumnProperties();

                m_grid->ForceRefresh();
            } );

    syncBomPresetSelection();
}


void DIALOG_FIELDS_TABLE::OnGridMouseMove( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    wxPoint pos = aEvent.GetPosition();
    int     ux, uy;
    m_grid->CalcUnscrolledPosition( pos.x, pos.y, &ux, &uy );
    int row = m_grid->YToRow( uy );
    int col = m_grid->XToCol( ux );

    if( row == wxNOT_FOUND || col == wxNOT_FOUND )
    {
        m_grid->GetGridWindow()->UnsetToolTip();
        return;
    }

    wxString rawValue = getDataModel()->GetValue( row, col );

    if( rawValue.Contains( wxT( "${" ) ) )
        m_grid->GetGridWindow()->SetToolTip( rawValue );
    else
        m_grid->GetGridWindow()->UnsetToolTip();
}


void DIALOG_FIELDS_TABLE::OnGridMouseWheel( wxMouseEvent& aEvent )
{
    if( aEvent.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL || aEvent.GetModifiers() != wxMOD_SHIFT )
    {
        aEvent.Skip();
        return;
    }

    // Accumulate so high-resolution wheels and trackpads aren't truncated.
    m_gridWheelRotation += aEvent.GetWheelRotation();

    int lines = m_gridWheelRotation / aEvent.GetWheelDelta();
    m_gridWheelRotation -= lines * aEvent.GetWheelDelta();

    if( lines == 0 )
        return;

    int scrollLines = aEvent.IsPageScroll() ? m_grid->GetScrollPageSize( wxHORIZONTAL )
                                            : aEvent.GetLinesPerAction();

    wxPoint viewStart = m_grid->GetViewStart();
    m_grid->Scroll( viewStart.x - lines * scrollLines, viewStart.y );
}


void DIALOG_FIELDS_TABLE::OnPageChanged( wxNotebookEvent& aEvent )
{
    if( getDataModel()->GetColsCount() )
        PreviewRefresh();
}


void DIALOG_FIELDS_TABLE::OnPreviewRefresh( wxCommandEvent& aEvent )
{
    PreviewRefresh();
    syncBomFmtPresetSelection();
}


void DIALOG_FIELDS_TABLE::OnOutputFileBrowseClicked( wxCommandEvent& aEvent )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );

    // Calculate the export filename
    wxFileName fn( Prj().AbsolutePath( m_parentFrame->GetCurrentFileName() ) );
    fn.SetExt( FILEEXT::CsvFileExtension );

    wxFileDialog saveDlg( this, _( "Bill of Materials Output File" ), path, fn.GetFullName(),
                          FILEEXT::CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &saveDlg );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName file = wxFileName( saveDlg.GetPath() );
    wxString   defaultPath = fn.GetPathWithSep();

    if( IsOK( this, wxString::Format( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath ) ) )
    {
        if( !file.MakeRelativeTo( defaultPath ) )
        {
            DisplayErrorMessage( this, _( "Cannot make path relative (target volume different from file "
                                          "volume)!" ) );
        }
    }

    m_outputFileName->SetValue( file.GetFullPath() );
}


void DIALOG_FIELDS_TABLE::OnExport( wxCommandEvent& aEvent )
{
    if( getDataModel()->IsEdited() )
    {
        if( OKOrCancelDialog( nullptr, _( "Unsaved data" ),
                              _( "Changes have not yet been saved. Export unsaved data?" ), "", _( "OK" ),
                              _( "Cancel" ) )
            == wxID_CANCEL )
        {
            return;
        }
    }

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver = [&]( wxString* token ) -> bool
    {
        return resolveTextVar( token );
    };

    wxString sourceFileName = m_parentFrame->GetCurrentFileName();
    wxString path = m_outputFileName->GetValue();

    if( path.IsEmpty() )
    {
        // Match the behaviour of other exporters and default to the source filename with a CSV
        // extension in the project directory when the user leaves the field blank.
        path = GetDefaultBomFileName( sourceFileName );

        if( path.IsEmpty() )
        {
            DisplayError( this, _( "No output file specified in Export tab." ) );
            return;
        }

        m_outputFileName->SetValue( path );
    }

    path = ExpandTextVars( NormalizeFilePathForTextVars( path ), &textResolver );
    path = ExpandEnvVarSubstitutions( path, &Prj() );

    wxFileName outputFile = wxFileName::FileName( path );
    wxString   msg;

    if( !EnsureFileDirectoryExists( &outputFile, Prj().AbsolutePath( sourceFileName ), &NULL_REPORTER::GetInstance() ) )
    {
        msg.Printf( _( "Could not open/create path '%s'." ), outputFile.GetPath() );
        DisplayError( this, msg );
        return;
    }

    wxFFile out( outputFile.GetFullPath(), "wb" );

    if( !out.IsOpened() )
    {
        msg.Printf( _( "Could not create BOM output '%s'." ), outputFile.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    PreviewRefresh();

    if( !out.Write( m_textOutput->GetValue() ) )
    {
        msg.Printf( _( "Could not write BOM output '%s'." ), outputFile.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    // close the file before we tell the user it's done with the info modal :workflow meme:
    out.Close();

    if( m_cfgBomSettings.m_BomExportFileName != m_outputFileName->GetValue() )
    {
        m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
        onBomSettingsChanged();
    }

    msg.Printf( _( "Wrote BOM output to '%s'" ), outputFile.GetFullPath() );
    DisplayInfoMessage( this, msg );
}


void DIALOG_FIELDS_TABLE::PreviewRefresh()
{
    bool saveIncludeExcudedFromBOM = getDataModel()->GetIncludeExcludedFromBOM();

    getDataModel()->SetIncludeExcludedFromBOM( false );
    getDataModel()->RebuildRows();

    m_textOutput->SetValue( getDataModel()->Export( GetCurrentBomFmtSettings() ) );

    if( saveIncludeExcudedFromBOM )
    {
        getDataModel()->SetIncludeExcludedFromBOM( true );
        getDataModel()->RebuildRows();
    }
}


wxString DIALOG_FIELDS_TABLE::getSelectedVariant() const
{
    wxString retv;

    int selection = m_variantListBox->GetSelection();

    if( ( selection == wxNOT_FOUND ) || ( m_variantListBox->GetString( selection ) == GetDefaultVariantName() ) )
        return retv;

    return m_variantListBox->GetString( selection );
}


std::vector<BOM_PRESET> DIALOG_FIELDS_TABLE::GetUserBomPresets() const
{
    std::vector<BOM_PRESET> ret;

    for( const std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

    return ret;
}


void DIALOG_FIELDS_TABLE::SetUserBomPresets( std::vector<BOM_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomPresets();

    for( const BOM_PRESET& preset : aPresetList )
    {
        if( m_bomPresets.count( preset.name ) )
            continue;

        m_bomPresets[preset.name] = preset;

        m_bomPresetMRU.Add( preset.name );
    }

    rebuildBomPresetsWidget();
}


void DIALOG_FIELDS_TABLE::ApplyBomPreset( const wxString& aPresetName )
{
    updateBomPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomPresetChanged( dummy );
}


void DIALOG_FIELDS_TABLE::ApplyBomPreset( const BOM_PRESET& aPreset )
{
    if( m_bomPresets.count( aPreset.name ) )
        m_currentBomPreset = &m_bomPresets[aPreset.name];
    else
        m_currentBomPreset = nullptr;

    if( m_currentBomPreset && !m_currentBomPreset->readOnly )
        m_lastSelectedBomPreset = m_currentBomPreset;
    else
        m_lastSelectedBomPreset = nullptr;

    updateBomPresetSelection( aPreset.name );
    doApplyBomPreset( aPreset );
}


void DIALOG_FIELDS_TABLE::doApplyBomPreset( const BOM_PRESET& aPreset )
{
    // Disable rebuilds while we're applying the preset otherwise we'll be
    // rebuilding the model constantly while firing off wx events
    getDataModel()->DisableRebuilds();

    // Basically, we apply the BOM preset to the data model and then
    // update our UI to reflect resulting the data model state, not the preset.
    getDataModel()->SetCurrentVariant( resolveVariant() );
    getDataModel()->ApplyBomPreset( aPreset );

    // BOM Presets can add, but not remove, columns, so make sure the view controls
    // grid has all of them before starting
    for( int i = 0; i < getDataModel()->GetColsCount(); i++ )
    {
        const wxString& fieldName( getDataModel()->GetColFieldName( i ) );
        bool            found = false;

        for( int j = 0; j < m_viewControlsDataModel->GetNumberRows(); j++ )
        {
            if( m_viewControlsDataModel->GetCanonicalFieldName( j ) == fieldName )
            {
                found = true;
                break;
            }
        }

        // Properties like label, etc. will be added in the next loop
        if( !found )
            AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), false, false );
    }

    // Sync all fields
    for( int i = 0; i < m_viewControlsDataModel->GetNumberRows(); i++ )
    {
        const wxString& fieldName( m_viewControlsDataModel->GetCanonicalFieldName( i ) );
        int             col = getDataModel()->GetFieldNameCol( fieldName );

        if( col == -1 )
        {
            wxASSERT_MSG( true, "Fields control has a field not found in the data model." );
            continue;
        }

        std::string fieldNameStr( fieldName.ToUTF8() );

        // Set column labels
        const wxString& label = getDataModel()->GetColLabelValue( col );
        m_viewControlsDataModel->SetValue( i, LABEL_COLUMN, label );
        m_grid->SetColLabelValue( col, label );

        if( m_cfgDialogSettings.field_widths.count( fieldNameStr ) )
            m_grid->SetColSize( col, m_cfgDialogSettings.field_widths.at( fieldNameStr ) );

        // Set shown columns
        bool show = getDataModel()->GetShowColumn( col );
        m_viewControlsDataModel->SetValueAsBool( i, SHOW_FIELD_COLUMN, show );

        if( show )
            m_grid->ShowCol( col );
        else
            m_grid->HideCol( col );

        // Set grouped columns
        bool groupBy = getDataModel()->GetGroupColumn( col );
        m_viewControlsDataModel->SetValueAsBool( i, GROUP_BY_COLUMN, groupBy );
    }

    m_grid->SetSortingColumn( getDataModel()->GetSortCol(), getDataModel()->GetSortAsc() );
    m_groupSymbolsBox->SetValue( getDataModel()->GetGroupingEnabled() );
    m_filter->ChangeValue( getDataModel()->GetFilter() );
    m_filterScope->SetSelection( static_cast<int>( getDataModel()->GetFilterScope() ) );

    SetupAllColumnProperties();

    // This will rebuild all rows and columns in the model such that the order
    // and labels are right, then we refresh the shown grid data to match
    getDataModel()->EnableRebuilds();
    getDataModel()->RebuildRows();

    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
    else
        m_grid->ForceRefresh();
}


void DIALOG_FIELDS_TABLE::loadDefaultBomPresets()
{
    m_bomPresets.clear();
    m_bomPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_PRESET& preset : getBuiltInBomPresets() )
    {
        m_bomPresets[preset.name] = preset;
        m_bomPresets[preset.name].readOnly = true;

        m_bomPresetMRU.Add( preset.name );
    }
}


std::vector<BOM_PRESET> DIALOG_FIELDS_TABLE::getBuiltInBomPresets() const
{
    return BOM_PRESET::BuiltInPresets();
}


void DIALOG_FIELDS_TABLE::rebuildBomPresetsWidget()
{
    m_cbBomPresets->Clear();

    int idx = 0;
    int default_idx = 0;

    for( const auto& [presetName, preset] : m_bomPresets )
    {
        m_cbBomPresets->Append( wxGetTranslation( presetName ), (void*) &preset );

        if( presetName == BOM_PRESET::DefaultEditing().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomPresets->Append( wxT( "---" ) );

    if( !m_job )
    {
        m_cbBomPresets->Append( _( "Save preset..." ) );
        m_cbBomPresets->Append( _( "Delete preset..." ) );
    }

    // At least the built-in presets should always be present
    wxASSERT( !m_bomPresets.empty() );

    m_cbBomPresets->SetSelection( default_idx );
    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( default_idx ) );
}


int DIALOG_FIELDS_TABLE::presetDashDashDashIndex( int aPresetCount ) const
{
    return aPresetCount - ( m_job ? 1 : 3 );
}


int DIALOG_FIELDS_TABLE::presetSavePresetIndex( int aPresetCount ) const
{
    return m_job ? wxNOT_FOUND : aPresetCount - 2;
}


int DIALOG_FIELDS_TABLE::presetDeletePresetIndex( int aPresetCount ) const
{
    return m_job ? wxNOT_FOUND : aPresetCount - 1;
}


BOM_PRESET DIALOG_FIELDS_TABLE::getDataModelBomPreset()
{
    return getDataModel()->GetBomSettings();
}


void DIALOG_FIELDS_TABLE::syncBomPresetSelection()
{
    BOM_PRESET current = getDataModelBomPreset();

    auto it = std::find_if( m_bomPresets.begin(), m_bomPresets.end(),
            [&]( const std::pair<const wxString, BOM_PRESET>& aPair )
            {
                const BOM_PRESET& preset = aPair.second;

                // Check the simple settings first
                if( !( preset.sortAsc == current.sortAsc
                       && preset.filterString == current.filterString
                       && preset.filterScope == current.filterScope
                       && preset.groupSymbols == current.groupSymbols
                       && preset.excludeDNP == current.excludeDNP
                       && preset.includeExcludedFromBOM == current.includeExcludedFromBOM ) )
                {
                    return false;
                }

                // We should compare preset.name and current.name.  Unfortunately current.name is
                // empty because m_dataModel->GetBomSettings() does not store the .name member.
                // So use sortField member as a (not very efficient) auxiliary filter.
                // As a further complication, sortField can be translated in m_bomPresets list, so
                // current.sortField needs to be translated.
                // Probably this not efficient and error prone test should be removed (JPC).
                if( preset.sortField != wxGetTranslation( current.sortField ) )
                    return false;

                // Only compare shown or grouped fields
                std::vector<BOM_FIELD> A, B;

                for( const BOM_FIELD& field : preset.fieldsOrdered )
                {
                    if( field.show || field.groupBy )
                        A.emplace_back( field );
                }

                for( const BOM_FIELD& field : current.fieldsOrdered )
                {
                    if( field.show || field.groupBy )
                        B.emplace_back( field );
                }

                return A == B;
            } );

    if( it != m_bomPresets.end() )
    {
        // Select the right m_cbBomPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;
        m_cbBomPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomPresets->SetSelection( presetDashDashDashIndex( m_cbBomPresets->GetCount() ) );
    }

    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( m_cbBomPresets->GetSelection() ) );
}


void DIALOG_FIELDS_TABLE::updateBomPresetSelection( const wxString& aName )
{
    // Look at m_userBomPresets to know if aName is a read only preset, or a user preset.
    // Read-only presets have translated names in UI, so we have to use a translated name
    // in UI selection.  But for a user preset name we search for the untranslated aName.
    wxString ui_label = aName;

    for( const auto& [presetName, preset] : m_bomPresets )
    {
        if( presetName == aName )
        {
            if( preset.readOnly == true )
                ui_label = wxGetTranslation( aName );

            break;
        }
    }

    int idx = m_cbBomPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomPresets->GetSelection() != idx )
    {
        m_cbBomPresets->SetSelection( idx );
        m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomPresets->SetSelection( presetDashDashDashIndex( m_cbBomPresets->GetCount() ) );
    }
}


void DIALOG_FIELDS_TABLE::onBomPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomPresets->GetCount();
    int index = m_cbBomPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomPreset )
                    m_cbBomPresets->SetStringSelection( m_currentBomPreset->name );
                else
                    m_cbBomPresets->SetSelection( presetDashDashDashIndex( m_cbBomPresets->GetCount() ) );
            };

    if( index == presetDashDashDashIndex( count ) )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == presetSavePresetIndex( count ) )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomPreset )
            name = m_lastSelectedBomPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomPresets.count( name );

        if( !exists )
        {
            m_bomPresets[name] = getDataModelBomPreset();
            m_bomPresets[name].readOnly = false;
            m_bomPresets[name].name = name;
        }

        BOM_PRESET* preset = &m_bomPresets[name];

        if( !exists )
        {
            index = m_cbBomPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = getDataModelBomPreset();
            preset->name = name;

            index = m_cbBomPresets->FindString( name );

            if( m_bomPresetMRU.Index( name ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( name );
        }

        m_currentBomPreset = preset;
        m_cbBomPresets->SetSelection( index );
        m_bomPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == presetDeletePresetIndex( count ) )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( const auto& [name, preset] : m_bomPresets )
        {
            if( !preset.readOnly )
            {
                wxArrayString item;
                item.Add( name );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomPresets.erase( presetName );

                m_cbBomPresets->Delete( idx );
                m_currentBomPreset = nullptr;
            }

            if( m_bomPresetMRU.Index( presetName ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( presetName );
        }

        resetSelection();
        return;
    }

    BOM_PRESET* preset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( index ) );
    m_currentBomPreset = preset;

    m_lastSelectedBomPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomPreset( *preset );
        syncBomPresetSelection();
        m_currentBomPreset = preset;

        if( !m_currentBomPreset->name.IsEmpty() )
        {
            if( m_bomPresetMRU.Index( preset->name ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( preset->name );

            m_bomPresetMRU.Insert( preset->name, 0 );
        }
    }
}


BOM_FMT_PRESET DIALOG_FIELDS_TABLE::GetCurrentBomFmtSettings()
{
    BOM_FMT_PRESET current;

    current.name = m_cbBomFmtPresets->GetStringSelection();
    current.fieldDelimiter = m_textFieldDelimiter->GetValue();
    current.stringDelimiter = m_textStringDelimiter->GetValue();
    current.refDelimiter = m_textRefDelimiter->GetValue();
    current.refRangeDelimiter = m_textRefRangeDelimiter->GetValue();
    current.keepTabs = m_checkKeepTabs->GetValue();
    current.keepLineBreaks = m_checkKeepLineBreaks->GetValue();
    current.includeByteOrderMark = m_checkIncludeByteOrderMark->GetValue();

    return current;
}


std::vector<BOM_FMT_PRESET> DIALOG_FIELDS_TABLE::GetUserBomFmtPresets() const
{
    std::vector<BOM_FMT_PRESET> ret;

    for( const auto& [name, preset] : m_bomFmtPresets )
    {
        if( !preset.readOnly )
            ret.emplace_back( preset );
    }

    return ret;
}


void DIALOG_FIELDS_TABLE::SetUserBomFmtPresets( std::vector<BOM_FMT_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomFmtPresets();

    for( const BOM_FMT_PRESET& preset : aPresetList )
    {
        if( m_bomFmtPresets.count( preset.name ) )
            continue;

        m_bomFmtPresets[preset.name] = preset;

        m_bomFmtPresetMRU.Add( preset.name );
    }

    rebuildBomFmtPresetsWidget();
}


void DIALOG_FIELDS_TABLE::ApplyBomFmtPreset( const wxString& aPresetName )
{
    updateBomFmtPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomFmtPresetChanged( dummy );
}


void DIALOG_FIELDS_TABLE::ApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    m_currentBomFmtPreset = nullptr;
    m_lastSelectedBomFmtPreset = nullptr;

    if( m_bomFmtPresets.count( aPreset.name ) )
        m_currentBomFmtPreset = &m_bomFmtPresets[aPreset.name];

    if( m_currentBomFmtPreset && !m_currentBomFmtPreset->readOnly )
        m_lastSelectedBomFmtPreset =m_currentBomFmtPreset;

    updateBomFmtPresetSelection( aPreset.name );
    doApplyBomFmtPreset( aPreset );
}


void DIALOG_FIELDS_TABLE::doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    m_textFieldDelimiter->ChangeValue( aPreset.fieldDelimiter );
    m_textStringDelimiter->ChangeValue( aPreset.stringDelimiter );
    m_textRefDelimiter->ChangeValue( aPreset.refDelimiter );
    m_textRefRangeDelimiter->ChangeValue( aPreset.refRangeDelimiter );
    m_checkKeepTabs->SetValue( aPreset.keepTabs );
    m_checkKeepLineBreaks->SetValue( aPreset.keepLineBreaks );
    m_checkIncludeByteOrderMark->SetValue( aPreset.includeByteOrderMark );

    // Refresh the preview if that's the current page
    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
}


void DIALOG_FIELDS_TABLE::loadDefaultBomFmtPresets()
{
    m_bomFmtPresets.clear();
    m_bomFmtPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_FMT_PRESET& preset : BOM_FMT_PRESET::BuiltInPresets() )
    {
        m_bomFmtPresets[preset.name] = preset;
        m_bomFmtPresets[preset.name].readOnly = true;

        m_bomFmtPresetMRU.Add( preset.name );
    }
}


void DIALOG_FIELDS_TABLE::rebuildBomFmtPresetsWidget()
{
    m_cbBomFmtPresets->Clear();

    int idx = 0;
    int default_idx = 0;

    for( const auto& [presetName, preset] : m_bomFmtPresets )
    {
        m_cbBomFmtPresets->Append( wxGetTranslation( presetName ), (void*) &preset );

        if( presetName == BOM_FMT_PRESET::CSV().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomFmtPresets->Append( wxT( "---" ) );

    if( !m_job )
    {
        m_cbBomFmtPresets->Append( _( "Save preset..." ) );
        m_cbBomFmtPresets->Append( _( "Delete preset..." ) );
    }

    // At least the built-in presets should always be present
    wxASSERT( !m_bomFmtPresets.empty() );

    m_cbBomFmtPresets->SetSelection( default_idx );
    m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( default_idx ) );
}


void DIALOG_FIELDS_TABLE::syncBomFmtPresetSelection()
{
    BOM_FMT_PRESET current = GetCurrentBomFmtSettings();

    auto it = std::find_if( m_bomFmtPresets.begin(), m_bomFmtPresets.end(),
                            [&]( const std::pair<const wxString, BOM_FMT_PRESET>& aPair )
                            {
                                return ( aPair.second.fieldDelimiter == current.fieldDelimiter
                                         && aPair.second.stringDelimiter == current.stringDelimiter
                                         && aPair.second.refDelimiter == current.refDelimiter
                                         && aPair.second.refRangeDelimiter == current.refRangeDelimiter
                                         && aPair.second.keepTabs == current.keepTabs
                                         && aPair.second.keepLineBreaks == current.keepLineBreaks
                                         && aPair.second.includeByteOrderMark == current.includeByteOrderMark );
                            } );

    if( it != m_bomFmtPresets.end() )
    {
        // Select the right m_cbBomFmtPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;

        m_cbBomFmtPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomFmtPresets->SetSelection( presetDashDashDashIndex( m_cbBomFmtPresets->GetCount() ) );
    }

    int idx = m_cbBomFmtPresets->GetSelection();
    m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( idx ) );
}


void DIALOG_FIELDS_TABLE::updateBomFmtPresetSelection( const wxString& aName )
{
    // look at m_userBomFmtPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( const auto& [presetName, preset] : m_bomFmtPresets )
    {
        if( presetName == aName )
        {
            if( preset.readOnly )
                ui_label = wxGetTranslation( aName );

            break;
        }
    }

    int idx = m_cbBomFmtPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomFmtPresets->GetSelection() != idx )
    {
        m_cbBomFmtPresets->SetSelection( idx );
        m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomFmtPresets->SetSelection( presetDashDashDashIndex( m_cbBomFmtPresets->GetCount() ) );
    }
}


void DIALOG_FIELDS_TABLE::onBomFmtPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomFmtPresets->GetCount();
    int index = m_cbBomFmtPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomFmtPreset )
                    m_cbBomFmtPresets->SetStringSelection( m_currentBomFmtPreset->name );
                else
                    m_cbBomFmtPresets->SetSelection( presetDashDashDashIndex( m_cbBomFmtPresets->GetCount() ) );
            };

    if( index == presetDashDashDashIndex( count ) )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == presetSavePresetIndex( count ) )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomFmtPreset )
            name = m_lastSelectedBomFmtPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomFmtPresets.count( name );

        if( !exists )
        {
            m_bomFmtPresets[name] = GetCurrentBomFmtSettings();
            m_bomFmtPresets[name].readOnly = false;
            m_bomFmtPresets[name].name = name;
        }

        BOM_FMT_PRESET* preset = &m_bomFmtPresets[name];

        if( !exists )
        {
            index = m_cbBomFmtPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = GetCurrentBomFmtSettings();
            preset->name = name;

            index = m_cbBomFmtPresets->FindString( name );

            if( m_bomFmtPresetMRU.Index( name ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( name );
        }

        m_currentBomFmtPreset = preset;
        m_cbBomFmtPresets->SetSelection( index );
        m_bomFmtPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == presetDeletePresetIndex( count ) )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomFmtPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomFmtPresets.erase( presetName );

                m_cbBomFmtPresets->Delete( idx );
                m_currentBomFmtPreset = nullptr;
            }

            if( m_bomFmtPresetMRU.Index( presetName ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( presetName );
        }

        resetSelection();
        return;
    }

    auto* preset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( index ) );
    m_currentBomFmtPreset = preset;

    m_lastSelectedBomFmtPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomFmtPreset( *preset );
        syncBomFmtPresetSelection();
        m_currentBomFmtPreset = preset;

        if( !m_currentBomFmtPreset->name.IsEmpty() )
        {
            if( m_bomFmtPresetMRU.Index( preset->name ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( preset->name );

            m_bomFmtPresetMRU.Insert( preset->name, 0 );
        }
    }
}
