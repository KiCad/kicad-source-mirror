/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#include <kiface_base.h>
#include <confirm.h>
#include <dialogs/dialog_print_generic.h>
#include <gerbview_printout.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <tool/tool_manager.h>
#include <tools/gerbview_actions.h>
#include <tools/gerbview_control.h>
#include <wx/checklst.h>

// TODO(JE)
#define OPTKEY_LAYERBASE             wxT( "PlotLayer_%d" )


class DIALOG_PRINT_GERBVIEW : public DIALOG_PRINT_GENERIC
{
public:
    DIALOG_PRINT_GERBVIEW( GERBVIEW_FRAME* aParent, BOARD_PRINTOUT_SETTINGS* aSettings );
    ~DIALOG_PRINT_GERBVIEW() = default;

private:
    BOARD_PRINTOUT_SETTINGS* settings() const
    {
        wxASSERT( dynamic_cast<BOARD_PRINTOUT_SETTINGS*>( m_settings ) );
        return static_cast<BOARD_PRINTOUT_SETTINGS*>( m_settings );
    }

    bool TransferDataToWindow() override;

    void createExtraOptions();
    void createLeftPanel();

    void onSelectAllClick( wxCommandEvent& event );
    void onDeselectAllClick( wxCommandEvent& event );

    ///< (Un)check all items in a checklist box
    void setListBoxValue( wxCheckListBox* aList, bool aValue );

    ///< Check whether a layer is enabled in a listbox
    bool isLayerEnabled( unsigned int aLayer ) const;

    ///< Update layerset basing on the selected layers
    int setLayerSetFromList();

    void saveSettings() override;

    wxPrintout* createPrintout( const wxString& aTitle ) override
    {
        return new GERBVIEW_PRINTOUT( m_parent->GetGerberLayout(), *settings(), m_parent->GetCanvas()->GetView(),
                                      aTitle );
    }

    GERBVIEW_FRAME* m_parent;

    // Number of layers in each list
    static constexpr unsigned int LAYER_PER_LIST = 16;

    // Number of layer list widgets
    static constexpr unsigned int LAYER_LIST_COUNT = 2;

    // Extra widgets
    wxCheckListBox* m_layerLists[LAYER_LIST_COUNT];
    wxButton* m_buttonSelectAll;
    wxButton* m_buttonDeselectAll;
    wxCheckBox* m_checkboxMirror;

    // Map layer numbers to items on the list
    std::unordered_map<int, int> m_layerToItemMap;
};


DIALOG_PRINT_GERBVIEW::DIALOG_PRINT_GERBVIEW( GERBVIEW_FRAME* aParent, BOARD_PRINTOUT_SETTINGS* aSettings ) :
        DIALOG_PRINT_GENERIC( aParent, aSettings ),
        m_parent( aParent )
{
    createExtraOptions();
    createLeftPanel();
}


bool DIALOG_PRINT_GERBVIEW::TransferDataToWindow()
{
    if( !DIALOG_PRINT_GENERIC::TransferDataToWindow() )
        return false;

    GERBER_FILE_IMAGE_LIST* images = m_parent->GetGerberLayout()->GetImagesList();
    int itemIdx = 0;

    // Create layer list
    for( unsigned ii = 0; ii < images->ImagesMaxCount(); ++ii )
    {
        unsigned int listIdx = itemIdx / LAYER_PER_LIST;

        if( listIdx >= LAYER_LIST_COUNT )
        {
            wxFAIL;
            break;
        }

        GERBER_FILE_IMAGE* gbrImage = images->GetGbrImage( ii );

        if( !gbrImage )
            continue;

        wxFileName filename( gbrImage->m_FileName );
        wxCheckListBox* listBox = m_layerLists[listIdx];
        listBox->Append( filename.GetFullName() );

        if( settings()->m_LayerSet.test( ii) )
            listBox->Check( ii, true );

        wxASSERT( m_layerToItemMap.count( ii ) == 0 );
        m_layerToItemMap[ii] = itemIdx;

        ++itemIdx;
    }

    m_checkboxMirror->SetValue( settings()->m_Mirror );

    // Update the dialog layout when layers are added
    GetSizer()->Fit( this );

    return true;
}


void DIALOG_PRINT_GERBVIEW::createExtraOptions()
{
    wxGridBagSizer* optionsSizer = getOptionsSizer();
    wxStaticBox* box = getOptionsBox();
    int rows = optionsSizer->GetEffectiveRowsCount();
    int cols = optionsSizer->GetEffectiveColsCount();

    // Print mirrored
    m_checkboxMirror = new wxCheckBox( box, wxID_ANY, _( "Print mirrored" ) );
    optionsSizer->Add( m_checkboxMirror, wxGBPosition( rows, 0 ), wxGBSpan( 1, cols ), wxBOTTOM|wxRIGHT|wxLEFT, 5 );
}


void DIALOG_PRINT_GERBVIEW::createLeftPanel()
{
    wxStaticBox* box = new wxStaticBox( this, wxID_ANY, _( "Include Layers" ) );
    wxStaticBoxSizer* sbLayersSizer = new wxStaticBoxSizer( box, wxVERTICAL );

    // Layer lists
    wxBoxSizer* bLayerListsSizer = new wxBoxSizer( wxHORIZONTAL );

    for( unsigned int i = 0; i < LAYER_LIST_COUNT; ++i )
    {
        m_layerLists[i] = new wxCheckListBox( sbLayersSizer->GetStaticBox(), wxID_ANY );
        bLayerListsSizer->Add( m_layerLists[i], 1, wxEXPAND, 5 );
    }

    // Select/Unselect all buttons
    m_buttonSelectAll = new wxButton( sbLayersSizer->GetStaticBox(), wxID_ANY, _( "Select all" ) );
    m_buttonDeselectAll = new wxButton( sbLayersSizer->GetStaticBox(), wxID_ANY, _( "Deselect all" ) );

    m_buttonSelectAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                wxCommandEventHandler( DIALOG_PRINT_GERBVIEW::onSelectAllClick ), nullptr, this );
    m_buttonDeselectAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                  wxCommandEventHandler( DIALOG_PRINT_GERBVIEW::onDeselectAllClick ), nullptr, this );

    wxBoxSizer* buttonSizer = new wxBoxSizer( wxHORIZONTAL );
    buttonSizer->Add( m_buttonSelectAll, 1, wxALL, 5 );
    buttonSizer->Add( m_buttonDeselectAll, 1, wxALL, 5 );

    // Static box sizer layout
    sbLayersSizer->Add( bLayerListsSizer, 1, wxEXPAND, 5 );
    sbLayersSizer->Add( buttonSizer, 0, wxEXPAND, 5 );

    getMainSizer()->Insert( 0, sbLayersSizer, 1, wxEXPAND | wxALL, 5 );
}


void DIALOG_PRINT_GERBVIEW::onSelectAllClick( wxCommandEvent& event )
{
    for( wxCheckListBox* checkbox : m_layerLists )
        setListBoxValue( checkbox, true );
}


void DIALOG_PRINT_GERBVIEW::onDeselectAllClick( wxCommandEvent& event )
{
    for( wxCheckListBox* checkbox : m_layerLists )
        setListBoxValue( checkbox, false );
}


void DIALOG_PRINT_GERBVIEW::setListBoxValue( wxCheckListBox* aList, bool aValue )
{
    for( unsigned int i = 0; i < aList->GetCount(); ++i )
        aList->Check( i, aValue );
}


bool DIALOG_PRINT_GERBVIEW::isLayerEnabled( unsigned int aLayer ) const
{
    auto layerMapIt = m_layerToItemMap.find( aLayer );

    if( layerMapIt == m_layerToItemMap.end() )
        return false;

    unsigned int itemNr = layerMapIt->second;
    unsigned int listIdx = itemNr / LAYER_PER_LIST;
    unsigned int itemIdx = itemNr % LAYER_PER_LIST;
    wxCHECK( listIdx < LAYER_LIST_COUNT, false );
    wxCheckListBox* listBox = m_layerLists[listIdx];

    return itemIdx < listBox->GetCount() && listBox->IsChecked( itemIdx );
}


int DIALOG_PRINT_GERBVIEW::setLayerSetFromList()
{
    settings()->m_LayerSet = LSET();
    int& pageCount = settings()->m_pageCount;
    pageCount = 0;

    unsigned int layer = 0;

    for( unsigned int j = 0; j < LAYER_LIST_COUNT; ++j )
    {
        for( unsigned int i = 0; i < LAYER_PER_LIST; ++i )
        {
            if( isLayerEnabled( layer ) )
            {
                settings()->m_LayerSet.set( layer );
                ++pageCount;
            }

            ++layer;
        }
    }

    return pageCount;
}


void DIALOG_PRINT_GERBVIEW::saveSettings()
{
    setLayerSetFromList();

    settings()->m_Mirror = m_checkboxMirror->GetValue();

    DIALOG_PRINT_GENERIC::saveSettings();
}


int GERBVIEW_CONTROL::Print( const TOOL_EVENT& aEvent )
{
    // Selection affects the original item visibility
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    BOARD_PRINTOUT_SETTINGS settings( m_frame->GetPageSettings() );
    settings.m_colorSettings = m_frame->GetColorSettings();
    DIALOG_PRINT_GERBVIEW dlg( m_frame, &settings );
    dlg.ForcePrintBorder( false );
    dlg.ShowModal();

    return 0;
}
