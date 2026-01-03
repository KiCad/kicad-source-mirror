/**
 * @file sel_layer.cpp
 * @brief minor dialogs for one layer selection and a layer pair selection.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/bitmap.h>

#include <kiplatform/ui.h>
#include <confirm.h>
#include <lset.h>
#include <board.h>
#include <pgm_base.h>
#include <project.h>
#include <pcb_base_frame.h>
#include <pcb_layer_presentation.h>
#include <footprint_editor_settings.h>
#include <layer_pairs.h>
#include <dialogs/dialog_layer_selection_base.h>
#include <project/project_file.h>
#include <router/router_tool.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <tools/pcb_actions.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/layer_box_selector.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>


// Column position by function:
#define SELECT_COLNUM 0
#define COLOR_COLNUM 1
#define LAYERNAME_COLNUM 2
#define LAYER_HK_COLUMN 3


PCB_LAYER_PRESENTATION::PCB_LAYER_PRESENTATION( PCB_BASE_FRAME* aFrame ) : m_boardFrame( aFrame )
{
}

COLOR4D PCB_LAYER_PRESENTATION::getLayerColor( int aLayer ) const
{
    if( m_boardFrame )
    {
        return m_boardFrame->GetColorSettings()->GetColor( aLayer );
    }
    else
    {
        FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
        COLOR_SETTINGS*            current = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

        return current->GetColor( aLayer );
    }
}

wxString PCB_LAYER_PRESENTATION::getLayerName( int aLayer ) const
{
    if( m_boardFrame )
        return m_boardFrame->GetBoard()->GetLayerName( ToLAYER_ID( aLayer ) );
    else
        return BOARD::GetStandardLayerName( ToLAYER_ID( aLayer ) );
}

LSEQ PCB_LAYER_PRESENTATION::getOrderedEnabledLayers() const
{
    return m_boardFrame->GetBoard()->GetEnabledLayers().UIOrder();
}

wxString PCB_LAYER_PRESENTATION::getLayerPairName( const LAYER_PAIR& aPair ) const
{
    const wxString layerAName = getLayerName( aPair.GetLayerA() );
    const wxString layerBName = getLayerName( aPair.GetLayerB() );

    return layerAName + wxT( " / " ) + layerBName;
}


/**
 * Display a PCB layers list in a dialog to select one layer from this list.
 */
class PCB_ONE_LAYER_SELECTOR : public DIALOG_LAYER_SELECTION_BASE
{
public:
    PCB_ONE_LAYER_SELECTOR( PCB_BASE_FRAME* aParent, PCB_LAYER_ID aDefaultLayer,
                            LSET aNotAllowedLayersMask, bool aHideCheckBoxes = false );
    ~PCB_ONE_LAYER_SELECTOR();

    int GetLayerSelection() { return m_layerSelected; }

private:
    // Event handlers
    void OnLeftGridCellClick( wxGridEvent& aEvent ) override;
    void OnRightGridCellClick( wxGridEvent& aEvent ) override;
    void OnMouseMove( wxUpdateUIEvent& aEvent ) override;

    // Will close the dialog on ESC key
    void onCharHook( wxKeyEvent& event );

    wxString getLayerHotKey( PCB_LAYER_ID aLayer ) const
    {
        int code = PCB_ACTIONS::LayerIDToAction( aLayer )->GetHotKey();
        return AddHotkeyName( wxS( "" ), code, IS_COMMENT );
    }

    void buildList();

    PCB_LAYER_PRESENTATION    m_layerPresentation;
    PCB_LAYER_ID              m_layerSelected;
    LSET                      m_notAllowedLayersMask;
    std::vector<PCB_LAYER_ID> m_layersIdLeftColumn;
    std::vector<PCB_LAYER_ID> m_layersIdRightColumn;
};


PCB_ONE_LAYER_SELECTOR::PCB_ONE_LAYER_SELECTOR( PCB_BASE_FRAME* aParent, PCB_LAYER_ID aDefaultLayer,
                                                LSET aNotAllowedLayersMask, bool aHideCheckBoxes ) :
        DIALOG_LAYER_SELECTION_BASE( aParent ), m_layerPresentation( aParent )
{
    m_useCalculatedSize = true;

    m_layerSelected = aDefaultLayer;
    m_notAllowedLayersMask = aNotAllowedLayersMask;

    m_leftGridLayers->SetCellHighlightPenWidth( 0 );
    m_rightGridLayers->SetCellHighlightPenWidth( 0 );
    m_leftGridLayers->SetColFormatBool( SELECT_COLNUM );
    m_rightGridLayers->SetColFormatBool( SELECT_COLNUM );

    m_leftGridLayers->AppendCols( 1 );

    buildList();

    if( aHideCheckBoxes )
    {
        m_leftGridLayers->HideCol( SELECT_COLNUM );
        m_rightGridLayers->HideCol( SELECT_COLNUM );
    }

    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( PCB_ONE_LAYER_SELECTOR::onCharHook ) );

    Layout();
    GetSizer()->SetSizeHints( this );
    SetFocus();
}


PCB_ONE_LAYER_SELECTOR::~PCB_ONE_LAYER_SELECTOR()
{
    Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( PCB_ONE_LAYER_SELECTOR::onCharHook ) );
}


void PCB_ONE_LAYER_SELECTOR::OnMouseMove( wxUpdateUIEvent& aEvent )
{
    /// We have to assign this in UpdateUI events because the wxGrid is not properly receiving
    /// MouseMove events.  It seems to only get them on the edges.  So, for now we use this
    /// workaround

    wxPoint mouse_pos = KIPLATFORM::UI::GetMousePosition();
    wxPoint left_pos = m_leftGridLayers->ScreenToClient( mouse_pos );
    wxPoint right_pos = m_rightGridLayers->ScreenToClient( mouse_pos );

    if( m_leftGridLayers->HitTest( left_pos ) == wxHT_WINDOW_INSIDE )
    {
        int row = m_leftGridLayers->YToRow( left_pos.y );

        if( row != wxNOT_FOUND && row < static_cast<int>( m_layersIdLeftColumn.size() ) )
        {
            m_layerSelected = m_layersIdLeftColumn[row];
            m_leftGridLayers->SelectBlock( row, LAYERNAME_COLNUM, row, LAYER_HK_COLUMN );
            return;
        }
    }

    if( m_rightGridLayers->HitTest( right_pos ) == wxHT_WINDOW_INSIDE )
    {
        int row = m_rightGridLayers->YToRow( right_pos.y );

        if( row == wxNOT_FOUND || row >= static_cast<int>( m_layersIdRightColumn.size() ) )
            return;

        m_layerSelected = m_layersIdRightColumn[row];
        m_rightGridLayers->SelectBlock( row, LAYERNAME_COLNUM, row, LAYERNAME_COLNUM );
    }
}


void PCB_ONE_LAYER_SELECTOR::onCharHook( wxKeyEvent& event )
{
    if( event.GetKeyCode() == WXK_ESCAPE )
        Close();
}


void PCB_ONE_LAYER_SELECTOR::buildList()
{
    wxColour bg = m_layerPresentation.getLayerColor( LAYER_PCB_BACKGROUND ).ToColour();
    int      left_row = 0;
    int      right_row = 0;
    wxString layername;

    for( PCB_LAYER_ID layerid : m_layerPresentation.getOrderedEnabledLayers() )
    {
        if( m_notAllowedLayersMask[layerid] )
            continue;

        wxColour fg = m_layerPresentation.getLayerColor( layerid ).ToColour();
        wxColour color( wxColour::AlphaBlend( fg.Red(), bg.Red(), fg.Alpha() / 255.0 ),
                        wxColour::AlphaBlend( fg.Green(), bg.Green(), fg.Alpha() / 255.0 ),
                        wxColour::AlphaBlend( fg.Blue(), bg.Blue(), fg.Alpha() / 255.0 ) );

        layername = wxT( " " ) + m_layerPresentation.getLayerName( layerid );

        if( IsCopperLayer( layerid ) )
        {
            if( left_row )
                m_leftGridLayers->AppendRows( 1 );

            m_leftGridLayers->SetCellBackgroundColour( left_row, COLOR_COLNUM, color );
            m_leftGridLayers->SetCellValue( left_row, LAYERNAME_COLNUM, layername );
            m_leftGridLayers->SetCellValue( left_row, LAYER_HK_COLUMN, getLayerHotKey( layerid ) );

            if( m_layerSelected == layerid )
                m_leftGridLayers->SetCellValue( left_row, SELECT_COLNUM, wxT( "1" ) );

            m_layersIdLeftColumn.push_back( layerid );
            left_row++;
        }
        else
        {
            if( right_row )
                m_rightGridLayers->AppendRows( 1 );

            m_rightGridLayers->SetCellBackgroundColour( right_row, COLOR_COLNUM, color );
            m_rightGridLayers->SetCellValue( right_row, LAYERNAME_COLNUM, layername );

            if( m_layerSelected == layerid )
                m_rightGridLayers->SetCellValue( right_row, SELECT_COLNUM, wxT( "1" ) );

            m_layersIdRightColumn.push_back( layerid );
            right_row++;
        }
    }

    // Show only populated lists:
    if( left_row <= 0 )
        m_leftGridLayers->Show( false );

    if( right_row <= 0 )
        m_rightGridLayers->Show( false );

    // Now fix min grid column size (it also sets a minimal size)
    m_leftGridLayers->AutoSizeColumns();
    m_rightGridLayers->AutoSizeColumns();
}


void PCB_ONE_LAYER_SELECTOR::OnLeftGridCellClick( wxGridEvent& event )
{
    m_layerSelected = m_layersIdLeftColumn[event.GetRow()];

    if( IsQuasiModal() )
        EndQuasiModal( 1 );
    else
        EndDialog( 1 );
}


void PCB_ONE_LAYER_SELECTOR::OnRightGridCellClick( wxGridEvent& event )
{
    m_layerSelected = m_layersIdRightColumn[event.GetRow()];

    if( IsQuasiModal() )
        EndQuasiModal( 2 );
    else
        EndDialog( 2 );
}


PCB_LAYER_ID PCB_BASE_FRAME::SelectOneLayer( PCB_LAYER_ID aDefaultLayer,
                                             const LSET& aNotAllowedLayersMask,
                                             wxPoint aDlgPosition )
{
    PCB_ONE_LAYER_SELECTOR dlg( this, aDefaultLayer, aNotAllowedLayersMask, true );

    if( aDlgPosition != wxDefaultPosition )
    {
        wxSize dlgSize = dlg.GetSize();
        aDlgPosition.x -= dlgSize.x / 2;
        aDlgPosition.y -= dlgSize.y / 2;
        dlg.SetPosition( aDlgPosition );
    }

    if( dlg.ShowModal() != wxID_CANCEL )
        return ToLAYER_ID( dlg.GetLayerSelection() );
    else
        return UNDEFINED_LAYER;
}


/**
 * Class that manages the UI for the copper layer pair presets list
 * based on an injected layer pair store.
 */
class COPPER_LAYERS_PAIR_PRESETS_UI
{
    enum class COLNUMS
    {
        ENABLED,
        SWATCH,
        LAYERNAMES,
        USERNAME,
    };

public:
    COPPER_LAYERS_PAIR_PRESETS_UI( WX_GRID& aGrid, PCB_LAYER_PRESENTATION& aPresentation,
                                   LAYER_PAIR_SETTINGS& aLayerPairSettings ) :
            m_layerPresentation( aPresentation ),
            m_grid( aGrid ),
            m_layerPairSettings( aLayerPairSettings )
    {
        wxASSERT_MSG( m_grid.GetNumberRows() == 0, "Grid should be empty at controller start" );

        configureGrid();
        fillGridFromStore();

        m_grid.Bind( wxEVT_GRID_CELL_CHANGED,
                     [this]( wxGridEvent& aEvent )
                     {
                         const int col = aEvent.GetCol();
                         const int row = aEvent.GetRow();
                         if( col == (int) COLNUMS::USERNAME )
                         {
                             onUserNameChanged( row, m_grid.GetCellValue( row, col ) );
                         }
                         else if( col == (int) COLNUMS::ENABLED )
                         {
                             onEnableChanged( row, m_grid.GetCellValue( row, col ) == wxS( "1" ) );
                         }
                     } );

        m_grid.Bind( wxEVT_GRID_CELL_LEFT_DCLICK,
                     [&]( wxGridEvent& aEvent )
                     {
                         const int row = aEvent.GetRow();
                         const int col = aEvent.GetCol();

                         if( col == (int) COLNUMS::LAYERNAMES || col == (int) COLNUMS::SWATCH )
                         {
                             onPairActivated( row );
                         }
                     } );
    }

    void OnLayerPairAdded( const LAYER_PAIR& aLayerPair )
    {
        LAYER_PAIR_INFO layerPairInfo{ aLayerPair, true, std::nullopt };

        const bool added = m_layerPairSettings.AddLayerPair( layerPairInfo );

        if( added )
        {
            m_grid.AppendRows( 1 );
            fillRowFromLayerPair( m_grid.GetNumberRows() - 1, layerPairInfo );
        }
    }

    void OnDeleteSelectedLayerPairs()
    {
        m_grid.OnDeleteRows(
                [&]( int row )
                {
                    const LAYER_PAIR_INFO& layerPairInfo = m_layerPairSettings.GetLayerPairs()[row];

                    if( m_layerPairSettings.RemoveLayerPair( layerPairInfo.GetLayerPair() ) )
                        m_grid.DeleteRows( row );
                } );
    }

private:
    void configureGrid()
    {
        m_grid.UseNativeColHeader( true );

        m_grid.SetCellHighlightPenWidth( 0 );
        m_grid.SetColFormatBool( (int) COLNUMS::ENABLED );
        m_grid.SetupColumnAutosizer( (int) COLNUMS::USERNAME );

        m_grid.SetSelectionMode( wxGrid::wxGridSelectionModes::wxGridSelectRows );
    }

    void fillGridFromStore()
    {
        std::span<const LAYER_PAIR_INFO> storePairs = m_layerPairSettings.GetLayerPairs();

        m_grid.AppendRows( storePairs.size() );

        int row = 0;
        for( const LAYER_PAIR_INFO& layerPairInfo : storePairs )
        {
            fillRowFromLayerPair( row, layerPairInfo );
            row++;
        }
    }

    void fillRowFromLayerPair( int aRow, const LAYER_PAIR_INFO& aLayerPairInfo )
    {
        wxASSERT_MSG( aRow < m_grid.GetNumberRows(), "Row index out of bounds" );

        const LAYER_PAIR& layerPair = aLayerPairInfo.GetLayerPair();

        const wxString layerNames = m_layerPresentation.getLayerPairName( layerPair );

        m_grid.SetCellValue( aRow, (int) COLNUMS::LAYERNAMES, layerNames );

        const std::optional<wxString> userName = aLayerPairInfo.GetName();
        if( userName )
        {
            m_grid.SetCellValue( aRow, (int) COLNUMS::USERNAME, *userName );
        }

        m_grid.SetCellValue( aRow, (int) COLNUMS::ENABLED,
                             aLayerPairInfo.IsEnabled() ? wxT( "1" ) : wxT( "0" ) );

        // Set the color swatch
        wxBitmapBundle swatch = m_layerPresentation.CreateLayerPairIcon( layerPair.GetLayerA(), layerPair.GetLayerB() );

        m_grid.SetCellRenderer( aRow, (int) COLNUMS::SWATCH, new GRID_CELL_ICON_RENDERER( swatch ) );

        m_grid.SetReadOnly( aRow, (int) COLNUMS::SWATCH );
        m_grid.SetReadOnly( aRow, (int) COLNUMS::LAYERNAMES );
    }

    void onUserNameChanged( int aRow, const wxString& aNewValue )
    {
        LAYER_PAIR_INFO& changedPair = m_layerPairSettings.GetLayerPairs()[aRow];
        changedPair.SetName( aNewValue );
    }

    void onEnableChanged( int aRow, bool aNewValue )
    {
        LAYER_PAIR_INFO& changedPair = m_layerPairSettings.GetLayerPairs()[aRow];
        changedPair.SetEnabled( aNewValue );
    }

    void onPairActivated( int aRow )
    {
        const LAYER_PAIR_INFO& layerPairInfo = m_layerPairSettings.GetLayerPairs()[aRow];
        const LAYER_PAIR&      layerPair = layerPairInfo.GetLayerPair();

        m_layerPairSettings.SetCurrentLayerPair( layerPair );
    }

    PCB_LAYER_PRESENTATION& m_layerPresentation;
    WX_GRID&                m_grid;
    LAYER_PAIR_SETTINGS&    m_layerPairSettings;
};


/**
 * Class that manages the UI for the copper layer pair selection
 * (left and right grids).
 */
class COPPER_LAYERS_PAIR_SELECTION_UI
{
    enum class CU_LAYER_COLNUMS
    {
        SELECT = 0,
        COLOR = 1,
        LAYERNAME = 2,
    };

public:
    COPPER_LAYERS_PAIR_SELECTION_UI( wxGrid& aLeftGrid, wxGrid& aRightGrid,
                                     PCB_LAYER_PRESENTATION& aPresentation,
                                     LAYER_PAIR_SETTINGS&    aLayerPairSettings ) :
            m_layerPresentation( aPresentation ), m_layerPairSettings( aLayerPairSettings ),
            m_leftGrid( aLeftGrid ), m_rightGrid( aRightGrid )
    {
        configureGrid( m_leftGrid );
        configureGrid( m_rightGrid );

        for( const PCB_LAYER_ID& layerId : m_layerPresentation.getOrderedEnabledLayers() )
        {
            if( IsCopperLayer( layerId ) )
                m_layersId.push_back( layerId );
        }

        fillLayerGrid( m_leftGrid );
        fillLayerGrid( m_rightGrid );

        m_leftGrid.Bind( wxEVT_GRID_CELL_LEFT_CLICK,
                         [this]( wxGridEvent& aEvent )
                         {
                             onLeftGridRowSelected( aEvent.GetRow() );
                         } );

        m_rightGrid.Bind( wxEVT_GRID_CELL_LEFT_CLICK,
                          [this]( wxGridEvent& aEvent )
                          {
                              onRightGridRowSelected( aEvent.GetRow() );
                          } );

        m_layerPairSettings.Bind( PCB_CURRENT_LAYER_PAIR_CHANGED,
                                  [this]( wxCommandEvent& aEvent )
                                  {
                                      const LAYER_PAIR& newPair = m_layerPairSettings.GetCurrentLayerPair();
                                      setCurrentSelection( rowForLayer( newPair.GetLayerA() ),
                                                           rowForLayer( newPair.GetLayerB() ) );
                                  } );
    }

private:
    void configureGrid( wxGrid& aGrid )
    {
        aGrid.SetCellHighlightPenWidth( 0 );
        aGrid.SetColFormatBool( (int) CU_LAYER_COLNUMS::SELECT );
    }

    void fillLayerGrid( wxGrid& aGrid )
    {
        const wxColour bg = m_layerPresentation.getLayerColor( LAYER_PCB_BACKGROUND ).ToColour();

        aGrid.AppendRows( m_layersId.size() - 1 );

        int row = 0;
        for( const PCB_LAYER_ID& layerId : m_layersId )
        {
            const wxColour fg = m_layerPresentation.getLayerColor( layerId ).ToColour();
            const wxColour color( wxColour::AlphaBlend( fg.Red(), bg.Red(), fg.Alpha() / 255.0 ),
                                  wxColour::AlphaBlend( fg.Green(), bg.Green(), fg.Alpha() / 255.0 ),
                                  wxColour::AlphaBlend( fg.Blue(), bg.Blue(), fg.Alpha() / 255.0 ) );

            const wxString layerName = wxT( " " ) + m_layerPresentation.getLayerName( layerId );

            aGrid.SetCellBackgroundColour( row, (int) CU_LAYER_COLNUMS::COLOR, color );
            aGrid.SetCellValue( row, (int) CU_LAYER_COLNUMS::LAYERNAME, layerName );

            row++;
        };

        // Now fix min grid layer name column size (it also sets a minimal size)
        aGrid.AutoSizeColumn( (int) CU_LAYER_COLNUMS::LAYERNAME );
    }

    PCB_LAYER_ID layerForRow( int aRow ) { return m_layersId.at( aRow ); }

    int rowForLayer( PCB_LAYER_ID aLayerId )
    {
        for( unsigned i = 0; i < m_layersId.size(); ++i )
        {
            if( m_layersId[i] == aLayerId )
                return i;
        }

        wxASSERT_MSG( false, wxString::Format( "Unknown layer in grid: %d", aLayerId ) );
        return 0;
    }

    void onLeftGridRowSelected( int aRow )
    {
        LAYER_PAIR newPair{
            layerForRow( aRow ),
            layerForRow( m_rightCurrRow ),
        };
        setCurrentSelection( aRow, m_rightCurrRow );
        m_layerPairSettings.SetCurrentLayerPair( newPair );
    }

    void onRightGridRowSelected( int aRow )
    {
        LAYER_PAIR newPair{
            layerForRow( m_leftCurrRow ),
            layerForRow( aRow ),
        };
        setCurrentSelection( m_leftCurrRow, aRow );
        m_layerPairSettings.SetCurrentLayerPair( newPair );
    }

    /**
     * Set the current layer selection.
     *
     * The layer pair must be copper layers that the selector this class
     * was constructed with knows about.
     */
    void setCurrentSelection( int aLeftRow, int aRightRow )
    {
        const auto selectGridRow =
                []( wxGrid& aGrid, int aRow, bool aSelect )
                {
                    // At start, there is no old row
                    if( aRow < 0 )
                        return;

                    const wxString val = aSelect ? wxT( "1" ) : wxEmptyString;
                    aGrid.SetCellValue( aRow, (int) CU_LAYER_COLNUMS::SELECT, val );
                    aGrid.SetGridCursor( aRow, (int) CU_LAYER_COLNUMS::COLOR );
                };

        if( m_leftCurrRow != aLeftRow )
        {
            selectGridRow( m_leftGrid, m_leftCurrRow, false );
            selectGridRow( m_leftGrid, aLeftRow, true );
            m_leftCurrRow = aLeftRow;
        }

        if( m_rightCurrRow != aRightRow )
        {
            selectGridRow( m_rightGrid, m_rightCurrRow, false );
            selectGridRow( m_rightGrid, aRightRow, true );
            m_rightCurrRow = aRightRow;
        }
    }

    PCB_LAYER_PRESENTATION&   m_layerPresentation;
    LAYER_PAIR_SETTINGS&      m_layerPairSettings;
    std::vector<PCB_LAYER_ID> m_layersId;
    wxGrid&                   m_leftGrid;
    wxGrid&                   m_rightGrid;

    int m_leftCurrRow = -1;
    int m_rightCurrRow = -1;
};


/**
 * Display a pair PCB copper layers list in a dialog to select a layer pair from these lists.
 *
 * This is a higher level class that mostly glues together other controller classes and UI
 * elements.
 */
class SELECT_COPPER_LAYERS_PAIR_DIALOG : public DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE
{
public:
    SELECT_COPPER_LAYERS_PAIR_DIALOG( PCB_BASE_FRAME&      aParent,
                                      LAYER_PAIR_SETTINGS& aBoardSettings ) :
            DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( &aParent ),
            m_boardPairSettings( aBoardSettings ), m_dialogPairSettings( aBoardSettings ),
            m_layerPresentation( &aParent ),
            m_pairSelectionController( *m_leftGridLayers, *m_rightGridLayers, m_layerPresentation,
                                       m_dialogPairSettings ),
            m_presetsGridController( *m_presetsGrid, m_layerPresentation, m_dialogPairSettings )
    {
        m_addToPresetsButton->SetBitmap( KiBitmapBundle( BITMAPS::right ) );
        m_deleteRowButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

        m_addToPresetsButton->Bind( wxEVT_BUTTON,
                                    [this]( wxCommandEvent& aEvent )
                                    {
                                        const LAYER_PAIR newPair = m_dialogPairSettings.GetCurrentLayerPair();
                                        m_presetsGridController.OnLayerPairAdded( newPair );
                                    } );

        m_deleteRowButton->Bind( wxEVT_BUTTON,
                                 [this]( wxCommandEvent& aEvent )
                                 {
                                     m_presetsGridController.OnDeleteSelectedLayerPairs();
                                 } );

        SetFocus();

        GetSizer()->SetSizeHints( this );
        Center();
    }

    bool TransferDataToWindow() override
    {
        m_presetsGrid->Freeze();
        m_leftGridLayers->Freeze();
        m_rightGridLayers->Freeze();

        m_dialogPairSettings.SetCurrentLayerPair( m_boardPairSettings.GetCurrentLayerPair() );

        m_rightGridLayers->Thaw();
        m_leftGridLayers->Thaw();
        m_presetsGrid->Thaw();
        return true;
    }

    bool TransferDataFromWindow() override
    {
        // Pull out the dialog's stored pairs
        std::span<const LAYER_PAIR_INFO> storePairs = m_dialogPairSettings.GetLayerPairs();

        m_boardPairSettings.SetLayerPairs( storePairs );
        m_boardPairSettings.SetCurrentLayerPair( m_dialogPairSettings.GetCurrentLayerPair() );

        return true;
    }

private:
    // The BOARD's pair store to be updated
    LAYER_PAIR_SETTINGS& m_boardPairSettings;
    // A local copy while we modify it
    LAYER_PAIR_SETTINGS m_dialogPairSettings;
    // Information about the layer presentation (colors, etc)
    PCB_LAYER_PRESENTATION m_layerPresentation;
    // UI controllers
    COPPER_LAYERS_PAIR_SELECTION_UI m_pairSelectionController;
    COPPER_LAYERS_PAIR_PRESETS_UI   m_presetsGridController;
};


int ROUTER_TOOL::SelectCopperLayerPair( const TOOL_EVENT& aEvent )
{
    LAYER_PAIR_SETTINGS* const boardSettings = frame()->GetLayerPairSettings();

    if( !boardSettings )
    {
        // Should only be used for suitable frame types with layer pairs
        wxASSERT_MSG( false, "Could not access layer pair settings" );
        return 0;
    }

    SELECT_COPPER_LAYERS_PAIR_DIALOG dlg( *frame(), *boardSettings );

    if( dlg.ShowModal() == wxID_OK )
    {
        const LAYER_PAIR layerPair = boardSettings->GetCurrentLayerPair();

        // select the same layer for both layers is allowed (normal in some boards)
        // but could be a mistake. So display an info message
        if( layerPair.GetLayerA() == layerPair.GetLayerB() )
        {
            DisplayInfoMessage( frame(), _( "Warning: top and bottom layers are same." ) );
        }
    }

    return 0;
}
