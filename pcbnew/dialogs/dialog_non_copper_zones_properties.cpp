/**
 * @file zones_non_copper_type_functions.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <base_units.h>

#include <class_board.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>

#include <wx/imaglist.h>    // needed for wx/listctrl.h, in wxGTK 2.8.12

#include <dialog_non_copper_zones_properties_base.h>

#define LAYER_BITMAP_SIZE_X     20
#define LAYER_BITMAP_SIZE_Y     10

/**
 * Class DIALOG_NON_COPPER_ZONES_EDITOR
 * is a dialog editor for non copper zones properties,
 * derived from DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE, which is maintained and
 * created by wxFormBuilder
 */
class DIALOG_NON_COPPER_ZONES_EDITOR : public DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE
{
private:
    PCB_BASE_FRAME* m_parent;
    ZONE_CONTAINER* m_zone;
    ZONE_SETTINGS*  m_ptr;
    ZONE_SETTINGS   m_settings;     // working copy of zone settings

    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void Init();

public:
    DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent,
                                    ZONE_CONTAINER* aZone, ZONE_SETTINGS* aSettings );

private:
    /**
     * Function makeLayerBitmap
     * creates the colored rectangle bitmaps used in the layer selection widget.
     * @param aColor is the color to fill the rectangle with.
     */
    wxBitmap makeLayerBitmap( EDA_COLOR_T aColor );
};


ZONE_EDIT_T InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aParent,
                                        ZONE_CONTAINER* aZone, ZONE_SETTINGS* aSettings )
{
    DIALOG_NON_COPPER_ZONES_EDITOR  dlg( aParent, aZone, aSettings );

    ZONE_EDIT_T result = ZONE_EDIT_T( dlg.ShowModal() );

    return result;
}


DIALOG_NON_COPPER_ZONES_EDITOR::DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent,
                                                                ZONE_CONTAINER* aZone,
                                                                ZONE_SETTINGS* aSettings ) :
    DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;

    m_zone = aZone;
    m_ptr  = aSettings;
    m_settings = *aSettings;

    Init();

    // the size of some items has changed, so we must call SetSizeHints()
    GetSizer()->SetSizeHints( this );
}


void DIALOG_NON_COPPER_ZONES_EDITOR::Init()
{
    BOARD* board = m_parent->GetBoard();

    SetReturnCode( ZONE_ABORT );  // Will be changed on button click

    AddUnitSymbol( *m_MinThicknessValueTitle, g_UserUnit );
    wxString msg = StringFromValue( g_UserUnit, m_settings.m_ZoneMinThickness );
    m_ZoneMinThicknessCtrl->SetValue( msg );

    if( m_settings.m_Zone_45_Only )
        m_OrientEdgesOpt->SetSelection( 1 );

    switch( m_settings.m_Zone_HatchingStyle )
    {
    case CPolyLine::NO_HATCH:
        m_OutlineAppearanceCtrl->SetSelection( 0 );
        break;

    case CPolyLine::DIAGONAL_EDGE:
        m_OutlineAppearanceCtrl->SetSelection( 1 );
        break;

    case CPolyLine::DIAGONAL_FULL:
        m_OutlineAppearanceCtrl->SetSelection( 2 );
        break;
    }

    // Create one column in m_LayerSelectionCtrl
    wxListItem column0;
    column0.SetId( 0 );
    m_LayerSelectionCtrl->InsertColumn( 0, column0 );

    // Create an icon list:
    wxImageList* imageList = new wxImageList( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
    m_LayerSelectionCtrl->AssignImageList( imageList, wxIMAGE_LIST_SMALL );

    int lyrSelect = ( (PCB_SCREEN*) m_parent->GetScreen() )->m_Active_Layer;

    if( m_zone )
        lyrSelect = m_zone->GetLayer();

    int ctrlWidth = 0;  // Min width for m_LayerSelectionCtrl to show the layers names
    int imgIdx = 0;

    for( LSEQ seq = LSET::AllNonCuMask().Seq(); seq; ++seq, ++imgIdx )
    {
        LAYER_ID layer = *seq;

        EDA_COLOR_T layerColor = board->GetLayerColor( layer );
        imageList->Add( makeLayerBitmap( layerColor ) );

        wxString msg = board->GetLayerName( layer );
        msg.Trim();

        int itemIndex = m_LayerSelectionCtrl->InsertItem(
                m_LayerSelectionCtrl->GetItemCount(), msg, imgIdx );

        if(lyrSelect == layer )
            m_LayerSelectionCtrl->Select( itemIndex );

        wxSize tsize( GetTextSize( msg, m_LayerSelectionCtrl ) );
        ctrlWidth = std::max( ctrlWidth, tsize.x );
    }

    // The most easy way to ensure the right size is to use wxLIST_AUTOSIZE
    // unfortunately this option does not work well both on
    // wxWidgets 2.8 ( column witdth too small), and
    // wxWidgets 2.9 ( column witdth too large)
    ctrlWidth += LAYER_BITMAP_SIZE_X + 25;      // Add bitmap width + margin between bitmap and text
    m_LayerSelectionCtrl->SetColumnWidth( 0, ctrlWidth );

    ctrlWidth += 25;        // add small margin between text and window borders
                            // and room for vertical scroll bar
    m_LayerSelectionCtrl->SetMinSize( wxSize( ctrlWidth, -1 ) );
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnOkClick( wxCommandEvent& event )
{
    wxString txtvalue = m_ZoneMinThicknessCtrl->GetValue();

    m_settings.m_ZoneMinThickness = ValueFromString( g_UserUnit, txtvalue );

    if( m_settings.m_ZoneMinThickness < 10 )
    {
        DisplayError( this,
                      _( "Error :\nyou must choose a min thickness value bigger than 0.001 inch (or 0.0254 mm)" ) );
        return;
    }

    m_settings.m_FillMode = 0;  // Use always polygon fill mode

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        m_settings.m_Zone_HatchingStyle = CPolyLine::NO_HATCH;
        break;

    case 1:
        m_settings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;
        break;

    case 2:
        m_settings.m_Zone_HatchingStyle = CPolyLine::DIAGONAL_FULL;
        break;
    }

    wxConfigBase* cfg = Kiface().KifaceSettings();
    wxASSERT( cfg );

    cfg->Write( ZONE_NET_OUTLINES_HATCH_OPTION_KEY, (long) m_settings.m_Zone_HatchingStyle );

    if( m_OrientEdgesOpt->GetSelection() == 0 )
        m_settings.m_Zone_45_Only = false;
    else
        m_settings.m_Zone_45_Only = true;

    // Get the layer selection for this zone
    int ii = m_LayerSelectionCtrl->GetFirstSelected();

    if( ii < 0 )
    {
        DisplayError( this, _( "Error : you must choose a layer" ) );
        return;
    }

    LSEQ seq = LSET::AllNonCuMask().Seq();

    m_settings.m_CurrentZone_Layer = seq[ii];

    *m_ptr = m_settings;

    EndModal( ZONE_OK );
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnCancelClick( wxCommandEvent& event )
{
    // do not save the edits.

    EndModal( ZONE_ABORT );
}


wxBitmap DIALOG_NON_COPPER_ZONES_EDITOR::makeLayerBitmap( EDA_COLOR_T aColor )
{
    wxBitmap    bitmap( LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );
    wxBrush     brush;
    wxMemoryDC  iconDC;

    iconDC.SelectObject( bitmap );
    brush.SetColour( MakeColour( aColor ) );

#if wxCHECK_VERSION( 3, 0, 0 )
    brush.SetStyle( wxBRUSHSTYLE_SOLID );
#else
    brush.SetStyle( wxSOLID );
#endif

    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, LAYER_BITMAP_SIZE_X, LAYER_BITMAP_SIZE_Y );

    return bitmap;
}
