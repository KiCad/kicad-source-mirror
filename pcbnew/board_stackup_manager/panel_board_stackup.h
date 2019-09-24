/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_SETUP_BOARD_STACKUP_H
#define PANEL_SETUP_BOARD_STACKUP_H


#include <fctsys.h>
#include <pcbnew.h>
#include <class_board.h>
#include <widgets/unit_binder.h>

#include "panel_board_stackup_base.h"
#include "class_board_stackup.h"
#include "stackup_predefined_prms.h"
#include "dielectric_material.h"

class wxBitmapComboBox;
class PANEL_SETUP_LAYERS;


// A helper class to handle UI items managed by m_fgGridSizer
// in PANEL_SETUP_BOARD_STACKUP
// these items are shown or not in m_fgGridSizer, depending on
// the enabled layers in the current board.
// So we need to store the list of these UI items int m_fgGridSizer
// row by row
struct BOARD_STACKUP_ROW_UI_ITEM
{
    bool            m_isEnabled;        // True if the row is in board
                                        // false if not (this row is not shown on the panel)
    wxStaticBitmap* m_Icon;             // Color icon in first column (column 1)
    wxStaticText*   m_LayerName;        // string shown in column 2
    wxControl*      m_LayerTypeCtrl;    // control shown in column 3
    wxControl*      m_MaterialCtrl;     // control shown in column 4, with m_MaterialButt
    wxButton*       m_MaterialButt;     // control shown in column 4, with m_MaterialCtrl
    wxControl*      m_ThicknessCtrl;    // control shown in column 5
    wxControl*      m_ThicknessLockCtrl;// control shown in column 6
    wxControl*      m_ColorCtrl;        // control shown in column 7
    wxControl*      m_EpsilonCtrl;      // control shown in column 8
    wxControl*      m_LossTgCtrl;       // control shown in column 9

    BOARD_STACKUP_ROW_UI_ITEM() :
        m_isEnabled( true ), m_Icon( nullptr ), m_LayerName( nullptr ),
        m_LayerTypeCtrl( nullptr ),
        m_MaterialCtrl( nullptr ),m_MaterialButt( nullptr ),
        m_ThicknessCtrl( nullptr ), m_ThicknessLockCtrl( nullptr ),
        m_ColorCtrl( nullptr ),
        m_EpsilonCtrl( nullptr ), m_LossTgCtrl( nullptr )
    {}
};


class PANEL_SETUP_BOARD_STACKUP : public PANEL_SETUP_BOARD_STACKUP_BASE
{
public:
    PANEL_SETUP_BOARD_STACKUP( PAGED_DIALOG* aParent,
                               PCB_EDIT_FRAME* aFrame,
                               PANEL_SETUP_LAYERS* aPanelLayers );
    ~PANEL_SETUP_BOARD_STACKUP();

    void ImportSettingsFrom( BOARD* aBoard );

    /** Must be called if the copper layers count has changed
     * or solder mask, solder paste or silkscreen layers are
     * enabled or disabled
     * Rebuild the Layer Stack Panel if the new layer set differs
     * from the current layet set
     */
    void OnLayersOptionsChanged( LSET aNewLayerSet );

    BOARD_STACKUP_ITEM* GetStackupItem( int aIndex );

    /// Return the color currently selected for the row aRow
    wxColor GetSelectedColor( int aRow ) const;

    BOARD_STACKUP&  GetStackup() { return m_stackup; }
    int GetPcbTickness();

    // Called by wxWidgets: transfer current settings stored in m_stackup to the board
    bool TransferDataFromWindow() override;

    std::vector<wxColor> m_UserColors;  // the list of user colors for each grid row
                                        // other colors are defined colors, and are not stored
private:
    /** add a Spacer in m_fgGridSizer when a empty cell is needed
     */
    wxControl* addSpacer();

    /** add a control (a wxTextCtrl + a button) in m_fgGridSizer to select a material
     * @param aId is the wxControl id, used to know the event source
     * @param aMaterialName is the the name of the currently selected material (can be null)
     * @param aUiRowItem is the the BOARD_STACKUP_ROW_UI_ITEM to store the controls
     * created
     */
    void addMaterialChooser( wxWindowID aId, const wxString * aMaterialName,
                             BOARD_STACKUP_ROW_UI_ITEM& aUiRowItem );

    /** Populate m_fgGridSizer with items to handle stackup parameters
     * This is a full list:
     * all copper layers and all tech layers that are supported by the stackup
     * items not in the current board stackup will be not shown, but they are
     * existing in list
     */
    void buildLayerStackPanel();

    /** Show or do not show items in m_fgGridSizer according to the stackup of the
     * current board and optionally update the stackup params (thickness, color ... )
     * @param aFullSync = true to update stackup params, false to only update the list
     * of shown items
     */
    void synchronizeWithBoard( bool aFullSync );

    /** Populate m_fgGridSizer with items to handle stackup parameters
     * If previous items are in list, remove old items
     */
    void RebuildLayerStackPanel();

    /** Transfer current UI settings to m_stackup but not to the board
     */
    bool transferDataFromUIToStackup();

	void onUpdateThicknessValue( wxUpdateUIEvent& event ) override;
	void onCalculateDielectricThickness( wxCommandEvent& event ) override;

    void onColorSelected( wxCommandEvent& event );
	void onMaterialChange( wxCommandEvent& event );
	void onThicknessChange( wxCommandEvent& event );
	void onExportToClipboard( wxCommandEvent& event ) override;

    /** Update the icons color (swatches in first grid column)
     * @param aRow is the row (index in m_rowUiItemsList) that manages the icon to update.
     * if -1 all icons will be updated
     */
    void updateIconColor( int aRow = -1 );

    /** @return the color of the BOARD_STACKUP_ITEM at row aRow,
     * to draw a bitmap color according to the selected color
     * or the best default color (for dielectric or copper item)
     * @param aRow is the row index to find the color.
     */
    wxColor getColorIconItem( int aRow );

    /** creates a bitmap combobox to select a layer color
     * @return the created wxBitmapComboBox
     * @param aStackupItem = the BOARD_STACKUP_ITEM realted to the bitmap combobox
     * (to set the user color, if any)
     * can be nullptr
     * @param aRow = the row index in the wxFlexGridSizer (used to build a wxWidget unique id)
     */
    wxBitmapComboBox* createBmComboBox( BOARD_STACKUP_ITEM* aStackupItem, int aRow );

    /**
     * disconnect event handlers connected to wxControl items
     * found in list m_controlItemsList
     */
    void disconnectEvents();

    BOARD_STACKUP   m_stackup;
    PANEL_SETUP_LAYERS* m_panelLayers;      // The associated PANEL_SETUP_LAYERS, to know
                                            // enabled layers and copper layer names
    LSET            m_enabledLayers;        // the current enabled layers in this panel
                                            // restricted to allowed layers in stackup.
                                            // when do not match the enabled layers
                                            // in PANEL_SETUP_LAYERS the stackup is not up to date
    // a list of currently available dielectric materials
    DIELECTRIC_SUBSTRATE_LIST m_delectricMatList;
    // a list of currently available solder mask materials
    DIELECTRIC_SUBSTRATE_LIST m_solderMaskMatList;
    // a list of currently available solder mask materials
    DIELECTRIC_SUBSTRATE_LIST m_silkscreenMatList;
    // List of items in m_fgGridSizer
    std::vector<BOARD_STACKUP_ROW_UI_ITEM> m_rowUiItemsList;
    BOARD*          m_board;
    BOARD_DESIGN_SETTINGS*  m_brdSettings;
    EDA_UNITS_T     m_units;
    PCB_EDIT_FRAME* m_frame;
    wxSize          m_numericTextCtrlSize;  // Best size to enter values with units in wxTextCtrl
    wxSize          m_numericFieldsSize;    // Best size to enter double values in wxTextCtrl
    wxArrayString   m_core_prepreg_choice;  // Used to display the option list in dialog
    wxSize          m_colorSwatchesSize;    // the size of color swatches in the wxBitmapComboBox.
    wxSize          m_colorComboSize;       // the size of the wxBitmapComboBox.
    wxSize          m_colorIconsSize;       // the size of color swatches in grid, left column.

    // The list of controls (wxChoice, wxBitmapComboBox, wxTextCtrl) added to the panel
    // when building the BOARD_STACKUP_ITEM list editor and connected to command events
    // Used to disconnect event handlers
    std::vector<wxControl*> m_controlItemsList;
};

#endif      // #ifndef PANEL_SETUP_BOARD_STACKUP_H
