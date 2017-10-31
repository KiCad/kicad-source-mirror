#include <pcb_test_frame.h>
// #include <tools/outline_editor.h>
#include <tool/tool_manager.h>

class OED_TEST_FRAME : public PCB_TEST_FRAME
{
public:
    OED_TEST_FRAME( wxFrame* frame,
            const wxString& title,
            const wxPoint& pos  = wxDefaultPosition,
            const wxSize& size  = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE ) :
        PCB_TEST_FRAME( frame, title, pos, size, style )
    {
        registerTools();
    }

    void registerTools();

    virtual ~OED_TEST_FRAME() {}
};

wxFrame* CreateMainFrame( const std::string& aFileName )
{
    auto frame = new OED_TEST_FRAME( nullptr, wxT( "Outline Editor Test" ) );

    if( aFileName != "" )
    {
        frame->LoadAndDisplayBoard( aFileName );
    }

    return frame;
}

void OED_TEST_FRAME::registerTools()
{
//    m_toolManager->RegisterTool( new OUTLINE_EDITOR );
    m_toolManager->InitTools();
    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );
}
