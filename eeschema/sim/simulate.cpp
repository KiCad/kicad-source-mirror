#include <schframe.h>
#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <gestfich.h>
#include <confirm.h>
#include <base_units.h>
#include <msgpanel.h>
#include <html_messagebox.h>

#include <general.h>
#include <eeschema_id.h>
#include <netlist.h>
#include <lib_pin.h>
#include <class_library.h>
#include <schframe.h>
#include <sch_component.h>

#include <dialog_helpers.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <hotkeys.h>
#include <eeschema_config.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>

#include <invoke_sch_dialog.h>
#include <dialogs/dialog_schematic_find.h>

#include <wx/display.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>

#include <netlist_exporter_kicad.h>
#include <kiway.h>

#include <netlist_exporters/netlist_exporter_pspice.h>

#include <sim/sim_plot_frame.h>

void SCH_EDIT_FRAME::OnSimulationRun( wxCommandEvent& event )
{
    #if 0

    NETLIST_OBJECT_LIST* net_atoms = BuildNetListBase();
    NETLIST_EXPORTER_PSPICE exporter( net_atoms, Prj().SchLibs() );
    STRING_FORMATTER formatter;

    exporter.Format( &formatter, GNL_ALL );

    printf("*******************\n%s\n", (const char *)formatter.GetString().c_str());
    #endif

    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) Kiway().Player( FRAME_SIMULATOR, false );

    if( !simFrame )
    {
        simFrame = (SIM_PLOT_FRAME*) Kiway().Player( FRAME_SIMULATOR, true );
        simFrame->Show( true );
    }


    // On Windows, Raise() does not bring the window on screen, when iconized
    if( simFrame->IsIconized() )
        simFrame->Iconize( false );

    simFrame->Raise();

    simFrame->SetSchFrame( this );
    simFrame->StartSimulation();
}

void SCH_EDIT_FRAME::OnSimulationStop( wxCommandEvent& event )
{}

void SCH_EDIT_FRAME::OnSimulationAddProbe( wxCommandEvent& event )
{}
