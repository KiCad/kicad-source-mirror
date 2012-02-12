/************************************/
/* dialog_graphic_items_options.cpp */
/************************************/


#include <fctsys.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>

#include <pcbnew_id.h>
#include <module_editor_frame.h>
#include <class_board.h>

#include <dialog_graphic_items_options.h>

extern int g_DrawDefaultLineThickness;


void PCB_EDIT_FRAME::OnConfigurePcbOptions( wxCommandEvent& aEvent )
{
    DIALOG_GRAPHIC_ITEMS_OPTIONS dlg( this );

    dlg.ShowModal();
}


void FOOTPRINT_EDIT_FRAME::InstallOptionsFrame( const wxPoint& pos )
{
    DIALOG_GRAPHIC_ITEMS_OPTIONS dlg( this );
    dlg.ShowModal();
}


/*
 * DIALOG_GRAPHIC_ITEMS_OPTIONS constructor
 */

DIALOG_GRAPHIC_ITEMS_OPTIONS::DIALOG_GRAPHIC_ITEMS_OPTIONS( PCB_BASE_FRAME* parent )
    : DIALOG_GRAPHIC_ITEMS_OPTIONS_BASE( parent )
{
    m_parent = parent;
    m_brdSettings = m_parent->GetDesignSettings();
    initValues(  );

    m_sdbSizer1OK->SetDefault();
    GetSizer()->SetSizeHints( this );

    Centre();
}

DIALOG_GRAPHIC_ITEMS_OPTIONS::~DIALOG_GRAPHIC_ITEMS_OPTIONS()
{
}


void DIALOG_GRAPHIC_ITEMS_OPTIONS::initValues()
{
    SetFocus();

    /* Drawings width */
    AddUnitSymbol( *m_GraphicSegmWidthTitle );
    PutValueInLocalUnits( *m_OptPcbSegmWidth,
                           m_brdSettings.m_DrawSegmentWidth,
                          PCB_INTERNAL_UNIT );
    /* Edges width */
    AddUnitSymbol( *m_BoardEdgesWidthTitle );
    PutValueInLocalUnits( *m_OptPcbEdgesWidth,
                           m_brdSettings.m_EdgeSegmentWidth,
                          PCB_INTERNAL_UNIT );

    /* Pcb Textes (Size & Width) */
    AddUnitSymbol( *m_CopperTextWidthTitle );
    PutValueInLocalUnits( *m_OptPcbTextWidth,
                           m_brdSettings.m_PcbTextWidth, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextSizeVTitle );
    PutValueInLocalUnits( *m_OptPcbTextVSize,
                           m_brdSettings.m_PcbTextSize.y, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextSizeHTitle );
    PutValueInLocalUnits( *m_OptPcbTextHSize,
                           m_brdSettings.m_PcbTextSize.x, PCB_INTERNAL_UNIT );


    /* Modules: Edges width */
    AddUnitSymbol( *m_EdgeModWidthTitle );
    PutValueInLocalUnits( *m_OptModuleEdgesWidth,
                          m_brdSettings.m_ModuleSegmentWidth, PCB_INTERNAL_UNIT );

    /* Modules: Texts: Size & width */
    AddUnitSymbol( *m_TextModWidthTitle );
    PutValueInLocalUnits( *m_OptModuleTextWidth,
                          m_brdSettings.m_ModuleTextWidth, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextModSizeVTitle );
    PutValueInLocalUnits( *m_OptModuleTextVSize,
                          m_brdSettings.m_ModuleTextSize.y, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_TextModSizeHTitle );
    PutValueInLocalUnits( *m_OptModuleTextHSize,
                          m_brdSettings.m_ModuleTextSize.x, PCB_INTERNAL_UNIT );

    AddUnitSymbol( *m_DefaultPenSizeTitle );
    PutValueInLocalUnits( *m_DefaultPenSizeCtrl,
                          g_DrawDefaultLineThickness, PCB_INTERNAL_UNIT );
}


void DIALOG_GRAPHIC_ITEMS_OPTIONS::OnOkClick( wxCommandEvent& event )
{
    m_brdSettings.m_DrawSegmentWidth =
        ReturnValueFromTextCtrl( *m_OptPcbSegmWidth, PCB_INTERNAL_UNIT );
    m_brdSettings.m_EdgeSegmentWidth =
        ReturnValueFromTextCtrl( *m_OptPcbEdgesWidth, PCB_INTERNAL_UNIT );
    m_brdSettings.m_PcbTextWidth =
        ReturnValueFromTextCtrl( *m_OptPcbTextWidth, PCB_INTERNAL_UNIT );
    m_brdSettings.m_PcbTextSize.y =
        ReturnValueFromTextCtrl( *m_OptPcbTextVSize, PCB_INTERNAL_UNIT );
    m_brdSettings.m_PcbTextSize.x =
        ReturnValueFromTextCtrl( *m_OptPcbTextHSize, PCB_INTERNAL_UNIT );

    m_parent->GetBoard()->SetDesignSettings( m_brdSettings );

    m_brdSettings.m_ModuleSegmentWidth =
        ReturnValueFromTextCtrl( *m_OptModuleEdgesWidth, PCB_INTERNAL_UNIT );
    m_brdSettings.m_ModuleTextWidth =
        ReturnValueFromTextCtrl( *m_OptModuleTextWidth, PCB_INTERNAL_UNIT );
    m_brdSettings.m_ModuleTextSize.y =
        ReturnValueFromTextCtrl( *m_OptModuleTextVSize, PCB_INTERNAL_UNIT );
    m_brdSettings.m_ModuleTextSize.x =
        ReturnValueFromTextCtrl( *m_OptModuleTextHSize, PCB_INTERNAL_UNIT );

    g_DrawDefaultLineThickness =
        ReturnValueFromTextCtrl( *m_DefaultPenSizeCtrl, PCB_INTERNAL_UNIT );

    if( g_DrawDefaultLineThickness < 0 )
        g_DrawDefaultLineThickness = 0;

    m_parent->SetDesignSettings( m_brdSettings );

    EndModal( wxID_OK );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_GRAPHIC_ITEMS_OPTIONS::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}
