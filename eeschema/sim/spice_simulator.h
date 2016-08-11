#ifndef __NGSPICE_H
#define __NGSPICE_H

#include <string>
#include <vector>

enum SimTraceType
{
SIM_AC_MAG = 0x1,
SIM_AC_PHASE = 0x2,
SIM_TR_VOLTAGE = 0x4,
SIM_TR_CURRENT = 0x8,
SIM_TR_FFT = 0x10
};

class REPORTER;

class SPICE_SIMULATOR {

public:
    typedef void (*ConsoleCallback)( bool isError, const wxString& message, void *userData );

    static SPICE_SIMULATOR *CreateInstance( const std::string name );

    SPICE_SIMULATOR(){}
    virtual ~SPICE_SIMULATOR() = 0;

    virtual void Init() = 0;
    virtual bool LoadNetlist(const std::string& netlist) = 0;
    virtual bool Command(const std::string& cmd) = 0;
    virtual void SetConsoleReporter ( REPORTER *rep )
    {
        m_consoleReporter = rep;
    }

    virtual const std::vector<double> GetPlot( std::string name, int max_len = -1) = 0;

protected:
    REPORTER *m_consoleReporter;

};


#endif
