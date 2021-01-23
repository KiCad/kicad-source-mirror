
#ifndef PATHS_H
#define PATHS_H

class PATHS
{
public:
    static wxString GetUserScriptingPath();
    static wxString GetUserTemplatesPath();
    static wxString GetUserPluginsPath();
    static wxString GetUserPlugins3DPath();
    static wxString GetDefaultUserProjectsPath();
    static wxString GetStockScriptingPath();
    static wxString GetStockPluginsPath();
    static wxString GetStockPlugins3DPath();
};

#endif