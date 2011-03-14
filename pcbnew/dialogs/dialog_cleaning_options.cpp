/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_cleaning_options.cpp
// Author:      jean-pierre Charras
/////////////////////////////////////////////////////////////////////////////

#include "wx/wx.h"

#include "dialog_cleaning_options.h"


DIALOG_CLEANING_OPTIONS::DIALOG_CLEANING_OPTIONS( wxWindow* parent ):
    DIALOG_CLEANING_OPTIONS_BASE( parent )
{
    m_cleanViasOpt->SetValue( cleanVias );
    m_mergeSegmOpt->SetValue( mergeSegments );
    m_deleteUnconnectedOpt->SetValue( deleteUnconnectedSegm );
    m_reconnectToPadsOpt->SetValue( connectToPads );

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints(this);
    Centre();
}

// Static members of DIALOG_CLEANING_OPTIONS
bool DIALOG_CLEANING_OPTIONS::cleanVias = true;
bool DIALOG_CLEANING_OPTIONS::mergeSegments = true;
bool DIALOG_CLEANING_OPTIONS::deleteUnconnectedSegm = true;
bool DIALOG_CLEANING_OPTIONS::connectToPads = false;

