#include "dialog_sch_sheet_props.h"

DIALOG_SCH_SHEET_PROPS::DIALOG_SCH_SHEET_PROPS( wxWindow* parent ) :
    DIALOG_SCH_SHEET_PROPS_BASE( parent )
{
    m_textFileName->SetFocus();
    m_sdbSizer1OK->SetDefault();
}
