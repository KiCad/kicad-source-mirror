#include <dialog_sch_edit_sheet_pin.h>


static wxString sheetPinTypes[] =
{
    _( "Input" ),
    _( "Output" ),
    _( "Bidirectional" ),
    _( "Tri-state" ),
    _( "Passive" )
};


#define SHEET_PIN_TYPE_CNT   ( sizeof( sheetPinTypes ) / sizeof( wxString ) )


DIALOG_SCH_EDIT_SHEET_PIN::DIALOG_SCH_EDIT_SHEET_PIN( wxWindow* parent ) :
    DIALOG_SCH_EDIT_SHEET_PIN_BASE( parent )
{
    for( size_t i = 0;  i < SHEET_PIN_TYPE_CNT;  i++ )
        m_choiceConnectionType->Append( sheetPinTypes[ i ] );

    m_choiceConnectionType->SetSelection( 0 );
    m_textName->SetFocus();
    m_sdbSizer2OK->SetDefault();
}
