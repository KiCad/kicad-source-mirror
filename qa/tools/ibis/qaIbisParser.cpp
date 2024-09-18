#include "../../eeschema/sim/kibis/kibis.h"
#include <wx/textfile.h>

int main( void )
{
    //KIBIS* k1 = new KIBIS( "ibis_v1_1.ibs" );
    //KIBIS* k2 = new KIBIS( "ibis_v2_1.ibs" );
    //KIBIS* k4 = new KIBIS( "sn74lvc541a.ibs" );
    KIBIS* k4 = new KIBIS( "ibis_lan8670_1_2_1v3.ibs" );

    KIBIS_COMPONENT& comp = k4->m_components.at( 0 );

    std::cout << "Component: " << comp.m_name << std::endl;

    for( KIBIS_PIN& pin : comp.m_pins )
    {
        std::cout << "--Pin: " << pin.m_pinNumber << " " << pin.m_signalName << std::endl;
    }


    KIBIS_PIN* pin1 = comp.GetPin( "30" );
    KIBIS_PIN* pin2 = comp.GetPin( "31" );


    std::cout << "pin1 name: " << pin1->m_signalName << std::endl;
    std::cout << "pin2 name: " << pin2->m_signalName << std::endl;
    std::cout << "pin1 model length: " << pin1->m_models.size()<< std::endl;
    std::cout << "pin2 model length: " << pin2->m_models.size()<< std::endl;
    std::string*  tmp1 = new std::string();
    std::string*  tmp2 = new std::string();
    std::string*  tmp3 = new std::string();
    std::string*  tmp4 = new std::string();

    KIBIS_WAVEFORM_RECTANGULAR* wave = new KIBIS_WAVEFORM_RECTANGULAR();
    wave->m_ton = 80e-9;
    wave->m_toff = 80e-9;
    wave->m_cycles = 10;
    wave->m_delay = 0;
    //KIBIS_WAVEFORM_STUCK_HIGH* wave = new KIBIS_WAVEFORM_STUCK_HIGH();

    std::cout << "WAVEFORM TYPE IN QA: " << wave->GetType() << std::endl;
    std::cout << pin2->m_models.at(0)->m_name << std::endl;

    KIBIS_PARAMETER params;

    params.m_waveform = new KIBIS_WAVEFORM_RECTANGULAR();

    pin2->writeSpiceDevice( *tmp4, "device_typ", *( pin2->m_models.at( 0 ) ), params );

    KIBIS_MODEL* model1 = pin1->m_models.at( 0 );
    std::cout << "Model used for driver: " << model1->m_name << std::endl;
    pin1->writeSpiceDiffDriver( *tmp1, "driver_typ", *( model1 ), params );

    wxTextFile file( "output.sp" );
    if( file.Exists() )
    {
        file.Clear();
    }
    else
    {
        file.Create();
    }
    file.AddLine( *tmp1 );
    file.AddLine( *tmp2 );
    file.AddLine( *tmp3 );
    file.AddLine( *tmp4 );

    wxString simul = "";
    simul += "x1 0 OUT_1 OUT_2 driver_typ \n";
    simul += "R1 OUT_1 COM 25\n";
    simul += "R2 OUT_2 COM 25\n";
    simul += "V1 COM 0 1.25\n";
    simul += ".tran 0.1n 1000n \n";
    simul += ".control run \n";
    simul += "run \n";
    simul += "plot v(OUT_1) v(OUT_2)\n";
    //simul += "plot v(x1.KU) v(x1.KD) v(1) v(x1.DIEBUFF) \n";
    simul += ".endc \n";
    simul += ".end \n";


    file.AddLine( simul );

    file.Write();


    std::cout << "Done" << std::endl;

    return 1;
}
