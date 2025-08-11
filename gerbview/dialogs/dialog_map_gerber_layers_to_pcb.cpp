/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <X2_gerber_attributes.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerbview_settings.h>
#include <kiface_base.h>
#include <layer_ids.h>
#include <lset.h>

#include <dialogs/dialog_map_gerber_layers_to_pcb.h>

#include <wx/msgdlg.h>
#include <gestfich.h>


enum swap_layer_id {
    ID_LAYERS_MAP_DIALOG = ID_GERBER_END_LIST,
    ID_BUTTON_0,
    ID_TEXT_0 = ID_BUTTON_0 + GERBER_DRAWLAYERS_COUNT
};


int DIALOG_MAP_GERBER_LAYERS_TO_PCB::m_exportBoardCopperLayersCount = 2;


BEGIN_EVENT_TABLE( DIALOG_MAP_GERBER_LAYERS_TO_PCB, DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE )
    EVT_COMMAND_RANGE( ID_BUTTON_0, ID_BUTTON_0 + GERBER_DRAWLAYERS_COUNT-1,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       DIALOG_MAP_GERBER_LAYERS_TO_PCB::OnSelectLayer )
END_EVENT_TABLE()


DIALOG_MAP_GERBER_LAYERS_TO_PCB::DIALOG_MAP_GERBER_LAYERS_TO_PCB( GERBVIEW_FRAME* parent ) :
        DIALOG_MAP_GERBER_LAYERS_TO_PCB_BASE( parent )
{
    m_Parent = parent;
    initDialog();

    // Resize the dialog
    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_MAP_GERBER_LAYERS_TO_PCB::initDialog()
{
    wxStaticText* label;
    wxStaticText* text;
    int           item_ID;
    wxString      msg;
    wxSize        goodSize;
    GERBVIEW_SETTINGS* config = static_cast<GERBVIEW_SETTINGS*>( Kiface().KifaceSettings() );

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        // Specify the default value for each member of these arrays.
        m_buttonTable[ii]       = -1;
        m_layersLookUpTable[ii] = UNSELECTED_LAYER;
    }

    // Ensure we have:
    //    At least 2 copper layers and less than max pcb copper layers count
    //    Even number of layers because a board *must* have even layers count
    normalizeBrdLayersCount();

    int idx = ( m_exportBoardCopperLayersCount / 2 ) - 1;
    m_comboCopperLayersCount->SetSelection( idx );

    m_gerberActiveLayersCount      = 0;
    GERBER_FILE_IMAGE_LIST* images = m_Parent->GetGerberLayout()->GetImagesList();

    for( unsigned ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        if( images->GetGbrImage( ii ) == nullptr )
            break;

        m_buttonTable[m_gerberActiveLayersCount] = ii;
        m_gerberActiveLayersCount++;
    }

    if( m_gerberActiveLayersCount <= GERBER_DRAWLAYERS_COUNT / 2 ) // Only one list is enough
        m_staticlineSep->Hide();

    wxFlexGridSizer* flexColumnBoxSizer = m_flexLeftColumnBoxSizer;

    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        // Each Gerber layer has an associated static text string (to
        // identify that layer), a button (for invoking a child dialog
        // box to change which Pcbnew layer that the Gerber layer is
        // mapped to), and a second static text string (to depict which
        // Pcbnew layer that the Gerber layer has been mapped to). Each
        // of those items are placed into the left hand column, middle
        // column, and right hand column (respectively) of the Flexgrid
        // sizer, and the color of the second text string is set to
        // fuchsia or blue (to respectively indicate whether the Gerber
        // layer has been mapped to a Pcbnew layer or is not being
        // exported at all).  (Experimentation has shown that if a text
        // control is used to depict which Pcbnew layer that each Gerber
        // layer is mapped to (instead of a static text string), then
        // those controls do not behave in a fully satisfactory manner
        // in the Linux version. Even when the read-only attribute is
        // specified for all of those controls, they can still be selected
        // when the arrow keys or Tab key is used to step through all of
        // the controls within the dialog box, and directives to set the
        // foreground color of the text of each such control to blue (to
        // indicate that the text is of a read-only nature) are disregarded.
        // Specify a FlexGrid sizer with an appropriate number of rows
        // and three columns.  If nb_items < 16, then the number of rows
        // is nb_items; otherwise, the number of rows is 16 (with two
        // separate columns of controls being used if nb_items > 16).

        if( ii == GERBER_DRAWLAYERS_COUNT / 2 )
            flexColumnBoxSizer = m_flexRightColumnBoxSizer;

        // Provide a text string to identify the Gerber layer
        msg.Printf( _( "Layer %d:" ), m_buttonTable[ii] + 1 );

        label = new wxStaticText( this, wxID_STATIC, msg );
        flexColumnBoxSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        /* Add file name and extension without path.  */
        wxFileName fn( images->GetGbrImage( ii )->m_FileName );
        label = new wxStaticText( this, wxID_STATIC, fn.GetFullName() );
        flexColumnBoxSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        // Provide a button for this layer (which will invoke a child dialog box)
        item_ID = ID_BUTTON_0 + ii;
        wxButton * Button = new wxButton( this, item_ID, wxT( "..." ), wxDefaultPosition, wxDefaultSize,
                                          wxBU_EXACTFIT );

        flexColumnBoxSizer->Add( Button, 0, wxALIGN_CENTER_VERTICAL | wxALL );

        // Provide another text string to specify which Pcbnew layer that this
        // Gerber layer is mapped to.  All layers initially default to
        // "Do NotExport" (which corresponds to UNSELECTED_LAYER).  Whenever
        // a layer is set to "Do Not Export" it's displayed in blue.  When a
        // user selects a specific KiCad layer to map to, it's displayed in
        // magenta which indicates it will be exported.
        item_ID = ID_TEXT_0 + ii;

        // All layers default to "Do Not Export" displayed in blue
        msg  = _( "Do not export" );
        text = new wxStaticText( this, item_ID, msg );
        text->SetForegroundColour( *wxBLUE );

        // When the first of these text strings is being added, determine what
        // size is necessary to to be able to display any possible string
        // without it being truncated. Then specify that size as the minimum
        // size for all of these text strings. (If this minimum size is not
        // determined in this fashion, then it is possible for the display of
        // one or more of these strings to be truncated after different Pcbnew
        // layers are selected.)

        if( ii == 0 )
        {
            goodSize = text->GetSize();

            for( int jj = 0; jj < GERBER_DRAWLAYERS_COUNT; ++jj )
            {
                text->SetLabel( LSET::Name( PCB_LAYER_ID( jj ) ) );

                if( goodSize.x < text->GetSize().x )
                    goodSize.x = text->GetSize().x;
            }
            text->SetLabel( msg ); // Reset label to default text
        }

        text->SetMinSize( goodSize );
        flexColumnBoxSizer->Add( text, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        m_layersList[ii] = text;
    }

    // If the user has never stored any Gerber to KiCad layer mapping,
    // then disable the button to retrieve it
    if( config->m_GerberToPcbLayerMapping.size() == 0 )
        m_buttonRetrieve->Enable( false );

    std::vector<int> gerber2KicadMapping;

    // See how many of the loaded Gerbers can be mapped to KiCad layers automatically
    int numMappedGerbers = findKnownGerbersLoaded( gerber2KicadMapping );

    if( numMappedGerbers > 0 )
    {
        // See if the user wants to map the Altium Gerbers to known KiCad PCB layers
        int returnVal = wxMessageBox( wxString::Format( _( "Gerbers with known layers: %d" ), numMappedGerbers )
                                              + wxT( "\n\n" ) + _( "Assign to matching PCB layers?" ),
                                      _( "Automatic Layer Assignment" ),
                                      wxOK | wxCANCEL | wxOK_DEFAULT );

        if( returnVal == wxOK )
        {
            int total_copper = 0;

            for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
            {
                int currLayer = gerber2KicadMapping[ii];

                if( IsCopperLayer( currLayer ) )
                    total_copper++;

                // Default to "Do Not Export" for unselected or undefined layer
                if( currLayer == UNSELECTED_LAYER )
                {
                    m_layersList[ii]->SetLabel( _( "Do not export" ) );
                    m_layersList[ii]->SetForegroundColour( *wxBLUE );

                    // Set the layer internally to unselected
                    m_layersLookUpTable[ii] = UNSELECTED_LAYER;
                }
                else
                {
                    m_layersList[ii]->SetLabel( LSET::Name( PCB_LAYER_ID( currLayer ) ) );
                    m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );

                    // Set the layer internally to the matching KiCad layer
                    m_layersLookUpTable[ii] = currLayer;
                }
            }

            // Reset the number of copper layers to the total found
            m_exportBoardCopperLayersCount = std::max( total_copper, 2 );
            m_comboCopperLayersCount->SetSelection( ( m_exportBoardCopperLayersCount / 2 ) - 1 );
        }
    }
}


void DIALOG_MAP_GERBER_LAYERS_TO_PCB::normalizeBrdLayersCount()
{
    if( ( m_exportBoardCopperLayersCount & 1 ) )
        m_exportBoardCopperLayersCount++;

    if( m_exportBoardCopperLayersCount > GERBER_DRAWLAYERS_COUNT )
        m_exportBoardCopperLayersCount = GERBER_DRAWLAYERS_COUNT;

    if( m_exportBoardCopperLayersCount < 2 )
        m_exportBoardCopperLayersCount = 2;

}


void DIALOG_MAP_GERBER_LAYERS_TO_PCB::OnBrdLayersCountSelection( wxCommandEvent& event )
{
    int id = event.GetSelection();
    m_exportBoardCopperLayersCount = ( id + 1 ) * 2;
}


void DIALOG_MAP_GERBER_LAYERS_TO_PCB::OnResetClick( wxCommandEvent& event )
{
    for( int ii = 0; ii < m_gerberActiveLayersCount; ++ii )
    {
        m_layersLookUpTable[ii] = UNSELECTED_LAYER;
        m_layersList[ii]->SetLabel( _( "Do not export" ) );
        m_layersList[ii]->SetForegroundColour( *wxBLUE );
        m_buttonTable[ii] = ii;
    }
    // wxWidgets doesn't appear to invalidate / update the StaticText displays for color change
    // so we do it manually
    Refresh();
    Update();
}


void DIALOG_MAP_GERBER_LAYERS_TO_PCB::OnStoreSetup( wxCommandEvent& event )
{
    GERBVIEW_SETTINGS* config = static_cast<GERBVIEW_SETTINGS*>( Kiface().KifaceSettings() );
    config->m_BoardLayersCount = m_exportBoardCopperLayersCount;

    config->m_GerberToPcbLayerMapping.clear();

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
        config->m_GerberToPcbLayerMapping.push_back( m_layersLookUpTable[ii] );

    // Enable the "Get Stored Choice" button in case it was disabled in "initDialog()"
    // due to no previously stored choices.
    m_buttonRetrieve->Enable( true );
}


void DIALOG_MAP_GERBER_LAYERS_TO_PCB::OnGetSetup( wxCommandEvent& event )
{
    GERBVIEW_SETTINGS* config = static_cast<GERBVIEW_SETTINGS*>( Kiface().KifaceSettings() );

    m_exportBoardCopperLayersCount = config->m_BoardLayersCount;
    normalizeBrdLayersCount();

    int idx = ( m_exportBoardCopperLayersCount / 2 ) - 1;
    m_comboCopperLayersCount->SetSelection( idx );

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        // Ensure the layer mapping in config exists for this layer, and store it
        if( (size_t)ii >= config->m_GerberToPcbLayerMapping.size() )
            break;

        m_layersLookUpTable[ii] = config->m_GerberToPcbLayerMapping[ ii ];
    }

    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        int layer = m_layersLookUpTable[ii];

        if( layer == UNSELECTED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Do not export" ) );
            m_layersList[ii]->SetForegroundColour( *wxBLUE );
        }
        else if( layer == UNDEFINED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Hole data" ) );
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
        else
        {
            m_layersList[ii]->SetLabel( LSET::Name( PCB_LAYER_ID( layer ) ) );
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
    }
}


void DIALOG_MAP_GERBER_LAYERS_TO_PCB::OnSelectLayer( wxCommandEvent& event )
{
    int ii = event.GetId() - ID_BUTTON_0;

    if( ii < 0 || ii >= GERBER_DRAWLAYERS_COUNT )
    {
        wxFAIL_MSG( wxT( "Bad layer id" ) );
        return;
    }

    int jj = m_layersLookUpTable[ m_buttonTable[ii] ];

    if( jj != UNSELECTED_LAYER && jj != UNDEFINED_LAYER && !IsValidLayer( jj ) )
        jj = B_Cu;  // (Defaults to "Copper" layer.)

    // Get file name of Gerber loaded on this layer
    wxFileName fn( m_Parent->GetGerberLayout()->GetImagesList()->GetGbrImage( ii )->m_FileName );

    // Surround it with quotes to make it stand out on the dialog title bar
    wxString layerName = wxT( "\"" ) + fn.GetFullName() + wxT( "\"" );

    // Display dialog to let user select a layer for the Gerber
    jj = m_Parent->SelectPCBLayer( jj, m_exportBoardCopperLayersCount, layerName );

    if( jj != UNSELECTED_LAYER && jj != UNDEFINED_LAYER && !IsValidLayer( jj ) )
        return;

    if( jj != m_layersLookUpTable[m_buttonTable[ii]] )
    {
        m_layersLookUpTable[m_buttonTable[ii]] = jj;

        if( jj == UNSELECTED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Do not export" ) );

            // Change the text color to blue (to highlight
            // that this layer is *not* being exported)
            m_layersList[ii]->SetForegroundColour( *wxBLUE );
        }
        else if( jj == UNDEFINED_LAYER )
        {
            m_layersList[ii]->SetLabel( _( "Hole data" ) );

            // Change the text color to fuchsia (to highlight
            // that this layer *is* being exported)
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
        else
        {
            m_layersList[ii]->SetLabel( LSET::Name( PCB_LAYER_ID( jj ) ) );

            // Change the text color to fuchsia (to highlight
            // that this layer *is* being exported)
            m_layersList[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
    }
    // wxWidgets doesn't appear to invalidate / update the StaticText displays for color change
    // so we do it manually
    Refresh();
    Update();
}


bool DIALOG_MAP_GERBER_LAYERS_TO_PCB::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // Board must have enough copper layers to handle selected internal layers.
    normalizeBrdLayersCount();

    int inner_layer_max = 0;

    for( int ii = 0; ii < GERBER_DRAWLAYERS_COUNT; ++ii )
    {
        if( IsInnerCopperLayer( m_layersLookUpTable[ii] ) )
        {
            if( m_layersLookUpTable[ii ] > inner_layer_max )
                inner_layer_max = m_layersLookUpTable[ii];
        }
    }

    // inner_layer_max must be less than or equal to the number of internal copper layers
    // internal copper layers = m_exportBoardCopperLayersCount-2
    if( (int) CopperLayerToOrdinal( ToLAYER_ID( inner_layer_max ) ) > m_exportBoardCopperLayersCount-2 )
    {
        wxMessageBox( _( "Exported board does not have enough copper layers to handle selected "
                         "inner layers" ) );
        return false;
    }

    return true;
}


int DIALOG_MAP_GERBER_LAYERS_TO_PCB::findKnownGerbersLoaded( std::vector<int>& aGerber2KicadMapping )
{
    int numKnownGerbers = 0;

    // Check if any of the loaded Gerbers are X2 Gerbers and if they contain
    // layer information in "File Functions". For info about X2 Gerbers see
    // http://www.ucamco.com/files/downloads/file/81/the_gerber_file_format_specification.pdf
    numKnownGerbers += findNumX2GerbersLoaded( aGerber2KicadMapping );

    // Finally, check if any of the loaded Gerbers use the KiCad naming conventions
    numKnownGerbers += findNumKiCadGerbersLoaded( aGerber2KicadMapping );

    // The last option is to match using just the file extension
    // This checkes for known Altium/Protel file extensions
    numKnownGerbers += findNumAltiumGerbersLoaded( aGerber2KicadMapping );

    return numKnownGerbers;
}


int DIALOG_MAP_GERBER_LAYERS_TO_PCB::findNumAltiumGerbersLoaded( std::vector<int>& aGerber2KicadMapping )
{
    // The next comment preserves initializer formatting below it
    // clang-format off
    // This map contains the known Altium file extensions for Gerbers that we care about,
    // along with their corresponding KiCad layer
    std::map<wxString, PCB_LAYER_ID> altiumExt{
        { wxT( "GTL" ), F_Cu },      // Top copper
        { wxT( "G1" ), In1_Cu },     // Inner layers 1 - 30
        { wxT( "G2" ), In2_Cu },
        { wxT( "G3" ), In3_Cu },
        { wxT( "G4" ), In4_Cu },
        { wxT( "G5" ), In5_Cu },
        { wxT( "G6" ), In6_Cu },
        { wxT( "G7" ), In7_Cu },
        { wxT( "G8" ), In8_Cu },
        { wxT( "G9" ), In9_Cu },
        { wxT( "G10" ), In10_Cu },
        { wxT( "G11" ), In11_Cu },
        { wxT( "G12" ), In12_Cu },
        { wxT( "G13" ), In13_Cu },
        { wxT( "G14" ), In14_Cu },
        { wxT( "G15" ), In15_Cu },
        { wxT( "G16" ), In16_Cu },
        { wxT( "G17" ), In17_Cu },
        { wxT( "G18" ), In18_Cu },
        { wxT( "G19" ), In19_Cu },
        { wxT( "G20" ), In20_Cu },
        { wxT( "G21" ), In21_Cu },
        { wxT( "G22" ), In22_Cu },
        { wxT( "G23" ), In23_Cu },
        { wxT( "G24" ), In24_Cu },
        { wxT( "G25" ), In25_Cu },
        { wxT( "G26" ), In26_Cu },
        { wxT( "G27" ), In27_Cu },
        { wxT( "G28" ), In28_Cu },
        { wxT( "G29" ), In29_Cu },
        { wxT( "G30" ), In30_Cu },
        { wxT( "GBL" ), B_Cu },      // Bottom copper
        { wxT( "GTP" ), F_Paste },   // Paste top
        { wxT( "GBP" ), B_Paste },   // Paste bottom
        { wxT( "GTO" ), F_SilkS },   // Silkscreen top
        { wxT( "GBO" ), B_SilkS },   // Silkscreen bottom
        { wxT( "GTS" ), F_Mask },    // Soldermask top
        { wxT( "GBS" ), B_Mask },    // Soldermask bottom
        { wxT( "GM1" ), Eco1_User }, // Altium mechanical layer 1
        { wxT( "GM2" ), Eco2_User }, // Altium mechanical layer 2
        { wxT( "GKO" ), Edge_Cuts }  // PCB Outline
    };
    // clang-format on

    std::map<wxString, PCB_LAYER_ID>::iterator it;

    int numAltiumMatches = 0; // Assume we won't find Altium Gerbers

    GERBER_FILE_IMAGE_LIST* images = m_Parent->GetGerberLayout()->GetImagesList();

    // If the passed vector isn't empty but is too small to hold the loaded
    // Gerbers, then bail because something isn't right.

    if( ( aGerber2KicadMapping.size() != 0 )
            && ( aGerber2KicadMapping.size() != (size_t) m_gerberActiveLayersCount ) )
        return numAltiumMatches;

    // If the passed vector is empty, set it to the same number of elements as there
    // are loaded Gerbers, and set each to "UNSELECTED_LAYER"

    if( aGerber2KicadMapping.size() == 0 )
        aGerber2KicadMapping.assign( m_gerberActiveLayersCount, UNSELECTED_LAYER );

    // Loop through all loaded Gerbers looking for any with Altium specific extensions
    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        if( images->GetGbrImage( ii ) )
        {
            // Get file name of Gerber loaded on this layer.
            wxFileName fn( images->GetGbrImage( ii )->m_FileName );

            // Get uppercase version of file extension
            wxString FileExt = fn.GetExt();
            FileExt.MakeUpper();

            // Check for matching Altium Gerber file extension we'll handle
            it = altiumExt.find( FileExt );

            if( it != altiumExt.end() )
            {
                // We got a match, so store the KiCad layer number.  We verify it's set to
                // "UNSELECTED_LAYER" in case the passed vector already had entries
                // matched to other known Gerber files.   This will preserve them.

                if( aGerber2KicadMapping[ii] == UNSELECTED_LAYER )
                {
                    aGerber2KicadMapping[ii] = it->second;
                    numAltiumMatches++;
                }
            }
        }
    }

    // Return number of Altium Gerbers we found.  Each index in the passed vector corresponds to
    // a loaded Gerber layer, and the entry will contain the index to the matching
    // KiCad layer for Altium Gerbers, or "UNSELECTED_LAYER" for the rest.
    return numAltiumMatches;
}


int DIALOG_MAP_GERBER_LAYERS_TO_PCB::findNumKiCadGerbersLoaded( std::vector<int>& aGerber2KicadMapping )
{
    // The next comment preserves initializer formatting below it
    // clang-format off
    // This map contains the known KiCad suffixes used for Gerbers that we care about,
    // along with their corresponding KiCad layer
    std::map<wxString, PCB_LAYER_ID> kicadLayers
    {
        { "-F_Cu",        F_Cu },
        { "-In1_Cu",      In1_Cu },
        { "-In2_Cu",      In2_Cu },
        { "-In3_Cu",      In3_Cu },
        { "-In4_Cu",      In4_Cu },
        { "-In5_Cu",      In5_Cu },
        { "-In6_Cu",      In6_Cu },
        { "-In7_Cu",      In7_Cu },
        { "-In8_Cu",      In8_Cu },
        { "-In9_Cu",      In9_Cu },
        { "-In10_Cu",     In10_Cu },
        { "-In11_Cu",     In11_Cu },
        { "-In12_Cu",     In12_Cu },
        { "-In13_Cu",     In13_Cu },
        { "-In14_Cu",     In14_Cu },
        { "-In15_Cu",     In15_Cu },
        { "-In16_Cu",     In16_Cu },
        { "-In17_Cu",     In17_Cu },
        { "-In18_Cu",     In18_Cu },
        { "-In19_Cu",     In19_Cu },
        { "-In20_Cu",     In20_Cu },
        { "-In21_Cu",     In21_Cu },
        { "-In22_Cu",     In22_Cu },
        { "-In23_Cu",     In23_Cu },
        { "-In24_Cu",     In24_Cu },
        { "-In25_Cu",     In25_Cu },
        { "-In26_Cu",     In26_Cu },
        { "-In27_Cu",     In27_Cu },
        { "-In28_Cu",     In28_Cu },
        { "-In29_Cu",     In29_Cu },
        { "-In30_Cu",     In30_Cu },
        { "-B_Cu",        B_Cu },
        { "-B_Adhes",     B_Adhes },
        { "-F_Adhes",     F_Adhes },
        { "-B_Adhesive",  B_Adhes },
        { "-F_Adhesive",  F_Adhes },
        { "-B_Paste",     B_Paste },
        { "-F_Paste",     F_Paste },
        { "-B_SilkS",     B_SilkS },
        { "-F_SilkS",     F_SilkS },
        { "-B_Silkscreen",B_SilkS },
        { "-F_Silkscreen",F_SilkS },
        { "-B_Mask",      B_Mask },
        { "-F_Mask",      F_Mask },
        { "-F_Fab",       F_Fab },
        { "-B_Fab",       B_Fab },
        { "-Dwgs_User",   Dwgs_User },
        { "-Cmts_User",   Cmts_User },
        { "-Eco1_User",   Eco1_User },
        { "-Eco2_User",   Eco2_User },
        { "-Edge_Cuts",   Edge_Cuts },
        { "-Margin",      Margin },
        { "-F_Courtyard", F_CrtYd },
        { "-B_Courtyard", B_CrtYd },
    };
    // clang-format on

    std::map<wxString, PCB_LAYER_ID>::iterator it;

    int numKicadMatches = 0; // Assume we won't find KiCad Gerbers

    GERBER_FILE_IMAGE_LIST* images = m_Parent->GetGerberLayout()->GetImagesList();

    // If the passed vector isn't empty but is too small to hold the loaded
    // Gerbers, then bail because something isn't right.

    if( ( aGerber2KicadMapping.size() != 0 )
            && ( aGerber2KicadMapping.size() < (size_t) m_gerberActiveLayersCount ) )
        return numKicadMatches;

    // If the passed vector is empty, set it to the same number of elements as there
    // are loaded Gerbers, and set each to "UNSELECTED_LAYER"

    if( aGerber2KicadMapping.size() == 0 )
        aGerber2KicadMapping.assign( m_gerberActiveLayersCount, UNSELECTED_LAYER );

    // Loop through all loaded Gerbers looking for any with KiCad specific layer names
    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        if( images->GetGbrImage( ii ) )
        {
            // Get file name of Gerber loaded on this layer.
            wxFileName fn( images->GetGbrImage( ii )->m_FileName );

            wxString layerName = fn.GetName();

            // To create Gerber file names, KiCad appends a suffix consisting of a "-" and the
            // name of the layer to the project name.  We need to isolate the suffix if present
            // and see if it's a known KiCad layer name.  Start by looking for the last "-" in
            // the file name.
            int dashPos = layerName.Find( '-', true );

            // If one was found, isolate the suffix from the "-" to the end of the file name
            wxString suffix;

            if( dashPos != wxNOT_FOUND )
                suffix = layerName.Right( layerName.length() - dashPos );

            // Check if the string we've isolated matches any known KiCad layer names
            it = kicadLayers.find( suffix );

            if( it != kicadLayers.end() )
            {
                // We got a match, so store the KiCad layer number.  We verify it's set to
                // "UNSELECTED_LAYER" in case the passed vector already had entries
                // matched to other known Gerber files.  This will preserve them.

                if( aGerber2KicadMapping[ii] == UNSELECTED_LAYER )
                {
                    aGerber2KicadMapping[ii] = it->second;
                    numKicadMatches++;
                }
            }
        }
    }

    // Return number of KiCad Gerbers we found.  Each index in the passed vector corresponds to
    // a loaded Gerber layer, and the entry will contain the index to the matching
    // KiCad layer for KiCad Gerbers, or "UNSELECTED_LAYER" for the rest.
    return numKicadMatches;
}


int DIALOG_MAP_GERBER_LAYERS_TO_PCB::findNumX2GerbersLoaded( std::vector<int>& aGerber2KicadMapping )
{
    // The next comment preserves initializer formatting below it
    // clang-format off
    // This map contains the known KiCad X2 "File Function" values used for Gerbers that we
    // care about, along with their corresponding KiCad layer
    std::map<wxString, PCB_LAYER_ID> kicadLayers
    {
        { wxT( "Top" ),   F_Cu },
        { wxT( "L2" ),    In1_Cu },
        { wxT( "L3" ),    In2_Cu },
        { wxT( "L4" ),    In3_Cu },
        { wxT( "L5" ),    In4_Cu },
        { wxT( "L6" ),    In5_Cu },
        { wxT( "L7" ),    In6_Cu },
        { wxT( "L8" ),    In7_Cu },
        { wxT( "L9" ),    In8_Cu },
        { wxT( "L10" ),   In9_Cu },
        { wxT( "L11" ),   In10_Cu },
        { wxT( "L12" ),   In11_Cu },
        { wxT( "L13" ),   In12_Cu },
        { wxT( "L14" ),   In13_Cu },
        { wxT( "L15" ),   In14_Cu },
        { wxT( "L16" ),   In15_Cu },
        { wxT( "L17" ),   In16_Cu },
        { wxT( "L18" ),   In17_Cu },
        { wxT( "L19" ),   In18_Cu },
        { wxT( "L20" ),   In19_Cu },
        { wxT( "L21" ),   In20_Cu },
        { wxT( "L22" ),   In21_Cu },
        { wxT( "L23" ),   In22_Cu },
        { wxT( "L24" ),   In23_Cu },
        { wxT( "L25" ),   In24_Cu },
        { wxT( "L26" ),   In25_Cu },
        { wxT( "L27" ),   In26_Cu },
        { wxT( "L28" ),   In27_Cu },
        { wxT( "L29" ),   In28_Cu },
        { wxT( "L30" ),   In29_Cu },
        { wxT( "Bot" ),   B_Cu },
        { wxT( "BotGlue" ),            B_Adhes },
        { wxT( "TopGlue" ),            F_Adhes },
        { wxT( "BotPaste" ),           B_Paste },
        { wxT( "TopPaste" ),           F_Paste },
        { wxT( "BotLegend" ),          B_SilkS },
        { wxT( "TopLegend" ),          F_SilkS },
        { wxT( "BotSoldermask" ),      B_Mask },
        { wxT( "TopSoldermask" ),      F_Mask },
        { wxT( "FabricationDrawing" ), Dwgs_User },
        { wxT( "OtherDrawing" ),       Cmts_User },
        { wxT( "TopAssemblyDrawing" ), F_Fab },
        { wxT( "BotAssemblyDrawing" ), B_Fab },
        { wxT( "PProfile" ),           Edge_Cuts }, // Plated PCB outline
        { wxT( "NPProfile" ),          Edge_Cuts }  // Non-plated PCB outline
    };
    // clang-format on

    std::map<wxString, PCB_LAYER_ID>::iterator it;

    int numKicadMatches = 0; // Assume we won't find KiCad Gerbers

    wxString mapThis;

    GERBER_FILE_IMAGE_LIST* images = m_Parent->GetGerberLayout()->GetImagesList();

    // If the passed vector isn't empty but is too small to hold the loaded
    // Gerbers, then bail because something isn't right.

    if( ( aGerber2KicadMapping.size() != 0 )
            && ( aGerber2KicadMapping.size() < (size_t) m_gerberActiveLayersCount ) )
    {
        return numKicadMatches;
    }

    // If the passed vector is empty, set it to the same number of elements as there
    // are loaded Gerbers, and set each to "UNSELECTED_LAYER"

    if( aGerber2KicadMapping.size() == 0 )
        aGerber2KicadMapping.assign( m_gerberActiveLayersCount, UNSELECTED_LAYER );

    // Loop through all loaded Gerbers looking for any with X2 File Functions
    for( int ii = 0; ii < m_gerberActiveLayersCount; ii++ )
    {
        if( images->GetGbrImage( ii ) )
        {
            X2_ATTRIBUTE_FILEFUNCTION* x2 = images->GetGbrImage( ii )->m_FileFunction;

            mapThis = "";

            if( images->GetGbrImage( ii )->m_IsX2_file )
            {
                wxCHECK( x2, numKicadMatches );

                if( x2->IsCopper() )
                {
                    // This is a copper layer, so figure out which one
                    mapThis = x2->GetBrdLayerSide(); // Returns "Top", "Bot" or "Inr"

                    // To map inner layers properly, we need the layer number
                    if( mapThis.IsSameAs( wxT( "Inr" ), false ) )
                        mapThis = x2->GetBrdLayerId(); // Returns "L2", "L5", etc
                }
                else
                {
                    // Create strings like "TopSolderMask" or "BotPaste" for non-copper layers
                    mapThis << x2->GetBrdLayerId() << x2->GetFileType();
                }

                // Check if the string we've isolated matches any known X2 layer names
                it = kicadLayers.find( mapThis );

                if( it != kicadLayers.end() )
                {
                    // We got a match, so store the KiCad layer number.  We verify it's set to
                    // "UNSELECTED_LAYER" in case the passed vector already had entries
                    // matched to other known Gerber files.   This will preserve them.

                    if( aGerber2KicadMapping[ii] == UNSELECTED_LAYER )
                    {
                        aGerber2KicadMapping[ii] = it->second;
                        numKicadMatches++;
                    }
                }
            }
        }
    }

    return numKicadMatches;
}
