# -*- coding: utf-8 -*-
"""KiCad Python Shell.

This module provides the python shell for KiCad.

KiCad starts the shell once, by calling makePcbnewShellWindow() the
first time it is opened, subsequently the shell window is just hidden
or shown, as per user requirements.

IF makePcbnewShellWindow() is called again, a second/third shell window
can be created.

Note:
**DO NOT** import pcbnew module if not called from Pcbnew: on msys2 it creates a serious issue:
the python script import is broken because the pcbnew application is not running, and for instance
Pgm() returns a nullptr and Kicad crashes when Pgm is invoked.

"""

import wx
import sys
import os
import pcbnew

from wx.py import crust, version, dispatcher

from .kicad_pyeditor import KiCadEditorNotebookFrame
from .kicad_pyeditor import KiCadEditorNotebook

class KiCadPyShell(KiCadEditorNotebookFrame):
    def __init__(self, parent):
        # Search if a pcbnew frame is open, because import pcbnew can be made only if it exists.
        # frame names are "SchematicFrame" and "PcbFrame"
        #
        # Note we must do this before the KiCadEditorNotebookFrame __init__ call because it ends up calling _setup back on us
        frame = wx.FindWindowByName( "PcbFrame" )

        self.isPcbframe = frame is not None

        KiCadEditorNotebookFrame.__init__(self, parent)

    def _setup_startup(self):
        if self.config_dir != "":
            """Initialise the startup script."""
            # Create filename for startup script.
            self.startup_file = os.path.join(self.config_dir,
                                             "PyShell_pcbnew_startup.py")
            self.execStartupScript = True

            # Check if startup script exists
            if not os.path.isfile(self.startup_file):
                # Not, so try to create a default.
                try:
                    default_startup = open(self.startup_file, 'w')
                    # provide the content for the default startup file.
                    default_startup.write(
                        "### DEFAULT STARTUP FILE FOR KiCad Python Shell\n" +
                        "# Enter any Python code you would like to execute when" +
                        " the PCBNEW python shell first runs.\n" +
                        "\n" +
                        "# For example, uncomment the following lines to import the current board\n" +
                        "\n" +
                        "# import pcbnew\n" +
                        "# import eeschema\n" +
                        "# board = pcbnew.GetBoard()\n" +
                        "# sch = eeschema.GetSchematic()\n")
                    default_startup.close()
                except:
                    pass
        else:
            self.startup_file = ""
            self.execStartupScript = False

    def _setup(self):
        """
        Setup prior to first buffer creation.

        Called automatically by base class during init.
        """
        self.notebook = KiCadEditorNotebook(parent=self.parent)
        intro = 'Py %s' % version.VERSION

        intro += """\n\nDeprecation Notice:\n
This SWIG-based Python interface to the PCB editor is deprecated and will be removed 
in a future version of KiCad.  Please plan to move to the new IPC API and/or make 
use of the kicad-cli tool for your KiCad automation needs.\n\n"""

        import types
        import builtins
        module = types.ModuleType('__main__')
        module.__dict__['__builtins__'] = builtins
        namespace = module.__dict__.copy()

        '''
        Import pcbnew **only** if the board editor exists, to avoid strange behavior if not.
        pcbnew.SETTINGS_MANAGER should be in fact called only if the python console is created
        from the board editor, and if created from schematic editor, should use something like
        eeschema.SETTINGS_MANAGER
        '''
        if self.isPcbframe:
            import pcbnew
            self.config_dir = pcbnew.SETTINGS_MANAGER.GetUserSettingsPath()
        else:
            self.config_dir = ""

        self.dataDir = self.config_dir

        self._setup_startup()
        self.history_file = os.path.join(self.config_dir,
                                         "PyShell_pcbnew.history")

        self.config = None
        # self.config_file = os.path.join(self.config_dir,
        #                                 "PyShell_pcbnew.cfg")
        # self.config = wx.FileConfig(localFilename=self.config_file)
        # self.config.SetRecordDefaults(True)

        self.autoSaveSettings = False
        self.autoSaveHistory = False
        self.LoadSettings()

        self.crust = crust.Crust(parent=self.notebook,
                                 intro=intro, locals=namespace,
                                 rootLabel="locals()",
                                 startupScript=self.startup_file,
                                 execStartupScript=self.execStartupScript)

        self.crust._CheckShouldSplit()
        self.shell = self.crust.shell
        # Override the filling so that status messages go to the status bar.
        self.crust.filling.tree.setStatusText = self.parent.SetStatusText
        # Override the shell so that status messages go to the status bar.
        self.shell.setStatusText = self.parent.SetStatusText
        # Fix a problem with the sash shrinking to nothing.
        self.crust.filling.SetSashPosition(200)
        self.notebook.AddPage(page=self.crust, text='*Shell*', select=True)
        self.setEditor(self.crust.editor)
        self.crust.editor.SetFocus()

        self.LoadHistory()

    def OnAbout(self, event):
        """Display an About window."""
        title = 'About : KiCad - Python Shell'
        text = "Enhanced Python Shell for KiCad\n\n" + \
               "KiCad Revision: %s\n" % pcbnew.FullVersion() + \
               "Platform: %s\n" % sys.platform + \
               "Python Version: %s\n" % sys.version.split()[0] + \
               "wxPython Version: %s\n" % wx.VERSION_STRING + \
               ("\t(%s)\n" % ", ".join(wx.PlatformInfo[1:]))

        dialog = wx.MessageDialog(self.parent, text, title,
                                  wx.OK | wx.ICON_INFORMATION)
        dialog.ShowModal()
        dialog.Destroy()

    def EditStartupScript(self):
        """Open a Edit buffer of the startup script file."""
        self.bufferCreate(filename=self.startup_file)

    def LoadSettings(self):
        """Load settings for the shell."""
        if self.config is not None:
            KiCadEditorNotebookFrame.LoadSettings(self.parent, self.config)
            self.autoSaveSettings = \
                self.config.ReadBool('Options/AutoSaveSettings', False)
            self.execStartupScript = \
                self.config.ReadBool('Options/ExecStartupScript', True)
            self.autoSaveHistory = \
                self.config.ReadBool('Options/AutoSaveHistory', False)
            self.hideFoldingMargin = \
                self.config.ReadBool('Options/HideFoldingMargin', True)

    def SaveSettings(self, force=False):
        """
        Save settings for the shell.

        Arguments:

        force -- False - Autosaving.  True - Manual Saving.
        """
        if self.config is not None:
            # always save these
            self.config.WriteBool('Options/AutoSaveSettings',
                                  self.autoSaveSettings)
            if self.autoSaveSettings or force:
                KiCadEditorNotebookFrame.SaveSettings(self, self.config)

                self.config.WriteBool('Options/AutoSaveHistory',
                                      self.autoSaveHistory)
                self.config.WriteBool('Options/ExecStartupScript',
                                      self.execStartupScript)
                self.config.WriteBool('Options/HideFoldingMargin',
                                      self.hideFoldingMargin)
            if self.autoSaveHistory:
                self.SaveHistory()

    def DoSaveSettings(self):
        """Menu function to trigger saving the shells settings."""
        if self.config is not None:
            self.SaveSettings(force=True)
            self.config.Flush()

    def SaveHistory(self):
        """Save shell history to the shell history file."""
        if self.dataDir:
            try:
                name = self.history_file
                f = file(name, 'w')
                hist = []
                enc = wx.GetDefaultPyEncoding()
                for h in self.shell.history:
                    if isinstance(h, unicode):
                        h = h.encode(enc)
                    hist.append(h)
                hist = '\x00\n'.join(hist)
                f.write(hist)
                f.close()
            except:
                d = wx.MessageDialog(self, "Error saving history file.",
                                     "Error", wx.ICON_EXCLAMATION | wx.OK)
                d.ShowModal()
                d.Destroy()
                raise

    def LoadHistory(self):
        """Load shell history from the shell history file."""
        if self.dataDir:
            name = self.history_file
            if os.path.exists(name):
                try:
                    f = file(name, 'U')
                    hist = f.read()
                    f.close()
                    self.shell.history = hist.split('\x00\n')
                    dispatcher.send(signal="Shell.loadHistory",
                                    history=self.shell.history)
                except:
                    d = wx.MessageDialog(self,
                                         "Error loading history file!",
                                         "Error", wx.ICON_EXCLAMATION | wx.OK)
                    d.ShowModal()
                    d.Destroy()


def makePcbnewShellWindow(parentid):
    """
    Create a new Shell Window and return its handle.

    Arguments:
    parent -- The parent window to attach to.

    Returns:
    The handle to the new window.
    """

    parent = wx.FindWindowById( parentid )

    frmname = parent.GetName()
    return KiCadPyShell(parent)
