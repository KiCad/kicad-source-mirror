/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file swap_layers.cpp
 * @brief Dialog to swap layers.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <dialog_shim.h>

#include <class_board.h>
#include <class_track.h>
#include <class_drawsegment.h>

#include <pcbnew.h>

#include <wx/statline.h>


#define NO_CHANGE     LAYER_ID(-3)


enum swap_layer_id {
    ID_WINEDA_SWAPLAYERFRAME = 1800,
    ID_BUTTON_0,
    ID_TEXT_0 = ID_BUTTON_0 + LAYER_ID_COUNT
};


class SWAP_LAYERS_DIALOG : public DIALOG_SHIM
{
public:
    SWAP_LAYERS_DIALOG( PCB_BASE_FRAME* parent, LAYER_ID* aArray );
    // ~SWAP_LAYERS_DIALOG() { };

private:
    PCB_BASE_FRAME*         m_Parent;
    wxBoxSizer*             OuterBoxSizer;
    wxBoxSizer*             MainBoxSizer;
    wxFlexGridSizer*        FlexColumnBoxSizer;
    wxStaticText*           label;
    wxButton*               Button;
    wxStaticText*           text;
    wxStaticLine*           Line;
    wxStdDialogButtonSizer* StdDialogButtonSizer;

    LAYER_ID*               m_callers_nlayers;          // DIM() is LAYER_ID_COUNT
    wxStaticText*           layer_list[LAYER_ID_COUNT];

    void Sel_Layer( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( SWAP_LAYERS_DIALOG, wxDialog )
    EVT_COMMAND_RANGE( ID_BUTTON_0, ID_BUTTON_0 + LAYER_ID_COUNT - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED, SWAP_LAYERS_DIALOG::Sel_Layer )

    EVT_BUTTON( wxID_OK, SWAP_LAYERS_DIALOG::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, SWAP_LAYERS_DIALOG::OnCancelClick )
END_EVENT_TABLE()


SWAP_LAYERS_DIALOG::SWAP_LAYERS_DIALOG( PCB_BASE_FRAME* parent, LAYER_ID* aArray ) :
    DIALOG_SHIM( parent, -1, _( "Swap Layers:" ), wxPoint( -1, -1 ),
                 wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
    m_callers_nlayers( aArray )
{
    memset( layer_list, 0, sizeof( layer_list ) );

    BOARD* board = parent->GetBoard();

    OuterBoxSizer = NULL;
    MainBoxSizer  = NULL;
    FlexColumnBoxSizer = NULL;
    label  = NULL;
    Button = NULL;
    text   = NULL;
    Line   = NULL;
    StdDialogButtonSizer = NULL;

    m_Parent = parent;

    int    item_ID;
    wxSize goodSize;

    /* Experimentation has shown that buttons in the Windows version can be
     * 20 pixels wide and 20 pixels high, but that they need to be 26 pixels
     * wide and 26 pixels high in the Linux version. (And although the
     * dimensions of those buttons could be set to 26 pixels wide and 26
     * pixels high in both of those versions, that would result in a dialog
     * box which would be excessively high in the Windows version.)
     */
#ifdef __WINDOWS__
    int w = 20;
    int h = 20;
#else
    int w = 26;
    int h = 26;
#endif

    /* As currently implemented, the dimensions of the buttons in the Mac
     * version are also 26 pixels wide and 26 pixels high. If appropriate,
     * the above code should be modified as required in the event that those
     * buttons should be some other size in that version.
     */

    OuterBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( OuterBoxSizer );

    MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    OuterBoxSizer->Add( MainBoxSizer, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    for( unsigned layer = 0; layer < DIM( layer_list );  ++layer )
    {
        // Provide a vertical line to separate the two FlexGrid sizers
        if( layer == 32 )
        {
            Line = new wxStaticLine( this,
                                     -1,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxLI_VERTICAL );
            MainBoxSizer->Add( Line, 0, wxGROW | wxLEFT | wxRIGHT, 5 );
        }

        // Provide a separate FlexGrid sizer for every sixteen sets of controls
        if( layer % 16 == 0 )
        {
            /* Each layer has an associated static text string (to identify
             * that layer), a button (for invoking a child dialog box to
             * change which layer that the layer is mapped to), and a second
             * static text string (to depict which layer that the layer has
             * been mapped to). Each of those items are placed into the left
             * hand column, middle column, and right hand column (respectively)
             * of the Flexgrid sizer, and the color of the second text string
             * is set to fuchsia or blue (to respectively indicate whether the
             * layer has been swapped to another layer or is not being swapped
             * at all).  (Experimentation has shown that if a text control is
             * used to depict which layer that each layer is mapped to (instead
             * of a static text string), then those controls do not behave in
             * a fully satisfactory manner in the Linux version. Even when the
             * read-only attribute is specified for all of those controls, they
             * can still be selected when the arrow keys or Tab key is used
             * to step through all of the controls within the dialog box, and
             * directives to set the foreground color of the text of each such
             * control to blue (to indicate that the text is of a read-only
             * nature) are disregarded.)
             *
             * Specify a FlexGrid sizer with sixteen rows and three columns.
             */
            FlexColumnBoxSizer = new wxFlexGridSizer( 16, 3, 0, 0 );

            // Specify that all of the rows can be expanded.
            for( int jj = 0; jj < 16; jj++ )
            {
                FlexColumnBoxSizer->AddGrowableRow( jj );
            }

            // Specify that (just) the right-hand column can be expanded.
            FlexColumnBoxSizer->AddGrowableCol( 2 );

            MainBoxSizer->Add( FlexColumnBoxSizer, 1, wxGROW | wxTOP, 5 );
        }

        /* Provide a text string to identify this layer (with trailing spaces
         * within that string being purged).
         */
        label = new wxStaticText( this, wxID_STATIC, board->GetLayerName( ToLAYER_ID( layer ) ),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxALIGN_RIGHT );

        FlexColumnBoxSizer->Add( label, 0,
                                 wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxBOTTOM,
                                 5 );

        // Provide a button for this layer (which will invoke a child dialog box)
        item_ID = ID_BUTTON_0 + layer;

        Button = new wxButton( this, item_ID, wxT( "..." ), wxDefaultPosition,
                               wxSize( w, h ), 0 );
        FlexColumnBoxSizer->Add( Button, 0,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxBOTTOM, 5 );

        /* Provide another text string to specify which layer that this layer
         * is mapped to, set the initial text to "No Change" (to indicate that
         * this layer is currently unmapped to any other layer), and set the
         * foreground color of the text to blue (which also indicates that the
         * layer is currently unmapped to any other layer).
         */
        item_ID = ID_TEXT_0 + layer;

        /* When the first of these text strings is being added, determine
         * what size is necessary to to be able to display the longest
         * string without truncation. Then use that size as the
         * minimum size for all text strings. (If the minimum
         * size is not this size, strings can be truncated after
         * some other layer is selected.)
         */
        if( layer == 0 )
        {
            text = new wxStaticText( this, item_ID, board->GetLayerName( LAYER_ID( 0 ) ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
            goodSize = text->GetSize();

            for( unsigned jj = 1;  jj < DIM( layer_list ); ++jj )
            {
                text->SetLabel( board->GetLayerName( ToLAYER_ID( jj ) ) );

                if( goodSize.x < text->GetSize().x )
                    goodSize.x = text->GetSize().x;
            }

            text->SetLabel( _( "No Change" ) );

            if( goodSize.x < text->GetSize().x )
                goodSize.x = text->GetSize().x;
        }
        else
        {
            text = new wxStaticText( this, item_ID, _( "No Change" ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
        }

        text->SetMinSize( goodSize );
        FlexColumnBoxSizer->Add( text, 1,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxRIGHT | wxBOTTOM, 5 );
        layer_list[layer] = text;
    }

    /* Provide spacers to occupy otherwise blank cells within the second
     * FlexGrid sizer. (Becuse there are three columns, three spacers
     * are thus required for each unused row.)
    for( int ii = 3 * NB_PCB_LAYERS; ii < 96; ii++ )
    {
        FlexColumnBoxSizer->Add( 5, h, 0, wxALIGN_CENTER_HORIZONTAL |
                                 wxALIGN_CENTER_VERTICAL | wxLEFT |
                                 wxRIGHT | wxBOTTOM, 5 );
    }
     */

    // Provide a line to separate the controls which have been provided so far
    // from the OK and Cancel buttons (which will be provided after this line)
    Line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    OuterBoxSizer->Add( Line, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    // Provide a StdDialogButtonSizer to accommodate the OK and Cancel buttons;
    // using that type of sizer results in those buttons being automatically
    // located in positions appropriate for each (OS) version of KiCad.
    StdDialogButtonSizer = new wxStdDialogButtonSizer;
    OuterBoxSizer->Add( StdDialogButtonSizer, 0, wxGROW | wxALL, 10 );

    Button = new wxButton( this, wxID_OK, _( "&OK" ), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetDefault();
    StdDialogButtonSizer->AddButton( Button );

    Button = new wxButton( this, wxID_CANCEL, _( "&Cancel" ),
                           wxDefaultPosition, wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton( Button );
    StdDialogButtonSizer->Realize();

    // Resize the dialog
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

    Center();
}


void SWAP_LAYERS_DIALOG::Sel_Layer( wxCommandEvent& event )
{
    int ii;

    ii = event.GetId();

    if( ii < ID_BUTTON_0 || ii >= ID_BUTTON_0 + LAYER_ID_COUNT )
        return;

    ii = event.GetId() - ID_BUTTON_0;

    LAYER_ID layer = m_callers_nlayers[ii];

    LSET notallowed_mask = IsCopperLayer( ii ) ? LSET::AllNonCuMask() : LSET::AllCuMask();

    layer = m_Parent->SelectLayer( layer == NO_CHANGE ? ToLAYER_ID( ii ): layer, notallowed_mask );

    if( !IsValidLayer( layer ) )
        return;

    if( layer != m_callers_nlayers[ii] )
    {
        m_callers_nlayers[ii] = layer;

        if( layer == NO_CHANGE || layer == ii )
        {
            layer_list[ii]->SetLabel( _( "No Change" ) );

            // Change the text color to blue (to highlight
            // that this layer is *not* being swapped)
            layer_list[ii]->SetForegroundColour( *wxBLUE );
        }
        else
        {
            layer_list[ii]->SetLabel( m_Parent->GetBoard()->GetLayerName( layer ) );

            // Change the text color to fuchsia (to highlight
            // that this layer *is* being swapped)
            layer_list[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
    }
}


void SWAP_LAYERS_DIALOG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


void SWAP_LAYERS_DIALOG::OnOkClick( wxCommandEvent& event )
{
    EndModal( 1 );
}


void PCB_EDIT_FRAME::Swap_Layers( wxCommandEvent& event )
{
    LAYER_ID    new_layer[LAYER_ID_COUNT];

    for( unsigned i = 0; i < DIM( new_layer );  ++i )
        new_layer[i] = NO_CHANGE;

    SWAP_LAYERS_DIALOG dlg( this, new_layer );

    if( dlg.ShowModal() != 1 )
        return;     // (Canceled dialog box returns -1 instead)

    // Change traces.
    for( TRACK* segm = GetBoard()->m_Track;  segm;  segm = segm->Next() )
    {
        OnModify();

        if( segm->Type() == PCB_VIA_T )
        {
            VIA* via = (VIA*) segm;

            if( via->GetViaType() == VIA_THROUGH )
                continue;

            LAYER_ID top_layer, bottom_layer;

            via->LayerPair( &top_layer, &bottom_layer );

            if( new_layer[bottom_layer] != NO_CHANGE )
                bottom_layer = new_layer[bottom_layer];

            if( new_layer[top_layer] != NO_CHANGE )
                top_layer = new_layer[top_layer];

            via->SetLayerPair( top_layer, bottom_layer );
        }
        else
        {
            int jj = segm->GetLayer();

            if( new_layer[jj] != NO_CHANGE )
                segm->SetLayer( new_layer[jj] );
        }
    }

    // Change zones.
    for( TRACK* segm = GetBoard()->m_Zone;  segm;  segm = segm->Next() )
    {
        OnModify();
        int jj = segm->GetLayer();

        if( new_layer[jj] != NO_CHANGE )
            segm->SetLayer( new_layer[jj] );
    }

    // Change other segments.
    for( EDA_ITEM* item = GetBoard()->m_Drawings; item; item = item->Next() )
    {
        if( item->Type() == PCB_LINE_T )
        {
            OnModify();

            DRAWSEGMENT* drawsegm = (DRAWSEGMENT*) item;
            int jj = drawsegm->GetLayer();

            if( new_layer[jj] != NO_CHANGE )
                drawsegm->SetLayer( new_layer[jj] );
        }
    }

    m_canvas->Refresh( true );
}
