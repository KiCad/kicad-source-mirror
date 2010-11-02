#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

#include "bitmaps.h"
#include "pcbnew_id.h"

#include "hotkeys.h"
#include "help_common_strings.h"

#include <wx/ownerdrw.h>
#include <wx/menuitem.h>
#include <wx/bmpcbox.h>
#include <wx/wx.h>

#include "class_layerchoicebox.h"

/* class to display a layer list.
 *
 */

    WinEDALayerChoiceBox::WinEDALayerChoiceBox( wxWindow* parent, wxWindowID id,
                     const wxPoint& pos, const wxSize& size,
                     int n, const wxString choices[] ) :
        wxBitmapComboBox( parent, id, wxEmptyString, pos, size,
                    n, choices, wxCB_READONLY )
    {
    }


    WinEDALayerChoiceBox::WinEDALayerChoiceBox( wxWindow* parent, wxWindowID id,
                     const wxPoint& pos, const wxSize& size,
                     const wxArrayString& choices ) :
        wxBitmapComboBox( parent, id, wxEmptyString, pos, size,
                    choices, wxCB_READONLY )
    {
    }

    // Get Current Item #
    int WinEDALayerChoiceBox::GetChoice()
    {
        return GetSelection();
    }

    // Get Current Layer
    int WinEDALayerChoiceBox::GetLayerChoice()
    {
        return (long) GetClientData(GetSelection());
    }

    // Set Layer #
    int WinEDALayerChoiceBox::SetLayerSelection(int layer)
    {
        int elements = GetCount();

        for( int i = 0 ; i < elements; i++) 
            if(GetClientData(i) == (void*)layer) 
                if(GetSelection() != i) // Element (i) is not selected
                {
                    SetSelection(i);
                    return i;
                }
                else
                    return i;           //If element already selected; do nothing

        // Not Found
        SetSelection(-1);
        return -1;
    }

    // Reload the Layers
    void WinEDALayerChoiceBox::Resync()
    {
        BOARD* board = ((WinEDA_BasePcbFrame*)GetParent())->GetBoard();
        wxASSERT(board != NULL);

        Clear();
        
        static DECLARE_LAYERS_ORDER_LIST(layertranscode);
        static DECLARE_LAYERS_HOTKEY(layerhk);
 
        for( int i=0 ; i < LAYER_COUNT ; i++)
        {
           wxBitmap    layerbmp (20,18);
           wxMemoryDC  bmpDC;
           wxBrush     brush;
           wxString    layername;
 
           int layerid = layertranscode[i];
           
           // Prepare Bitmap
           bmpDC.SelectObject(layerbmp);
           brush.SetColour( MakeColour(board->GetLayerColor( layerid ) ));
           brush.SetStyle( wxSOLID );
           
           bmpDC.SetBrush(brush);
           bmpDC.DrawRectangle(0,0,layerbmp.GetWidth(), layerbmp.GetHeight()); 
           bmpDC.SetBrush(*wxTRANSPARENT_BRUSH);
           bmpDC.SetPen(*wxBLACK_PEN);
           bmpDC.DrawRectangle(0,0,layerbmp.GetWidth(), layerbmp.GetHeight()); 

           layername = board->GetLayerName( layerid );

           layername.Append("\t");
           layername = AddHotkeyName( layername, s_Board_Editor_Hokeys_Descr, layerhk[layerid], false );


           if(board->IsLayerEnabled(layerid))
               Append(layername, layerbmp, (void*) layerid );
        }

    }

