
#include <dialogs/dialog_rc_job.h>
#include <jobs/job_sch_erc.h>

class DIALOG_ERC_JOB_CONFIG : public DIALOG_RC_JOB
{
public:
    DIALOG_ERC_JOB_CONFIG( wxWindow* parent, JOB_SCH_ERC* aJob );

private:
    JOB_SCH_ERC* m_ercJob;
};