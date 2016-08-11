#include "sharedspice.h"
#include <cstdio>
#include <sstream>

#include <wx/dynlib.h>

#include <string>
#include <vector>

#include "spice_simulator.h"

#include <reporter.h>

using namespace std;


class NGSPICE : public SPICE_SIMULATOR {

public:
    NGSPICE();
    virtual ~NGSPICE();

    void Init();
    bool LoadNetlist(const string& netlist);
    bool Command(const string& cmd);

    string GetConsole() const;

    const vector<double> GetPlot( std::string name, int max_len = -1);

    void dump();

private:



    typedef void (*ngSpice_Init)(SendChar*, SendStat*, ControlledExit*,
             SendData*, SendInitData*, BGThreadRunning*, void*);

    typedef int (*ngSpice_Circ)(char** circarray);
    typedef int (*ngSpice_Command)(char* command);
    typedef pvector_info (*ngGet_Vec_Info)(char* vecname);
    typedef char** (*ngSpice_AllVecs)(char* plotname);
    typedef char** (*ngSpice_AllPlots)(void);


    ngSpice_Init m_ngSpice_Init;
    ngSpice_Circ m_ngSpice_Circ;
    ngSpice_Command m_ngSpice_Command;
    ngGet_Vec_Info m_ngGet_Vec_Info;
    ngSpice_AllPlots m_ngSpice_AllPlots;
    ngSpice_AllVecs m_ngSpice_AllVecs;

    wxDynamicLibrary *m_dll;

    static int cbSendChar( char* what, int id, void* user)
    {
        NGSPICE *sim = reinterpret_cast<NGSPICE*>(user);

        printf("sim %p cr %p\n",sim, sim->m_consoleReporter );
        if(sim->m_consoleReporter)
            sim->m_consoleReporter->Report(what);
        return 0;
    }

    static int cbSendStat( char* what, int id, void* user)
    {
    /*    NGSPICE *sim = reinterpret_cast<NGSPICE*>(user);
        if(sim->m_consoleReporter)
            sim->m_consoleReporter->Report(what);*/
        return 0;
    }

};




NGSPICE::NGSPICE()
{
    m_dll = new wxDynamicLibrary("/home/twl/projects_sw/ngspice-26/src/.libs/libngspice.so.0.0.0"); //, wxDL_LAZY);

    printf("DLL at %p\n", m_dll);

    assert(m_dll);

    m_ngSpice_Init = (ngSpice_Init) m_dll->GetSymbol("ngSpice_Init");
    printf("Init @ %p\n", m_ngSpice_Init);


    m_ngSpice_Circ = (ngSpice_Circ) m_dll->GetSymbol("ngSpice_Circ");
    m_ngSpice_Command = (ngSpice_Command) m_dll->GetSymbol("ngSpice_Command");
    m_ngGet_Vec_Info = (ngGet_Vec_Info) m_dll->GetSymbol("ngGet_Vec_Info");
    m_ngSpice_AllPlots = (ngSpice_AllPlots) m_dll->GetSymbol("ngSpice_AllPlots");
    m_ngSpice_AllVecs = (ngSpice_AllVecs) m_dll->GetSymbol("ngSpice_AllVecs");


}

void NGSPICE::Init()
{
    m_ngSpice_Init( &cbSendChar, &cbSendStat, NULL, NULL, NULL, NULL, this);
}

const vector<double> NGSPICE::GetPlot( std::string name, int max_len )
{
    vector<double> data;

    vector_info *vi = m_ngGet_Vec_Info((char*)name.c_str());

    if(vi->v_realdata)
        for(int i = 0; i<vi->v_length;i++)
            data.push_back(vi->v_realdata[i]);

    return data;

}


static string loadFile(const string& filename)
{

    FILE *f=fopen(filename.c_str(),"rb");
    char buf[10000];
    int n = fread(buf, 1, 10000, f);
    fclose(f);
    buf[n] = 0;
    return buf;
}

bool NGSPICE::LoadNetlist(const string& netlist)
{
    char *lines[16384];
    stringstream ss(netlist);
    int n = 0;

    while(!ss.eof())
    {
        char line[1024];
        ss.getline(line, 1024);

        lines[n++] = strdup(line);
	    printf("l '%s'\n", line);
    }
    lines[n]= NULL;

    printf("netlist contains %d lines\n", n);
    m_ngSpice_Circ(lines);

    for(int i = 0; i < n; i++)
        delete lines[i];

    return true;
}


bool NGSPICE::Command(const string& cmd )
{
    m_ngSpice_Command( (char*)(cmd + string("\n")).c_str());
    dump();
    return true;
}


void NGSPICE::dump()
{
//    m_ngSpice_Command("run\n");
    char **plots = m_ngSpice_AllPlots();

    for(int i = 0; plots[i]; i++)
    {
        printf("-> plot : %s\n", plots[i]);
        char **vecs = m_ngSpice_AllVecs(plots[i]);

        for(int j = 0; vecs[j]; j++)
        {
            printf("   - vector %s\n", vecs[j]);

            vector_info *vi = m_ngGet_Vec_Info(vecs[j]);

            printf("       - v_type %x\n", vi->v_type);
            printf("       - v_flags %x\n", vi->v_flags);
            printf("       - v_length %d\n", vi->v_length);


        }

    }


}



NGSPICE::~NGSPICE()
{
    printf("Killing ngspice\n");
    delete m_dll;
}

#if 0
main()
{
    NGSPICE spice;
    spice.Init();
    spice.LoadNetlist(loadFile("1.ckt"));

    spice.Command("tran .05 1");
    spice.Command("save all");

    spice.Run();
    vector<double> t = spice.GetPlot("time");
    vector<double> v1 = spice.GetPlot("V(1)");
    vector<double> v2 = spice.GetPlot("V(2)");

    // Prepare data.

    // Plot line from given x and y data. Color is selected automatically.
    plt::plot(t, v1);
    // Plot a red dashed line from given x and y data.
    plt::plot(t, v2,"r--");

    for(int i=0;i<v1.size();i++)
        printf("%.10f\n",v2[i]);

    // Add graph title
    plt::title("Sample figure");
    // Enable legend.
    plt::legend();
    // save figure
    plt::show();


//    spice.Run();
}

#endif



std::string NGSPICE::GetConsole() const {
    return "";
}

SPICE_SIMULATOR::~SPICE_SIMULATOR()
{

}

SPICE_SIMULATOR *SPICE_SIMULATOR::CreateInstance( const std::string name )
{
    return new NGSPICE;
}
