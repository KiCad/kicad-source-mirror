#include <wx/string.h>
#include <dialog_sch_sheet_props.h>
#include <validators.h>


DIALOG_SCH_SHEET_PROPS::DIALOG_SCH_SHEET_PROPS( wxWindow* parent ) :
    DIALOG_SCH_SHEET_PROPS_BASE( parent )
{
    m_textFileName->SetValidator( FILE_NAME_WITH_PATH_CHAR_VALIDATOR() );
    m_textFileName->SetFocus();
    m_sdbSizer1OK->SetDefault();
}


void DIALOG_SCH_SHEET_PROPS::SetFileName( const wxString& aFileName )
{
    // Filenames are stored using unix notation
    wxString fname = aFileName;
#ifdef __WINDOWS__
    fname.Replace( wxT("/"), wxT("\\") );
#endif
    m_textFileName->SetValue( fname );
}


const wxString DIALOG_SCH_SHEET_PROPS::GetFileName()
{
    // Filenames are stored using unix notation
    wxString fname = m_textFileName->GetValue();
    fname.Replace( wxT("\\"), wxT("/") );
    return fname;
}
