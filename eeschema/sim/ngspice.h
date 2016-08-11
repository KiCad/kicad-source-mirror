#ifndef __NGSPICE_H
#define __NGSPICE_H

#include <string>
#include <vector>

class SPICE_SIMULATOR {

public:
    SPICE_SIMULATOR();
    ~SPICE_SIMULATOR();

    virtual void Init();
    virtual bool LoadNetlist(const std::string& netlist);
    virtual bool Command(const std::string& cmd);

    const std::vector<double> GetPlot( std::string name, int max_len = -1);
};

#endif
