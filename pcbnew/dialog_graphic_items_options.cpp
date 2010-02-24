/************************************/
/* dialog_graphic_items_options.cpp */
/************************************/


#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "pcbnew_id.h"

#include "dialog_graphic_items_options.h"

extern int g_DrawDefaultLineThickness;


void WinEDA_PcbFrame::InstallPcbOptionsFrame( int id )
{
    switch( id )
    {
    case ID_PCB_DRAWINGS_WIDTHS_SETUP:
    {
        DIALOG_GRAPHIC_ITEMS_OPTIONS dlg( this );
        dlg.ShowModal();
    }
    break;

    default:
        wxMessageBox( wxT( "InstallPcbOptionsFrame() id error" ) );
        break;
    }
}


void WinEDA_ModuleEditFrame::InstallOptionsFrame( const wxPoint& pos )
{
    DIALOG_GRAPHIC_ITEMS_OPTIONS dlg( this );
    dlg.ShowModal();
}


/*
 * DIALOG_GRAPHIC_ITEMS_OPTIONS constructor
 */

DIALOG_GRAPHIC_ITEMS_OPTIONS::DIALOG_GRAPHIC_ITEMS_OPTIONS( WinEDA_BasePcbFrame* parent )
    : DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE( parent )
{
    m_Parent = parent;
    m_BrdSettings = m_Parent->GetBoard()->GetBoardDesignSettings();
    initValues(  );

    GetSizer()->SetSizeHints( this );

    Centre();
}

DIALOG_GRAPHIC_ITEMS_OPTIONS::~DIALOG_GRAPHIC_ITEMS_OPTIONS(  )
{
}


void DIALOG_GRAPHIC_ITEMS_OPTIONS::initValues()
{
    /* Drawings width */
    AddUnitSymbol( *m_GraphicSegmWidthTitle );
    PutValueInLocalUnits( *m_OptPcbSegmWidth,
                           m_BrdSettings->m_DrawSegmentWidth,
                          PCB_INTERNAL_UNIT );
    /* Edges width */
    AddUnitSymbol( *m_BoardEdgesWidthTitle );
    PutValueInLocalUnits( *m_OptPcbEdgesWidth,
                           m_BrdSettings->m_EdgeSegmentWidth,
                          PCB_INTERNAL_UNIT );

    /* Pcb Textes (Size & Width) */
    AddUnitSymbol( *m_CopperTextWidthTitle );
    PutValueInLocalUnits( *m_OptPcbTextWidth,
                           m_BrdSettings->m_PcbTextWidth, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextSizeVTitle );
    PutValueInLocalUnits( *m_OptPcbTextVSize,
                           m_BrdSettings->m_PcbTextSize.y, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextSizeHTitle );
    PutValueInLocalUnits( *m_OptPcbTextHSize,
                           m_BrdSettings->m_PcbTextSize.x, PCB_INTERNAL_UNIT );


    /* Modules: Edges width */
    AddUnitSymbol( *m_EdgeModWidthTitle );
    PutValueInLocalUnits( *m_OptModuleEdgesWidth,
                          ModuleSegmentWidth, PCB_INTERNAL_UNIT );

    /* Modules: Texts: Size & width */
    AddUnitSymbol( *m_TextModWidthTitle );
    PutValueInLocalUnits( *m_OptModuleTextWidth,
                          ModuleTextWidth, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextModSizeVTitle );
    PutValueInLocalUnits( *m_OptModuleTextVSize,
                          ModuleTextSize.y, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextModSizeHTitle );
    PutValueInLocalUnits( *m_OptModuleTextHSize,
                          ModuleTextSize.x, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_DefaultPenSizeTitle );
    PutValueInLocalUnits( *m_DefaultPenSizeCtrl,
                          g_DrawDefaultLineThickness, PCB_INTERNAL_UNIT );
}


void DIALOG_GRAPHIC_ITEMS_OPTIONS::OnOkClick( wxCommandEvent& event )
{
     m_BrdSettings->m_DrawSegmentWidth =
        ReturnValueFromTextCtrl( *m_OptPcbSegmWidth, PCB_INTERNAL_UNIT );
     m_BrdSettings->m_EdgeSegmentWidth =
        ReturnValueFromTextCtrl( *m_OptPcbEdgesWidth, PCB_INTERNAL_UNIT );
     m_BrdSettings->m_PcbTextWidth =
        ReturnValueFromTextCtrl( *m_OptPcbTextWidth, PCB_INTERNAL_UNIT );
     m_BrdSettings->m_PcbTextSize.y =
        ReturnValueFromTextCtrl( *m_OptPcbTextVSize, PCB_INTERNAL_UNIT );
     m_BrdSettings->m_PcbTextSize.x =
        ReturnValueFromTextCtrl( *m_OptPcbTextHSize, PCB_INTERNAL_UNIT );

    ModuleSegmentWidth =
        ReturnValueFromTextCtrl( *m_OptModuleEdgesWidth, PCB_INTERNAL_UNIT );
    ModuleTextWidth =
        ReturnValueFromTextCtrl( *m_OptModuleTextWidth, PCB_INTERNAL_UNIT );
    ModuleTextSize.y =
        ReturnValueFromTextCtrl( *m_OptModuleTextVSize, PCB_INTERNAL_UNIT );
    ModuleTextSize.x =
        ReturnValueFromTextCtrl( *m_OptModuleTextHSize, PCB_INTERNAL_UNIT );
    
    g_DrawDefaultLineThickness =
        ReturnValueFromTextCtrl( *m_DefaultPenSizeCtrl, PCB_INTERNAL_UNIT );
    
    if( g_DrawDefaultLineThickness < 0 )
        g_DrawDefaultLineThickness = 0;

    EndModal( 1 );
}



/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_GRAPHIC_ITEMS_OPTIONS::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}
