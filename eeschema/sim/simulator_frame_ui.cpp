/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <type_traits>

#include <wx/event.h>
#include <wx/settings.h>
#include <fmt/format.h>
#include <wx/wfstream.h>
#include <wx/stdstream.h>
#include <wx/debug.h>
#include <wx/clipbrd.h>
#include <wx/colordlg.h>
#include <wx/log.h>
#include <wx/tokenzr.h>

#include <project/project_file.h>
#include <sch_edit_frame.h>
#include <confirm.h>
#include <wildcards_and_files_ext.h>
#include <widgets/tuner_slider.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/grid_combobox.h>
#include <widgets/std_bitmap_button.h>
#include <bitmaps.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>
#include <eda_pattern_match.h>
#include <string_utils.h>
#include <pgm_base.h>
#include <sim/simulator_frame_ui.h>
#include <sim/simulator_frame.h>
#include <sim/sim_plot_tab.h>
#include <sim/sim_signal_utils.h>
#include <sim/spice_simulator.h>
#include <dialogs/dialog_text_entry.h>
#include <dialogs/dialog_sim_format_value.h>
#include <eeschema_settings.h>
#include <advanced_config.h>
#include <magic_enum.hpp>
#include <widgets/wx_infobar.h>


SIM_TRACE_TYPE operator|( SIM_TRACE_TYPE aFirst, SIM_TRACE_TYPE aSecond )
{
    int res = static_cast<int>( aFirst ) | static_cast<int>( aSecond );

    return static_cast<SIM_TRACE_TYPE>( res );
}


enum CURSORS_GRID_COLUMNS
{
    COL_CURSOR_NAME = 0,
    COL_CURSOR_SIGNAL,
    COL_CURSOR_X,
    COL_CURSOR_Y,

    // everything from here on is appended only while the tab shows a Smith chart
    COL_CURSOR_SMITH_FIRST,

    COL_CURSOR_R = COL_CURSOR_SMITH_FIRST,
    COL_CURSOR_REACTANCE,
    COL_CURSOR_RL,
    COL_CURSOR_VSWR,

    COL_CURSOR_SMITH_END
};


static constexpr int SMITH_CURSOR_COLS = COL_CURSOR_SMITH_END - COL_CURSOR_SMITH_FIRST;


static bool smithColumnsShown( const WX_GRID* aGrid )
{
    return aGrid->GetNumberCols() > COL_CURSOR_SMITH_FIRST;
}


enum
{
    MYID_FOURIER = GRIDTRICKS_FIRST_CLIENT_ID,

    MYID_FORMAT_VALUE,
    MYID_DELETE_MEASUREMENT,
    MYID_MEASUREMENT_ADD,
    MYID_MEASUREMENT_EDIT,
    MYID_MEASURE_ALL
};


/// Signals-tree context menu ids; "..._BASE" ids have an index added.
enum
{
    MYID_TREE_COPY = 30000,
    MYID_TREE_NEW_CURSOR,
    MYID_TREE_DELETE_CURSOR,
    MYID_TREE_COLOR,

    MYID_TREE_VIEW_BASE = 30100,   // + index into getViewChoices() (0 == unplotted)
    MYID_TREE_YSCALE_BASE = 30200, // + index into getYScaleChoices()
    MYID_TREE_CURSOR_BASE = 30300, // + cursor id
    MYID_TREE_MEASURE_BASE = 30400 // + index into measurementFunctions()
};


/// Plot indicator: a dot in the trace colour when plotted, a hollow ring otherwise.
static wxBitmap makeSignalDotBitmap( const wxColour& aColor, bool aFilled, int aScale )
{
    const int    size = 12 * aScale;
    const double center = size / 2.0;
    const double outer = center - aScale;
    const double inner = outer - 1.8 * aScale; // ring thickness, hollow state only
    const int    ss = 3;                       // supersampling steps per axis

    wxImage image( size, size );
    image.InitAlpha();

    wxColour fg = aFilled ? aColor : wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

    for( int y = 0; y < size; ++y )
    {
        for( int x = 0; x < size; ++x )
        {
            int hits = 0;

            for( int sy = 0; sy < ss; ++sy )
            {
                for( int sx = 0; sx < ss; ++sx )
                {
                    double px = x + ( sx + 0.5 ) / ss - center;
                    double py = y + ( sy + 0.5 ) / ss - center;
                    double r = std::hypot( px, py );

                    if( r <= outer && ( aFilled || r >= inner ) )
                        hits++;
                }
            }

            image.SetRGB( x, y, fg.Red(), fg.Green(), fg.Blue() );
            image.SetAlpha( x, y, static_cast<unsigned char>( 255 * hits / ( ss * ss ) ) );
        }
    }

    return wxBitmap( image );
}


static wxBitmapBundle makeSignalDot( const wxColour& aColor, bool aFilled )
{
    // A 2x variant so scaled displays get a sharp dot instead of an upscaled one
    return wxBitmapBundle::FromBitmaps( makeSignalDotBitmap( aColor, aFilled, 1 ),
                                        makeSignalDotBitmap( aColor, aFilled, 2 ) );
}


wxBitmapBundle SIMULATOR_FRAME_UI::signalDot( const wxColour& aColor, bool aFilled )
{
    // The key must match the colour makeSignalDot() draws.
    const wxColour colour = aFilled ? aColor : wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );

    for( const SIGNAL_DOT& dot : m_signalDotCache )
    {
        if( dot.filled == aFilled && dot.colour == colour )
            return dot.bitmap;
    }

    if( m_signalDotCache.size() >= SIGNAL_DOT_CACHE_LIMIT )
        m_signalDotCache.clear();

    wxBitmapBundle bitmap = makeSignalDot( aColor, aFilled );
    m_signalDotCache.push_back( { colour, aFilled, bitmap } );

    return bitmap;
}


/// Fixed row height for every tree cell: wxGTK mis-measures unrealized rows.
static int simTreeRowHeight( wxWindow* aWindow )
{
    // Fallback for a renderer not yet attached to a view.
    int fontHeight = aWindow ? aWindow->GetCharHeight() : 16;

    return std::max( fontHeight, 12 );
}


/// Draws the plain-text cell of both trees without measuring it (see simTreeRowHeight()).
class SIM_TREE_TEXT_RENDERER : public wxDataViewCustomRenderer
{
public:
    SIM_TREE_TEXT_RENDERER() :
            wxDataViewCustomRenderer( wxS( "string" ), wxDATAVIEW_CELL_INERT, wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL )
    {
    }

    bool SetValue( const wxVariant& aValue ) override
    {
        m_value = aValue.GetString();
        return true;
    }

    bool GetValue( wxVariant& aValue ) const override
    {
        aValue = m_value;
        return true;
    }

    wxSize GetSize() const override
    {
        return wxSize( std::max( GetTextExtent( m_value ).x, 1 ), simTreeRowHeight( GetView() ) );
    }

    bool Render( wxRect aRect, wxDC* aDC, int aState ) override
    {
        RenderText( m_value, 0, aRect, aDC, aState );
        return true;
    }

#if wxUSE_ACCESSIBILITY
    wxString GetAccessibleDescription() const override { return m_value; }
#endif

private:
    wxString m_value;
};


/// Draws the "[dot] name" cell of both trees without measuring it (see simTreeRowHeight()).
class SIM_TREE_ICON_TEXT_RENDERER : public wxDataViewCustomRenderer
{
public:
    SIM_TREE_ICON_TEXT_RENDERER() :
            wxDataViewCustomRenderer( wxS( "wxDataViewIconText" ), wxDATAVIEW_CELL_INERT,
                                      wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL )
    {
    }

    bool SetValue( const wxVariant& aValue ) override
    {
        m_value << aValue;
        return true;
    }

    bool GetValue( wxVariant& aValue ) const override
    {
        aValue << m_value;
        return true;
    }

    wxSize GetSize() const override
    {
        int width = GetTextExtent( m_value.GetText() ).x + bitmapSize().x + gap();

        // A zero size would make wxDataViewCtrl fall back to measuring.
        return wxSize( std::max( width, 1 ), simTreeRowHeight( GetView() ) );
    }

    bool Render( wxRect aRect, wxDC* aDC, int aState ) override
    {
        int xoffset = 0;

        const wxBitmapBundle& bundle = m_value.GetBitmapBundle();

        if( bundle.IsOk() )
        {
            wxSize   bmpSize = bitmapSize();
            wxBitmap bmp = bundle.GetBitmapFor( GetView() );

            aDC->DrawBitmap( bmp, aRect.x, aRect.y + ( aRect.height - bmpSize.y ) / 2, true );
            xoffset = bmpSize.x + gap();
        }

        RenderText( m_value.GetText(), xoffset, aRect, aDC, aState );
        return true;
    }

#if wxUSE_ACCESSIBILITY
    wxString GetAccessibleDescription() const override { return m_value.GetText(); }
#endif

private:
    wxSize bitmapSize() const
    {
        const wxBitmapBundle& bundle = m_value.GetBitmapBundle();

        return bundle.IsOk() ? bundle.GetPreferredLogicalSizeFor( GetView() ) : wxSize( 0, 0 );
    }

    int gap() const { return m_value.GetBitmapBundle().IsOk() ? 4 : 0; }

    wxDataViewIconText m_value;
};


/// Two-column grouped tree (name + value) shared by the signals and measurements panels.
class SIM_TREE_MODEL : public wxDataViewModel
{
public:
    enum COLUMN
    {
        COL_NAME = 0,
        COL_VALUE,
        COL_COUNT
    };

    struct NODE
    {
        NODE( const wxString& aName, NODE* aParent ) :
                name( aName ),
                parent( aParent )
        {
        }

        wxString name;
        wxString key; ///< Group identity, independent of the label; empty for leaves.

        NODE*                              parent;
        std::vector<std::unique_ptr<NODE>> children;

        // Leaf state; the icon is cached because GetValue() runs per repaint.
        wxString       value;
        wxBitmapBundle icon;
        bool           greyed = false;

        // Leaf payload: plotted state and colour for signals, the measurement index for measurements.
        bool     plotted = false;
        wxColour color;
        int      index = -1;

        /// A node with children is a group, so groups can nest.
        bool IsGroup() const { return !children.empty(); }
    };

    struct ENTRY
    {
        wxString name;
        int      index = -1;
    };

    /// One group passed to SetContents().  Empty groups are dropped.
    struct GROUP
    {
        wxString           name;
        wxString           key; ///< See NODE::key.
        std::vector<ENTRY> entries;
        std::vector<GROUP> subGroups;

        bool IsEmpty() const
        {
            return entries.empty()
                   && std::all_of( subGroups.begin(), subGroups.end(),
                                   []( const GROUP& aSub )
                                   {
                                       return aSub.IsEmpty();
                                   } );
        }
    };

    SIM_TREE_MODEL() = default;

    void SetContents( const std::vector<GROUP>& aGroups )
    {
        m_groups.clear();

        for( const GROUP& group : aGroups )
        {
            if( group.IsEmpty() )
                continue;

            m_groups.push_back( buildGroup( group, nullptr ) );
        }

        Cleared();
    }

    std::vector<NODE*> Groups() const
    {
        std::vector<NODE*> groups;

        for( const std::unique_ptr<NODE>& group : m_groups )
            collect( group.get(), true, groups );

        return groups;
    }

    std::vector<NODE*> Leaves() const
    {
        std::vector<NODE*> leaves;

        for( const std::unique_ptr<NODE>& group : m_groups )
            collect( group.get(), false, leaves );

        return leaves;
    }

    static wxDataViewItem ToItem( NODE* aNode ) { return wxDataViewItem( aNode ); }

    static NODE* ToNode( const wxDataViewItem& aItem ) { return static_cast<NODE*>( aItem.GetID() ); }

    // wxDataViewModel
    unsigned int GetColumnCount() const override { return COL_COUNT; }

    wxString GetColumnType( unsigned int aCol ) const override
    {
        return aCol == COL_NAME ? wxS( "wxDataViewIconText" ) : wxS( "string" );
    }

    bool IsContainer( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() ) // the invisible root
            return true;

        return ToNode( aItem )->IsGroup();
    }

    wxDataViewItem GetParent( const wxDataViewItem& aItem ) const override
    {
        if( !aItem.IsOk() )
            return wxDataViewItem( nullptr );

        return wxDataViewItem( ToNode( aItem )->parent );
    }

    unsigned int GetChildren( const wxDataViewItem& aItem, wxDataViewItemArray& aChildren ) const override
    {
        if( !aItem.IsOk() )
        {
            for( const std::unique_ptr<NODE>& group : m_groups )
                aChildren.Add( wxDataViewItem( group.get() ) );

            return aChildren.GetCount();
        }

        for( const std::unique_ptr<NODE>& child : ToNode( aItem )->children )
            aChildren.Add( wxDataViewItem( child.get() ) );

        return aChildren.GetCount();
    }

    void GetValue( wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) const override
    {
        NODE* node = ToNode( aItem );

        if( !node )
            return;

        if( aCol == COL_NAME )
            aVariant << wxDataViewIconText( node->name, node->icon );
        else
            aVariant = node->value;
    }

    bool SetValue( const wxVariant& aVariant, const wxDataViewItem& aItem, unsigned int aCol ) override
    {
        return false; // the tree is driven by its context menu, never edited in place
    }

    bool GetAttr( const wxDataViewItem& aItem, unsigned int aCol, wxDataViewItemAttr& aAttr ) const override
    {
        NODE* node = ToNode( aItem );

        if( !node )
            return false;

        if( node->IsGroup() )
        {
            aAttr.SetBold( true );
            return true;
        }

        if( node->greyed )
        {
            aAttr.SetColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
            return true;
        }

        return false;
    }

private:
    static std::unique_ptr<NODE> buildGroup( const GROUP& aGroup, NODE* aParent )
    {
        std::unique_ptr<NODE> node = std::make_unique<NODE>( aGroup.name, aParent );
        node->key = aGroup.key;

        for( const ENTRY& entry : aGroup.entries )
        {
            std::unique_ptr<NODE> leaf = std::make_unique<NODE>( entry.name, node.get() );
            leaf->index = entry.index;
            node->children.push_back( std::move( leaf ) );
        }

        for( const GROUP& sub : aGroup.subGroups )
        {
            if( !sub.IsEmpty() )
                node->children.push_back( buildGroup( sub, node.get() ) );
        }

        return node;
    }

    static void collect( NODE* aNode, bool aWantGroups, std::vector<NODE*>& aResult )
    {
        if( aNode->IsGroup() == aWantGroups )
            aResult.push_back( aNode );

        for( const std::unique_ptr<NODE>& child : aNode->children )
            collect( child.get(), aWantGroups, aResult );
    }

    std::vector<std::unique_ptr<NODE>> m_groups;
};


class CURSORS_GRID_TRICKS : public GRID_TRICKS
{
public:
    CURSORS_GRID_TRICKS( SIMULATOR_FRAME_UI* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {
    }

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_FRAME_UI* m_parent;
    int                 m_menuRow;
    int                 m_menuCol;
};


void CURSORS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    m_menuRow = aEvent.GetRow();
    m_menuCol = aEvent.GetCol();

    if( m_menuCol == COL_CURSOR_X || m_menuCol == COL_CURSOR_Y )
    {
        wxString msg = m_grid->GetColLabelValue( m_menuCol );

        menu.Append( MYID_FORMAT_VALUE, wxString::Format( _( "Format %s..." ), msg ) );
        menu.AppendSeparator();
    }

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void CURSORS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    auto getSignalName =
            [this]( int row ) -> wxString
            {
                wxString signal = m_grid->GetCellValue( row, COL_CURSOR_SIGNAL );

                if( signal.EndsWith( "[2 - 1]" ) )
                    signal = signal.Left( signal.length() - 7 );

                return signal;
            };

    if( event.GetId() == MYID_FORMAT_VALUE )
    {
        int                     axis = m_menuCol - COL_CURSOR_X;
        SPICE_VALUE_FORMAT      format = m_parent->GetCursorFormat( m_menuRow, axis );
        DIALOG_SIM_FORMAT_VALUE formatDialog( m_parent, &format );

        if( formatDialog.ShowModal() == wxID_OK )
        {
            for( int row = 0; row < m_grid->GetNumberRows(); ++row )
            {
                if( getSignalName( row ) == getSignalName( m_menuRow ) )
                    m_parent->SetCursorFormat( row, axis, format );
            }
        }
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


class SUPPRESS_GRID_CELL_EVENTS
{
public:
    SUPPRESS_GRID_CELL_EVENTS( SIMULATOR_FRAME_UI* aFrame ) :
            m_frame( aFrame )
    {
        m_frame->m_SuppressGridEvents++;
    }

    ~SUPPRESS_GRID_CELL_EVENTS() { m_frame->m_SuppressGridEvents--; }

private:
    SIMULATOR_FRAME_UI* m_frame;
};


#define ID_SIM_REFRESH 10207
#define REFRESH_INTERVAL 50 // 20 frames/second.


SIMULATOR_FRAME_UI::SIMULATOR_FRAME_UI( SIMULATOR_FRAME* aSimulatorFrame, SCH_EDIT_FRAME* aSchematicFrame ) :
        SIMULATOR_FRAME_UI_BASE( aSimulatorFrame ),
        m_SuppressGridEvents( 0 ),
        m_simulatorFrame( aSimulatorFrame ),
        m_schematicFrame( aSchematicFrame ),
        m_darkMode( true ),
        m_plotNumber( 0 ),
        m_refreshTimer( this, ID_SIM_REFRESH )
{
    // The base class' idle handlers would override the sash positions SetSubWindowsSashSize() restores.
    m_splitterLeftRight->Disconnect(
            wxEVT_IDLE, wxIdleEventHandler( SIMULATOR_FRAME_UI_BASE::m_splitterLeftRightOnIdle ), nullptr, this );
    m_splitterCenterRight->Disconnect(
            wxEVT_IDLE, wxIdleEventHandler( SIMULATOR_FRAME_UI_BASE::m_splitterCenterRightOnIdle ), nullptr, this );
    m_splitterPlotAndConsole->Disconnect(
            wxEVT_IDLE, wxIdleEventHandler( SIMULATOR_FRAME_UI_BASE::m_splitterPlotAndConsoleOnIdle ), nullptr, this );
    m_splitterSignals->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIMULATOR_FRAME_UI_BASE::m_splitterSignalsOnIdle ),
                                   nullptr, this );
    m_splitterCursors->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIMULATOR_FRAME_UI_BASE::m_splitterCursorsOnIdle ),
                                   nullptr, this );

    // Get the previous size and position of windows:
    LoadSettings( m_schematicFrame->eeconfig() );

    m_filter->SetHint( _( "Filter" ) );

    Bind( EVT_SIM_VIEWS_CHANGED,
          [&]( wxCommandEvent& aEvent )
          {
              rebuildSignalsTree( m_filter->GetValue() );
              updatePlotCursors();
              OnModify();
          } );

    m_cursorsGrid->wxGrid::SetLabelFont( KIUI::GetStatusFont( this ) );

    m_cursorsGrid->PushEventHandler( new CURSORS_GRID_TRICKS( this, m_cursorsGrid ) );

    // Ctrl+C, as advertised in the context menu.
    m_signalsTree->Bind( wxEVT_CHAR_HOOK,
                         [this]( wxKeyEvent& aEvent )
                         {
                             if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKeyCode() == 'C' )
                                 copySignalNames( getSelectedSignals( wxDataViewItem() ) );
                             else
                                 aEvent.Skip();
                         } );

    m_signalsModel = new SIM_TREE_MODEL();
    m_signalsTree->AssociateModel( m_signalsModel );
    m_signalsModel->DecRef(); // AssociateModel() took its own reference

    m_signalsTree->AppendColumn( new wxDataViewColumn( _( "Signal" ), new SIM_TREE_ICON_TEXT_RENDERER(),
                                                       SIM_TREE_MODEL::COL_NAME, FromDIP( 180 ), wxALIGN_LEFT,
                                                       wxDATAVIEW_COL_RESIZABLE ) );
    m_signalsTree->AppendColumn( new wxDataViewColumn( _( "Plot" ), new SIM_TREE_TEXT_RENDERER(),
                                                       SIM_TREE_MODEL::COL_VALUE, FromDIP( 60 ), wxALIGN_LEFT,
                                                       wxDATAVIEW_COL_RESIZABLE ) );

    m_tunerNotebook = new wxNotebook( m_panelTuners, wxID_ANY );
    m_sizerTuners->Add( m_tunerNotebook, 1, wxEXPAND, 0 );

    for( wxScrolledWindow*& page : m_tunerPages )
        page = nullptr;

    m_measurementsModel = new SIM_TREE_MODEL();
    m_measurementsTree->AssociateModel( m_measurementsModel );
    m_measurementsModel->DecRef(); // AssociateModel() took its own reference

    // Backspace is the delete key on macOS keyboards
    m_measurementsTree->Bind( wxEVT_CHAR_HOOK,
                              [this]( wxKeyEvent& aEvent )
                              {
                                  int key = aEvent.GetKeyCode();

                                  if( aEvent.GetModifiers() == wxMOD_NONE
                                      && ( key == WXK_DELETE || key == WXK_BACK ) )
                                  {
                                      deleteMeasurements( getSelectedMeasurements( wxDataViewItem() ) );
                                  }
                                  else
                                  {
                                      aEvent.Skip();
                                  }
                              } );

    m_measurementsTree->AppendColumn( new wxDataViewColumn( _( "Measurement" ), new SIM_TREE_ICON_TEXT_RENDERER(),
                                                            SIM_TREE_MODEL::COL_NAME, FromDIP( 200 ), wxALIGN_LEFT,
                                                            wxDATAVIEW_COL_RESIZABLE ) );
    m_measurementsTree->AppendColumn( new wxDataViewColumn( _( "Value" ), new SIM_TREE_TEXT_RENDERER(),
                                                            SIM_TREE_MODEL::COL_VALUE, FromDIP( 90 ), wxALIGN_LEFT,
                                                            wxDATAVIEW_COL_RESIZABLE ) );

    applyTreeRowHeight();

    // Re-apply once realized so GTK doesn't keep a height measured too early.
    CallAfter(
            [this]()
            {
                applyTreeRowHeight();
            } );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_NAME, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_SIGNAL, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_Y, attr );

    CustomCursorsInit();

    // Prepare the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

    Bind( EVT_SIM_CURSOR_UPDATE, &SIMULATOR_FRAME_UI::onPlotCursorUpdate, this );

    // Hollow dots are drawn in the system grey, so a theme change needs new bitmaps
    Bind( wxEVT_SYS_COLOUR_CHANGED,
          [this]( wxSysColourChangedEvent& aEvent )
          {
              aEvent.Skip();
              m_signalDotCache.clear();

              for( SIM_TREE_MODEL::NODE* leaf : m_signalsModel->Leaves() )
                  leaf->icon = wxBitmapBundle();

              for( SIM_TREE_MODEL::NODE* group : m_measurementsModel->Groups() )
                  group->icon = wxBitmapBundle();

              updateSignalsTree();
          } );

    Bind( wxEVT_TIMER,
            [&]( wxTimerEvent& aEvent )
            {
                OnSimRefresh( false );

                if( m_simulatorFrame->GetSimulator()->IsRunning() )
                    m_refreshTimer.Start( REFRESH_INTERVAL, wxTIMER_ONE_SHOT );
            },
            m_refreshTimer.GetId() );

#ifndef wxHAS_NATIVE_TABART
    // Default non-native tab art has ugly gradients we don't want
    m_plotNotebook->SetArtProvider( new wxAuiSimpleTabArt() );
#endif
}


SIMULATOR_FRAME_UI::~SIMULATOR_FRAME_UI()
{
    // Delete the GRID_TRICKS.
    m_cursorsGrid->PopEventHandler( true );
}


void SIMULATOR_FRAME_UI::CustomCursorsInit()
{
    for( auto& m_cursorFormat : m_cursorFormats )
    {
        m_cursorFormat[0] = { 3, wxS( "~s" ) };
        m_cursorFormat[1] = { 3, wxS( "~V" ) };
    }

    // proper init and transfer/copy m_cursorFormats
    // we work on m_cursorFormatsDyn from now on.
    // TODO: rework +- LOC when m_cursorFormatsDyn and m_cursorFormats get merged.
    m_cursorFormatsDyn.clear();
    m_cursorFormatsDyn.resize( std::size( m_cursorFormats ) );

    for( size_t index = 0; index < std::size( m_cursorFormats ); index++ )
    {
        for( size_t index2 = 0; index2 < std::size( m_cursorFormats[0] ); index2++ )
            m_cursorFormatsDyn[index].push_back( m_cursorFormats[index][index2] );
    }

    // Cursors 1 and 2 always exist.
    m_customCursorsCnt = static_cast<int>( std::size( m_cursorFormats ) );
}


void SIMULATOR_FRAME_UI::CreateNewCursor()
{
    std::vector<SPICE_VALUE_FORMAT> tmp;
    // m_cursorFormatsDyn should be equal with m_cursorFormats on first entry here.
    m_cursorFormatsDyn.emplace_back( tmp );

    m_cursorFormatsDyn[m_customCursorsCnt].push_back( { 3, wxS( "~s" ) } );
    m_cursorFormatsDyn[m_customCursorsCnt].push_back( { 3, wxS( "~V" ) } );

    m_customCursorsCnt++;

    updateSignalsTree();
    updatePlotCursors();
    OnModify();
}


void SIMULATOR_FRAME_UI::DeleteCursor()
{
    // Cursors 1 and 2 are permanent; delete the extras last-first.
    if( m_customCursorsCnt <= static_cast<int>( std::size( m_cursorFormats ) ) )
        return;

    int doomed = m_customCursorsCnt - 1;

    // Any tab may hold it; a leftover would index past the formats popped below.
    for( size_t ii = 0; ii < m_plotNotebook->GetPageCount(); ++ii )
    {
        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( m_plotNotebook->GetPage( ii ) );

        if( !plotTab )
            continue;

        for( const auto& [name, trace] : plotTab->GetTraces() )
        {
            if( trace->HasCursor( doomed ) )
            {
                plotTab->DisableCursor( trace, doomed );
                break;
            }
        }
    }

    m_cursorFormatsDyn.pop_back();
    m_customCursorsCnt--;
    m_plotNotebook->Refresh();
    updateSignalsTree();
    updatePlotCursors();
    OnModify();
}


void SIMULATOR_FRAME_UI::ShowChangedLanguage()
{
    for( int ii = 0; ii < static_cast<int>( m_plotNotebook->GetPageCount() ); ++ii )
    {
        if( SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( ii ) ) )
        {
            simTab->OnLanguageChanged();

            wxString pageTitle( simulator()->TypeToName( simTab->GetSimType(), true ) );
            pageTitle.Prepend( wxString::Format( _( "Analysis %u - " ), ii + 1 /* 1-based */ ) );

            m_plotNotebook->SetPageText( ii, pageTitle );
        }
    }

    m_filter->SetHint( _( "Filter" ) );

    m_consoleFilterAll->SetLabel( _( "All" ) );
    m_consoleFilterWarnings->SetLabel( _( "Warnings" ) );
    m_consoleFilterErrors->SetLabel( _( "Errors" ) );
    m_panelConsole->Layout();

    if( m_signalsTree->GetColumnCount() >= 2 )
    {
        m_signalsTree->GetColumn( SIM_TREE_MODEL::COL_NAME )->SetTitle( _( "Signal" ) );
        m_signalsTree->GetColumn( SIM_TREE_MODEL::COL_VALUE )->SetTitle( _( "Plot" ) );
    }

    // The gain/phase suffixes in m_signals are translated
    rebuildSignalsList();
    rebuildSignalsTree( m_filter->GetValue() );

    if( m_measurementsTree->GetColumnCount() >= 2 )
    {
        m_measurementsTree->GetColumn( SIM_TREE_MODEL::COL_NAME )->SetTitle( _( "Measurement" ) );
        m_measurementsTree->GetColumn( SIM_TREE_MODEL::COL_VALUE )->SetTitle( _( "Value" ) );
    }

    // Group names and function labels are set by the rebuild.
    std::map<wxString, wxString> measurementValues = shownMeasurementValues();
    rebuildMeasurementsTree( &measurementValues );

    m_cursorsGrid->SetColLabelValue( COL_CURSOR_NAME, _( "Cursor" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_SIGNAL, _( "Signal" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, _( "Time" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, _( "Value" ) );
    setSmithCursorColumnLabels();
    updatePlotCursors();

    for( TUNER_SLIDER* tuner : m_tuners )
        tuner->ShowChangedLanguage();

    for( int ii = 0; ii < TUNERGRP_COUNT; ++ii )
    {
        if( m_tunerPages[ii] )
        {
            int pos = m_tunerNotebook->FindPage( m_tunerPages[ii] );

            if( pos != wxNOT_FOUND )
                m_tunerNotebook->SetPageText( pos, tunerGroupLabel( (TUNER_GROUP) ii ) );
        }
    }
}


void SIMULATOR_FRAME_UI::LoadSettings( EESCHEMA_SETTINGS* aCfg )
{
    const EESCHEMA_SETTINGS::SIMULATOR& settings = aCfg->m_Simulator;

    // Read subwindows sizes (should be > 0 )
    m_splitterLeftRightSashPosition = settings.view.side_panel_width;
    m_splitterPlotAndConsoleSashPosition = settings.view.plot_panel_height;
    m_splitterSignalsSashPosition = settings.view.signals_tuners_height;
    m_splitterCursorsSashPosition = settings.view.cursors_panel_height;
    m_splitterCenterRightSashPosition = settings.view.center_panel_width;
    m_darkMode = !settings.view.white_background;
    m_smithCursorColumns = settings.view.smith_cursor_columns;

    m_preferences = settings.preferences;
}


void SIMULATOR_FRAME_UI::SaveSettings( EESCHEMA_SETTINGS* aCfg )
{
    EESCHEMA_SETTINGS::SIMULATOR& settings = aCfg->m_Simulator;

    // A hidden panel's splitter is unsplit; save where its sash will go back to.
    auto sashPosition = []( wxSplitterWindow* aSplitter, int aHiddenPosition )
    {
        return aSplitter->IsSplit() ? aSplitter->GetSashPosition() : aHiddenPosition;
    };

    settings.view.side_panel_width = sashPosition( m_splitterLeftRight, m_splitterLeftRightSashPosition );
    settings.view.plot_panel_height = sashPosition( m_splitterPlotAndConsole, m_splitterPlotAndConsoleSashPosition );
    settings.view.signals_tuners_height = m_splitterSignals->GetSashPosition();
    settings.view.cursors_panel_height = m_splitterCursors->GetSashPosition();
    settings.view.center_panel_width = m_splitterCenterRight->GetSashPosition();
    settings.view.white_background = !m_darkMode;

    if( smithColumnsShown( m_cursorsGrid ) )
        rememberSmithCursorColumns();

    settings.view.smith_cursor_columns = m_smithCursorColumns;
}


void SIMULATOR_FRAME_UI::ApplyPreferences( const SIM_PREFERENCES& aPrefs )
{
    m_preferences = aPrefs;

    for( std::size_t i = 0; i < m_plotNotebook->GetPageCount(); ++i )
    {
        if( SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( i ) ) )
            simTab->ApplyPreferences( aPrefs );
    }
}


void SIMULATOR_FRAME_UI::InitWorkbook()
{
    wxString workbookFilename = simulator()->Settings()->GetWorkbookFilename();
    bool     loadFromSchematic = false;

    if( !workbookFilename.IsEmpty() )
    {
        wxFileName filename = workbookFilename;
        filename.SetPath( m_schematicFrame->Prj().GetProjectPath() );

        if( !filename.FileExists() )
        {
            WX_INFOBAR* infobar = m_simulatorFrame->GetInfoBar();
            wxString    msg = wxString::Format( _( "Workbook file '%s' not found. Loading simulation settings from "
                                                    "schematic." ),
                                                filename.GetFullPath() );
            infobar->ShowMessageFor( msg, 8000, wxICON_WARNING );

            simulator()->Settings()->SetWorkbookFilename( wxEmptyString );
            loadFromSchematic = true;
        }
        else if( !LoadWorkbook( filename.GetFullPath() ) )
        {
            simulator()->Settings()->SetWorkbookFilename( wxEmptyString );
        }
    }
    else
    {
        loadFromSchematic = true;
    }

    if( loadFromSchematic && m_simulatorFrame->LoadSimulator( wxEmptyString, 0 ) )
    {
        wxString schTextSimCommand = circuitModel()->GetSchTextSimCommand();

        if( !schTextSimCommand.IsEmpty() )
        {
            SIM_TAB* simTab = NewSimTab( schTextSimCommand );
            simTab->SetSimOptions( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS );
        }

        rebuildSignalsList();
        rebuildSignalsTree( m_filter->GetValue() );
    }
}


void SIMULATOR_FRAME_UI::SetSubWindowsSashSize()
{
    // The base class default is in raw pixels, which is too narrow on a scaled display
    if( m_splitterLeftRightSashPosition > 0 )
        m_splitterLeftRight->SetSashPosition( m_splitterLeftRightSashPosition );
    else
        m_splitterLeftRight->SetSashPosition( FromDIP( 300 ) );

    if( m_splitterPlotAndConsoleSashPosition > 0 )
        m_splitterPlotAndConsole->SetSashPosition( m_splitterPlotAndConsoleSashPosition );

    if( m_splitterSignalsSashPosition > 0 )
        m_splitterSignals->SetSashPosition( m_splitterSignalsSashPosition );

    if( m_splitterCursorsSashPosition > 0 )
        m_splitterCursors->SetSashPosition( m_splitterCursorsSashPosition );

    // A negative position sizes the right pane, so a first open fits the cursor and measurement columns
    if( m_splitterCenterRightSashPosition > 0 )
        m_splitterCenterRight->SetSashPosition( m_splitterCenterRightSashPosition );
    else
        m_splitterCenterRight->SetSashPosition( -FromDIP( 400 ) );
}


void sortSignals( std::vector<wxString>& signals )
{
    std::sort( signals.begin(), signals.end(),
            []( const wxString& lhs, const wxString& rhs )
            {
                // Sort voltages first
                if( lhs.Upper().StartsWith( 'V' ) && !rhs.Upper().StartsWith( 'V' ) )
                    return true;
                else if( !lhs.Upper().StartsWith( 'V' ) && rhs.Upper().StartsWith( 'V' ) )
                    return false;

                return StrNumCmp( lhs, rhs, CMP_T::IGNORE_CASE ) < 0;
            } );
}


wxString vectorNameFromSignalId( int aUserDefinedSignalId );
static wxString deviceVectorName( int aDeviceVoltageId );


wxString SimSignalBaseName( const wxString& aSignal )
{
    for( const wxString& suffix : { _( " (gain)" ), _( " (phase)" ), _( " (amplitude)" ) } )
    {
        if( aSignal.EndsWith( suffix ) )
            return aSignal.Left( aSignal.Length() - suffix.Length() );
    }

    return aSignal;
}


/// Signal identity across language changes: the translated suffix becomes a tag.
static wxString signalViewKey( const wxString& aSignal )
{
    wxString tag;

    if( aSignal.EndsWith( _( " (gain)" ) ) )
        tag = wxS( "gain" );
    else if( aSignal.EndsWith( _( " (phase)" ) ) )
        tag = wxS( "phase" );
    else if( aSignal.EndsWith( _( " (amplitude)" ) ) )
        tag = wxS( "amplitude" );

    if( tag.IsEmpty() )
        return aSignal;

    return SimSignalBaseName( aSignal ) + wxString( wxUniChar( 0x1f ) ) + tag;
}


enum SIGNAL_GROUP
{
    SIGGRP_VOLTAGES = 0,
    SIGGRP_CURRENTS,
    SIGGRP_POWER,
    SIGGRP_OTHER,
    SIGGRP_USER_DEFINED,

    SIGGRP_COUNT
};


/// Bucket a signal by its netlist-exporter prefix ("V(..)", "I(..)", ...).
static SIGNAL_GROUP signalGroup( const wxString& aSignal, bool aUserDefined )
{
    if( aUserDefined )
        return SIGGRP_USER_DEFINED;

    wxString upper = aSignal.Upper();

    if( upper.StartsWith( wxS( "V(" ) ) || upper.StartsWith( wxS( "VDB(" ) ) )
        return SIGGRP_VOLTAGES;
    else if( upper.StartsWith( wxS( "I(" ) ) )
        return SIGGRP_CURRENTS;
    else if( upper.StartsWith( wxS( "P(" ) ) )
        return SIGGRP_POWER;

    return SIGGRP_OTHER;
}


static wxString signalGroupLabel( SIGNAL_GROUP aGroup )
{
    switch( aGroup )
    {
    case SIGGRP_VOLTAGES: return _( "Voltages" );
    case SIGGRP_CURRENTS: return _( "Currents" );
    case SIGGRP_POWER: return _( "Power" );
    case SIGGRP_USER_DEFINED: return _( "User-defined" );
    default: return _( "Other" );
    }
}


/// The language-independent counterpart of signalGroupLabel().
static wxString signalGroupKey( SIGNAL_GROUP aGroup )
{
    switch( aGroup )
    {
    case SIGGRP_VOLTAGES: return wxS( "voltages" );
    case SIGGRP_CURRENTS: return wxS( "currents" );
    case SIGGRP_POWER: return wxS( "power" );
    case SIGGRP_USER_DEFINED: return wxS( "user-defined" );
    default: return wxS( "other" );
    }
}


void SIMULATOR_FRAME_UI::applyTreeRowHeight()
{
    if( !m_signalsTree || !m_measurementsTree )
        return;

    // wxGTK re-measures invalidated container rows against their subtree; use a uniform height.
    int rowHeight = std::max( m_signalsTree->GetCharHeight(), FromDIP( 12 ) ) + FromDIP( 6 );

    m_signalsTree->SetRowHeight( rowHeight );
    m_measurementsTree->SetRowHeight( rowHeight );
}


/// Group identity across rebuilds and language changes.
static wxString signalTreeGroupKey( const SIM_TREE_MODEL::NODE* aGroup )
{
    wxString key;

    for( const SIM_TREE_MODEL::NODE* node = aGroup; node; node = node->parent )
        key = node->key + wxS( "/" ) + key;

    return key;
}


void SIMULATOR_FRAME_UI::rebuildSignalsTree( wxString aFilter )
{
    if( !m_signalsModel )
        return;

    // Both "" and "*" match everything.
    bool filtering = !aFilter.IsEmpty() && aFilter != wxS( "*" );

    // A filtered tree is force-expanded, so don't record its state.
    if( !m_signalsTreeFiltered )
    {
        for( SIM_TREE_MODEL::NODE* group : m_signalsModel->Groups() )
            m_signalGroupExpanded[signalTreeGroupKey( group )] =
                    m_signalsTree->IsExpanded( SIM_TREE_MODEL::ToItem( group ) );
    }

    SIM_PLOT_TAB* plotPanel = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotPanel )
    {
        m_signalsTreeFiltered = filtering;
        m_signalsModel->SetContents( {} );
        return;
    }

    SIM_TYPE              simType = plotPanel->GetSimType();
    std::vector<wxString> signals;
    std::set<wxString>    userDefined;

    if( plotPanel->GetSimType() == ST_FFT )
    {
        wxStringTokenizer tokenizer( plotPanel->GetSimCommand(), " \t\r\n", wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() && tokenizer.GetNextToken().Lower() != wxT( "fft" ) )
        {
        };

        while( tokenizer.HasMoreTokens() )
            signals.emplace_back( tokenizer.GetNextToken() );
    }
    else
    {
        // NB: m_signals are already broken out into gain/phase, but m_userDefinedSignals are
        // as the user typed them

        for( const wxString& signal : m_signals )
            signals.push_back( signal );

        std::set<wxString> native( m_signals.begin(), m_signals.end() );

        auto addUserDefined = [&]( const wxString& aSignal )
        {
            // Same text as a native signal is the same expression, and a second row would drag the
            // native one into the user-defined group
            if( native.count( aSignal ) )
                return;

            signals.push_back( aSignal );
            userDefined.insert( aSignal );
        };

        for( const auto& [id, signal] : m_userDefinedSignals )
        {
            if( simType == ST_AC )
            {
                addUserDefined( signal + _( " (gain)" ) );
                addUserDefined( signal + _( " (phase)" ) );
            }
            else if( simType == ST_SP )
            {
                if( plotPanel->IsSmithMode() )
                {
                    addUserDefined( signal );
                }
                else
                {
                    addUserDefined( signal + _( " (amplitude)" ) );
                    addUserDefined( signal + _( " (phase)" ) );
                }
            }
            else
            {
                addUserDefined( signal );
            }
        }
    }

    if( aFilter.IsEmpty() )
        aFilter = wxS( "*" );

    EDA_COMBINED_MATCHER matcher( aFilter.Upper(), CTX_SIGNAL );

    std::vector<std::vector<wxString>> buckets( SIGGRP_COUNT );
    std::vector<wxString>              componentVoltages;

    for( const wxString& signal : signals )
    {
        if( !matcher.Find( signal.Upper() ) )
            continue;

        // Component voltages get a sub-group so they don't swamp the node voltages.
        if( m_deviceVoltageIds.count( SimSignalBaseName( signal ) ) > 0 )
            componentVoltages.push_back( signal );
        else
            buckets[signalGroup( signal, userDefined.count( signal ) > 0 )].push_back( signal );
    }

    auto sortSignalNames = []( std::vector<wxString>& aNames )
    {
        std::sort( aNames.begin(), aNames.end(),
                   []( const wxString& lhs, const wxString& rhs )
                   {
                       return StrNumCmp( lhs, rhs, CMP_T::IGNORE_CASE ) < 0;
                   } );
    };

    auto toEntries = []( const std::vector<wxString>& aNames )
    {
        std::vector<SIM_TREE_MODEL::ENTRY> entries;

        for( const wxString& signal : aNames )
            entries.push_back( { signal, -1 } );

        return entries;
    };

    sortSignalNames( componentVoltages );

    std::vector<SIM_TREE_MODEL::GROUP> groups;

    for( int ii = 0; ii < SIGGRP_COUNT; ++ii )
    {
        std::vector<wxString>& bucket = buckets[ii];
        SIGNAL_GROUP           groupId = static_cast<SIGNAL_GROUP>( ii );
        bool                   nestComponents = groupId == SIGGRP_VOLTAGES && !componentVoltages.empty();

        if( bucket.empty() && !nestComponents )
            continue;

        sortSignalNames( bucket );

        SIM_TREE_MODEL::GROUP group;
        size_t                count = bucket.size();

        group.entries = toEntries( bucket );

        if( nestComponents )
        {
            count += componentVoltages.size();

            SIM_TREE_MODEL::GROUP components;
            components.name = wxString::Format( wxS( "%s [%zu]" ), _( "Components" ), componentVoltages.size() );
            components.key = wxS( "components" );
            components.entries = toEntries( componentVoltages );
            group.subGroups.push_back( components );
        }

        group.name = wxString::Format( wxS( "%s [%zu]" ), signalGroupLabel( groupId ), count );
        group.key = signalGroupKey( groupId );
        groups.push_back( group );
    }

    // Signal names are unique, so they survive the rebuild where node pointers do not
    auto rowKey = [&]( SIM_TREE_MODEL::NODE* aNode ) -> wxString
    {
        if( !aNode )
            return wxEmptyString;

        return aNode->IsGroup() ? wxS( "\n" ) + signalTreeGroupKey( aNode ) : aNode->name;
    };

    wxDataViewItemArray selection;
    std::set<wxString>  selectedKeys;
    wxString            topKey = rowKey( SIM_TREE_MODEL::ToNode( m_signalsTree->GetTopItem() ) );

    m_signalsTree->GetSelections( selection );

    for( const wxDataViewItem& item : selection )
        selectedKeys.insert( rowKey( SIM_TREE_MODEL::ToNode( item ) ) );

    m_signalsModel->SetContents( groups );

    wxDataViewItem top;
    wxDataViewItem last;

    for( SIM_TREE_MODEL::NODE* group : m_signalsModel->Groups() )
    {
        auto it = m_signalGroupExpanded.find( signalTreeGroupKey( group ) );
        bool expanded = filtering || it == m_signalGroupExpanded.end() || it->second;

        if( expanded )
            m_signalsTree->Expand( SIM_TREE_MODEL::ToItem( group ) );

        // Select() and EnsureVisible() expand collapsed ancestors, so only rows already shown are restored
        bool shown = !group->parent || m_signalsTree->IsExpanded( SIM_TREE_MODEL::ToItem( group->parent ) );

        if( shown )
        {
            if( rowKey( group ) == topKey )
                top = SIM_TREE_MODEL::ToItem( group );

            last = SIM_TREE_MODEL::ToItem( group );
        }

        for( const std::unique_ptr<SIM_TREE_MODEL::NODE>& leaf : group->children )
        {
            if( leaf->IsGroup() || !shown || !expanded )
                continue;

            wxString key = rowKey( leaf.get() );

            if( selectedKeys.count( key ) )
                m_signalsTree->Select( SIM_TREE_MODEL::ToItem( leaf.get() ) );

            if( key == topKey )
                top = SIM_TREE_MODEL::ToItem( leaf.get() );

            last = SIM_TREE_MODEL::ToItem( leaf.get() );
        }
    }

    // EnsureVisible() scrolls the least, so coming from the end leaves the old top row at the top
    if( top.IsOk() && last.IsOk() )
    {
        m_signalsTree->EnsureVisible( last );
        m_signalsTree->EnsureVisible( top );
    }

    m_signalsTreeFiltered = filtering;

    applyTreeRowHeight();
    updateSignalsTree();
}


void SIMULATOR_FRAME_UI::updateSignalsTree()
{
    if( !m_signalsModel )
        return;

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    std::set<const SIM_TAB*> liveTabs;

    for( size_t ii = 0; ii < m_plotNotebook->GetPageCount(); ++ii )
        liveTabs.insert( dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( ii ) ) );

    std::erase_if( m_lastSignalView,
                   [&]( const auto& aEntry )
                   {
                       return liveTabs.find( aEntry.first ) == liveTabs.end();
                   } );

    for( SIM_TREE_MODEL::NODE* leaf : m_signalsModel->Leaves() )
    {
        int      traceType = SPT_UNKNOWN;
        wxString vectorName = vectorNameFromSignalName( plotTab, leaf->name, &traceType );
        TRACE*   trace = plotTab ? plotTab->GetTrace( vectorName, traceType ) : nullptr;

        bool plotted = trace && trace->GetView();

        wxColour color = plotted ? trace->GetPen().GetColour() : wxColour();

        // Rebuilding the dot is expensive; only when it changes.
        if( leaf->plotted != plotted || leaf->color != color || !leaf->icon.IsOk() )
            leaf->icon = signalDot( color, plotted );

        leaf->plotted = plotted;
        leaf->color = color;
        leaf->greyed = !plotted;
        leaf->value = plotted ? getViewLabel( plotTab, trace->GetView() ) : wxString();

        if( plotted )
            m_lastSignalView[plotTab][signalViewKey( leaf->name )] = plotTab->GetViewIndex( trace->GetView() );

        m_signalsModel->ItemChanged( SIM_TREE_MODEL::ToItem( leaf ) );
    }

    // The measurement groups show the same dots.
    if( refreshMeasurementGroupIcons() )
    {
        std::map<wxString, wxString> values = shownMeasurementValues();
        rebuildMeasurementsTree( &values );
    }
}


void SIMULATOR_FRAME_UI::copySignalNames( const std::vector<wxString>& aSignals )
{
    if( aSignals.empty() )
        return;

    wxLogNull doNotLog; // disable logging of failed clipboard actions
    wxString  txt;

    for( const wxString& signal : aSignals )
    {
        if( !txt.IsEmpty() )
            txt += '\r';

        txt += signal;
    }

    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( txt ) );
        wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
        wxTheClipboard->Close();
    }
}


std::vector<wxString> SIMULATOR_FRAME_UI::getSelectedSignals( const wxDataViewItem& aClickedItem ) const
{
    std::vector<wxString> signals;
    wxDataViewItemArray   selection;

    m_signalsTree->GetSelections( selection );

    for( const wxDataViewItem& item : selection )
    {
        SIM_TREE_MODEL::NODE* node = SIM_TREE_MODEL::ToNode( item );

        if( node && !node->IsGroup() )
            signals.push_back( node->name );
    }

    SIM_TREE_MODEL::NODE* clicked = SIM_TREE_MODEL::ToNode( aClickedItem );

    if( clicked && !clicked->IsGroup() && std::find( signals.begin(), signals.end(), clicked->name ) == signals.end() )
    {
        signals.clear();
        signals.push_back( clicked->name );
    }

    return signals;
}


SIM_VIEW* SIMULATOR_FRAME_UI::lastSignalView( SIM_PLOT_TAB* aPlotTab, const wxString& aSignal ) const
{
    if( !aPlotTab || aPlotTab->GetViewCount() == 0 )
        return nullptr;

    auto tabIt = m_lastSignalView.find( aPlotTab );

    if( tabIt != m_lastSignalView.end() )
    {
        auto signalIt = tabIt->second.find( signalViewKey( aSignal ) );

        // The remembered view may be gone.
        if( signalIt != tabIt->second.end() && signalIt->second >= 0 && signalIt->second < aPlotTab->GetViewCount() )
        {
            return aPlotTab->GetView( signalIt->second );
        }
    }

    return aPlotTab->GetView( 0 );
}


void SIMULATOR_FRAME_UI::setSignalView( const wxString& aSignal, SIM_VIEW* aView )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    int      traceType = SPT_UNKNOWN;
    wxString vectorName = vectorNameFromSignalName( plotTab, aSignal, &traceType );

    if( !aView )
    {
        plotTab->DeleteTrace( vectorName, traceType );
    }
    else
    {
        TRACE*                existing = plotTab->GetTrace( vectorName, traceType );
        std::map<int, double> cursorXs;

        if( existing && existing->GetView() != aView )
        {
            // The trace is re-created on the new view, so its cursors have to move with it
            for( const auto& [id, cursor] : existing->GetCursors() )
            {
                if( cursor )
                    cursorXs[id] = cursor->GetCoords().x;
            }

            plotTab->DeleteTrace( existing );
        }

        updateTrace( vectorName, traceType, plotTab, nullptr, false, aView );

        TRACE* moved = cursorXs.empty() ? nullptr : plotTab->GetTrace( vectorName, traceType );

        if( moved && moved->GetView() )
        {
            for( const auto& [id, x] : cursorXs )
            {
                plotTab->EnableCursor( moved, id, aSignal );

                if( CURSOR* cursor = moved->GetCursor( id ) )
                    cursor->SetCoordX( x );
            }
        }
    }
}


void SIMULATOR_FRAME_UI::autoSizeGridColumn( WX_GRID* aGrid, int aCol, int aMinWidth, int aExtraPadding )
{
    wxGridUpdateLocker deferRepaintsTillLeavingScope( aGrid );

    aGrid->AutoSizeColumn( aCol );
    aGrid->AutoSizeColLabelSize( aCol ); // Also grow (never shrink) to fit the header label

    if( aExtraPadding > 0 )
        aGrid->SetColSize( aCol, aGrid->GetColSize( aCol ) + aExtraPadding );

    if( aGrid->GetColSize( aCol ) < aMinWidth )
        aGrid->SetColSize( aCol, aMinWidth );
}


#define UNPLOTTED _( "none" )

wxArrayString SIMULATOR_FRAME_UI::getViewChoices( SIM_PLOT_TAB* aPlotTab ) const
{
    wxArrayString choices;
    choices.Add( UNPLOTTED );

    if( aPlotTab )
    {
        for( int ii = 0; ii < aPlotTab->GetViewCount(); ++ii )
            choices.Add( wxString::Format( _( "View %d" ), ii + 1 ) );
    }

    return choices;
}


wxString SIMULATOR_FRAME_UI::getViewLabel( SIM_PLOT_TAB* aPlotTab, SIM_VIEW* aView ) const
{
    if( !aView || !aPlotTab )
        return UNPLOTTED;

    int index = aPlotTab->GetViewIndex( aView );

    return index >= 0 ? wxString::Format( _( "View %d" ), index + 1 ) : UNPLOTTED;
}


std::vector<SIM_VIEW*> SIMULATOR_FRAME_UI::getYScaleTargetViews( SIM_PLOT_TAB* aPlotTab, SIM_VIEW* aOwnView ) const
{
    std::vector<SIM_VIEW*> views;

    if( aPlotTab )
    {
        for( SIM_VIEW* view : aPlotTab->GetViews() )
        {
            if( view != aOwnView )
                views.push_back( view );
        }
    }

    return views;
}


wxArrayString SIMULATOR_FRAME_UI::getYScaleChoices( SIM_PLOT_TAB* aPlotTab, SIM_VIEW* aOwnView ) const
{
    wxArrayString choices;
    choices.Add( _( "Default" ) );

    for( SIM_VIEW* view : getYScaleTargetViews( aPlotTab, aOwnView ) )
        choices.Add( getViewLabel( aPlotTab, view ) );

    return choices;
}


wxString SIMULATOR_FRAME_UI::getYScaleLabel( SIM_PLOT_TAB* aPlotTab, TRACE* aTrace ) const
{
    if( !aTrace || aTrace->IsYScaleDefault() )
        return _( "Default" );

    return getViewLabel( aPlotTab, aTrace->GetYScaleView() );
}


void SIMULATOR_FRAME_UI::rebuildDeviceVoltages()
{
    m_deviceVoltageIds.clear();
    m_deviceVoltageExprs.clear();
    m_deviceVoltageVectors.clear();

    // Ids are re-evaluated per plot.
    m_appliedDeviceVoltages.clear();

    if( !circuitModel() )
        return;

    // ngspice node names ignore case
    std::set<wxString> netsLower;

    for( const wxString& net : circuitModel()->GetNets() )
        netsLower.insert( net.Lower() );

    for( const SPICE_ITEM& item : circuitModel()->GetItems() )
    {
        // Only enabled two-pin parts have a voltage across them.
        if( !item.model || !item.model->IsEnabled() || item.model->GetPinCount() != 2 )
            continue;

        // Model pin order, so V(D1) is anode minus cathode like I(D1) and P(D1)
        std::vector<std::string> modelPins;

        for( const SIM_MODEL_PIN& pin : item.model->GetPins() )
            modelPins.push_back( pin.symbolPinNumber.ToStdString() );

        wxString expr = SIM_SIGNAL::DeviceVoltageExpression( modelPins, item.pinNumbers, item.pinNetNames );

        if( expr.IsEmpty() )
            continue;

        wxString refName = item.model->SpiceGenerator().ItemName( item );
        wxString name = wxString::Format( wxS( "V(%s)" ), refName );

        // A net named "R1" owns V(R1).  m_netnames may be stale, so ask the model.
        if( netsLower.count( refName.Lower() ) )
            continue;

        // Keep a device's id across netlist changes so existing traces stay valid.
        auto [reg, isNew] = m_deviceVoltageIdRegistry.try_emplace( name, m_nextDeviceVoltageId );

        if( isNew )
            m_nextDeviceVoltageId++;

        int id = reg->second;

        m_deviceVoltageIds[name] = id;
        m_deviceVoltageExprs[id] = expr;
        m_deviceVoltageVectors[deviceVectorName( id )] = id;
    }
}


void SIMULATOR_FRAME_UI::rebuildSignalsList()
{
    m_signals.clear();

    m_netnames.clear();

    m_deviceVoltageIds.clear();
    m_deviceVoltageExprs.clear();
    m_deviceVoltageVectors.clear();

    m_appliedDeviceVoltages.clear();

    int      options = m_simulatorFrame->GetCurrentOptions();
    SIM_TYPE simType = m_simulatorFrame->GetCurrentSimType();
    wxString unconnected = wxString( wxS( "unconnected-(" ) );

    if( simType == ST_UNKNOWN )
        simType = ST_TRAN;

    unconnected.Replace( '(', '_' ); // Convert to SPICE markup

    SIM_PLOT_TAB* curPlotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
    bool          smithMode = curPlotTab && curPlotTab->GetSimType() == ST_SP && curPlotTab->IsSmithMode();

    auto addSignal = [&]( const wxString& aSignalName )
    {
        if( simType == ST_AC )
        {
            m_signals.push_back( aSignalName + _( " (gain)" ) );
            m_signals.push_back( aSignalName + _( " (phase)" ) );
        }
        else if( simType == ST_SP )
        {
            if( smithMode )
            {
                m_signals.push_back( aSignalName );
            }
            else
            {
                m_signals.push_back( aSignalName + _( " (amplitude)" ) );
                m_signals.push_back( aSignalName + _( " (phase)" ) );
            }
        }
        else
        {
            m_signals.push_back( aSignalName );
        }
    };

    // Quoting needs every net, whatever the analysis or saved signals.
    for( const wxString& netname : circuitModel()->GetNets() )
    {
        if( netname.IsSameAs( wxS( "GND" ), false ) || netname == "0" || netname.StartsWith( unconnected ) )
            continue;

        m_netnames.emplace_back( netname );
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
        && ( simType == ST_TRAN || simType == ST_DC || simType == ST_AC || simType == ST_FFT ) )
    {
        for( const wxString& netname : circuitModel()->GetNets() )
        {
            if( netname.IsSameAs( wxS( "GND" ), false ) || netname == "0" || netname.StartsWith( unconnected ) )
                continue;

            addSignal( wxString::Format( wxS( "V(%s)" ), netname ) );
        }
    }

    if( simType == ST_TRAN || simType == ST_DC || simType == ST_AC || simType == ST_FFT )
    {
        // Always resolved, so workbook V(R1) traces survive; listed only when all voltages are saved.
        rebuildDeviceVoltages();

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
        {
            for( const auto& [name, id] : m_deviceVoltageIds )
                addSignal( name );
        }
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS )
        && ( simType == ST_TRAN || simType == ST_DC || simType == ST_AC ) )
    {
        for( const SPICE_ITEM& item : circuitModel()->GetItems() )
        {
            // Add all possible currents for the device.
            for( const std::string& name : item.model->SpiceGenerator().CurrentNames( item ) )
                addSignal( name );
        }
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS )
        && ( simType == ST_TRAN || simType == ST_DC ) )
    {
        for( const SPICE_ITEM& item : circuitModel()->GetItems() )
        {
            if( item.model->GetPinCount() >= 2 )
            {
                wxString name = item.model->SpiceGenerator().ItemName( item );
                addSignal( wxString::Format( wxS( "P(%s)" ), name ) );
            }
        }
    }

    if( simType == ST_NOISE )
    {
        addSignal( wxS( "inoise_spectrum" ) );
        addSignal( wxS( "onoise_spectrum" ) );
    }

    if( simType == ST_SP )
    {
        std::vector<std::string> portnums;

        for( const SPICE_ITEM& item : circuitModel()->GetItems() )
        {
            wxString name = item.model->SpiceGenerator().ItemName( item );

            // We are only looking for voltage sources in .SP mode
            if( !name.StartsWith( "V" ) )
                continue;

            std::string portnum = "";

            if( const SIM_MODEL::PARAM* portnum_param = item.model->FindParam( "portnum" ) )
                portnum = SIM_VALUE::ToSpice( portnum_param->value );

            if( portnum != "" )
                portnums.push_back( portnum );
        }

        for( const std::string& portnum1 : portnums )
        {
            for( const std::string& portnum2 : portnums )
            {
                // the Smith chart only makes sense for reflection parameters (S_i_i), the
                // transmission ones are not impedances
                if( smithMode && portnum1 != portnum2 )
                    continue;

                addSignal( wxString::Format( wxS( "S_%s_%s" ), portnum1, portnum2 ) );
            }
        }
    }

    // Add .SAVE and .PROBE directives
    for( const wxString& directive : circuitModel()->GetDirectives() )
    {
        wxStringTokenizer directivesTokenizer( directive, "\r\n", wxTOKEN_STRTOK );

        while( directivesTokenizer.HasMoreTokens() )
        {
            wxString line = directivesTokenizer.GetNextToken().Upper();
            wxString directiveParams;

            if( line.StartsWith( wxS( ".SAVE" ), &directiveParams )
                    || line.StartsWith( wxS( ".PROBE" ), &directiveParams ) )
            {
                wxStringTokenizer paramsTokenizer( directiveParams, " \t", wxTOKEN_STRTOK );

                while( paramsTokenizer.HasMoreTokens() )
                    addSignal( paramsTokenizer.GetNextToken() );
            }
        }
    }
}


SIM_TAB* SIMULATOR_FRAME_UI::NewSimTab( const wxString& aSimCommand )
{
    SIM_TAB* simTab = nullptr;
    SIM_TYPE simType = SPICE_CIRCUIT_MODEL::CommandToSimType( aSimCommand );

    if( SIM_TAB::IsPlottable( simType ) )
    {
        SIM_PLOT_TAB* panel = new SIM_PLOT_TAB( aSimCommand, m_plotNotebook );
        simTab = panel;
        panel->ApplyPreferences( m_preferences );
    }
    else
    {
        simTab = new SIM_NOPLOT_TAB( aSimCommand, m_plotNotebook );
    }

    wxString pageTitle( simulator()->TypeToName( simType, true ) );
    pageTitle.Prepend( wxString::Format( _( "Analysis %u - " ), static_cast<unsigned int>( ++m_plotNumber ) ) );

    m_plotNotebook->AddPage( simTab, pageTitle, true );

    return simTab;
}


void SIMULATOR_FRAME_UI::OnFilterText( wxCommandEvent& aEvent )
{
    rebuildSignalsTree( m_filter->GetValue() );
}


void SIMULATOR_FRAME_UI::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


wxString vectorNameFromSignalId( int aUserDefinedSignalId )
{
    return wxString::Format( wxS( "user%d" ), aUserDefinedSignalId );
}




static wxString deviceVectorName( int aDeviceVoltageId )
{
    return wxString::Format( wxS( "%s%d" ), SIM_SIGNAL::DEVICE_VECTOR_PREFIX, aDeviceVoltageId );
}


/**
 * For user-defined signals we display the user-oriented signal name such as "V(out)-V(in)",
 * but the simulator vector we actually have to plot will be "user0" or some-such.
 */
wxString SIMULATOR_FRAME_UI::vectorNameFromSignalName( SIM_PLOT_TAB* aPlotTab, const wxString& aSignalName,
                                                       int* aTraceType )
{
    auto looksLikePower = []( const wxString& aExpression ) -> bool
    {
        wxString exprUpper = aExpression.Upper();

        if( exprUpper.Contains( wxS( ":POWER" ) ) )
            return true;

        if( exprUpper.Find( '*' ) == wxNOT_FOUND )
            return false;

        if( !exprUpper.Contains( wxS( "V(" ) ) )
            return false;

        if( !exprUpper.Contains( wxS( "I(" ) ) )
            return false;

        return true;
    };

    std::map<wxString, int> suffixes;
    suffixes[_( " (amplitude)" )] = SPT_SP_AMP;
    suffixes[_( " (gain)" )] = SPT_AC_GAIN;
    suffixes[_( " (phase)" )] = SPT_AC_PHASE;

    if( aTraceType )
    {
        if( aPlotTab && aPlotTab->GetSimType() == ST_NOISE )
        {
            if( getNoiseSource().Upper().StartsWith( 'I' ) )
                *aTraceType = SPT_CURRENT;
            else
                *aTraceType = SPT_VOLTAGE;
        }
        else
        {
            wxUniChar firstChar = aSignalName.Upper()[0];

            if( firstChar == 'V' )
                *aTraceType = SPT_VOLTAGE;
            else if( firstChar == 'I' )
                *aTraceType = SPT_CURRENT;
            else if( firstChar == 'P' )
                *aTraceType = SPT_POWER;
        }
    }

    wxString name = aSignalName;

    for( const auto& [candidate, type] : suffixes )
    {
        if( name.EndsWith( candidate ) )
        {
            name = name.Left( name.Length() - candidate.Length() );

            if( aTraceType )
                *aTraceType |= type;

            break;
        }
    }

    // smith mode rows carry no suffix, the trace holds the complex reflection coefficient
    if( aTraceType && aPlotTab && aPlotTab->GetSimType() == ST_SP && aPlotTab->IsSmithMode()
        && !( *aTraceType & ( SPT_SP_AMP | SPT_AC_GAIN | SPT_AC_PHASE ) ) )
    {
        *aTraceType |= SPT_SP_SMITH;
    }

    if( auto it = m_deviceVoltageIds.find( name ); it != m_deviceVoltageIds.end() )
    {
        if( aTraceType )
            *aTraceType = ( *aTraceType & ( SPT_SP_MASK | SPT_AC_GAIN | SPT_AC_PHASE ) ) | SPT_VOLTAGE;

        return deviceVectorName( it->second );
    }

    for( const auto& [id, signal] : m_userDefinedSignals )
    {
        if( name == signal )
        {
            if( aTraceType && looksLikePower( signal ) )
            {
                int suffixBits = *aTraceType & ( SPT_SP_MASK | SPT_AC_GAIN );
                *aTraceType = suffixBits | SPT_POWER;
            }

            return vectorNameFromSignalId( id );
        }
    }

    return name;
};


wxString SIMULATOR_FRAME_UI::signalNameFromVectorName( const wxString& aVectorName ) const
{
    wxString suffix;

    if( aVectorName.EndsWith( _( " (phase)" ) ) )
        suffix = _( " (phase)" );
    else if( aVectorName.EndsWith( _( " (gain)" ) ) )
        suffix = _( " (gain)" );
    else if( aVectorName.EndsWith( _( " (amplitude)" ) ) )
        suffix = _( " (amplitude)" );

    wxString vectorName = aVectorName.Left( aVectorName.Length() - suffix.Length() );

    for( const auto& [id, signal] : m_userDefinedSignals )
    {
        if( vectorName == vectorNameFromSignalId( id ) )
            return signal + suffix;
    }

    // The name is the stable identity; ids are assigned per session.
    for( const auto& [name, id] : m_deviceVoltageIds )
    {
        if( vectorName == deviceVectorName( id ) )
            return name + suffix;
    }

    return aVectorName;
}


void SIMULATOR_FRAME_UI::onSignalsTreeActivated( wxDataViewEvent& aEvent )
{
    SIM_PLOT_TAB*         plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
    SIM_TREE_MODEL::NODE* node = SIM_TREE_MODEL::ToNode( aEvent.GetItem() );

    if( !plotTab || !node )
        return;

    if( node->IsGroup() )
    {
        wxDataViewItem item = aEvent.GetItem();

        if( m_signalsTree->IsExpanded( item ) )
            m_signalsTree->Collapse( item );
        else
            m_signalsTree->Expand( item );

        return;
    }

    if( node->plotted )
    {
        setSignalView( node->name, nullptr );
    }
    else if( SIM_VIEW* view = lastSignalView( plotTab, node->name ) )
    {
        setSignalView( node->name, view );
    }
    else
    {
        return;
    }

    for( SIM_VIEW* view : plotTab->GetViews() )
        view->UpdateAll();

    updateSignalsTree();
    updatePlotCursors();
    OnModify();
}


static const std::vector<wxString>& measurementFunctions();
static wxString measurementFunctionLabel( const wxString& aFunc, SIM_TYPE aSimType );


void SIMULATOR_FRAME_UI::onSignalsTreeContextMenu( wxDataViewEvent& aEvent )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    m_signalsMenuSignals = getSelectedSignals( aEvent.GetItem() );

    // The menu acts on the clicked row, so show it selected
    if( aEvent.GetItem().IsOk() && !m_signalsTree->IsSelected( aEvent.GetItem() ) )
    {
        m_signalsTree->UnselectAll();
        m_signalsTree->Select( aEvent.GetItem() );
    }

    wxMenu menu;

    if( !m_signalsMenuSignals.empty() )
    {
        const wxString& first = m_signalsMenuSignals.front();
        int             traceType = SPT_UNKNOWN;
        wxString        vectorName = vectorNameFromSignalName( plotTab, first, &traceType );
        TRACE*          trace = plotTab->GetTrace( vectorName, traceType );
        bool            single = m_signalsMenuSignals.size() == 1;

        wxMenu*       viewMenu = new wxMenu;
        wxArrayString viewChoices = getViewChoices( plotTab );

        for( size_t ii = 0; ii < viewChoices.GetCount(); ++ii )
        {
            wxMenuItem* item = viewMenu->AppendCheckItem( MYID_TREE_VIEW_BASE + (int) ii, viewChoices[ii] );

            if( single )
            {
                SIM_VIEW* view = trace ? trace->GetView() : nullptr;
                item->Check( getViewLabel( plotTab, view ) == viewChoices[ii] );
            }
        }

        menu.AppendSubMenu( viewMenu, _( "Plot On" ) );

        if( single && trace && trace->GetView() )
        {
            wxMenu*       scaleMenu = new wxMenu;
            wxArrayString scaleChoices = getYScaleChoices( plotTab, trace->GetView() );

            for( size_t ii = 0; ii < scaleChoices.GetCount(); ++ii )
            {
                wxMenuItem* item = scaleMenu->AppendCheckItem( MYID_TREE_YSCALE_BASE + (int) ii, scaleChoices[ii] );
                item->Check( getYScaleLabel( plotTab, trace ) == scaleChoices[ii] );
            }

            menu.AppendSubMenu( scaleMenu, _( "Y Scale" ) );
            menu.Append( MYID_TREE_COLOR, _( "Color..." ) );

            menu.AppendSeparator();

            for( int id = 1; id < m_customCursorsCnt; ++id )
            {
                wxMenuItem* item =
                        menu.AppendCheckItem( MYID_TREE_CURSOR_BASE + id, wxString::Format( _( "Cursor %d" ), id ) );
                item->Check( trace->HasCursor( id ) );
            }
        }

        menu.AppendSeparator();

        SIM_TYPE simType = plotTab->GetSimType();

        if( simType == ST_TRAN || simType == ST_AC || simType == ST_DC || simType == ST_SP )
        {
            wxMenu* measureMenu = new wxMenu;

            for( size_t ii = 0; ii < measurementFunctions().size(); ++ii )
            {
                measureMenu->Append( MYID_TREE_MEASURE_BASE + (int) ii,
                                     measurementFunctionLabel( measurementFunctions()[ii], simType ) );
            }

            measureMenu->AppendSeparator();
            measureMenu->Append( MYID_MEASURE_ALL, _( "All" ) );

            wxMenuItem* measureItem = menu.AppendSubMenu( measureMenu, _( "Add Measurement" ) );
            wxString    measured;

            // Phase traces have no MEAS equivalent.
            measureItem->Enable( std::any_of( m_signalsMenuSignals.begin(), m_signalsMenuSignals.end(),
                                              [&]( const wxString& aSignal )
                                              {
                                                  return measurementSignal( aSignal, measured );
                                              } ) );

            if( simType == ST_TRAN )
            {
                menu.AppendSeparator();
                menu.Append( MYID_FOURIER, _( "Perform Fourier Analysis..." ) );
            }

            menu.AppendSeparator();
        }

        menu.Append( MYID_TREE_COPY, _( "Copy Signal Name" ) + wxS( "\tCtrl+C" ) );
        menu.AppendSeparator();
    }

    menu.Append( MYID_TREE_NEW_CURSOR, _( "Create new cursor..." ) );

    if( m_customCursorsCnt > (int) std::size( m_cursorFormats ) )
    {
        menu.Append( MYID_TREE_DELETE_CURSOR, wxString::Format( _( "Delete Cursor %d..." ), m_customCursorsCnt - 1 ) );
    }

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &SIMULATOR_FRAME_UI::onSignalsTreeMenuSelection, this );

    m_signalsTree->PopupMenu( &menu );
}


void SIMULATOR_FRAME_UI::onSignalsTreeMenuSelection( wxCommandEvent& aEvent )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    const std::vector<wxString>& signals = m_signalsMenuSignals;
    int                          id = aEvent.GetId();

    if( id >= MYID_TREE_VIEW_BASE && id < MYID_TREE_VIEW_BASE + 100 )
    {
        int       choiceIndex = id - MYID_TREE_VIEW_BASE;
        SIM_VIEW* targetView = choiceIndex <= 0 ? nullptr : plotTab->GetView( choiceIndex - 1 );

        if( choiceIndex > 0 && !targetView )
            return;

        for( const wxString& signal : signals )
            setSignalView( signal, targetView );

        for( SIM_VIEW* view : plotTab->GetViews() )
            view->UpdateAll();

        updateSignalsTree();
        updatePlotCursors();
        OnModify();
    }
    else if( id >= MYID_TREE_YSCALE_BASE && id < MYID_TREE_YSCALE_BASE + 100 )
    {
        if( signals.empty() )
            return;

        int      traceType = SPT_UNKNOWN;
        wxString vectorName = vectorNameFromSignalName( plotTab, signals.front(), &traceType );
        TRACE*   trace = plotTab->GetTrace( vectorName, traceType );

        if( !trace )
            return;

        std::vector<SIM_VIEW*> targetViews = getYScaleTargetViews( plotTab, trace->GetView() );
        int                    choiceIndex = id - MYID_TREE_YSCALE_BASE;

        if( choiceIndex == 0 )
            trace->SetYScaleView( trace->GetView() );
        else if( choiceIndex - 1 < (int) targetViews.size() )
            trace->SetYScaleView( targetViews[choiceIndex - 1] );
        else
            return;

        plotTab->ResetScales( false );

        for( SIM_VIEW* view : plotTab->GetViews() )
            view->UpdateAll();

        OnModify();
    }
    else if( id >= MYID_TREE_CURSOR_BASE && id < MYID_TREE_CURSOR_BASE + 100 )
    {
        if( signals.empty() )
            return;

        int      cursorId = id - MYID_TREE_CURSOR_BASE;
        int      traceType = SPT_UNKNOWN;
        wxString vectorName = vectorNameFromSignalName( plotTab, signals.front(), &traceType );
        TRACE*   activeTrace = plotTab->GetTrace( vectorName, traceType );

        // Cursors act as radio buttons: enabling one takes it off every other signal.
        bool enable = !( activeTrace && activeTrace->HasCursor( cursorId ) );

        if( enable && activeTrace )
        {
            plotTab->EnableCursor( activeTrace, cursorId, signals.front() );
            OnModify();
        }
        else
        {
            activeTrace = nullptr;
        }

        for( const auto& [name, trace] : plotTab->GetTraces() )
        {
            if( trace != activeTrace && trace->HasCursor( cursorId ) )
            {
                plotTab->DisableCursor( trace, cursorId );
                OnModify();
            }
        }

        updateSignalsTree();
        updatePlotCursors();
    }
    else if( id == MYID_TREE_COLOR )
    {
        if( signals.empty() )
            return;

        int      traceType = SPT_UNKNOWN;
        wxString vectorName = vectorNameFromSignalName( plotTab, signals.front(), &traceType );
        TRACE*   trace = plotTab->GetTrace( vectorName, traceType );

        if( !trace )
            return;

        wxColourData data;
        data.SetColour( trace->GetPen().GetColour() );

        wxColourDialog dlg( this, &data );

        if( dlg.ShowModal() != wxID_OK )
            return;

        trace->SetTraceColour( dlg.GetColourData().GetColour() );
        plotTab->UpdateTraceStyle( trace );
        plotTab->UpdatePlotColors();
        updateSignalsTree();
        OnModify();
    }
    else if( id == MYID_TREE_COPY )
    {
        copySignalNames( signals );
    }
    else if( id == MYID_TREE_NEW_CURSOR )
    {
        CreateNewCursor();
    }
    else if( id == MYID_TREE_DELETE_CURSOR )
    {
        DeleteCursor();
    }
    else if( id == MYID_MEASURE_ALL )
    {
        for( const wxString& signal : signals )
            AddMeasurements( signal, measurementFunctions() );
    }
    else if( id >= MYID_TREE_MEASURE_BASE && id < MYID_TREE_MEASURE_BASE + (int) measurementFunctions().size() )
    {
        for( const wxString& signal : signals )
            AddMeasurements( signal, { measurementFunctions()[id - MYID_TREE_MEASURE_BASE] } );
    }
    else if( id == MYID_FOURIER )
    {
        wxString title;
        wxString fundamental = wxT( "1K" );

        if( signals.size() == 1 )
            title.Printf( _( "Fourier Analysis of %s" ), signals[0] );
        else
            title = _( "Fourier Analyses of Multiple Signals" );

        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Fundamental frequency:" ), title, fundamental );

        if( dlg.ShowModal() != wxID_OK )
            return;

        if( !dlg.GetValue().IsEmpty() )
            fundamental = dlg.GetValue();

        for( const wxString& signal : signals )
            DoFourier( signal, fundamental );

        // The results are only printed to the console.
        if( !IsSimConsoleShown() )
            ToggleSimConsole();

        FlushSimConsole();
    }
}


void SIMULATOR_FRAME_UI::onCursorsGridCellChanged( wxGridEvent& aEvent )
{
    if( m_SuppressGridEvents > 0 )
        return;

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    int      row = aEvent.GetRow();
    int      col = aEvent.GetCol();
    wxString text = m_cursorsGrid->GetCellValue( row, col );
    wxString cursorName = m_cursorsGrid->GetCellValue( row, COL_CURSOR_NAME );

    double value = SPICE_VALUE( text ).ToDouble();

    if( col == COL_CURSOR_X )
    {
        CURSOR* cursor1 = nullptr;
        CURSOR* cursor2 = nullptr;

        std::vector<CURSOR*> cursorsVec;
        cursorsVec.clear();

        for( const auto& [name, trace] : plotTab->GetTraces() )
        {
            if( CURSOR* cursor = trace->GetCursor( 1 ) )
                cursor1 = cursor;

            if( CURSOR* cursor = trace->GetCursor( 2 ) )
                cursor2 = cursor;

            int tmp = 3;

            if( !cursor1 )
                tmp--;
            if( !cursor2 )
                tmp--;

            for( int i = tmp; i < m_customCursorsCnt; i++ )
            {
                if( CURSOR* cursor = trace->GetCursor( i ) )
                {
                    cursorsVec.emplace_back( cursor );

                    if( cursorName == ( wxString( "" ) << i ) && cursor )
                        cursor->SetCoordX( value );
                }
            }
        }

        //double value = SPICE_VALUE( text ).ToDouble();

        if( cursorName == wxS( "1" ) && cursor1 )
            cursor1->SetCoordX( value );
        else if( cursorName == wxS( "2" ) && cursor2 )
            cursor2->SetCoordX( value );
        else if( cursorName == _( "Diff" ) && cursor1 && cursor2 )
            cursor2->SetCoordX( cursor1->GetCoords().x + value );

        updatePlotCursors();
        OnModify();
    }
    else
    {
        wxFAIL_MSG( wxT( "All other columns are supposed to be read-only!" ) );
    }
}


static const std::vector<wxString>& measurementFunctions()
{
    static const std::vector<wxString> funcs = { wxS( "MIN" ), wxS( "MAX" ),    wxS( "AVG" ),    wxS( "RMS" ),
                                                 wxS( "PP" ),  wxS( "MIN_AT" ), wxS( "MAX_AT" ), wxS( "INTEG" ) };
    return funcs;
}


static wxString measurementFunctionLabel( const wxString& aFunc, SIM_TYPE aSimType )
{
    bool     freqDomain = aSimType == ST_AC || aSimType == ST_SP;
    wxString func = aFunc.Upper();

    if( func == wxS( "MIN" ) )
        return _( "Min" );
    else if( func == wxS( "MAX" ) )
        return _( "Max" );
    else if( func == wxS( "AVG" ) )
        return _( "Average" );
    else if( func == wxS( "RMS" ) )
        return _( "RMS" );
    else if( func == wxS( "PP" ) )
        return _( "Peak-to-peak" );
    else if( func == wxS( "MIN_AT" ) )
        return freqDomain ? _( "Frequency of Min" ) : _( "Time of Min" );
    else if( func == wxS( "MAX_AT" ) )
        return freqDomain ? _( "Frequency of Max" ) : _( "Time of Max" );
    else if( func == wxS( "INTEG" ) )
        return _( "Integral" );

    return aFunc;
}


bool SIMULATOR_FRAME_UI::measurementSignal( const wxString& aSignal, wxString& aResult ) const
{
    // Suffixes are translated, so their length varies.
    if( aSignal.EndsWith( _( " (phase)" ) ) )
        return false;

    wxString suffix;

    if( aSignal.EndsWith( _( " (gain)" ) ) )
        suffix = _( " (gain)" );
    else if( aSignal.EndsWith( _( " (amplitude)" ) ) )
        suffix = _( " (amplitude)" );

    SIM_SIGNAL::PART part = SIM_SIGNAL::PART::VALUE;

    if( suffix == _( " (gain)" ) )
        part = SIM_SIGNAL::PART::GAIN;
    else if( suffix == _( " (amplitude)" ) )
        part = SIM_SIGNAL::PART::AMPLITUDE;

    return SIM_SIGNAL::MeasurableSignal( aSignal.Left( aSignal.Length() - suffix.Length() ), part, aResult );
}


SPICE_VALUE_FORMAT SIMULATOR_FRAME_UI::GetMeasureFormat( int aRow ) const
{
    SPICE_VALUE_FORMAT result;

    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        const std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

        if( aRow >= 0 && aRow < (int) measurements.size() )
            result.FromString( measurements[aRow].second );
    }

    return result;
}


void SIMULATOR_FRAME_UI::SetMeasureFormat( int aRow, const SPICE_VALUE_FORMAT& aFormat )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

        if( aRow >= 0 && aRow < (int) measurements.size() )
            measurements[aRow].second = aFormat.ToString();
    }
}


void SIMULATOR_FRAME_UI::DeleteMeasurement( int aIndex )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

        if( aIndex >= 0 && aIndex < (int) measurements.size() )
            measurements.erase( measurements.begin() + aIndex );
    }
}


std::map<wxString, wxString> SIMULATOR_FRAME_UI::shownMeasurementValues() const
{
    std::map<wxString, wxString> values;
    SIM_PLOT_TAB*                plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab || !m_measurementsModel )
        return values;

    const std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

    for( SIM_TREE_MODEL::NODE* leaf : m_measurementsModel->Leaves() )
    {
        if( leaf->index >= 0 && leaf->index < (int) measurements.size() )
            values[measurements[leaf->index].first] = leaf->value;
    }

    return values;
}


void SIMULATOR_FRAME_UI::rebuildMeasurementsTree( const std::map<wxString, wxString>* aKeepValues )
{
    if( !m_measurementsModel )
        return;

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
    {
        m_measurementsModel->SetContents( {} );
        return;
    }

    SIM_TYPE simType = plotTab->GetSimType();

    std::vector<wxString>                                  order;
    std::map<wxString, std::vector<SIM_TREE_MODEL::ENTRY>> bySignal;

    const std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

    for( int ii = 0; ii < (int) measurements.size(); ++ii )
    {
        wxString func, signal, params;
        SIM_SIGNAL::SplitMeasurement( measurements[ii].first, func, signal, params );

        if( signal.IsEmpty() ) // a measurement we can't parse; file it under its own text
            signal = measurements[ii].first;

        // ngspice ignores case, so "MAX v(d1)" joins the V(D1) group under its first spelling.
        wxString key = signal.Lower();

        if( bySignal.find( key ) == bySignal.end() )
            order.push_back( signal );

        bySignal[key].push_back( { measurementFunctionLabel( func, simType ), ii } );
    }

    std::vector<SIM_TREE_MODEL::GROUP> groups;

    for( const wxString& signal : order )
    {
        SIM_TREE_MODEL::GROUP group;
        group.name = signal;
        group.entries = bySignal[signal.Lower()];
        groups.push_back( group );
    }

    // Measurement indices may already be stale, so rows are matched by their labels
    auto rowKey = []( SIM_TREE_MODEL::NODE* aNode ) -> wxString
    {
        if( !aNode )
            return wxEmptyString;

        return aNode->parent ? aNode->parent->name + wxS( "\n" ) + aNode->name : aNode->name;
    };

    wxDataViewItemArray selection;
    std::set<wxString>  selectedKeys;
    wxString            topKey = rowKey( SIM_TREE_MODEL::ToNode( m_measurementsTree->GetTopItem() ) );

    m_measurementsTree->GetSelections( selection );

    for( const wxDataViewItem& item : selection )
        selectedKeys.insert( rowKey( SIM_TREE_MODEL::ToNode( item ) ) );

    std::set<wxString> collapsed;

    for( SIM_TREE_MODEL::NODE* group : m_measurementsModel->Groups() )
    {
        if( !m_measurementsTree->IsExpanded( SIM_TREE_MODEL::ToItem( group ) ) )
            collapsed.insert( group->name.Lower() );
    }

    m_measurementsModel->SetContents( groups );

    // Each MEAS is an ngspice round trip and a console echo; evaluate only new ones.
    std::vector<int> toEvaluate;

    for( SIM_TREE_MODEL::NODE* leaf : m_measurementsModel->Leaves() )
    {
        if( aKeepValues )
        {
            if( auto kept = aKeepValues->find( measurements[leaf->index].first ); kept != aKeepValues->end() )
            {
                leaf->value = kept->second;
                continue;
            }
        }

        toEvaluate.push_back( leaf->index );
    }

    // Seed the group dots before expanding; notifying container rows inflates them in wxGTK.
    refreshMeasurementGroupIcons();

    for( SIM_TREE_MODEL::NODE* group : m_measurementsModel->Groups() )
    {
        if( !collapsed.count( group->name.Lower() ) )
            m_measurementsTree->Expand( SIM_TREE_MODEL::ToItem( group ) );
    }

    applyTreeRowHeight();

    // Cleared() drops the selection, and on macOS also scrolls to the top
    wxDataViewItem top;
    wxDataViewItem firstSelected;
    wxDataViewItem last;

    for( SIM_TREE_MODEL::NODE* group : m_measurementsModel->Groups() )
    {
        if( rowKey( group ) == topKey )
            top = SIM_TREE_MODEL::ToItem( group );

        last = SIM_TREE_MODEL::ToItem( group );

        // Selecting or scrolling to a hidden row would expand its group again
        if( collapsed.count( group->name.Lower() ) )
            continue;

        for( const std::unique_ptr<SIM_TREE_MODEL::NODE>& leaf : group->children )
        {
            if( leaf->IsGroup() )
                continue;

            wxString key = rowKey( leaf.get() );

            if( selectedKeys.count( key ) )
            {
                m_measurementsTree->Select( SIM_TREE_MODEL::ToItem( leaf.get() ) );

                if( !firstSelected.IsOk() )
                    firstSelected = SIM_TREE_MODEL::ToItem( leaf.get() );
            }

            if( key == topKey )
                top = SIM_TREE_MODEL::ToItem( leaf.get() );

            last = SIM_TREE_MODEL::ToItem( leaf.get() );
        }
    }

    // EnsureVisible() scrolls the least, so coming from the end leaves the old top row at the top
    if( top.IsOk() && last.IsOk() )
    {
        m_measurementsTree->EnsureVisible( last );
        m_measurementsTree->EnsureVisible( top );
    }
    else if( firstSelected.IsOk() )
    {
        m_measurementsTree->EnsureVisible( firstSelected );
    }

    for( int index : toEvaluate )
        UpdateMeasurement( index );
}


bool SIMULATOR_FRAME_UI::refreshMeasurementGroupIcons()
{
    if( !m_measurementsModel )
        return false;

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
    SIM_TYPE      simType = plotTab ? plotTab->GetSimType() : ST_UNKNOWN;
    bool          magnitude = simType == ST_AC || ( simType == ST_SP && !plotTab->IsSmithMode() );
    bool          changed = false;

    // No notification: see rebuildMeasurementsTree().
    for( SIM_TREE_MODEL::NODE* group : m_measurementsModel->Groups() )
    {
        wxString signal = group->name;

        // Groups are named by the MEAS signal; undo measurementSignal() to find the plotted trace.
        if( magnitude )
        {
            if( signal.Upper().StartsWith( wxS( "VDB(" ) ) )
                signal = wxS( "V" ) + signal.Mid( 3 );
            else if( simType == ST_AC && signal.StartsWith( wxS( "db(" ) ) && signal.EndsWith( wxS( ")" ) ) )
                signal = signal.Mid( 3, signal.Length() - 4 );
            else if( simType == ST_SP && signal.StartsWith( wxS( "mag(" ) ) && signal.EndsWith( wxS( ")" ) ) )
                signal = signal.Mid( 4, signal.Length() - 5 );

            signal += simType == ST_AC ? _( " (gain)" ) : _( " (amplitude)" );
        }

        int      traceType = SPT_UNKNOWN;
        wxString vectorName = vectorNameFromSignalName( plotTab, signal, &traceType );
        TRACE*   trace = plotTab ? plotTab->GetTrace( vectorName, traceType ) : nullptr;

        // A typed measurement may differ in case from the plotted signal.
        if( !trace && plotTab )
        {
            for( const auto& [id, candidate] : plotTab->GetTraces() )
            {
                if( candidate->GetDisplayName().IsSameAs( signal, false ) )
                {
                    trace = candidate;
                    break;
                }
            }
        }

        bool     plotted = trace && trace->GetView();
        wxColour color = plotted ? trace->GetPen().GetColour() : wxColour();

        if( group->plotted != plotted || group->color != color || !group->icon.IsOk() )
        {
            group->icon = signalDot( color, plotted );
            group->plotted = plotted;
            group->color = color;
            changed = true;
        }
    }

    return changed;
}


void SIMULATOR_FRAME_UI::updateMeasurementValues()
{
    if( !m_measurementsModel )
        return;

    for( SIM_TREE_MODEL::NODE* leaf : m_measurementsModel->Leaves() )
        UpdateMeasurement( leaf->index );
}


void SIMULATOR_FRAME_UI::updateMeasurements()
{
    if( !m_measurementsModel )
        return;

    // Reload instead of notifying container rows, which inflates them.
    if( refreshMeasurementGroupIcons() )
        rebuildMeasurementsTree();
    else
        updateMeasurementValues();
}


std::vector<int> SIMULATOR_FRAME_UI::getSelectedMeasurements( const wxDataViewItem& aClickedItem ) const
{
    std::vector<int>    rows;
    wxDataViewItemArray selection;

    m_measurementsTree->GetSelections( selection );

    for( const wxDataViewItem& item : selection )
    {
        SIM_TREE_MODEL::NODE* node = SIM_TREE_MODEL::ToNode( item );

        if( node && !node->IsGroup() )
            rows.push_back( node->index );
    }

    // Right-clicking a group acts on all its measurements.
    SIM_TREE_MODEL::NODE* clicked = SIM_TREE_MODEL::ToNode( aClickedItem );

    if( clicked && clicked->IsGroup() )
    {
        rows.clear();

        for( const std::unique_ptr<SIM_TREE_MODEL::NODE>& child : clicked->children )
            rows.push_back( child->index );
    }
    else if( clicked && std::find( rows.begin(), rows.end(), clicked->index ) == rows.end() )
    {
        rows.clear();
        rows.push_back( clicked->index );
    }

    return rows;
}


void SIMULATOR_FRAME_UI::onMeasurementsTreeContextMenu( wxDataViewEvent& aEvent )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    m_measurementsMenuRows = getSelectedMeasurements( aEvent.GetItem() );

    // The menu acts on the clicked row, so show it selected
    if( aEvent.GetItem().IsOk() && !m_measurementsTree->IsSelected( aEvent.GetItem() ) )
    {
        m_measurementsTree->UnselectAll();
        m_measurementsTree->Select( aEvent.GetItem() );
    }

    wxMenu menu;

    if( m_measurementsMenuRows.size() == 1 )
    {
        menu.Append( MYID_MEASUREMENT_EDIT, _( "Edit Measurement..." ) );
        menu.Append( MYID_FORMAT_VALUE, _( "Format Value..." ) );
    }

    if( !m_measurementsMenuRows.empty() )
    {
        menu.Append( MYID_DELETE_MEASUREMENT,
                     wxPLURAL( "Delete Measurement", "Delete Measurements", m_measurementsMenuRows.size() ) );
        menu.AppendSeparator();
    }

    menu.Append( MYID_MEASUREMENT_ADD, _( "Add Measurement..." ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &SIMULATOR_FRAME_UI::onMeasurementsTreeMenuSelection, this );

    m_measurementsTree->PopupMenu( &menu );
}


void SIMULATOR_FRAME_UI::onMeasurementsTreeMenuSelection( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == MYID_FORMAT_VALUE )
    {
        if( m_measurementsMenuRows.size() != 1 )
            return;

        int                     row = m_measurementsMenuRows.front();
        SPICE_VALUE_FORMAT      format = GetMeasureFormat( row );
        DIALOG_SIM_FORMAT_VALUE formatDialog( this, &format );

        if( formatDialog.ShowModal() == wxID_OK )
        {
            SetMeasureFormat( row, format );
            UpdateMeasurement( row );
            OnModify();
        }
    }
    else if( aEvent.GetId() == MYID_MEASUREMENT_EDIT )
    {
        if( m_measurementsMenuRows.size() == 1 )
            editMeasurement( m_measurementsMenuRows.front() );
    }
    else if( aEvent.GetId() == MYID_DELETE_MEASUREMENT )
    {
        deleteMeasurements( m_measurementsMenuRows );
    }
    else if( aEvent.GetId() == MYID_MEASUREMENT_ADD )
    {
        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Measurement:" ), _( "Add Measurement" ), wxEmptyString );

        if( dlg.ShowModal() != wxID_OK || dlg.GetValue().IsEmpty() )
            return;

        AddMeasurement( dlg.GetValue() );
    }
}


void SIMULATOR_FRAME_UI::onMeasurementsTreeActivated( wxDataViewEvent& aEvent )
{
    SIM_TREE_MODEL::NODE* node = SIM_TREE_MODEL::ToNode( aEvent.GetItem() );

    if( node && !node->IsGroup() )
        editMeasurement( node->index );
}


void SIMULATOR_FRAME_UI::editMeasurement( int aRow )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab || aRow < 0 || aRow >= (int) plotTab->Measurements().size() )
        return;

    wxString             current = plotTab->Measurements()[aRow].first;
    WX_TEXT_ENTRY_DIALOG dlg( this, _( "Measurement:" ), _( "Edit Measurement" ), current );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString text = dlg.GetValue();
    text.Trim( true ).Trim( false );

    if( text.IsEmpty() || text == current )
        return;

    std::map<wxString, wxString> values = shownMeasurementValues();

    plotTab->Measurements()[aRow].first = text;

    rebuildMeasurementsTree( &values );
    OnModify();
}


void SIMULATOR_FRAME_UI::deleteMeasurements( std::vector<int> aRows )
{
    if( aRows.empty() )
        return;

    std::map<wxString, wxString> values = shownMeasurementValues();

    std::sort( aRows.begin(), aRows.end(), std::greater<>() );

    for( int row : aRows )
        DeleteMeasurement( row );

    rebuildMeasurementsTree( &values );
    OnModify();
}


void SIMULATOR_FRAME_UI::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        mpWindow* plotWin = plotTab->GetPlotWin();
        wxSize    plotWinSize = plotWin ? plotWin->GetSize() : wxSize();

        if( plotWinSize != plotTab->m_LastPlotWinSize )
        {
            // A resize moves the legend too; don't report that as a drag.
            plotTab->m_LastPlotWinSize = plotWinSize;
            plotTab->m_LastLegendPosition = plotTab->GetLegendPosition();
        }
        else if( wxPoint legendPos = plotTab->GetLegendPosition(); legendPos != plotTab->m_LastLegendPosition )
        {
            plotTab->m_LastLegendPosition = legendPos;
            OnModify();
        }
    }
}


/**
 * The user measurement looks something like:
 *    MAX V(out)
 *
 * We need to send ngspice a "MEAS" command with the analysis type, an output variable name,
 * and the signal name.  For our example above, this looks something like:
 *    MEAS TRAN meas_result_0 MAX V(out)
 *
 * This is also a good time to harvest the signal name prefix so we know what units to show on
 * the result.  For instance, for:
 *    MAX P(out)
 * we want to show:
 *    15W
 */
void SIMULATOR_FRAME_UI::UpdateMeasurement( int aRow )
{
    static wxRegEx measureParamsRegEx( wxT( "^"
                                            " *"
                                            "([a-zA-Z_]+)"
                                            " +"
                                            "([a-zA-Z]*)\\(([^\\)]+)\\)" ) );

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    const std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

    if( aRow < 0 || aRow >= (int) measurements.size() )
        return;

    wxString text = measurements[aRow].first;

    if( text.IsEmpty() )
    {
        setMeasurementValue( aRow, wxEmptyString );
        return;
    }

    wxString simType = simulator()->TypeToName( plotTab->GetSimType(), true );
    wxString resultName = wxString::Format( wxS( "meas_result_%u" ), aRow );
    wxString result = wxS( "?" );

    if( measureParamsRegEx.Matches( text ) )
    {
        wxString           func = measureParamsRegEx.GetMatch( text, 1 ).Upper();
        wxString           signalType = measureParamsRegEx.GetMatch( text, 2 ).Upper();
        wxString           deviceName = measureParamsRegEx.GetMatch( text, 3 );
        wxString           units;
        SPICE_VALUE_FORMAT fmt = GetMeasureFormat( aRow );

        if( signalType.EndsWith( wxS( "DB" ) ) )
        {
            units = wxS( "dB" );
        }
        else if( signalType.StartsWith( 'I' ) )
        {
            units = wxS( "A" );
        }
        else if( signalType.StartsWith( 'P' ) )
        {
            units = wxS( "W" );
            // Our syntax is different from ngspice for power signals
            text = func + " " + deviceName + ":power";
        }
        else if( signalType == wxS( "MAG" ) )
        {
            // S-parameters are dimensionless
            if( deviceName.Upper().StartsWith( wxS( "I(" ) ) )
                units = wxS( "A" );
            else if( deviceName.Upper().StartsWith( wxS( "V(" ) ) )
                units = wxS( "V" );
        }
        else
        {
            units = wxS( "V" );
        }

        if( func.EndsWith( wxS( "_AT" ) ) )
        {
            if( plotTab->GetSimType() == ST_AC || plotTab->GetSimType() == ST_SP )
                units = wxS( "Hz" );
            else
                units = wxS( "s" );
        }
        else if( func.StartsWith( wxS( "INTEG" ) ) )
        {
            switch( plotTab->GetSimType() )
            {
            case ST_TRAN:
                if ( signalType.StartsWith( 'P' ) )
                    units = wxS( "J" );
                else
                    units += wxS( ".s" );

                break;

            case ST_AC:
            case ST_SP:
            case ST_DISTO:
            case ST_NOISE:
            case ST_FFT:
            case ST_SENS:       // If there is a vector, it is frequency
                units += wxS( "·Hz" );
                break;

            case ST_DC:         // Could be a lot of things : V, A, deg C, ohm, ...
            case ST_OP:         // There is no vector for integration
            case ST_PZ:         // There is no vector for integration
            case ST_TF:         // There is no vector for integration
            default:
                units += wxS( "·?" );
                break;
            }
        }

        fmt.UpdateUnits( units );
        SetMeasureFormat( aRow, fmt );
    }

    if( m_simulatorFrame->SimFinished() )
    {
        wxString measExpr = text;
        wxString measFunc, measSignal, measParams;

        SIM_SIGNAL::SplitMeasurement( text, measFunc, measSignal, measParams );

        // WHEN, FIND, TRIG etc. have their own syntax.
        bool simpleFunc = std::any_of( measurementFunctions().begin(), measurementFunctions().end(),
                                       [&]( const wxString& aFunc )
                                       {
                                           return measFunc.IsSameAs( aFunc, false );
                                       } );

        if( simpleFunc && !measSignal.IsEmpty() )
        {
            wxString spiceSig = spiceSignal( measSignal );

            // MEAS takes a single vector, not an expression.
            if( spiceSig != measSignal || !SIM_SIGNAL::IsMeasVector( measSignal ) )
            {
                if( !SIM_SIGNAL::IsDerivedSignalVector( spiceSig ) )
                {
                    wxString vector = wxString::Format( wxS( "meas_signal_%u" ), aRow );

                    simulator()->Command(
                            ( wxS( "let " ) + vector + wxS( " = " ) + quoteNetNames( spiceSig ) ).ToStdString() );
                    spiceSig = vector;
                }

                measExpr = measFunc + wxS( " " ) + spiceSig;

                if( !measParams.IsEmpty() )
                    measExpr += wxS( " " ) + measParams;
            }
        }

        // A failed MEAS leaves the vector alone, so a deleted measurement's result would show instead
        for( const std::string& vector : simulator()->AllVectors() )
        {
            if( resultName.IsSameAs( vector, false ) )
            {
                simulator()->Command( "unlet " + resultName.ToStdString() );
                break;
            }
        }

        wxString cmd = wxString::Format( wxS( "meas %s %s %s" ), simType, resultName, measExpr );
        simulator()->Command( "echo " + cmd.ToStdString() );
        simulator()->Command( cmd.ToStdString() );

        std::vector<double> resultVec = simulator()->GetGainVector( resultName.ToStdString() );

        if( resultVec.size() > 0 )
        {
            result = SPICE_VALUE( resultVec[0] ).ToString( GetMeasureFormat( aRow ) );
        }
        else
        {
            appendConsoleText( wxString::Format( _( "Cannot evaluate measurement '%s'.\n" ),
                                                 measurements[aRow].first ),
                               RPT_SEVERITY_ERROR );
        }
    }

    setMeasurementValue( aRow, result );
}


void SIMULATOR_FRAME_UI::setMeasurementValue( int aIndex, const wxString& aValue )
{
    if( !m_measurementsModel )
        return;

    for( SIM_TREE_MODEL::NODE* leaf : m_measurementsModel->Leaves() )
    {
        if( leaf->index == aIndex )
        {
            leaf->value = aValue;
            m_measurementsModel->ValueChanged( SIM_TREE_MODEL::ToItem( leaf ), SIM_TREE_MODEL::COL_VALUE );
            return;
        }
    }
}


void SIMULATOR_FRAME_UI::AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
    {
        DisplayErrorMessage( nullptr, _( "The current analysis must have a plot in order to tune "
                                         "the value of a passive R, L, C model or voltage or "
                                         "current source." ) );
        return;
    }

    wxString ref = aSymbol->GetRef( &aSheetPath );

    // Do not add multiple instances for the same component.
    for( TUNER_SLIDER* tuner : m_tuners )
    {
        if( tuner->GetSymbolRef() == ref )
            return;
    }

    if( const SPICE_ITEM* item = GetExporter()->FindItem( ref ) )
    {
        try
        {
            wxScrolledWindow* page = getOrCreateTunerPage( tunerGroup( *item->model ) );
            TUNER_SLIDER*     tuner = new TUNER_SLIDER( this, page, aSheetPath, aSymbol );

            page->GetSizer()->Add( tuner, 0, wxEXPAND | wxBOTTOM, 2 );
            m_tuners.push_back( tuner );

            page->FitInside();
            page->Layout();
            m_tunerNotebook->SetSelection( m_tunerNotebook->FindPage( page ) );
            m_panelTuners->Layout();
            OnModify();
        }
        catch( const KI_PARAM_ERROR& e )
        {
            // Before the modal dialog, which would otherwise show the empty tab behind it
            pruneEmptyTunerPages();

            DisplayErrorMessage( nullptr, e.What() );
        }
    }
}


SIMULATOR_FRAME_UI::TUNER_GROUP SIMULATOR_FRAME_UI::tunerGroup( const SIM_MODEL& aModel )
{
    switch( aModel.GetDeviceType() )
    {
    case SIM_MODEL::DEVICE_T::R: return TUNERGRP_RESISTORS;
    case SIM_MODEL::DEVICE_T::C: return TUNERGRP_CAPACITORS;
    case SIM_MODEL::DEVICE_T::L: return TUNERGRP_INDUCTORS;

    case SIM_MODEL::DEVICE_T::V:
    case SIM_MODEL::DEVICE_T::I: return TUNERGRP_SOURCES;

    default: return TUNERGRP_OTHER;
    }
}


wxString SIMULATOR_FRAME_UI::tunerGroupLabel( TUNER_GROUP aGroup )
{
    switch( aGroup )
    {
    case TUNERGRP_RESISTORS: return _( "Resistors" );
    case TUNERGRP_CAPACITORS: return _( "Capacitors" );
    case TUNERGRP_INDUCTORS: return _( "Inductors" );
    case TUNERGRP_SOURCES: return _( "Sources" );
    default: return _( "Other" );
    }
}


wxScrolledWindow* SIMULATOR_FRAME_UI::getOrCreateTunerPage( TUNER_GROUP aGroup )
{
    if( m_tunerPages[aGroup] )
        return m_tunerPages[aGroup];

    wxScrolledWindow* page = new wxScrolledWindow( m_tunerNotebook, wxID_ANY );

    // Scroll rather than clip when a tab fills up.
    page->SetScrollRate( 0, 5 );
    page->SetSizer( new wxBoxSizer( wxVERTICAL ) );

    size_t pos = 0;

    for( int ii = 0; ii < aGroup; ++ii )
    {
        if( m_tunerPages[ii] )
            pos++;
    }

    m_tunerNotebook->InsertPage( pos, page, tunerGroupLabel( aGroup ) );
    m_tunerPages[aGroup] = page;

    return page;
}


void SIMULATOR_FRAME_UI::pruneEmptyTunerPages()
{
    for( int ii = 0; ii < TUNERGRP_COUNT; ++ii )
    {
        wxScrolledWindow* page = m_tunerPages[ii];

        if( !page || page->GetSizer()->GetItemCount() > 0 )
            continue;

        int pos = m_tunerNotebook->FindPage( page );

        if( pos != wxNOT_FOUND )
            m_tunerNotebook->DeletePage( pos ); // destroys the page

        m_tunerPages[ii] = nullptr;
    }
}


void SIMULATOR_FRAME_UI::UpdateTunerValue( const SCH_SHEET_PATH& aSheetPath, const KIID& aSymbol, const wxString& aRef,
                                           const wxString& aValue )
{
    SCHEMATIC&  schematic = m_schematicFrame->Schematic();
    wxString    variant = schematic.GetCurrentVariant();
    SCH_ITEM*   item = aSheetPath.ResolveItem( aSymbol );
    SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

    if( !symbol )
    {
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( " " )
                                           + wxString::Format( _( "%s not found" ), aRef ) );
        return;
    }

    NULL_REPORTER devnull;
    SIM_LIB_MGR   mgr( &m_schematicFrame->Prj() );

    std::vector<EMBEDDED_FILES*> embeddedFilesStack;
    embeddedFilesStack.push_back( m_schematicFrame->Schematic().GetEmbeddedFiles() );

    if( symbol->GetLibSymbolRef() )
        symbol->GetLibSymbolRef()->AppendParentEmbeddedFiles( embeddedFilesStack );

    if( EMBEDDED_FILES* symbolEmbeddedFiles = symbol->GetEmbeddedFiles() )
        embeddedFilesStack.push_back( symbolEmbeddedFiles );

    mgr.SetFilesStack( std::move( embeddedFilesStack ) );

    SIM_MODEL& model = mgr.CreateModel( &aSheetPath, *symbol, true, 0, variant, devnull ).model;

    const SIM_MODEL::PARAM* tunerParam = model.GetTunerParam();

    if( !tunerParam )
    {
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( " " )
                                           + wxString::Format( _( "%s is not tunable" ), aRef ) );
        return;
    }

    model.SetParamValue( tunerParam->info.name, std::string( aValue.ToUTF8() ) );
    model.WriteFields( symbol->GetFields(), &aSheetPath, variant );

    m_schematicFrame->UpdateItem( symbol, false, true );
    m_schematicFrame->OnModify();
}


void SIMULATOR_FRAME_UI::RemoveTuner( TUNER_SLIDER* aTuner )
{
    m_tuners.remove( aTuner );

    if( std::find( m_multiRunState.tuners.begin(), m_multiRunState.tuners.end(), aTuner )
            != m_multiRunState.tuners.end() )
    {
        clearMultiRunState( true );
    }

    m_tunerOverrides.erase( aTuner );

    wxWindow* page = aTuner->GetParent();

    aTuner->Destroy();

    if( page )
    {
        page->Layout();
        static_cast<wxScrolledWindow*>( page )->FitInside();
    }

    pruneEmptyTunerPages();
    m_panelTuners->Layout();
    OnModify();
}


void SIMULATOR_FRAME_UI::AddMeasurement( const wxString& aCmd )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab || aCmd.IsEmpty() )
        return;

    std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

    for( const auto& [cmd, format] : measurements )
    {
        if( cmd == aCmd )
            return; // Don't create duplicates
    }

    std::map<wxString, wxString> values = shownMeasurementValues();

    measurements.emplace_back( aCmd, wxString() );

    rebuildMeasurementsTree( &values );
    OnModify();
}


void SIMULATOR_FRAME_UI::AddMeasurements( const wxString& aSignal, const std::vector<wxString>& aFunctions )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    wxString signal;

    if( !measurementSignal( aSignal, signal ) )
        return;

    std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();
    std::map<wxString, wxString>                values = shownMeasurementValues();
    bool                                        added = false;

    for( const wxString& func : aFunctions )
    {
        wxString cmd = func + wxS( " " ) + signal;
        bool     duplicate = false;

        for( const auto& [existing, format] : measurements )
        {
            if( existing == cmd )
            {
                duplicate = true;
                break;
            }
        }

        if( !duplicate )
        {
            measurements.emplace_back( cmd, wxString() );
            added = true;
        }
    }

    if( !added )
        return;

    rebuildMeasurementsTree( &values );
    OnModify();
}


void SIMULATOR_FRAME_UI::DoFourier( const wxString& aSignal, const wxString& aFundamental )
{
    wxString signal = spiceSignal( SimSignalBaseName( aSignal ) );
    wxString cmd = wxString::Format( wxS( "fourier %s %s" ), SPICE_VALUE( aFundamental ).ToSpiceString(), signal );

    simulator()->Command( cmd.ToStdString() );
}


const SPICE_CIRCUIT_MODEL* SIMULATOR_FRAME_UI::GetExporter() const
{
    return circuitModel().get();
}


void SIMULATOR_FRAME_UI::AddTrace( const wxString& aName, SIM_TRACE_TYPE aType )
{
    if( !GetCurrentSimTab() )
    {
        appendConsoleText( _( "Error: no current simulation.\n" ), RPT_SEVERITY_ERROR );
        return;
    }

    SIM_TYPE simType = SPICE_CIRCUIT_MODEL::CommandToSimType( GetCurrentSimTab()->GetSimCommand() );

    if( simType == ST_UNKNOWN )
    {
        appendConsoleText( _( "Error: simulation type not defined.\n" ), RPT_SEVERITY_ERROR );
        return;
    }
    else if( !SIM_TAB::IsPlottable( simType ) )
    {
        appendConsoleText( _( "Error: simulation type doesn't support plotting.\n" ), RPT_SEVERITY_ERROR );
        return;
    }

    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        if( simType == ST_AC )
        {
            updateTrace( aName, aType | SPT_AC_GAIN, plotTab );
            updateTrace( aName, aType | SPT_AC_PHASE, plotTab );
        }
        else if( simType == ST_SP )
        {
            if( plotTab->IsSmithMode() )
            {
                updateTrace( aName, aType | SPT_SP_SMITH, plotTab );
            }
            else
            {
                updateTrace( aName, aType | SPT_SP_AMP, plotTab );
                updateTrace( aName, aType | SPT_AC_PHASE, plotTab );
            }
        }
        else
        {
            updateTrace( aName, aType, plotTab );
        }

        for( SIM_VIEW* view : plotTab->GetViews() )
            view->UpdateAll();
    }

    updateSignalsTree();
    OnModify();
}


void SIMULATOR_FRAME_UI::SetUserDefinedSignals( const std::map<int, wxString>& aNewSignals )
{
    for( size_t ii = 0; ii < m_plotNotebook->GetPageCount(); ++ii )
    {
        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( m_plotNotebook->GetPage( ii ) );

        if( !plotTab )
            continue;

        for( const auto& [id, existingSignal] : m_userDefinedSignals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( plotTab, existingSignal, &traceType );

            if( aNewSignals.count( id ) == 0 )
            {
                if( plotTab->GetSimType() == ST_AC )
                {
                    for( int subType : { SPT_AC_GAIN, SPT_AC_PHASE } )
                        plotTab->DeleteTrace( vectorName, traceType | subType );
                }
                else if( plotTab->GetSimType() == ST_SP )
                {
                    for( int subType : { SPT_SP_AMP, SPT_AC_PHASE, SPT_SP_SMITH } )
                        plotTab->DeleteTrace( vectorName, traceType | subType );
                }
                else
                {
                    plotTab->DeleteTrace( vectorName, traceType );
                }
            }
            else
            {
                if( plotTab->GetSimType() == ST_AC )
                {
                    for( int subType : { SPT_AC_GAIN, SPT_AC_PHASE } )
                    {
                        if( TRACE* trace = plotTab->GetTrace( vectorName, traceType | subType ) )
                            trace->SetName( aNewSignals.at( id ) );
                    }
                }
                else if( plotTab->GetSimType() == ST_SP )
                {
                    for( int subType : { SPT_SP_AMP, SPT_AC_PHASE, SPT_SP_SMITH } )
                    {
                        if( TRACE* trace = plotTab->GetTrace( vectorName, traceType | subType ) )
                            trace->SetName( aNewSignals.at( id ) );
                    }
                }
                else
                {
                    if( TRACE* trace = plotTab->GetTrace( vectorName, traceType ) )
                        trace->SetName( aNewSignals.at( id ) );
                }
            }
        }
    }

    m_userDefinedSignals = aNewSignals;

    if( m_simulatorFrame->SimFinished() )
        applyUserDefinedSignals();

    rebuildSignalsList();
    rebuildSignalsTree( m_filter->GetValue() );
    updateSignalsTree();
    updatePlotCursors();
    OnModify();
}


double SIMULATOR_FRAME_UI::getSmithPortImpedance( const wxString& aVectorName )
{
    long responsePort, drivePort;

    // normalize to the response port z0
    if( !SMITH_MATH::ParseSParamPorts( aVectorName, &responsePort, &drivePort ) )
        return 0.0;

    for( const SPICE_ITEM& item : circuitModel()->GetItems() )
    {
        if( !item.model )
            continue;

        const SIM_MODEL::PARAM* portnumParam = item.model->FindParam( "portnum" );
        long                    portnum = 0;

        if( !portnumParam
            || !wxString( SIM_VALUE::ToSpice( portnumParam->value ) ).ToLong( &portnum )
            || portnum != responsePort )
        {
            continue;
        }

        if( const SIM_MODEL::PARAM* z0Param = item.model->FindParam( "z0" ) )
        {
            double z0 = SIM_VALUE::ToDouble( z0Param->value, 50.0 );

            if( z0 > 0 )
                return z0;
        }

        // this item has no usable z0, another item may still carry it for this port
    }

    return 0.0;
}


void SIMULATOR_FRAME_UI::updateTrace( const wxString& aVectorName, int aTraceType, SIM_PLOT_TAB* aPlotTab,
                                      std::vector<double>* aDataX, bool aClearData, SIM_VIEW* aView )
{
    if( !aView )
    {
        TRACE* existing = aPlotTab->GetTrace( aVectorName, aTraceType );
        aView = existing && existing->GetView() ? existing->GetView() : aPlotTab->GetDefaultView();
    }

    // Only user vectors have a display name; skip the lookup for the rest.
    auto nameTrace = [this]( TRACE* aTrace, const wxString& aName )
    {
        if( !aTrace || !SIM_SIGNAL::IsDerivedSignalVector( aName ) )
            return;

        wxString displayName = signalNameFromVectorName( aName );

        if( displayName != aTrace->GetName() )
            aTrace->SetName( displayName );
    };

    if( !m_simulatorFrame->SimFinished() && !simulator()->IsRunning() )
    {
        nameTrace( aPlotTab->GetOrAddTrace( aVectorName, aTraceType, aView ), aVectorName );
        return;
    }

    SIM_TYPE simType = SPICE_CIRCUIT_MODEL::CommandToSimType( aPlotTab->GetSimCommand() );

    aTraceType &= aTraceType & SPT_Y_AXIS_MASK;
    aTraceType |= getXAxisType( simType );

    wxString simVectorName = aVectorName;

    if( aTraceType & SPT_POWER )
        simVectorName = simVectorName.AfterFirst( '(' ).BeforeLast( ')' ) + wxS( ":power" );

    if( !SIM_TAB::IsPlottable( simType ) )
    {
        // There is no plot to be shown
        simulator()->Command( wxString::Format( wxT( "print %s" ), aVectorName ).ToStdString() );

        return;
    }

    std::vector<double> data_x;
    std::vector<double> data_y;
    std::vector<double> frequencies;

    if( !aDataX || aClearData )
        aDataX = &data_x;

    // First, handle the x axis
    if( aDataX->empty() && !aClearData )
    {
        wxString xAxisName( simulator()->GetXAxis( simType ) );

        if( xAxisName.IsEmpty() )
            return;

        *aDataX = simulator()->GetGainVector( (const char*) xAxisName.c_str() );
    }

    unsigned int size = aDataX->size();

    // Don't fetch when only clearing: the "userN" vectors don't exist mid-run.
    if( !aClearData )
    {
        // Evaluated on demand, so this may be the first request for this plot.
        ensureDeviceVoltage( simVectorName );

        switch( simType )
        {
        case ST_AC:
            if( aTraceType & SPT_AC_GAIN )
                data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), size );
            else if( aTraceType & SPT_AC_PHASE )
                data_y = simulator()->GetPhaseVector( (const char*) simVectorName.c_str(), size );
            else
                wxFAIL_MSG( wxT( "Plot type missing AC_PHASE or AC_MAG bit" ) );

            break;
        case ST_SP:
            if( aTraceType & SPT_SP_SMITH )
            {
                // reflection coefficient locus, Re on X and Im on Y, frequency kept aside
                frequencies = *aDataX;
                data_y = simulator()->GetImaginaryVector( (const char*) simVectorName.c_str(), size );
                *aDataX = simulator()->GetRealVector( (const char*) simVectorName.c_str(), size );
            }
            else if( aTraceType & SPT_SP_AMP )
            {
                data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), size );
            }
            else if( aTraceType & SPT_AC_PHASE )
            {
                data_y = simulator()->GetPhaseVector( (const char*) simVectorName.c_str(), size );
            }
            else
            {
                wxFAIL_MSG( wxT( "Plot type missing AC_PHASE, SPT_SP_AMP or SPT_SP_SMITH bit" ) );
            }

            break;

        case ST_DC: data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), -1 ); break;

        case ST_NOISE:
        case ST_TRAN:
        case ST_FFT: data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), size ); break;

        default: wxFAIL_MSG( wxT( "Unhandled plot type" ) );
        }
    }

    SPICE_DC_PARAMS source1, source2;
    int             sweepCount = 1;
    size_t          sweepSize = std::numeric_limits<size_t>::max();

    if( simType == ST_DC
            && circuitModel()->ParseDCCommand( aPlotTab->GetSimCommand(), &source1, &source2 )
            && !source2.m_source.IsEmpty() )
    {
        SPICE_VALUE v = ( source2.m_vend - source2.m_vstart ) / source2.m_vincrement;

        sweepCount = KiROUND( v.ToDouble() ) + 1;
        sweepSize = aDataX->size() / sweepCount;
    }

    bool smithTrace = ( aTraceType & SPT_SP_SMITH ) > 0;

    if( m_multiRunState.storePending )
        recordMultiRunData( aVectorName, aTraceType, *aDataX, data_y );

    if( hasMultiRunTrace( aVectorName, aTraceType ) )
    {
        const std::string key = multiRunTraceKey( aVectorName, aTraceType );
        const auto        traceIt = m_multiRunState.traces.find( key );

        if( traceIt != m_multiRunState.traces.end() )
        {
            const MULTI_RUN_TRACE& traceData = traceIt->second;

            if( !traceData.xValues.empty() && !traceData.yValues.empty() )
            {
                size_t sweepSizeMulti = traceData.xValues.size();
                size_t runCount = traceData.yValues.size();

                if( sweepSizeMulti > 0 && runCount > 0 )
                {
                    std::vector<double> combinedX;
                    std::vector<double> combinedY;
                    std::vector<double> combinedFreq;

                    combinedX.reserve( sweepSizeMulti * runCount );
                    combinedY.reserve( sweepSizeMulti * runCount );

                    for( size_t run = 0; run < traceData.yValues.size(); run++ )
                    {
                        const std::vector<double>& runY = traceData.yValues[run];

                        if( runY.size() != sweepSizeMulti )
                            continue;

                        if( smithTrace )
                        {
                            // per-run x, Re(gamma) differs between runs
                            if( run >= traceData.xRuns.size() || traceData.xRuns[run].size() != sweepSizeMulti )
                                continue;

                            combinedX.insert( combinedX.end(), traceData.xRuns[run].begin(),
                                              traceData.xRuns[run].end() );
                        }
                        else
                        {
                            combinedX.insert( combinedX.end(), traceData.xValues.begin(), traceData.xValues.end() );
                        }

                        combinedY.insert( combinedY.end(), runY.begin(), runY.end() );

                        // the sweep frequencies are the same every run, repeat them per run
                        if( frequencies.size() == sweepSizeMulti )
                            combinedFreq.insert( combinedFreq.end(), frequencies.begin(), frequencies.end() );
                    }

                    if( TRACE* trace = aPlotTab->GetOrAddTrace( aVectorName, aTraceType, aView ) )
                    {
                        nameTrace( trace, aVectorName );

                        if( SMITH_TRACE* smith = dynamic_cast<SMITH_TRACE*>( trace ) )
                        {
                            smith->SetReferenceImpedance( getSmithPortImpedance( aVectorName ) );
                            aPlotTab->UpdateSmithReferenceImpedance();

                            if( combinedFreq.size() == combinedX.size() )
                                smith->SetFrequencies( combinedFreq );
                        }

                        if( combinedY.size() >= combinedX.size() && sweepSizeMulti > 0 )
                        {
                            int sweepCountCombined =
                                    combinedX.empty() ? 0 : static_cast<int>( combinedY.size() / sweepSizeMulti );

                            if( sweepCountCombined > 0 )
                            {
                                // Generate labels for each run based on tuner values
                                std::vector<wxString> labels;
                                labels.reserve( sweepCountCombined );

                                for( int i = 0; i < sweepCountCombined && i < (int) m_multiRunState.steps.size(); ++i )
                                {
                                    const MULTI_RUN_STEP& step = m_multiRunState.steps[i];
                                    wxString              label;

                                    for( auto it = step.overrides.begin(); it != step.overrides.end(); ++it )
                                    {
                                        if( it != step.overrides.begin() )
                                            label += wxS( ", " );

                                        const TUNER_SLIDER* tuner = it->first;
                                        double              value = it->second;

                                        SPICE_VALUE spiceVal( value );
                                        label += tuner->GetSymbolRef() + wxS( "=" ) + spiceVal.ToSpiceString();
                                    }

                                    labels.push_back( label );
                                }

                                aPlotTab->SetTraceData( trace, combinedX, combinedY, sweepCountCombined, sweepSizeMulti,
                                                        true, labels );
                            }
                        }
                    }

                    return;
                }
            }
        }
    }

    if( TRACE* trace = aPlotTab->GetOrAddTrace( aVectorName, aTraceType, aView ) )
    {
        nameTrace( trace, aVectorName );

        if( data_y.size() >= size )
        {
            if( SMITH_TRACE* smith = dynamic_cast<SMITH_TRACE*>( trace ) )
            {
                smith->SetFrequencies( frequencies );
                smith->SetReferenceImpedance( getSmithPortImpedance( aVectorName ) );
                aPlotTab->UpdateSmithReferenceImpedance();
            }

            aPlotTab->SetTraceData( trace, *aDataX, data_y, sweepCount, sweepSize );
        }
    }
}


wxString SIMULATOR_FRAME_UI::quoteNetNames( const wxString& aExpression ) const
{
    return SIM_SIGNAL::QuoteNetNames( aExpression, m_netnames );
}


void SIMULATOR_FRAME_UI::ensureDeviceVoltage( int aId )
{
    auto expr = m_deviceVoltageExprs.find( aId );

    // A command would read the vectors while the background thread reallocates them
    if( expr == m_deviceVoltageExprs.end() || !simulator() || simulator()->IsRunning() )
        return;

    if( !m_appliedDeviceVoltages[simulator()->CurrentPlotName()].insert( aId ).second )
        return;

    simulator()->Command( fmt::format( "let {} = {}", deviceVectorName( aId ).ToStdString(),
                                       quoteNetNames( expr->second ).ToStdString() ) );
}


void SIMULATOR_FRAME_UI::ensureDeviceVoltage( const wxString& aVectorName )
{
    auto it = m_deviceVoltageVectors.find( aVectorName );

    if( it != m_deviceVoltageVectors.end() )
        ensureDeviceVoltage( it->second );
}


wxString SIMULATOR_FRAME_UI::expandDeviceVoltages( const wxString& aExpression ) const
{
    std::map<wxString, wxString> deviceExprs;

    for( const auto& [name, id] : m_deviceVoltageIds )
    {
        if( auto expr = m_deviceVoltageExprs.find( id ); expr != m_deviceVoltageExprs.end() )
            deviceExprs[name.Mid( 2, name.Length() - 3 )] = expr->second; // name is "V(R1)"
    }

    return SIM_SIGNAL::ExpandDeviceVoltages( aExpression, deviceExprs );
}


wxString SIMULATOR_FRAME_UI::spiceSignal( const wxString& aSignal )
{
    for( const auto& [id, signal] : m_userDefinedSignals )
    {
        if( aSignal == signal )
            return vectorNameFromSignalId( id );
    }

    for( const auto& [name, id] : m_deviceVoltageIds )
    {
        if( aSignal.IsSameAs( name, false ) )
        {
            ensureDeviceVoltage( id );
            return deviceVectorName( id );
        }
    }

    // A parenthesised user-defined signal, as in "db(V(a)-V(b))", is its vector.
    wxString expanded = aSignal;

    for( const auto& [id, signal] : m_userDefinedSignals )
        expanded.Replace( wxS( "(" ) + signal + wxS( ")" ), wxS( "(" ) + vectorNameFromSignalId( id ) + wxS( ")" ) );

    expanded = expandDeviceVoltages( expanded );

    if( expanded == aSignal )
        return aSignal;

    return quoteNetNames( expanded );
}


void SIMULATOR_FRAME_UI::applyUserDefinedSignals()
{
    constexpr const char* cmd = "let user{} = {}";

    std::set<wxString> plottedVectors;

    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        for( const auto& [name, trace] : plotTab->GetTraces() )
            plottedVectors.insert( trace->GetVectorName() );
    }

    for( const auto& [id, signal] : m_userDefinedSignals )
    {
        wxString expanded = expandDeviceVoltages( signal );
        wxString vector = vectorNameFromSignalId( id );

        simulator()->Command( "echo " + fmt::format( cmd, id, expanded.ToStdString() ) );
        simulator()->Command( fmt::format( cmd, id, quoteNetNames( expanded ).ToStdString() ) );

        // ngspice can reject an expression without a word, which would leave an empty trace
        if( plottedVectors.count( vector ) && simulator()->GetGainVector( vector.ToStdString(), 1 ).empty() )
        {
            appendConsoleText( wxString::Format( _( "Cannot evaluate user-defined signal '%s'.\n" ), signal ),
                               RPT_SEVERITY_ERROR );
        }
    }

    // Component voltages are re-evaluated on demand for this plot.
    if( simulator() )
        m_appliedDeviceVoltages.erase( simulator()->CurrentPlotName() );
}


void SIMULATOR_FRAME_UI::applyTuners()
{
    WX_STRING_REPORTER reporter;

    for( const TUNER_SLIDER* tuner : m_tuners )
    {
        SCH_SHEET_PATH sheetPath;
        wxString       ref = tuner->GetSymbolRef();
        KIID           symbolId = tuner->GetSymbol( &sheetPath );
        SCH_ITEM*      schItem = sheetPath.ResolveItem( symbolId );
        SCH_SYMBOL*    symbol = dynamic_cast<SCH_SYMBOL*>( schItem );

        if( !symbol )
        {
            reporter.Report( wxString::Format( _( "%s not found" ), ref ) );
            continue;
        }

        const SPICE_ITEM* item = GetExporter()->FindItem( tuner->GetSymbolRef() );

        if( !item || !item->model->GetTunerParam() )
        {
            reporter.Report( wxString::Format( _( "%s is not tunable" ), ref ) );
            continue;
        }

        double floatVal;

        auto overrideIt = m_tunerOverrides.find( tuner );

        if( overrideIt != m_tunerOverrides.end() )
            floatVal = overrideIt->second;
        else
            floatVal = tuner->GetValue().ToDouble();

        simulator()->Command( item->model->SpiceGenerator().TunerCommand( *item, floatVal ) );
    }

    if( reporter.HasMessage() )
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( "\n" ) + reporter.GetMessages() );
}

bool SIMULATOR_FRAME_UI::LoadWorkbook( const wxString& aPath )
{
    wxTextFile file( aPath );

    if( !file.Open() )
        return false;

    wxString firstLine = file.GetFirstLine();
    long     dummy;
    bool     legacy = firstLine.StartsWith( wxT( "version " ) ) || firstLine.ToLong( &dummy );

    file.Close();

    m_plotNotebook->DeleteAllPages();
    m_userDefinedSignals.clear();

    // New tabs can reuse the addresses of the deleted ones
    m_lastSignalView.clear();

    if( legacy )
    {
        if( !loadLegacyWorkbook( aPath ) )
            return false;
    }
    else
    {
        if( !loadJsonWorkbook( aPath ) )
            return false;
    }

    rebuildSignalsList();

    rebuildSignalsTree( m_filter->GetValue() );
    updateSignalsTree();
    updatePlotCursors();
    rebuildMeasurementsTree();

    wxFileName filename( aPath );
    filename.MakeRelativeTo( m_schematicFrame->Prj().GetProjectPath() );

    // Remember the loaded workbook filename.
    simulator()->Settings()->SetWorkbookFilename( filename.GetFullPath() );

    return true;
}


/// Workbook keys for the plot margins, in SIM_VIEW::GetUserMargin() order.
static const char* const MARGIN_KEYS[4] = { "top", "right", "bottom", "left" };


bool SIMULATOR_FRAME_UI::loadJsonWorkbook( const wxString& aPath )
{
    wxFFileInputStream fp( aPath, wxT( "rt" ) );
    wxStdInputStream   fstream( fp );

    if( !fp.IsOk() )
        return false;

    try
    {
        nlohmann::json js = nlohmann::json::parse( fstream, nullptr, true, true );

        std::map<SIM_PLOT_TAB*, nlohmann::json> traceInfo;

        for( const nlohmann::json& tab_js : js["tabs"] )
        {
            wxString simCommand;
            int      simOptions = NETLIST_EXPORTER_SPICE::OPTION_ADJUST_PASSIVE_VALS
                                    | NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS;

            if( !tab_js.contains( "commands" ) || !tab_js["commands"].is_array() )
                continue;

            for( const nlohmann::json& cmd : tab_js[ "commands" ] )
            {
                if( cmd == ".kicad adjustpaths" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS;
                else if( cmd == ".save all" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;
                else if( cmd == ".probe alli" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
                else if( cmd == ".probe allp" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;
                else if( cmd == ".kicad esavenone" )
                    simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS;
                else
                    simCommand += wxString( cmd.get<wxString>() ).Trim();
            }

            SIM_TAB*      simTab = NewSimTab( simCommand );
            SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( simTab );

            simTab->SetSimOptions( simOptions );

            if( plotTab )
            {
                int viewCount = tab_js.contains( "viewCount" ) ? (int) tab_js["viewCount"] : 1;

                for( int ii = 1; ii < viewCount; ++ii )
                    plotTab->AddView();

                if( tab_js.contains( "traces" ) )
                    traceInfo[plotTab] = tab_js["traces"];

                if( tab_js.contains( "measurements" ) && tab_js["measurements"].is_array() )
                {
                    for( const nlohmann::json& m_js : tab_js[ "measurements" ] )
                    {
                        plotTab->Measurements().emplace_back( wxString( m_js.value( "expr", "" ) ),
                                                              wxString( m_js.value( "format", "" ) ) );
                    }
                }

                plotTab->SetDottedSecondary( tab_js.value( "dottedSecondary", false ) );
                plotTab->SetTraceWidth( tab_js.value( "traceWidth", 2 ) );
                plotTab->ShowGrid( tab_js.value( "showGrid", true ) );
                plotTab->EnableTrackCursor( tab_js.value( "showTrackCursor", false ) );

                if( tab_js.value( "smithMode", false ) )
                {
                    plotTab->SetSmithMode( true );
                    plotTab->SetSmithView( tab_js.value( "smithZoom", 1.0 ),
                                           tab_js.value( "smithPanX", 0.0 ),
                                           tab_js.value( "smithPanY", 0.0 ) );

                    if( tab_js.contains( "smithStashedTraces" ) && tab_js["smithStashedTraces"].is_array() )
                    {
                        for( const nlohmann::json& stash_js : tab_js["smithStashedTraces"] )
                        {
                            wxString vector( stash_js.value( "vector", "" ) );
                            wxString name( stash_js.value( "name", "" ) );

                            if( vector.IsEmpty() )
                                continue;

                            plotTab->SmithStashedTraces().push_back( { vector,
                                                                       name.IsEmpty() ? vector : name,
                                                                       stash_js.value( "base_type", 0 ) } );
                        }
                    }

                    if( tab_js.contains( "smithStashedCursors" ) && tab_js["smithStashedCursors"].is_array() )
                    {
                        for( const nlohmann::json& stash_js : tab_js["smithStashedCursors"] )
                        {
                            wxString vector( stash_js.value( "vector", "" ) );
                            int      id = stash_js.value( "id", 0 );

                            if( vector.IsEmpty() || ( id < 1 && id != -1 ) )
                                continue;

                            plotTab->SmithStashedCursors().push_back( { id,
                                                                        vector,
                                                                        stash_js.value( "base_type", 0 ),
                                                                        stash_js.value( "sub_type", 0 ),
                                                                        stash_js.value( "frequency",
                                                                        std::nan( "" ) ) } );
                        }
                    }
                }

                auto loadScale =
                        [&]( const char* aKey, auto&& aSetter )
                        {
                            if( !tab_js.contains( aKey ) )
                                return;

                            // older workbooks can hold null here (non-finite bounds saved as json null)
                            nlohmann::json min_js = tab_js[aKey].value( "min", nlohmann::json() );
                            nlohmann::json max_js = tab_js[aKey].value( "max", nlohmann::json() );

                            if( !min_js.is_number() || !max_js.is_number() )
                                return;

                            double min = min_js.get<double>();
                            double max = max_js.get<double>();

                            if( min < max )
                                aSetter( min, max );
                        };

                loadScale( "fixedY1scale",
                           [&]( double min, double max )
                           {
                               plotTab->SetY1Scale( true, min, max );
                           } );

                loadScale( "fixedY2scale",
                           [&]( double min, double max )
                           {
                               plotTab->SetY2Scale( true, min, max );
                           } );

                loadScale( "fixedY3scale",
                           [&]( double min, double max )
                           {
                               plotTab->EnsureThirdYAxisExists();
                               plotTab->SetY3Scale( true, min, max );
                           } );

                if( tab_js.contains( "legend" ) )
                {
                    const nlohmann::json& legend_js = tab_js[ "legend" ];
                    plotTab->SetLegendPosition( wxPoint( legend_js.value( "x", 0 ), legend_js.value( "y", 0 ) ) );
                    plotTab->ShowLegend( true );
                }

                // Only user-entered margins are pinned; the others follow the plot.
                {
                    int userMargins[4] = { -1, -1, -1, -1 };

                    if( tab_js.contains( "userMargins" ) && tab_js["userMargins"].is_object() )
                    {
                        const nlohmann::json& user_js = tab_js["userMargins"];

                        for( int idx = 0; idx < 4; ++idx )
                            userMargins[idx] = user_js.value( MARGIN_KEYS[idx], -1 );
                    }
                    else if( tab_js.contains( "margins" ) && tab_js["margins"].is_object() )
                    {
                        // Older workbooks saved every margin.  Values other than the old fixed defaults
                        // (30, 70 or 160, 45, 70) were entered by the user.
                        const nlohmann::json& margins_js = tab_js["margins"];
                        const int             defaults[4] = { 30, 70, 45, 70 };
                        SIM_VIEW*             view = plotTab->GetDefaultView();

                        // Workbooks from before this change may hold a bottom margin grown to fit the font
                        int autoBottom = view ? std::max( view->calcBottomMargin(), defaults[2] ) : defaults[2];

                        for( int idx = 0; idx < 4; ++idx )
                        {
                            int  value = margins_js.value( MARGIN_KEYS[idx], defaults[idx] );
                            bool automatic = value == defaults[idx] || ( idx == 1 && value == 160 )
                                             || ( idx == 2 && value >= defaults[2] && value <= autoBottom );

                            if( !automatic )
                                userMargins[idx] = value;
                        }
                    }

                    plotTab->SetUserMargins( userMargins[0], userMargins[1], userMargins[2], userMargins[3] );
                }
            }
        }

        int ii = 0;

        if( js.contains( "user_defined_signals" ) )
        {
            for( const nlohmann::json& signal_js : js["user_defined_signals"] )
                m_userDefinedSignals[ii++] = wxString( signal_js.get<wxString>() );
        }

        if( SIM_TAB* simTab = GetCurrentSimTab() )
        {
            m_simulatorFrame->LoadSimulator( simTab->GetSimCommand(), simTab->GetSimOptions() );

            if( SIM_TAB* firstTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( 0 ) ) )
                firstTab->SetLastSchTextSimCommand( wxString( js.value( "last_sch_text_sim_command", "" ) ) );
        }

        // 2 is kind of virtual, for the initial loading of the new setting
        int tempCustomCursorsCnt = js.value( "custom_cursors", 2 );

        if( ( tempCustomCursorsCnt > m_customCursorsCnt ) && m_customCursorsCnt > 2 )
            tempCustomCursorsCnt = 2 * tempCustomCursorsCnt - m_customCursorsCnt;

        for( int yy = 0; yy <= ( tempCustomCursorsCnt - m_customCursorsCnt ); yy++ )
            CreateNewCursor();

        auto addCursor = [=, this]( SIM_PLOT_TAB* aPlotTab, TRACE* aTrace, const wxString& aSignalName, int aCursorId,
                                    const nlohmann::json& aCursor_js )
        {
            if( aCursorId >= 1 )
            {
                aPlotTab->EnableCursor( aTrace, aCursorId, aSignalName );

                // tolerate a missing or null position from an older workbook saved
                // with a non-finite cursor coordinate
                nlohmann::json position = aCursor_js.value( "position", nlohmann::json() );

                if( position.is_number() )
                {
                    if( CURSOR* cursor = aTrace->GetCursor( aCursorId ) )
                        cursor->SetCoordX( position.get<double>() );
                }
            }

            wxString xFormat( aCursor_js.value( "x_format", "" ) );
            wxString yFormat( aCursor_js.value( "y_format", "" ) );
            int      formatSlot;

            if( aCursorId == -1 )
                formatSlot = 2; // we are a "cursorD"
            else if( aCursorId < 3 )
                formatSlot = aCursorId - 1;
            else
                formatSlot = aCursorId;

            if( !xFormat.IsEmpty() )
                m_cursorFormatsDyn[formatSlot][0].FromString( xFormat );

            if( !yFormat.IsEmpty() )
                m_cursorFormatsDyn[formatSlot][1].FromString( yFormat );
        };

        // Component voltages resolve only after the netlist has been walked.
        rebuildDeviceVoltages();

        for( const auto& [plotTab, traces_js] : traceInfo )
        {
            for( const nlohmann::json& trace_js : traces_js )
            {
                int      traceType = trace_js.value( "trace_type", (int) SPT_UNKNOWN );
                wxString signalName( trace_js.value( "signal", "" ) );

                if( signalName.IsEmpty() || traceType == SPT_UNKNOWN )
                    continue;

                wxString vectorName = vectorNameFromSignalName( plotTab, signalName, nullptr );

                int       viewIndex = trace_js.contains( "view" ) ? (int) trace_js["view"] : 0;
                SIM_VIEW* view = plotTab->GetView( viewIndex );

                if( !view )
                    view = plotTab->GetDefaultView();

                TRACE* trace = plotTab->GetOrAddTrace( vectorName, traceType, view );

                if( trace )
                {
                    // The trace adds the " (gain)" / " (phase)" suffix itself.
                    wxString traceName = signalName;

                    if( traceType & ( SPT_AC_GAIN | SPT_AC_PHASE ) )
                        traceName = SimSignalBaseName( signalName );

                    trace->SetName( traceName );

                    int yScaleIndex = trace_js.contains( "yScaleView" ) ? (int) trace_js["yScaleView"]
                                                                        : viewIndex;

                    if( SIM_VIEW* yScaleView = plotTab->GetView( yScaleIndex ) )
                        trace->SetYScaleView( yScaleView );

                    if( trace_js.contains( "cursorD" ) )
                        addCursor( plotTab, trace, signalName, -1, trace_js["cursorD"] );

                    std::vector<const char*> aVec;
                    aVec.clear();

                    for( int i = 1; i <= tempCustomCursorsCnt; i++ )
                    {
                        wxString str = "cursor" + std::to_string( i );
                        aVec.emplace_back( str.c_str() );

                        if( trace_js.contains( aVec[i - 1] ) )
                            addCursor( plotTab, trace, signalName, i, trace_js[aVec[i - 1]] );
                    }

                    if( trace_js.contains( "color" ) )
                    {
                        wxColour color;
                        color.Set( wxString( trace_js["color"].get<wxString>() ) );
                        trace->SetTraceColour( color );
                        plotTab->UpdateTraceStyle( trace );
                    }
                }
            }

            plotTab->UpdatePlotColors();
        }
    }
    catch( nlohmann::json::parse_error& error )
    {
        wxLogTrace( traceSettings, wxT( "Json parse error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( nlohmann::json::type_error& error )
    {
        wxLogTrace( traceSettings, wxT( "Json type error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( nlohmann::json::invalid_iterator& error )
    {
        wxLogTrace( traceSettings, wxT( "Json invalid_iterator error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( nlohmann::json::out_of_range& error )
    {
        wxLogTrace( traceSettings, wxT( "Json out_of_range error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, wxT( "Error reading %s" ), aPath );
        return false;
    }

    return true;
}

void SIMULATOR_FRAME_UI::SaveCursorToWorkbook( nlohmann::json& aTraceJs, TRACE* aTrace, int aCursorId )
{
    int cursorIdAfterD = aCursorId;

    if( aCursorId > 3 )
        cursorIdAfterD = cursorIdAfterD - 1;


    if( CURSOR* cursor = aTrace->GetCursor( aCursorId ) )
    {
        nlohmann::json cursor_js =
                nlohmann::json( { { "x_format", m_cursorFormatsDyn[cursorIdAfterD][0].ToString() },
                                  { "y_format", m_cursorFormatsDyn[cursorIdAfterD][1].ToString() } } );

        // json serializes a non-finite double as null, which would poison the load,
        // leave the position out instead and the loader keeps its default placement
        double position = cursor->GetCoords().x;

        if( std::isfinite( position ) )
            cursor_js["position"] = position;

        aTraceJs["cursor" + wxString( "" ) << aCursorId] = cursor_js;
    }

    if( cursorIdAfterD < 3 && ( aTrace->GetCursor( 1 ) || aTrace->GetCursor( 2 ) ) )
    {
        aTraceJs["cursorD"] = nlohmann::json( { { "x_format", m_cursorFormatsDyn[2][0].ToString() },
                                                { "y_format", m_cursorFormatsDyn[2][1].ToString() } } );
    }
}


bool SIMULATOR_FRAME_UI::SaveWorkbook( const wxString& aPath )
{
    wxFileName filename = aPath;
    filename.SetExt( FILEEXT::WorkbookFileExtension );

    wxFile file;

    file.Create( filename.GetFullPath(), true /* overwrite */ );

    if( !file.IsOpened() )
        return false;

    nlohmann::json tabs_js = nlohmann::json::array();

    for( size_t i = 0; i < m_plotNotebook->GetPageCount(); i++ )
    {
        SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( i ) );

        if( !simTab )
            continue;

        SIM_TYPE simType = simTab->GetSimType();

        nlohmann::json commands_js = nlohmann::json::array();

        commands_js.push_back( simTab->GetSimCommand() );

        int options = simTab->GetSimOptions();

        if( options & NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS )
            commands_js.push_back( ".kicad adjustpaths" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
            commands_js.push_back( ".save all" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS )
            commands_js.push_back( ".probe alli" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS )
            commands_js.push_back( ".probe allp" );

        if( !( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS ) )
            commands_js.push_back( ".kicad esavenone" );

        nlohmann::json tab_js = nlohmann::json( { { "analysis", SPICE_SIMULATOR::TypeToName( simType, true ) },
                                                  { "commands", commands_js } } );

        if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( simTab ) )
        {
            nlohmann::json traces_js = nlohmann::json::array();

            for( const auto& [name, trace] : plotTab->GetTraces() )
            {
                nlohmann::json trace_js =
                        nlohmann::json( { { "trace_type", (int) trace->GetType() },
                                          { "signal", signalNameFromVectorName( trace->GetDisplayName() ) },
                                          { "color", COLOR4D( trace->GetTraceColour() ).ToCSSString() },
                                          { "view", plotTab->GetViewIndex( trace->GetView() ) },
                                          { "yScaleView", plotTab->GetViewIndex( trace->GetYScaleView() ) } } );

                for( int ii = 1; ii <= m_customCursorsCnt; ii++ )
                    SaveCursorToWorkbook( trace_js, trace, ii );

                if( trace->GetCursor( 1 ) || trace->GetCursor( 2 ) )
                {
                    trace_js["cursorD"] = nlohmann::json( { { "x_format", m_cursorFormatsDyn[2][0].ToString() },
                                                            { "y_format", m_cursorFormatsDyn[2][1].ToString() } } );
                }

                traces_js.push_back( trace_js );
            }

            nlohmann::json measurements_js = nlohmann::json::array();

            for( const auto& [measurement, format] : plotTab->Measurements() )
            {
                measurements_js.push_back( nlohmann::json( { { "expr",   measurement },
                                                             { "format", format } } ) );
            }

            tab_js["traces"] = traces_js;
            tab_js["measurements"] = measurements_js;
            tab_js["dottedSecondary"] = plotTab->GetDottedSecondary();
            tab_js["traceWidth"] = plotTab->GetTraceWidth();
            tab_js["showGrid"] = plotTab->IsGridShown();
            tab_js["smithMode"] = plotTab->IsSmithMode();

            if( plotTab->IsSmithMode() )
            {
                tab_js["smithZoom"] = plotTab->GetSmithZoom();
                tab_js["smithPanX"] = plotTab->GetSmithPan().x;
                tab_js["smithPanY"] = plotTab->GetSmithPan().y;

                // traces and cursors the smith toggle set aside, saved so a workbook
                // written in smith mode does not lose them across a reload
                nlohmann::json stashedTraces_js = nlohmann::json::array();

                for( const SMITH_STASHED_TRACE& stashed : plotTab->SmithStashedTraces() )
                {
                    stashedTraces_js.push_back( nlohmann::json( { { "vector",    stashed.vectorName },
                                                                  { "name",      stashed.displayName },
                                                                  { "base_type", stashed.baseType } } ) );
                }

                if( !stashedTraces_js.empty() )
                    tab_js["smithStashedTraces"] = stashedTraces_js;

                nlohmann::json stashedCursors_js = nlohmann::json::array();

                for( const SMITH_STASHED_CURSOR& stashed : plotTab->SmithStashedCursors() )
                {
                    nlohmann::json cursor_js = nlohmann::json( { { "id",        stashed.id },
                                                                 { "vector",    stashed.vectorName },
                                                                 { "base_type", stashed.baseType },
                                                                 { "sub_type",  stashed.subType } } );

                    if( std::isfinite( stashed.frequency ) )
                        cursor_js["frequency"] = stashed.frequency;

                    stashedCursors_js.push_back( cursor_js );
                }

                if( !stashedCursors_js.empty() )
                    tab_js["smithStashedCursors"] = stashedCursors_js;
            }

            tab_js["viewCount"] = plotTab->GetViewCount();
            tab_js["showTrackCursor"] = plotTab->IsTrackCursorEnabled();

            double min, max;

            // json serializes a non-finite double as null, which would poison the load
            auto saveScale =
                    [&]( const char* aKey, double aMin, double aMax )
                    {
                        if( std::isfinite( aMin ) && std::isfinite( aMax ) )
                            tab_js[aKey] = nlohmann::json( { { "min", aMin }, { "max", aMax } } );
                    };

            if( plotTab->GetY1Scale( &min, &max ) )
                saveScale( "fixedY1scale", min, max );

            if( plotTab->GetY2Scale( &min, &max ) )
                saveScale( "fixedY2scale", min, max );

            if( plotTab->GetY3Scale( &min, &max ) )
                saveScale( "fixedY3scale", min, max );

            if( plotTab->IsLegendShown() )
            {
                tab_js["legend"] = nlohmann::json( { { "x", plotTab->GetLegendPosition().x },
                                                     { "y", plotTab->GetLegendPosition().y } } );
            }

            mpWindow* plotWin = plotTab->GetPlotWin();

            tab_js["margins"] = nlohmann::json( { { "left",   plotWin->GetMarginLeft() },
                                                  { "right",  plotWin->GetMarginRight() },
                                                  { "top",    plotWin->GetMarginTop() },
                                                  { "bottom", plotWin->GetMarginBottom() } } );

            // "margins" is what older versions read; "userMargins" holds only what the user entered.
            nlohmann::json userMargins_js = nlohmann::json::object();

            for( int idx = 0; idx < 4; ++idx )
            {
                if( plotTab->GetUserMargin( idx ) >= 0 )
                    userMargins_js[MARGIN_KEYS[idx]] = plotTab->GetUserMargin( idx );
            }

            tab_js["userMargins"] = userMargins_js;
        }

        tabs_js.push_back( tab_js );
    }

    nlohmann::json userDefinedSignals_js = nlohmann::json::array();

    for( const auto& [id, signal] : m_userDefinedSignals )
        userDefinedSignals_js.push_back( signal );

    // clang-format off
    nlohmann::json js = nlohmann::json( { { "version",              8 },
                                          { "tabs",                 tabs_js },
                                          { "user_defined_signals", userDefinedSignals_js },
                                          { "custom_cursors",       m_customCursorsCnt - 1 } } ); // Since we start +1 on init
    // clang-format on

    // Store the value of any simulation command found on the schematic sheet in a SCH_TEXT
    // object.  If this changes we want to warn the user and ask them if they want to update
    // the corresponding panel's sim command.
    if( m_plotNotebook->GetPageCount() > 0 )
    {
        SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( 0 ) );
        js["last_sch_text_sim_command"] = simTab->GetLastSchTextSimCommand();
    }

    std::stringstream buffer;
    buffer << std::setw( 2 ) << js << std::endl;

    bool res = file.Write( buffer.str() );
    file.Close();

    // Store the filename of the last saved workbook.
    if( res )
    {
        filename.MakeRelativeTo( m_schematicFrame->Prj().GetProjectPath() );
        simulator()->Settings()->SetWorkbookFilename( filename.GetFullPath() );
    }

    return res;
}


SIM_TRACE_TYPE SIMULATOR_FRAME_UI::getXAxisType( SIM_TYPE aType ) const
{
    switch( aType )
    {
    /// @todo SPT_LOG_FREQUENCY
    case ST_AC:    return SPT_LIN_FREQUENCY;
    case ST_SP:    return SPT_LIN_FREQUENCY;
    case ST_FFT:   return SPT_LIN_FREQUENCY;
    case ST_DC:    return SPT_SWEEP;
    case ST_TRAN:  return SPT_TIME;
    case ST_NOISE: return SPT_LIN_FREQUENCY;

    default:
        wxFAIL_MSG( wxString::Format( wxS( "Unhandled simulation type: %d" ), (int) aType ) );
        return SPT_UNKNOWN;
    }
}


wxString SIMULATOR_FRAME_UI::getNoiseSource() const
{
    wxString    output;
    wxString    ref;
    wxString    source;
    wxString    scale;
    SPICE_VALUE pts;
    SPICE_VALUE fStart;
    SPICE_VALUE fStop;
    bool        saveAll;

    if( GetCurrentSimTab() )
    {
        circuitModel()->ParseNoiseCommand( GetCurrentSimTab()->GetSimCommand(), &output, &ref, &source, &scale, &pts,
                                           &fStart, &fStop, &saveAll );
    }

    return source;
}


void SIMULATOR_FRAME_UI::TogglePanel( wxPanel* aPanel, wxSplitterWindow* aSplitterWindow, int& aSashPosition )
{
    // Unsplit: a sash parked at an edge still leaves the minimum pane size blank.
    if( aSplitterWindow->IsSplit() )
    {
        aSashPosition = aSplitterWindow->GetSashPosition();
        m_hiddenPanelFirst[aSplitterWindow] = aSplitterWindow->GetWindow1() == aPanel;
        aSplitterWindow->Unsplit( aPanel );
    }
    else
    {
        // Once unsplit, the remaining pane is window 1.
        wxWindow* other = aSplitterWindow->GetWindow1();
        bool      first = m_hiddenPanelFirst[aSplitterWindow];
        wxWindow* window1 = first ? static_cast<wxWindow*>( aPanel ) : other;
        wxWindow* window2 = first ? other : static_cast<wxWindow*>( aPanel );

        if( aSplitterWindow->GetSplitMode() == wxSPLIT_VERTICAL )
            aSplitterWindow->SplitVertically( window1, window2, aSashPosition );
        else
            aSplitterWindow->SplitHorizontally( window1, window2, aSashPosition );
    }

    m_parent->Refresh();
    m_parent->Layout();
}


bool SIMULATOR_FRAME_UI::IsSimConsoleShown()
{
    return m_panelConsole->IsShown();
}


void SIMULATOR_FRAME_UI::ToggleSimConsole()
{
    TogglePanel( m_panelConsole, m_splitterPlotAndConsole, m_splitterPlotAndConsoleSashPosition );
}


bool SIMULATOR_FRAME_UI::IsSimSidePanelShown()
{
    return m_sidePanel->IsShown();
}


void SIMULATOR_FRAME_UI::ToggleSimSidePanel()
{
    TogglePanel( m_sidePanel, m_splitterLeftRight, m_splitterLeftRightSashPosition );
}


void SIMULATOR_FRAME_UI::ToggleDarkModePlots()
{
    m_darkMode = !m_darkMode;

    // Rebuild the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

    // Now send changes to all SIM_PLOT_TAB
    for( size_t page = 0; page < m_plotNotebook->GetPageCount(); page++ )
    {
        wxWindow* curPage = m_plotNotebook->GetPage( page );

        // ensure it is truly a plot plotTab and not the (zero plots) placeholder
        // which is only SIM_TAB
        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( curPage );

        if( plotTab )
            plotTab->UpdatePlotColors();
    }
}


void SIMULATOR_FRAME_UI::ToggleSmithChart()
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab || plotTab->GetSimType() != ST_SP )
        return;

    bool smithMode = !plotTab->IsSmithMode();

    // collect the shown signals, then rebuild their traces in the new coordinate system,
    // keyed by vector name since user-defined traces are renamed to their display text
    struct SHOWN_VECTOR
    {
        wxString  vectorName;
        wxString  displayName;
        int       baseType;
        SIM_VIEW* view; // the view it was plotted on, so the rebuild stays where it was
    };

    std::vector<SHOWN_VECTOR> shownVectors;

    // remember cursors so they survive the rebuild
    struct SAVED_CURSOR
    {
        int      id;
        wxString vectorName;
        int      baseType;
        int      subType; // the SP subtype the cursor lived on
        double   coordX;
    };

    std::vector<SAVED_CURSOR> savedCursors;

    for( const auto& [id, trace] : plotTab->GetTraces() )
    {
        int      baseType = SPT_UNKNOWN;
        wxString vectorName = vectorNameFromSignalName( plotTab, trace->GetName(), &baseType );
        bool     seen = false;
        int      subType = trace->GetType() & ( SPT_SP_AMP | SPT_AC_PHASE | SPT_SP_SMITH );

        baseType &= ~( SPT_SP_MASK | SPT_AC_GAIN );

        for( const SHOWN_VECTOR& sv : shownVectors )
            seen |= sv.vectorName == vectorName;

        if( !seen )
            shownVectors.push_back( { vectorName, trace->GetName(), baseType, trace->GetView() } );

        for( const auto& [cursorId, cursor] : trace->GetCursors() )
        {
            if( cursor )
                savedCursors.push_back( { cursorId, vectorName, baseType, subType, cursor->GetCoords().x } );
        }
    }

    // a transmission S-parameter (S_i_j, i != j) is not an impedance, drop it from the chart
    auto isReflection =
            []( const wxString& aName ) -> bool
            {
                long response, drive;

                if( SMITH_MATH::ParseSParamPorts( aName, &response, &drive ) )
                    return response == drive;

                return true;
            };

    std::vector<SMITH_STASHED_TRACE>&  stashedTraces = plotTab->SmithStashedTraces();
    std::vector<SMITH_STASHED_CURSOR>& stashedCursors = plotTab->SmithStashedCursors();

    if( smithMode )
    {
        // set aside what the Smith view cannot show, so leaving the mode restores it
        stashedTraces.clear();
        stashedCursors.clear();

        for( const SHOWN_VECTOR& sv : shownVectors )
        {
            if( !isReflection( sv.vectorName ) )
                stashedTraces.push_back( { sv.vectorName, sv.displayName, sv.baseType } );
        }

        for( const SAVED_CURSOR& saved : savedCursors )
            stashedCursors.push_back( { saved.id, saved.vectorName, saved.baseType, saved.subType, saved.coordX } );
    }
    else
    {
        // bring the stashed transmission traces back into the rebuild list
        for( const SMITH_STASHED_TRACE& stashed : stashedTraces )
        {
            bool seen = false;

            for( const SHOWN_VECTOR& sv : shownVectors )
                seen |= sv.vectorName == stashed.vectorName;

            if( !seen )
            {
                shownVectors.push_back( { stashed.vectorName, stashed.displayName, stashed.baseType,
                                          plotTab->GetDefaultView() } );
            }
        }
    }

    for( const SHOWN_VECTOR& sv : shownVectors )
    {
        for( int subType : { SPT_SP_AMP, SPT_AC_PHASE, SPT_SP_SMITH } )
            plotTab->DeleteTrace( sv.vectorName, sv.baseType | subType );
    }

    plotTab->SetSmithMode( smithMode );

    for( const SHOWN_VECTOR& sv : shownVectors )
    {
        std::vector<int> subTypes;

        if( smithMode )
        {
            if( !isReflection( sv.vectorName ) )
                continue;

            subTypes = { SPT_SP_SMITH };
        }
        else
        {
            subTypes = { SPT_SP_AMP, SPT_AC_PHASE };
        }

        for( int subType : subTypes )
        {
            updateTrace( sv.vectorName, sv.baseType | subType, plotTab, nullptr, false, sv.view );

            if( TRACE* trace = plotTab->GetTrace( sv.vectorName, sv.baseType | subType ) )
                trace->SetName( sv.displayName );
        }
    }

    // restore cursors on the rebuilt traces at the same frequency
    if( smithMode )
    {
        for( const SAVED_CURSOR& saved : savedCursors )
        {
            TRACE* trace = plotTab->GetTrace( saved.vectorName, saved.baseType | SPT_SP_SMITH );

            // amplitude and phase cursors with the same id collapse onto one locus marker,
            // keep the first, the stash remembers both for the way back
            if( !trace || trace->GetCursor( saved.id ) )
                continue;

            plotTab->EnableCursor( trace, saved.id, trace->GetName() );

            if( CURSOR* cursor = trace->GetCursor( saved.id ) )
            {
                if( std::isfinite( saved.coordX ) )
                    cursor->SetCoordX( saved.coordX );
            }
        }
    }
    else
    {
        auto findStashed =
                [&]( int aId, const wxString& aVectorName ) -> const SMITH_STASHED_CURSOR*
                {
                    for( const SMITH_STASHED_CURSOR& stashed : stashedCursors )
                    {
                        if( stashed.id == aId && stashed.vectorName == aVectorName )
                            return &stashed;
                    }

                    return nullptr;
                };

        auto restoreCursor =
                [&]( int aId, const wxString& aVectorName, int aBaseType, int aSubType, double aFreq ) -> bool
                {
                    int wantSubType = aSubType == SPT_AC_PHASE ? SPT_AC_PHASE : SPT_SP_AMP;

                    if( TRACE* trace = plotTab->GetTrace( aVectorName, aBaseType | wantSubType ) )
                    {
                        plotTab->EnableCursor( trace, aId, trace->GetName() );

                        // a stash entry without a usable frequency keeps the default placement
                        if( std::isfinite( aFreq ) )
                        {
                            if( CURSOR* cursor = trace->GetCursor( aId ) )
                                cursor->SetCoordX( aFreq );
                        }

                        return true;
                    }

                    return false;
                };

        std::vector<int> restoredIds;

        // live smith cursors go back to the subtype they came from, at their current frequency
        for( const SAVED_CURSOR& saved : savedCursors )
        {
            const SMITH_STASHED_CURSOR* stashed = findStashed( saved.id, saved.vectorName );

            if( restoreCursor( saved.id, saved.vectorName, saved.baseType,
                               stashed ? stashed->subType : SPT_SP_AMP, saved.coordX ) )
            {
                restoredIds.push_back( saved.id );
            }
        }

        // cursors whose trace could not exist in Smith mode come back from the stash alone,
        // unless the same cursor id was moved to another trace while in smith mode
        for( const SMITH_STASHED_CURSOR& stashed : stashedCursors )
        {
            if( isReflection( stashed.vectorName ) )
                continue;

            if( std::find( restoredIds.begin(), restoredIds.end(), stashed.id ) != restoredIds.end() )
                continue;

            restoreCursor( stashed.id, stashed.vectorName, stashed.baseType, stashed.subType, stashed.frequency );
        }

        stashedTraces.clear();
        stashedCursors.clear();
    }

    for( SIM_VIEW* view : plotTab->GetViews() )
        view->UpdateAll();

    rebuildSignalsList();
    rebuildSignalsTree( m_filter->GetValue() );
    updateSignalsTree();
    updatePlotCursors();
    OnModify();
}


void SIMULATOR_FRAME_UI::onPlotClose( wxAuiNotebookEvent& event )
{
    OnModify();
}


void SIMULATOR_FRAME_UI::onPlotClosed( wxAuiNotebookEvent& event )
{
    CallAfter(
            [this]()
            {
                rebuildSignalsList();
                rebuildSignalsTree( m_filter->GetValue() );
                updatePlotCursors();

                //To avoid a current side effect in dynamic cursors while closing one out of many sim tabs
                updateSignalsTree();

                SIM_TAB* panel = GetCurrentSimTab();

                if( !panel || panel->GetSimType() != ST_OP )
                {
                    SCHEMATIC& schematic = m_schematicFrame->Schematic();
                    schematic.ClearOperatingPoints();
                    m_schematicFrame->RefreshOperatingPointDisplay();
                    m_schematicFrame->GetCanvas()->Refresh();
                }
            } );
}


void SIMULATOR_FRAME_UI::onPlotChanging( wxAuiNotebookEvent& event )
{
    if( m_measurementsModel )
        m_measurementsModel->SetContents( {} );

    event.Skip();
}


void SIMULATOR_FRAME_UI::OnPlotSettingsChanged()
{
    rebuildSignalsList();
    rebuildSignalsTree( m_filter->GetValue() );
    updatePlotCursors();

    rebuildMeasurementsTree();
}


void SIMULATOR_FRAME_UI::onPlotChanged( wxAuiNotebookEvent& event )
{
    if( SIM_TAB* simTab = GetCurrentSimTab() )
        simulator()->Command( "setplot " + simTab->GetSpicePlotName().ToStdString() );

    OnPlotSettingsChanged();

    //To avoid a current side effect in dynamic cursors while switching sim tabs
    updateSignalsTree();

    event.Skip();
}


void SIMULATOR_FRAME_UI::onPlotDragged( wxAuiNotebookEvent& event )
{
}


std::shared_ptr<SPICE_SIMULATOR> SIMULATOR_FRAME_UI::simulator() const
{
    return m_simulatorFrame->GetSimulator();
}


std::shared_ptr<SPICE_CIRCUIT_MODEL> SIMULATOR_FRAME_UI::circuitModel() const
{
    return m_simulatorFrame->GetCircuitModel();
}


void SIMULATOR_FRAME_UI::updateSmithCursorColumns( bool aSmithMode )
{
    if( aSmithMode == smithColumnsShown( m_cursorsGrid ) )
        return;

    if( aSmithMode )
    {
        m_cursorsGrid->AppendCols( SMITH_CURSOR_COLS );

        for( int col = COL_CURSOR_SMITH_FIRST; col < COL_CURSOR_SMITH_END; ++col )
        {
            wxGridCellAttr* attr = new wxGridCellAttr;
            attr->SetReadOnly();
            m_cursorsGrid->SetColAttr( col, attr );
        }

        // the panel is narrow, so keep the added columns off the horizontal scrollbar
        m_cursorsGrid->SetColSize( COL_CURSOR_R, 80 );
        m_cursorsGrid->SetColSize( COL_CURSOR_REACTANCE, 80 );
        m_cursorsGrid->SetColSize( COL_CURSOR_RL, 85 );
        m_cursorsGrid->SetColSize( COL_CURSOR_VSWR, 55 );

        // a hidden column reports zero, so only a width the user really dragged overrides
        for( size_t ii = 0; ii < m_smithCursorWidths.size(); ++ii )
        {
            if( m_smithCursorWidths[ii] > 0 )
                m_cursorsGrid->SetColSize( COL_CURSOR_SMITH_FIRST + (int) ii, m_smithCursorWidths[ii] );
        }

        setSmithCursorColumnLabels();
        applySmithCursorColumns();
    }
    else
    {
        rememberSmithCursorColumns();

        // DeleteCols would otherwise throw away anything the user dragged
        m_smithCursorWidths.clear();

        for( int col = COL_CURSOR_SMITH_FIRST; col < COL_CURSOR_SMITH_END; ++col )
            m_smithCursorWidths.push_back( m_cursorsGrid->GetColSize( col ) );

        m_cursorsGrid->DeleteCols( COL_CURSOR_SMITH_FIRST, SMITH_CURSOR_COLS );
    }
}


void SIMULATOR_FRAME_UI::setSmithCursorColumnLabels()
{
    if( !smithColumnsShown( m_cursorsGrid ) )
        return;

    m_cursorsGrid->SetColLabelValue( COL_CURSOR_R, wxS( "R" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_REACTANCE, wxS( "X" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_RL, _( "Return loss" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_VSWR, wxS( "VSWR" ) );
}


void SIMULATOR_FRAME_UI::rememberSmithCursorColumns()
{
    // only the Smith columns, so hiding one cannot follow the user onto a non-Smith tab
    m_smithCursorColumns.Empty();

    for( int col = COL_CURSOR_SMITH_FIRST; col < COL_CURSOR_SMITH_END; ++col )
    {
        if( !m_cursorsGrid->IsColShown( col ) )
            m_smithCursorColumns << col - COL_CURSOR_SMITH_FIRST << ' ';
    }
}


void SIMULATOR_FRAME_UI::applySmithCursorColumns()
{
    wxStringTokenizer tokens( m_smithCursorColumns, wxS( " " ), wxTOKEN_STRTOK );

    while( tokens.HasMoreTokens() )
    {
        long offset;

        if( tokens.GetNextToken().ToLong( &offset ) && offset >= 0 && offset < SMITH_CURSOR_COLS )
            m_cursorsGrid->HideCol( COL_CURSOR_SMITH_FIRST + (int) offset );
    }
}


void SIMULATOR_FRAME_UI::fillSmithCursorRow( int aRow, CURSOR* aCursor, TRACE* aTrace )
{
    SMITH_CURSOR* smithCursor = dynamic_cast<SMITH_CURSOR*>( aCursor );
    SMITH_TRACE*  smithTrace = dynamic_cast<SMITH_TRACE*>( aTrace );

    if( !smithCursor || !smithTrace )
        return;

    for( int col = COL_CURSOR_SMITH_FIRST; col < COL_CURSOR_SMITH_END; ++col )
        m_cursorsGrid->SetCellValue( aRow, col, wxS( "--" ) );

    // gamma sits at the origin until the trace has a locus to snap to, which would read as a
    // perfect match rather than as no answer
    if( smithTrace->GetDataX().empty() )
        return;

    const wxRealPoint&       gamma = smithCursor->GetGamma();
    double                   z0 = smithTrace->GetReferenceImpedance();
    double                   zr, zi;
    const SPICE_VALUE_FORMAT ohms = { 3, wxS( "~Ω" ) };

    // ohms need the port impedance, the match figures only need the magnitude
    if( z0 > 0.0 && SMITH_MATH::GammaToImpedance( gamma.x, gamma.y, z0, zr, zi ) )
    {
        m_cursorsGrid->SetCellValue( aRow, COL_CURSOR_R, SPICE_VALUE( zr ).ToString( ohms ) );
        m_cursorsGrid->SetCellValue( aRow, COL_CURSOR_REACTANCE, SPICE_VALUE( zi ).ToString( ohms ) );
    }

    double gm = std::hypot( gamma.x, gamma.y );
    double rl = SMITH_MATH::ReturnLoss( gm );
    double vswr = SMITH_MATH::VSWR( gm );

    if( std::isfinite( rl ) )
        m_cursorsGrid->SetCellValue( aRow, COL_CURSOR_RL, wxString::Format( wxS( "%.1f dB" ), rl ) );
    else if( std::isinf( rl ) )
        m_cursorsGrid->SetCellValue( aRow, COL_CURSOR_RL, wxS( "inf dB" ) );

    if( std::isfinite( vswr ) )
        m_cursorsGrid->SetCellValue( aRow, COL_CURSOR_VSWR, wxString::Format( wxS( "%.2f" ), vswr ) );
    else if( std::isinf( vswr ) )
        m_cursorsGrid->SetCellValue( aRow, COL_CURSOR_VSWR, wxS( "inf" ) );
}


void SIMULATOR_FRAME_UI::updatePlotCursors()
{
    SUPPRESS_GRID_CELL_EVENTS raii( this );

    m_cursorsGrid->ClearRows();

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
    {
        updateSmithCursorColumns( false );
        return;
    }

    updateSmithCursorColumns( plotTab->GetSimType() == ST_SP && plotTab->IsSmithMode() );

    // Update cursor values
    CURSOR*  cursor1 = nullptr;
    wxString cursor1Name;
    wxString cursor1Units;
    CURSOR*  cursor2 = nullptr;
    wxString cursor2Name;
    wxString cursor2Units;

    auto getUnitsY =
            [&]( TRACE* aTrace ) -> wxString
            {
                // a smith cursor's y is the reflection coefficient magnitude, unitless
                if( aTrace->GetType() & SPT_SP_SMITH )
                    return wxString();

                if( plotTab->GetSimType() == ST_AC )
                {
                    if( aTrace->GetType() & SPT_AC_PHASE )
                        return plotTab->GetUnitsY2();
                    else
                        return plotTab->GetUnitsY1();
                }
                else
                {
                    if( aTrace->GetType() & SPT_POWER )
                        return plotTab->GetUnitsY3();
                    else if( aTrace->GetType() & SPT_CURRENT )
                        return plotTab->GetUnitsY2();
                    else
                        return plotTab->GetUnitsY1();
                }
            };

    auto getNameY =
            [&]( TRACE* aTrace ) -> wxString
            {
                if( aTrace->GetType() & SPT_SP_SMITH )
                    return _( "Refl. Coeff." );

                if( plotTab->GetSimType() == ST_AC )
                {
                    if( aTrace->GetType() & SPT_AC_PHASE )
                        return plotTab->GetLabelY2();
                    else
                        return plotTab->GetLabelY1();
                }
                else
                {
                    if( aTrace->GetType() & SPT_POWER )
                        return plotTab->GetLabelY3();
                    else if( aTrace->GetType() & SPT_CURRENT )
                        return plotTab->GetLabelY2();
                    else
                        return plotTab->GetLabelY1();
                }
            };

    auto formatValue =
            [this]( double aValue, int aCursorId, int aCol ) -> wxString
            {
                if( ( !m_simulatorFrame->SimFinished() && aCol == 1 ) || std::isnan( aValue ) )
                    return wxS( "--" );
                else
                    return SPICE_VALUE( aValue ).ToString( m_cursorFormatsDyn[aCursorId][aCol] );
            };

    for( const auto& [name, trace] : plotTab->GetTraces() )
    {
        if( CURSOR* cursor = trace->GetCursor( 1 ) )
        {
            cursor1 = cursor;
            cursor1Name = getNameY( trace );
            cursor1Units = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            m_cursorFormatsDyn[0][0].UpdateUnits( plotTab->GetUnitsX() );
            m_cursorFormatsDyn[0][1].UpdateUnits( cursor1Units );

            m_cursorsGrid->AppendRows( 1 );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "1" ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, cursor->GetName() );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, 0, 0 ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, 0, 1 ) );
            fillSmithCursorRow( row, cursor, trace );
            break;
        }
    }

    for( const auto& [name, trace] : plotTab->GetTraces() )
    {
        if( CURSOR* cursor = trace->GetCursor( 2 ) )
        {
            cursor2 = cursor;
            cursor2Name = getNameY( trace );
            cursor2Units = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            m_cursorFormatsDyn[1][0].UpdateUnits( plotTab->GetUnitsX() );
            m_cursorFormatsDyn[1][1].UpdateUnits( cursor2Units );

            m_cursorsGrid->AppendRows( 1 );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "2" ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, cursor->GetName() );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, 1, 0 ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, 1, 1 ) );
            fillSmithCursorRow( row, cursor, trace );
            break;
        }
    }

    if( cursor1 && cursor2 && cursor1Units == cursor2Units )
    {
        wxRealPoint coords = cursor2->GetCoords() - cursor1->GetCoords();
        wxString    signal;

        m_cursorFormatsDyn[2][0].UpdateUnits( plotTab->GetUnitsX() );
        m_cursorFormatsDyn[2][1].UpdateUnits( cursor1Units );

        if( cursor1->GetName() == cursor2->GetName() )
            signal = wxString::Format( wxS( "%s[2 - 1]" ), cursor2->GetName() );
        else
            signal = wxString::Format( wxS( "%s - %s" ), cursor2->GetName(), cursor1->GetName() );

        m_cursorsGrid->AppendRows( 1 );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_NAME, _( "Diff" ) );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_SIGNAL, signal );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_X, formatValue( coords.x, 2, 0 ) );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_Y, formatValue( coords.y, 2, 1 ) );
    }
    // Set up the labels
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, plotTab->GetLabelX() );

    wxString valColName = _( "Value" );

    if( !cursor1Name.IsEmpty() )
    {
        if( cursor2Name.IsEmpty() || cursor1Name == cursor2Name )
            valColName = cursor1Name;
    }
    else if( !cursor2Name.IsEmpty() )
    {
        valColName = cursor2Name;
    }

    m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, valColName );

    if( m_customCursorsCnt > 3 ) // 2 for the default hardocded cursors plus the initial + 1
    {
        for( int i = 3; i < m_customCursorsCnt; i++ )
        {
            for( const auto& [name, trace] : plotTab->GetTraces() )
            {
                if( CURSOR* cursor = trace->GetCursor( i ) )
                {
                    CURSOR*  curs = cursor;
                    wxString cursName = getNameY( trace );
                    wxString cursUnits = getUnitsY( trace );

                    wxRealPoint coords = cursor->GetCoords();
                    int         row = m_cursorsGrid->GetNumberRows();

                    m_cursorFormatsDyn[i][0].UpdateUnits( plotTab->GetUnitsX() );
                    m_cursorFormatsDyn[i][1].UpdateUnits( cursUnits );

                    m_cursorsGrid->AppendRows( 1 );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "" ) + wxString( "" ) << i );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, curs->GetName() );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, i, 0 ) );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, i, 1 ) );
                    fillSmithCursorRow( row, curs, trace );

                    // Set up the labels
                    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, plotTab->GetLabelX() );

                    valColName = _( "Value" );

                    if( !cursName.IsEmpty() && m_cursorsGrid->GetColLabelValue( COL_CURSOR_Y ) == cursName )
                        valColName = cursName;

                    m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, valColName );
                    break;
                }
            }
        }
    }

    autoSizeGridColumn( m_cursorsGrid, COL_CURSOR_NAME, 45 );
    autoSizeGridColumn( m_cursorsGrid, COL_CURSOR_SIGNAL, 162 );
    autoSizeGridColumn( m_cursorsGrid, COL_CURSOR_X, 90 );
    autoSizeGridColumn( m_cursorsGrid, COL_CURSOR_Y, 90 );
}


void SIMULATOR_FRAME_UI::onPlotCursorUpdate( wxCommandEvent& aEvent )
{
    updatePlotCursors();
    OnModify();
}


void SIMULATOR_FRAME_UI::OnSimUpdate()
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
        plotTab->ResetScales( true );

    // Drop any buffered output from the previous run and clear the console widget.
    m_simulatorFrame->TakeSimReportEntries();
    m_consoleEntries.clear();
    m_simConsole->Clear();

    prepareMultiRunState();

    // Do not export netlist, it is already stored in the simulator
    applyTuners();

    m_refreshTimer.Start( REFRESH_INTERVAL, wxTIMER_ONE_SHOT );
}


bool SIMULATOR_FRAME_UI::consoleEntryMatchesFilter( SEVERITY aSeverity ) const
{
    switch( m_consoleFilter )
    {
    case SIM_CONSOLE_FILTER::WARNINGS: return aSeverity == RPT_SEVERITY_WARNING;
    case SIM_CONSOLE_FILTER::ERRORS: return aSeverity == RPT_SEVERITY_ERROR;
    default: return true;
    }
}


void SIMULATOR_FRAME_UI::appendConsoleText( const wxString& aText, SEVERITY aSeverity )
{
    m_consoleEntries.push_back( { aSeverity, aText } );

    if( consoleEntryMatchesFilter( aSeverity ) )
    {
        m_simConsole->AppendText( aText );
        m_simConsole->SetInsertionPointEnd();
    }
}


void SIMULATOR_FRAME_UI::OnConsoleFilterChanged( wxCommandEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_consoleFilterWarnings && m_consoleFilterWarnings->GetValue() )
        m_consoleFilter = SIM_CONSOLE_FILTER::WARNINGS;
    else if( aEvent.GetEventObject() == m_consoleFilterErrors && m_consoleFilterErrors->GetValue() )
        m_consoleFilter = SIM_CONSOLE_FILTER::ERRORS;
    else
        m_consoleFilter = SIM_CONSOLE_FILTER::ALL;

    m_consoleFilterAll->SetValue( m_consoleFilter == SIM_CONSOLE_FILTER::ALL );
    m_consoleFilterWarnings->SetValue( m_consoleFilter == SIM_CONSOLE_FILTER::WARNINGS );
    m_consoleFilterErrors->SetValue( m_consoleFilter == SIM_CONSOLE_FILTER::ERRORS );

    wxString shown;

    for( const SIM_CONSOLE_ENTRY& entry : m_consoleEntries )
    {
        if( consoleEntryMatchesFilter( entry.severity ) )
            shown += entry.text;
    }

    // ChangeValue() avoids a repaint with an empty widget.
    m_simConsole->ChangeValue( shown );
    m_simConsole->SetInsertionPointEnd();
}


void SIMULATOR_FRAME_UI::FlushSimConsole()
{
    // AppendText is slow on MSW; write the buffered lines at once.
    std::vector<SIM_CONSOLE_ENTRY> entries = m_simulatorFrame->TakeSimReportEntries();
    wxString                       shown;

    for( const SIM_CONSOLE_ENTRY& entry : entries )
    {
        if( consoleEntryMatchesFilter( entry.severity ) )
            shown += entry.text;

        m_consoleEntries.push_back( entry );
    }

    if( shown.IsEmpty() )
        return;

    m_simConsole->AppendText( shown );
    m_simConsole->SetInsertionPointEnd();
}


std::vector<wxString> SIMULATOR_FRAME_UI::SimPlotVectors() const
{
    std::vector<wxString> signals;

    for( const std::string& vec : simulator()->AllVectors() )
        signals.emplace_back( vec );

    return signals;
}


std::vector<wxString> SIMULATOR_FRAME_UI::DeviceVoltages() const
{
    std::vector<wxString> names;

    for( const auto& [name, id] : m_deviceVoltageIds )
        names.push_back( name );

    return names;
}


std::vector<wxString> SIMULATOR_FRAME_UI::Signals() const
{
    std::vector<wxString> signals;

    for( const wxString& signal : m_signals )
        signals.emplace_back( signal );

    for( const auto& [id, signal] : m_userDefinedSignals )
        signals.emplace_back( signal );

    sortSignals( signals );

    return signals;
}


void SIMULATOR_FRAME_UI::OnSimRefresh( bool aFinal )
{
    FlushSimConsole();

    if( aFinal )
        m_refreshTimer.Stop();

    SIM_TAB* simTab = GetCurrentSimTab();

    if( !simTab )
        return;

    bool storeMultiRun = false;

    if( aFinal && m_multiRunState.active )
    {
        if( m_multiRunState.currentStep < m_multiRunState.steps.size() )
        {
            storeMultiRun = true;
            m_multiRunState.storePending = true;
        }
    }
    else
    {
        m_multiRunState.storePending = false;
    }

    SIM_TYPE simType = simTab->GetSimType();
    wxString msg;

    // FFT is run synchronously in StartSimulation(), which stores the new FFT plot name before
    // refreshing.  Ensure ngspice is on that plot before accessing its vectors.  Other simulation
    // refreshes must not run setplot here, or a rerun would switch ngspice back to a stale plot.
    if( aFinal && simType == ST_FFT )
    {
        const wxString spicePlotName = simTab->GetSpicePlotName();

        if( !spicePlotName.IsEmpty() )
            simulator()->Command( "setplot " + spicePlotName.ToStdString() );
    }

    if( aFinal )
    {
        applyUserDefinedSignals();
        updateSignalsTree();
    }

    // If there are any signals plotted, update them
    if( SIM_TAB::IsPlottable( simType ) )
    {
        simTab->SetSpicePlotName( simulator()->CurrentPlotName() );

        if( simType == ST_NOISE && aFinal )
        {
            appendConsoleText( _( "\n\nSimulation results:\n\n" ) );

            // The simulator will create noise1 & noise2 on the first run, noise3 and noise4
            // on the second, etc.  The first plot for each run contains the spectral density
            // noise vectors and second contains the integrated noise.
            long number;
            simulator()->CurrentPlotName().Mid( 5 ).ToLong( &number );

            for( const std::string& vec : simulator()->AllVectors() )
            {
                std::vector<double> val_list = simulator()->GetRealVector( vec, 1 );
                wxString            value = SPICE_VALUE( val_list[0] ).ToSpiceString();

                msg.Printf( wxS( "%s: %sV\n" ), vec, value );

                appendConsoleText( msg );
            }

            simulator()->Command( fmt::format( "setplot noise{}", number - 1 ) );
            simTab->SetSpicePlotName( simulator()->CurrentPlotName() );
        }

        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( simTab );
        wxCHECK_RET( plotTab, wxString::Format( wxT( "No SIM_PLOT_TAB for: %s" ), magic_enum::enum_name( simType ) ) );

        struct TRACE_INFO
        {
            wxString Vector;
            int      TraceType;
            bool     ClearData;
        };

        std::map<TRACE*, TRACE_INFO> traceMap;

        for( const auto& [name, trace] : plotTab->GetTraces() )
            traceMap[trace] = { wxEmptyString, SPT_UNKNOWN, false };

        // NB: m_signals are already broken out into gain/phase, but m_userDefinedSignals are
        // as the user typed them

        for( const wxString& signal : m_signals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( plotTab, signal, &traceType );

            // Not defined until the final refresh; clear the trace mid-run.
            bool synthesised = m_deviceVoltageIds.count( SimSignalBaseName( signal ) ) > 0;

            if( TRACE* trace = plotTab->GetTrace( vectorName, traceType ) )
                traceMap[trace] = { vectorName, traceType, synthesised && !aFinal };
        }

        for( const auto& [id, signal] : m_userDefinedSignals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( plotTab, signal, &traceType );

            if( simType == ST_AC )
            {
                int baseType = traceType &= ~( SPT_AC_GAIN | SPT_AC_PHASE );

                for( int subType : { baseType | SPT_AC_GAIN, baseType | SPT_AC_PHASE } )
                {
                    if( TRACE* trace = plotTab->GetTrace( vectorName, subType ) )
                        traceMap[trace] = { vectorName, subType, !aFinal };
                }
            }
            else if( simType == ST_SP )
            {
                int baseType = traceType &= ~SPT_SP_MASK;

                for( int subType : { baseType | SPT_SP_AMP, baseType | SPT_AC_PHASE, baseType | SPT_SP_SMITH } )
                {
                    if( TRACE* trace = plotTab->GetTrace( vectorName, subType ) )
                        traceMap[trace] = { vectorName, subType, !aFinal };
                }
            }
            else
            {
                if( TRACE* trace = plotTab->GetTrace( vectorName, traceType ) )
                    traceMap[trace] = { vectorName, traceType, !aFinal };
            }
        }

        // Component voltages are listed only when all voltages are saved, but a plotted one stays
        for( auto& [trace, info] : traceMap )
        {
            if( info.Vector.IsEmpty() && m_deviceVoltageVectors.count( trace->GetVectorName() ) )
                info = { trace->GetVectorName(), trace->GetType(), !aFinal };
        }

        // Two passes so that DC-sweep sub-traces get deleted and re-created:

        for( const auto& [trace, traceInfo] : traceMap )
        {
            if( traceInfo.Vector.IsEmpty() )
                plotTab->DeleteTrace( trace );
        }

        for( const auto& [trace, info] : traceMap )
        {
            std::vector<double> data_x;

            if( !info.Vector.IsEmpty() )
                updateTrace( info.Vector, info.TraceType, plotTab, &data_x, info.ClearData );
        }

        for( SIM_VIEW* view : plotTab->GetViews() )
            view->UpdateAll();

        if( aFinal )
        {
            updateMeasurements();

            plotTab->ResetScales( true );
        }

        for( SIM_VIEW* view : plotTab->GetViews() )
            view->Fit();

        updatePlotCursors();
    }
    else if( simType == ST_OP && aFinal )
    {
        appendConsoleText( _( "\n\nSimulation results:\n\n" ) );

        for( const std::string& vec : simulator()->AllVectors() )
        {
            std::vector<double> val_list = simulator()->GetRealVector( vec, 1 );

            if( val_list.empty() )
                continue;

            wxString       value = SPICE_VALUE( val_list[0] ).ToSpiceString();
            wxString       signal;
            SIM_TRACE_TYPE type = circuitModel()->VectorToSignal( vec, signal );

            const size_t tab = 25; //characters
            size_t       padding = ( signal.length() < tab ) ? ( tab - signal.length() ) : 1;

            switch( type )
            {
            case SPT_VOLTAGE: value.Append( wxS( "V" ) ); break;
            case SPT_CURRENT: value.Append( wxS( "A" ) ); break;
            case SPT_POWER:   value.Append( wxS( "W" ) ); break;
            default:          value.Append( wxS( "?" ) ); break;
            }

            msg.Printf( wxT( "%s%s\n" ),
                        ( signal + wxT( ":" ) ).Pad( padding, wxUniChar( ' ' ) ),
                        value );

            appendConsoleText( msg );

            if( type == SPT_VOLTAGE || type == SPT_CURRENT || type == SPT_POWER )
                signal = signal.SubString( 2, signal.Length() - 2 );

            if( type == SPT_POWER )
                signal += wxS( ":power" );

            m_schematicFrame->Schematic().SetOperatingPoint( signal, val_list.at( 0 ) );
        }
    }
    else if( simType == ST_PZ && aFinal )
    {
        appendConsoleText( _( "\n\nSimulation results:\n\n" ) );
        simulator()->Command( "print all" );
    }

    // Non-plottable analyses (op, pz, tf, sens, disto) still create an ngspice plot; record its
    // name so a rerun can destroy it instead of leaking the vectors.  Plottable tabs already
    // stored their (possibly noise-adjusted) plot name above.  A shared/stale plot name is caught
    // when destroying, not here.
    if( aFinal && !SIM_TAB::IsPlottable( simType ) )
        simTab->SetSpicePlotName( simulator()->CurrentPlotName() );

    if( storeMultiRun )
    {
        m_multiRunState.storePending = false;
        m_multiRunState.storedSteps = m_multiRunState.currentStep + 1;
    }

    if( aFinal && m_multiRunState.active )
    {
        if( m_multiRunState.currentStep + 1 < m_multiRunState.steps.size() )
        {
            m_multiRunState.currentStep++;

            wxQueueEvent( m_simulatorFrame, new wxCommandEvent( EVT_SIM_UPDATE ) );
        }
        else
        {
            m_multiRunState.active = false;
            m_multiRunState.steps.clear();
            m_multiRunState.currentStep = 0;
            m_multiRunState.storePending = false;
            m_tunerOverrides.clear();

            if( !m_multiRunState.traces.empty() )
            {
                auto iter = m_multiRunState.traces.begin();

                if( iter != m_multiRunState.traces.end() )
                    m_multiRunState.storedSteps = iter->second.yValues.size();
            }
        }
    }

    // The let and meas commands above report through ngspice, which would otherwise wait for the next run
    if( aFinal )
        FlushSimConsole();
}


void SIMULATOR_FRAME_UI::clearMultiRunState( bool aClearTraces )
{
    m_multiRunState.active = false;
    m_multiRunState.tuners.clear();
    m_multiRunState.steps.clear();
    m_multiRunState.currentStep = 0;
    m_multiRunState.storePending = false;

    if( aClearTraces )
    {
        m_multiRunState.traces.clear();
        m_multiRunState.storedSteps = 0;
    }

    m_tunerOverrides.clear();
}


void SIMULATOR_FRAME_UI::prepareMultiRunState()
{
    m_tunerOverrides.clear();

    std::vector<TUNER_SLIDER*> multiTuners;

    for( TUNER_SLIDER* tuner : m_tuners )
    {
        if( tuner->GetRunMode() == TUNER_SLIDER::RUN_MODE::MULTI )
            multiTuners.push_back( tuner );
    }

    if( multiTuners.empty() )
    {
        clearMultiRunState( true );
        return;
    }

    bool tunersChanged = multiTuners != m_multiRunState.tuners;

    if( m_multiRunState.active && tunersChanged )
        clearMultiRunState( true );

    if( !m_multiRunState.active )
    {
        if( tunersChanged || m_multiRunState.storedSteps > 0 || !m_multiRunState.traces.empty() )
            clearMultiRunState( true );

        m_multiRunState.tuners = multiTuners;
        m_multiRunState.steps = calculateMultiRunSteps( multiTuners );
        m_multiRunState.currentStep = 0;
        m_multiRunState.storePending = false;

        if( m_multiRunState.steps.size() >= 2 )
        {
            m_multiRunState.active = true;
            m_multiRunState.storedSteps = 0;
        }
        else
        {
            m_multiRunState.steps.clear();
            return;
        }
    }
    else if( tunersChanged )
    {
        m_multiRunState.tuners = multiTuners;
    }

    if( m_multiRunState.active && m_multiRunState.currentStep < m_multiRunState.steps.size() )
    {
        const MULTI_RUN_STEP& step = m_multiRunState.steps[m_multiRunState.currentStep];

        for( const auto& entry : step.overrides )
            m_tunerOverrides[entry.first] = entry.second;
    }
}


std::vector<SIMULATOR_FRAME_UI::MULTI_RUN_STEP>
SIMULATOR_FRAME_UI::calculateMultiRunSteps( const std::vector<TUNER_SLIDER*>& aTuners ) const
{
    std::vector<MULTI_RUN_STEP> steps;

    if( aTuners.empty() )
        return steps;

    std::vector<std::vector<double>> tunerValues;
    tunerValues.reserve( aTuners.size() );

    for( TUNER_SLIDER* tuner : aTuners )
    {
        if( !tuner )
            return steps;

        double startValue = tuner->GetMin().ToDouble();
        double endValue = tuner->GetMax().ToDouble();
        int    stepCount = std::max( 2, tuner->GetStepCount() );

        if( stepCount < 2 )
            stepCount = 2;

        double increment = ( endValue - startValue ) / static_cast<double>( stepCount - 1 );

        std::vector<double> values;
        values.reserve( stepCount );

        for( int ii = 0; ii < stepCount; ++ii )
            values.push_back( startValue + increment * ii );

        tunerValues.push_back( std::move( values ) );
    }

    int limit = ADVANCED_CFG::GetCfg().m_SimulatorMultiRunCombinationLimit;

    if( limit < 1 )
        limit = 1;

    std::vector<double> currentValues( aTuners.size(), 0.0 );

    auto generate =
            [&]( auto&& self, size_t depth ) -> void
            {
                if( steps.size() >= static_cast<size_t>( limit ) )
                    return;

                if( depth == aTuners.size() )
                {
                    MULTI_RUN_STEP step;

                    for( size_t ii = 0; ii < aTuners.size(); ++ii )
                        step.overrides.emplace( aTuners[ii], currentValues[ii] );

                    steps.push_back( std::move( step ) );
                    return;
                }

                for( double value : tunerValues[depth] )
                {
                    currentValues[depth] = value;
                    self( self, depth + 1 );

                    if( steps.size() >= static_cast<size_t>( limit ) )
                        return;
                }
            };

    generate( generate, 0 );

    return steps;
}


std::string SIMULATOR_FRAME_UI::multiRunTraceKey( const wxString& aVectorName, int aTraceType ) const
{
    return fmt::format( "{}|{}", aVectorName.ToStdString(), aTraceType );
}


void SIMULATOR_FRAME_UI::recordMultiRunData( const wxString& aVectorName, int aTraceType, const std::vector<double>& aX,
                                             const std::vector<double>& aY )
{
    if( aX.empty() || aY.empty() )
        return;

    std::string      key = multiRunTraceKey( aVectorName, aTraceType );
    MULTI_RUN_TRACE& trace = m_multiRunState.traces[key];

    trace.traceType = aTraceType;

    if( trace.xValues.empty() )
        trace.xValues = aX;

    if( trace.xValues.size() != aX.size() )
        return;

    size_t index = m_multiRunState.currentStep;

    if( trace.yValues.size() <= index )
        trace.yValues.resize( index + 1 );

    trace.yValues[index] = aY;

    if( aTraceType & SPT_SP_SMITH )
    {
        if( trace.xRuns.size() <= index )
            trace.xRuns.resize( index + 1 );

        trace.xRuns[index] = aX;
    }
}


bool SIMULATOR_FRAME_UI::hasMultiRunTrace( const wxString& aVectorName, int aTraceType ) const
{
    std::string key = multiRunTraceKey( aVectorName, aTraceType );
    auto        it = m_multiRunState.traces.find( key );

    if( it == m_multiRunState.traces.end() )
        return false;

    const MULTI_RUN_TRACE& trace = it->second;

    return !trace.xValues.empty() && !trace.yValues.empty();
}


void SIMULATOR_FRAME_UI::OnModify()
{
    m_simulatorFrame->OnModify();
}
