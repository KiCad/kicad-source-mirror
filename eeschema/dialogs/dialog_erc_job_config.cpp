#include <dialogs/dialog_erc_job_config.h>

DIALOG_ERC_JOB_CONFIG::DIALOG_ERC_JOB_CONFIG( wxWindow* parent, JOB_SCH_ERC* aJob ) :
        DIALOG_RC_JOB( parent, aJob, _( "ERC Job Settings" ) ),
        m_ercJob( aJob )
{
    m_cbAllTrackViolations->Hide();
    m_cbSchParity->Hide();

    Fit();
    Layout();
}