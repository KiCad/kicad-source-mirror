'''
Provides the backend for a basic python editor in KiCad.

This takes most code from PyShell/PyCrust but adapts it to the KiCad
environment where the Python doesn't create a frame but instead hooks
into the existing KIWAY_PLAYER

Original PyCrust code used from
https://github.com/wxWidgets/Phoenix/tree/master/wx/py
'''

import wx

from wx.py import crust, version, dispatcher, editwindow
from wx.py.editor import Editor, openSingle, openMultiple, saveSingle, messageDialog
from wx.py.buffer import Buffer
from wx import stc

ID_NEW = wx.ID_NEW
ID_OPEN = wx.ID_OPEN
ID_REVERT = wx.ID_REVERT
ID_CLOSE = wx.ID_CLOSE
ID_SAVE = wx.ID_SAVE
ID_SAVEAS = wx.ID_SAVEAS
ID_PRINT = wx.ID_PRINT
ID_EXIT = wx.ID_EXIT
ID_UNDO = wx.ID_UNDO
ID_REDO = wx.ID_REDO
ID_CUT = wx.ID_CUT
ID_COPY = wx.ID_COPY
ID_PASTE = wx.ID_PASTE
ID_CLEAR = wx.ID_CLEAR
ID_SELECTALL = wx.ID_SELECTALL
ID_EMPTYBUFFER = 1
ID_ABOUT = wx.ID_ABOUT
ID_HELP = 1
ID_AUTOCOMP_SHOW = 1
ID_AUTOCOMP_MAGIC = 1
ID_AUTOCOMP_SINGLE = 1
ID_AUTOCOMP_DOUBLE = 1
ID_CALLTIPS_SHOW = 1
ID_CALLTIPS_INSERT = 1
ID_COPY_PLUS = 1
ID_NAMESPACE = 1
ID_PASTE_PLUS = 1
ID_WRAP = 1
ID_TOGGLE_MAXIMIZE = 1
ID_SHOW_LINENUMBERS = 1
ID_ENABLESHELLMODE = 1
ID_ENABLEAUTOSYMPY = 1
ID_AUTO_SAVESETTINGS = 1
ID_SAVEACOPY = 1
ID_SAVEHISTORY = 1
ID_SAVEHISTORYNOW = 1
ID_CLEARHISTORY = 1
ID_SAVESETTINGS = 1
ID_DELSETTINGSFILE = 1
ID_EDITSTARTUPSCRIPT = 1
ID_EXECSTARTUPSCRIPT = 1
ID_SHOWPYSLICESTUTORIAL = 1
ID_FIND = wx.ID_FIND
ID_FINDNEXT = 1
ID_FINDPREVIOUS = 1
ID_SHOWTOOLS = 1
ID_HIDEFOLDINGMARGIN = 1

INTRO = "KiCad - Python Shell"

class KiCadPyFrame():

    def __init__(self, parent):
        """Create a Frame instance."""

        self.parent = parent
        self.parent.CreateStatusBar()
        self.parent.SetStatusText('Frame')
        self.shellName='KiCad Python Editor'
        self.__createMenus()

        self.iconized = False
        self.findDlg = None
        self.findData = wx.FindReplaceData()
        self.findData.SetFlags(wx.FR_DOWN)

        self.parent.Bind(wx.EVT_ICONIZE, self.OnIconize)

    def SetIDs( self ):
        ID_EMPTYBUFFER = self.parent.NewControlId()
        ID_HELP = self.parent.NewControlId()
        ID_AUTOCOMP_SHOW = self.parent.NewControlId()
        ID_AUTOCOMP_MAGIC = self.parent.NewControlId()
        ID_AUTOCOMP_SINGLE = self.parent.NewControlId()
        ID_AUTOCOMP_DOUBLE = self.parent.NewControlId()
        ID_CALLTIPS_SHOW = self.parent.NewControlId()
        ID_CALLTIPS_INSERT = self.parent.NewControlId()
        ID_COPY_PLUS = self.parent.NewControlId()
        ID_NAMESPACE = self.parent.NewControlId()
        ID_PASTE_PLUS = self.parent.NewControlId()
        ID_WRAP = self.parent.NewControlId()
        ID_TOGGLE_MAXIMIZE = self.parent.NewControlId()
        ID_SHOW_LINENUMBERS = self.parent.NewControlId()
        ID_ENABLESHELLMODE = self.parent.NewControlId()
        ID_ENABLEAUTOSYMPY = self.parent.NewControlId()
        ID_AUTO_SAVESETTINGS = self.parent.NewControlId()
        ID_SAVEACOPY = self.parent.NewControlId()
        ID_SAVEHISTORY = self.parent.NewControlId()
        ID_SAVEHISTORYNOW = self.parent.NewControlId()
        ID_CLEARHISTORY = self.parent.NewControlId()
        ID_SAVESETTINGS = self.parent.NewControlId()
        ID_DELSETTINGSFILE = self.parent.NewControlId()
        ID_EDITSTARTUPSCRIPT = self.parent.NewControlId()
        ID_EXECSTARTUPSCRIPT = self.parent.NewControlId()
        ID_SHOWPYSLICESTUTORIAL = self.parent.NewControlId()
        ID_FINDNEXT = self.parent.NewControlId()
        ID_FINDPREVIOUS = self.parent.NewControlId()
        ID_SHOWTOOLS = self.parent.NewControlId()
        ID_HIDEFOLDINGMARGIN = self.parent.NewControlId()

    def OnIconize(self, event):
        """Event handler for Iconize."""
        self.iconized = event.IsIconized()


    def __createMenus(self):
        # File Menu
        m = self.fileMenu = wx.Menu()
        m.Append(ID_NEW, '&New \tCtrl+N',
                 'New file')
        m.Append(ID_OPEN, '&Open... \tCtrl+O',
                 'Open file')
        m.AppendSeparator()
        m.Append(ID_REVERT, '&Revert \tCtrl+R',
                 'Revert to last saved version')
        m.Append(ID_CLOSE, '&Close \tCtrl+W',
                 'Close file')
        m.AppendSeparator()
        m.Append(ID_SAVE, '&Save... \tCtrl+S',
                 'Save file')
        m.Append(ID_SAVEAS, 'Save &As \tCtrl+Shift+S',
                 'Save file with new name')
        m.AppendSeparator()
        m.Append(ID_PRINT, '&Print... \tCtrl+P',
                 'Print file')
        m.AppendSeparator()
        m.Append(ID_NAMESPACE, '&Update Namespace \tCtrl+Shift+N',
                 'Update namespace for autocompletion and calltips')
        m.AppendSeparator()
        m.Append(ID_EXIT, 'E&xit\tCtrl+Q', 'Exit Program')

        # Edit
        m = self.editMenu = wx.Menu()
        m.Append(ID_UNDO, '&Undo \tCtrl+Z',
                 'Undo the last action')
        m.Append(ID_REDO, '&Redo \tCtrl+Y',
                 'Redo the last undone action')
        m.AppendSeparator()
        m.Append(ID_CUT, 'Cu&t \tCtrl+X',
                 'Cut the selection')
        m.Append(ID_COPY, '&Copy \tCtrl+C',
                 'Copy the selection')
        m.Append(ID_COPY_PLUS, 'Cop&y Plus \tCtrl+Shift+C',
                 'Copy the selection - retaining prompts')
        m.Append(ID_PASTE, '&Paste \tCtrl+V', 'Paste from clipboard')
        m.Append(ID_PASTE_PLUS, 'Past&e Plus \tCtrl+Shift+V',
                 'Paste and run commands')
        m.AppendSeparator()
        m.Append(ID_CLEAR, 'Cle&ar',
                 'Delete the selection')
        m.Append(ID_SELECTALL, 'Select A&ll \tCtrl+A',
                 'Select all text')
        m.AppendSeparator()
        m.Append(ID_EMPTYBUFFER, 'E&mpty Buffer...',
                 'Delete all the contents of the edit buffer')
        m.Append(ID_FIND, '&Find Text... \tCtrl+F',
                 'Search for text in the edit buffer')
        m.Append(ID_FINDNEXT, 'Find &Next \tCtrl+G',
                 'Find next instance of the search text')
        m.Append(ID_FINDPREVIOUS, 'Find Pre&vious \tCtrl+Shift+G',
                 'Find previous instance of the search text')

        # View
        m = self.viewMenu = wx.Menu()
        m.Append(ID_WRAP, '&Wrap Lines\tCtrl+Shift+W',
                 'Wrap lines at right edge', wx.ITEM_CHECK)
        m.Append(ID_SHOW_LINENUMBERS, '&Show Line Numbers\tCtrl+Shift+L',
                 'Show Line Numbers', wx.ITEM_CHECK)
        m.Append(ID_TOGGLE_MAXIMIZE, '&Toggle Maximize\tF11',
                 'Maximize/Restore Application')

        # # Options
        # m = self.optionsMenu = wx.Menu()
        #
        # self.historyMenu = wx.Menu()
        # self.historyMenu.Append(ID_SAVEHISTORY, '&Autosave History',
        #          'Automatically save history on close', wx.ITEM_CHECK)
        # self.historyMenu.Append(ID_SAVEHISTORYNOW, '&Save History Now',
        #          'Save history')
        # self.historyMenu.Append(ID_CLEARHISTORY, '&Clear History ',
        #          'Clear history')
        # m.AppendSubMenu(self.historyMenu, "&History", "History Options")
        #
        # self.startupMenu = wx.Menu()
        # self.startupMenu.Append(ID_EXECSTARTUPSCRIPT,
        #                         'E&xecute Startup Script',
        #                         'Execute Startup Script', wx.ITEM_CHECK)
        # self.startupMenu.Append(ID_EDITSTARTUPSCRIPT,
        #                         '&Edit Startup Script...',
        #                         'Edit Startup Script')
        # m.AppendSubMenu(self.startupMenu, '&Startup', 'Startup Options')

        self.settingsMenu = wx.Menu()
        self.settingsMenu.Append(ID_AUTO_SAVESETTINGS,
                                 '&Auto Save Settings',
                                 'Automatically save settings on close', wx.ITEM_CHECK)
        self.settingsMenu.Append(ID_SAVESETTINGS,
                                 '&Save Settings',
                                 'Save settings now')
        self.settingsMenu.Append(ID_DELSETTINGSFILE,
                                 '&Revert to default',
                                 'Revert to the default settings')
        m.AppendSubMenu(self.settingsMenu, '&Settings', 'Settings Options')

        m = self.helpMenu = wx.Menu()
        m.Append(ID_HELP, '&Help\tF1', 'Help!')
        m.AppendSeparator()
        m.Append(ID_ABOUT, '&About...', 'About this program')

        b = self.menuBar = wx.MenuBar()
        b.Append(self.fileMenu, '&File')
        b.Append(self.editMenu, '&Edit')
        b.Append(self.viewMenu, '&View')
        # b.Append(self.optionsMenu, '&Options')
        b.Append(self.helpMenu, '&Help')
        self.parent.SetMenuBar(b)

        self.parent.Bind(wx.EVT_MENU, self.OnFileNew, id=ID_NEW)
        self.parent.Bind(wx.EVT_MENU, self.OnFileOpen, id=ID_OPEN)
        self.parent.Bind(wx.EVT_MENU, self.OnFileRevert, id=ID_REVERT)
        self.parent.Bind(wx.EVT_MENU, self.OnFileClose, id=ID_CLOSE)
        self.parent.Bind(wx.EVT_MENU, self.OnFileSave, id=ID_SAVE)
        self.parent.Bind(wx.EVT_MENU, self.OnFileSaveAs, id=ID_SAVEAS)
        self.parent.Bind(wx.EVT_MENU, self.OnFileSaveACopy, id=ID_SAVEACOPY)
        self.parent.Bind(wx.EVT_MENU, self.OnFileUpdateNamespace, id=ID_NAMESPACE)
        self.parent.Bind(wx.EVT_MENU, self.OnFilePrint, id=ID_PRINT)
        self.parent.Bind(wx.EVT_MENU, self.OnExit, id=ID_EXIT)
        self.parent.Bind(wx.EVT_MENU, self.OnUndo, id=ID_UNDO)
        self.parent.Bind(wx.EVT_MENU, self.OnRedo, id=ID_REDO)
        self.parent.Bind(wx.EVT_MENU, self.OnCut, id=ID_CUT)
        self.parent.Bind(wx.EVT_MENU, self.OnCopy, id=ID_COPY)
        self.parent.Bind(wx.EVT_MENU, self.OnCopyPlus, id=ID_COPY_PLUS)
        self.parent.Bind(wx.EVT_MENU, self.OnPaste, id=ID_PASTE)
        self.parent.Bind(wx.EVT_MENU, self.OnPastePlus, id=ID_PASTE_PLUS)
        self.parent.Bind(wx.EVT_MENU, self.OnClear, id=ID_CLEAR)
        self.parent.Bind(wx.EVT_MENU, self.OnSelectAll, id=ID_SELECTALL)
        self.parent.Bind(wx.EVT_MENU, self.OnEmptyBuffer, id=ID_EMPTYBUFFER)
        self.parent.Bind(wx.EVT_MENU, self.OnAbout, id=ID_ABOUT)
        self.parent.Bind(wx.EVT_MENU, self.OnHelp, id=ID_HELP)
        self.parent.Bind(wx.EVT_MENU, self.OnAutoCompleteShow, id=ID_AUTOCOMP_SHOW)
        self.parent.Bind(wx.EVT_MENU, self.OnAutoCompleteMagic, id=ID_AUTOCOMP_MAGIC)
        self.parent.Bind(wx.EVT_MENU, self.OnAutoCompleteSingle, id=ID_AUTOCOMP_SINGLE)
        self.parent.Bind(wx.EVT_MENU, self.OnAutoCompleteDouble, id=ID_AUTOCOMP_DOUBLE)
        self.parent.Bind(wx.EVT_MENU, self.OnCallTipsShow, id=ID_CALLTIPS_SHOW)
        self.parent.Bind(wx.EVT_MENU, self.OnCallTipsInsert, id=ID_CALLTIPS_INSERT)
        self.parent.Bind(wx.EVT_MENU, self.OnWrap, id=ID_WRAP)
        self.parent.Bind(wx.EVT_MENU, self.OnToggleMaximize, id=ID_TOGGLE_MAXIMIZE)
        self.parent.Bind(wx.EVT_MENU, self.OnShowLineNumbers, id=ID_SHOW_LINENUMBERS)
        self.parent.Bind(wx.EVT_MENU, self.OnEnableShellMode, id=ID_ENABLESHELLMODE)
        self.parent.Bind(wx.EVT_MENU, self.OnEnableAutoSympy, id=ID_ENABLEAUTOSYMPY)
        self.parent.Bind(wx.EVT_MENU, self.OnAutoSaveSettings, id=ID_AUTO_SAVESETTINGS)
        self.parent.Bind(wx.EVT_MENU, self.OnSaveHistory, id=ID_SAVEHISTORY)
        self.parent.Bind(wx.EVT_MENU, self.OnSaveHistoryNow, id=ID_SAVEHISTORYNOW)
        self.parent.Bind(wx.EVT_MENU, self.OnClearHistory, id=ID_CLEARHISTORY)
        self.parent.Bind(wx.EVT_MENU, self.OnSaveSettings, id=ID_SAVESETTINGS)
        self.parent.Bind(wx.EVT_MENU, self.OnDelSettingsFile, id=ID_DELSETTINGSFILE)
        self.parent.Bind(wx.EVT_MENU, self.OnEditStartupScript, id=ID_EDITSTARTUPSCRIPT)
        self.parent.Bind(wx.EVT_MENU, self.OnExecStartupScript, id=ID_EXECSTARTUPSCRIPT)
        self.parent.Bind(wx.EVT_MENU, self.OnShowPySlicesTutorial, id=ID_SHOWPYSLICESTUTORIAL)
        self.parent.Bind(wx.EVT_MENU, self.OnFindText, id=ID_FIND)
        self.parent.Bind(wx.EVT_MENU, self.OnFindNext, id=ID_FINDNEXT)
        self.parent.Bind(wx.EVT_MENU, self.OnFindPrevious, id=ID_FINDPREVIOUS)
        self.parent.Bind(wx.EVT_MENU, self.OnToggleTools, id=ID_SHOWTOOLS)
        self.parent.Bind(wx.EVT_MENU, self.OnHideFoldingMargin, id=ID_HIDEFOLDINGMARGIN)

        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_NEW)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_OPEN)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_REVERT)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_CLOSE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SAVE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SAVEAS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_NAMESPACE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_PRINT)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_UNDO)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_REDO)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_CUT)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_COPY)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_COPY_PLUS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_PASTE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_PASTE_PLUS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_CLEAR)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SELECTALL)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_EMPTYBUFFER)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_AUTOCOMP_SHOW)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_AUTOCOMP_MAGIC)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_AUTOCOMP_SINGLE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_AUTOCOMP_DOUBLE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_CALLTIPS_SHOW)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_CALLTIPS_INSERT)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_WRAP)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SHOW_LINENUMBERS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_ENABLESHELLMODE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_ENABLEAUTOSYMPY)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_AUTO_SAVESETTINGS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SAVESETTINGS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_DELSETTINGSFILE)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_EXECSTARTUPSCRIPT)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SHOWPYSLICESTUTORIAL)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SAVEHISTORY)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SAVEHISTORYNOW)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_CLEARHISTORY)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_EDITSTARTUPSCRIPT)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_FIND)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_FINDNEXT)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_FINDPREVIOUS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_SHOWTOOLS)
        self.parent.Bind(wx.EVT_UPDATE_UI, self.OnUpdateMenu, id=ID_HIDEFOLDINGMARGIN)

        self.parent.Bind(wx.EVT_ACTIVATE, self.OnActivate)
        self.parent.Bind(wx.EVT_FIND, self.OnFindNext)
        self.parent.Bind(wx.EVT_FIND_NEXT, self.OnFindNext)
        self.parent.Bind(wx.EVT_FIND_CLOSE, self.OnFindClose)

    def OnShowLineNumbers(self, event):
        win = wx.Window.FindFocus()
        if hasattr(win, 'lineNumbers'):
            win.lineNumbers = event.IsChecked()
            win.setDisplayLineNumbers(win.lineNumbers)

    def OnFileNew(self, event):
        self.bufferNew()

    def OnFileOpen(self, event):
        self.bufferOpen()

    def OnFileRevert(self, event):
        self.bufferRevert()

    def OnFileClose(self, event):
        self.bufferClose()

    def OnFileSave(self, event):
        self.bufferSave()

    def OnFileSaveAs(self, event):
        self.bufferSaveAs()

    def OnFileSaveACopy(self, event):
        self.bufferSaveACopy()

    def OnFileUpdateNamespace(self, event):
        self.updateNamespace()

    def OnFilePrint(self, event):
        self.bufferPrint()

    def OnExit(self, event):
        self.parent.Close(False)

    def OnUndo(self, event):
        win = wx.Window.FindFocus()
        win.Undo()

    def OnRedo(self, event):
        win = wx.Window.FindFocus()
        win.Redo()

    def OnCut(self, event):
        win = wx.Window.FindFocus()
        win.Cut()

    def OnCopy(self, event):
        win = wx.Window.FindFocus()
        win.Copy()

    def OnCopyPlus(self, event):
        win = wx.Window.FindFocus()
        win.CopyWithPrompts()

    def OnPaste(self, event):
        win = wx.Window.FindFocus()
        win.Paste()

    def OnPastePlus(self, event):
        win = wx.Window.FindFocus()
        win.PasteAndRun()

    def OnClear(self, event):
        win = wx.Window.FindFocus()
        win.Clear()

    def OnEmptyBuffer(self, event):
        win = wx.Window.FindFocus()
        d = wx.MessageDialog(self,
                             "Are you sure you want to clear the edit buffer,\n"
                             "deleting all the text?",
                             "Empty Buffer", wx.OK | wx.CANCEL | wx.ICON_QUESTION)
        answer = d.ShowModal()
        d.Destroy()
        if (answer == wx.ID_OK):
            win.ClearAll()
            if hasattr(win,'prompt'):
                win.prompt()

    def OnSelectAll(self, event):
        win = wx.Window.FindFocus()
        win.SelectAll()

    def OnAbout(self, event):
        """Display an About window."""
        title = 'About'
        text = 'Your message here.'
        dialog = wx.MessageDialog(self.parent, text, title,
                                  wx.OK | wx.ICON_INFORMATION)
        dialog.ShowModal()
        dialog.Destroy()

    def OnHelp(self, event):
        """Display a Help window."""
        title = 'Help'
        text = "Type 'shell.help()' in the shell window."
        dialog = wx.MessageDialog(self.parent, text, title,
                                  wx.OK | wx.ICON_INFORMATION)
        dialog.ShowModal()
        dialog.Destroy()

    def OnAutoCompleteShow(self, event):
        win = wx.Window.FindFocus()
        win.autoComplete = event.IsChecked()

    def OnAutoCompleteMagic(self, event):
        win = wx.Window.FindFocus()
        win.autoCompleteIncludeMagic = event.IsChecked()

    def OnAutoCompleteSingle(self, event):
        win = wx.Window.FindFocus()
        win.autoCompleteIncludeSingle = event.IsChecked()

    def OnAutoCompleteDouble(self, event):
        win = wx.Window.FindFocus()
        win.autoCompleteIncludeDouble = event.IsChecked()

    def OnCallTipsShow(self, event):
        win = wx.Window.FindFocus()
        win.autoCallTip = event.IsChecked()

    def OnCallTipsInsert(self, event):
        win = wx.Window.FindFocus()
        win.callTipInsert = event.IsChecked()

    def OnWrap(self, event):
        win = wx.Window.FindFocus()
        win.SetWrapMode(event.IsChecked())
        wx.CallLater(1, self.shell.EnsureCaretVisible)

    def OnToggleMaximize(self, event):
        self.parent.Maximize(not self.parent.IsMaximized())

    def OnSaveHistory(self, event):
        self.autoSaveHistory = event.IsChecked()

    def OnSaveHistoryNow(self, event):
        self.SaveHistory()

    def OnClearHistory(self, event):
        self.shell.clearHistory()

    def OnEnableShellMode(self, event):
        self.enableShellMode = event.IsChecked()

    def OnEnableAutoSympy(self, event):
        self.enableAutoSympy = event.IsChecked()

    def OnHideFoldingMargin(self, event):
        self.hideFoldingMargin = event.IsChecked()

    def OnAutoSaveSettings(self, event):
        self.autoSaveSettings = event.IsChecked()

    def OnSaveSettings(self, event):
        self.DoSaveSettings()

    def OnDelSettingsFile(self, event):
        if self.config is not None:
            d = wx.MessageDialog(
                self.parent, "Do you want to revert to the default settings?\n" +
                "A restart is needed for the change to take effect",
                "Warning", wx.OK | wx.CANCEL | wx.ICON_QUESTION)
            answer = d.ShowModal()
            d.Destroy()
            if (answer == wx.ID_OK):
                self.config.DeleteAll()
                self.LoadSettings()


    def OnEditStartupScript(self, event):
        if hasattr(self, 'EditStartupScript'):
            self.EditStartupScript()

    def OnExecStartupScript(self, event):
        self.execStartupScript = event.IsChecked()
        self.SaveSettings(force=True)

    def OnShowPySlicesTutorial(self,event):
        self.showPySlicesTutorial = event.IsChecked()
        self.SaveSettings(force=True)

    def OnFindText(self, event):
        if self.findDlg is not None:
            return
        win = wx.Window.FindFocus()
        if self.shellName == 'PyCrust':
            self.findDlg = wx.FindReplaceDialog(win, self.findData,
                                               "Find",wx.FR_NOWHOLEWORD)
        else:
            self.findDlg = wx.FindReplaceDialog(win, self.findData,
                "Find & Replace", wx.FR_NOWHOLEWORD|wx.FR_REPLACEDIALOG)
        self.findDlg.Show()

    def OnFindNext(self, event,backward=False):
        if backward and (self.findData.GetFlags() & wx.FR_DOWN):
            self.findData.SetFlags( self.findData.GetFlags() ^ wx.FR_DOWN )
        elif not backward and not (self.findData.GetFlags() & wx.FR_DOWN):
            self.findData.SetFlags( self.findData.GetFlags() ^ wx.FR_DOWN )

        if not self.findData.GetFindString():
            self.OnFindText(event)
            return
        if isinstance(event, wx.FindDialogEvent):
            win = self.findDlg.GetParent()
        else:
            win = wx.Window.FindFocus()
        win.DoFindNext(self.findData, self.findDlg)
        if self.findDlg is not None:
            self.OnFindClose(None)

    def OnFindPrevious(self, event):
        self.OnFindNext(event,backward=True)

    def OnFindClose(self, event):
        self.findDlg.Destroy()
        self.findDlg = None

    def OnToggleTools(self, event):
        self.ToggleTools()


    def OnUpdateMenu(self, event):
        """Update menu items based on current status and context."""
        win = wx.Window.FindFocus()
        id = event.GetId()
        event.Enable(True)
        try:
            if id == ID_NEW:
                event.Enable(hasattr(self, 'bufferNew'))
            elif id == ID_OPEN:
                event.Enable(hasattr(self, 'bufferOpen'))
            elif id == ID_REVERT:
                event.Enable(hasattr(self, 'bufferRevert')
                             and self.hasBuffer())
            elif id == ID_CLOSE:
                event.Enable(hasattr(self, 'bufferClose')
                             and self.hasBuffer())
            elif id == ID_SAVE:
                event.Enable(hasattr(self, 'bufferSave')
                             and self.bufferHasChanged())
            elif id == ID_SAVEAS:
                event.Enable(hasattr(self, 'bufferSaveAs')
                             and self.hasBuffer())
            elif id == ID_SAVEACOPY:
                event.Enable(hasattr(self, 'bufferSaveACopy')
                             and self.hasBuffer())
            elif id == ID_NAMESPACE:
                event.Enable(hasattr(self, 'updateNamespace')
                             and self.hasBuffer())
            elif id == ID_PRINT:
                event.Enable(hasattr(self, 'bufferPrint')
                             and self.hasBuffer())
            elif id == ID_UNDO:
                event.Enable(win.CanUndo())
            elif id == ID_REDO:
                event.Enable(win.CanRedo())
            elif id == ID_CUT:
                event.Enable(win.CanCut())
            elif id == ID_COPY:
                event.Enable(win.CanCopy())
            elif id == ID_COPY_PLUS:
                event.Enable(win.CanCopy() and hasattr(win, 'CopyWithPrompts'))
            elif id == ID_PASTE:
                event.Enable(win.CanPaste())
            elif id == ID_PASTE_PLUS:
                event.Enable(win.CanPaste() and hasattr(win, 'PasteAndRun'))
            elif id == ID_CLEAR:
                event.Enable(win.CanCut())
            elif id == ID_SELECTALL:
                event.Enable(hasattr(win, 'SelectAll'))
            elif id == ID_EMPTYBUFFER:
                event.Enable(hasattr(win, 'ClearAll') and not win.GetReadOnly())
            elif id == ID_AUTOCOMP_SHOW:
                event.Check(win.autoComplete)
            elif id == ID_AUTOCOMP_MAGIC:
                event.Check(win.autoCompleteIncludeMagic)
            elif id == ID_AUTOCOMP_SINGLE:
                event.Check(win.autoCompleteIncludeSingle)
            elif id == ID_AUTOCOMP_DOUBLE:
                event.Check(win.autoCompleteIncludeDouble)
            elif id == ID_CALLTIPS_SHOW:
                event.Check(win.autoCallTip)
            elif id == ID_CALLTIPS_INSERT:
                event.Check(win.callTipInsert)
            elif id == ID_WRAP:
                event.Check(win.GetWrapMode())

            elif id == ID_SHOW_LINENUMBERS:
                event.Check(win.lineNumbers)
            elif id == ID_ENABLESHELLMODE:
                event.Check(self.enableShellMode)
                event.Enable(self.config is not None)
            elif id == ID_ENABLEAUTOSYMPY:
                event.Check(self.enableAutoSympy)
                event.Enable(self.config is not None)
            # elif id == ID_AUTO_SAVESETTINGS:
            #     event.Check(self.autoSaveSettings)
            #     event.Enable(self.config is not None)
            # elif id == ID_SAVESETTINGS:
            #     event.Enable(self.config is not None and
            #                  hasattr(self, 'DoSaveSettings'))
            # elif id == ID_DELSETTINGSFILE:
            #     event.Enable(self.config is not None)
            #
            # elif id == ID_EXECSTARTUPSCRIPT:
            #     event.Check(self.execStartupScript)
            #     event.Enable(self.config is not None)
            #
            # elif id == ID_SAVEHISTORY:
            #     event.Check(self.autoSaveHistory)
            #     event.Enable(self.dataDir is not None)
            # elif id == ID_SAVEHISTORYNOW:
            #     event.Enable(self.dataDir is not None and
            #                  hasattr(self, 'SaveHistory'))
            # elif id == ID_CLEARHISTORY:
            #     event.Enable(self.dataDir is not None)
            #
            # elif id == ID_EDITSTARTUPSCRIPT:
            #     event.Enable(hasattr(self, 'EditStartupScript'))
            #     event.Enable(self.dataDir is not None)

            elif id == ID_FIND:
                event.Enable(hasattr(win, 'DoFindNext'))
            elif id == ID_FINDNEXT:
                event.Enable(hasattr(win, 'DoFindNext'))
            elif id == ID_FINDPREVIOUS:
                event.Enable(hasattr(win, 'DoFindNext'))

            elif id == ID_SHOWTOOLS:
                event.Check(self.ToolsShown())

            elif id == ID_HIDEFOLDINGMARGIN:
                event.Check(self.hideFoldingMargin)
                event.Enable(self.config is not None)

            else:
                event.Enable(False)
        except AttributeError:
            # This menu option is not supported in the current context.
            event.Enable(False)


    def OnActivate(self, event):
        """
        Event Handler for losing the focus of the Frame. Should close
        Autocomplete listbox, if shown.
        """
        if not event.GetActive():
            # If autocomplete active, cancel it.  Otherwise, the
            # autocomplete list will stay visible on top of the
            # z-order after switching to another application
            win = wx.Window.FindFocus()
            if hasattr(win, 'AutoCompActive') and win.AutoCompActive():
                win.AutoCompCancel()
        event.Skip()



    def LoadSettings(self, config):
        """Called by derived classes to load settings specific to the Frame"""
        pos  = wx.Point(config.ReadInt('Window/PosX', -1),
                        config.ReadInt('Window/PosY', -1))

        size = wx.Size(config.ReadInt('Window/Width', -1),
                       config.ReadInt('Window/Height', -1))

        self.SetSize(size)
        self.Move(pos)


    def SaveSettings(self, config):
        """Called by derived classes to save Frame settings to a wx.Config object"""

        # TODO: track position/size so we can save it even if the
        # frame is maximized or iconized.
        if not self.iconized and not self.parent.IsMaximized():
            w, h = self.GetSize()
            config.WriteInt('Window/Width', w)
            config.WriteInt('Window/Height', h)

            px, py = self.GetPosition()
            config.WriteInt('Window/PosX', px)
            config.WriteInt('Window/PosY', py)

class KiCadEditorFrame(KiCadPyFrame):
    def __init__(self, parent=None, id=-1, title='KiCad Python'):

        """Create EditorFrame instance."""
        KiCadPyFrame.__init__(self, parent)
        self.buffers = {}
        self.buffer = None  # Current buffer.
        self.editor = None
        self._defaultText = title
        self._statusText = self._defaultText
        self.parent.SetStatusText(self._statusText)
        self.parent.Bind( wx.EVT_IDLE, self.OnIdle )
        self.parent.Bind( wx.EVT_CLOSE, self.OnClose )
        self._setup()

    def _setup(self):
        """Setup prior to first buffer creation.

        Useful for subclasses."""
        pass

    def setEditor(self, editor):
        self.editor = editor
        self.buffer = self.editor.buffer
        self.buffers[self.buffer.id] = self.buffer

    def OnClose(self, event):
        """Event handler for closing."""
        for buffer in self.buffers.values():
            self.buffer = buffer
            if buffer.hasChanged():
                cancel = self.bufferSuggestSave()
                if cancel and event.CanVeto():
                    event.Veto()
                    return

        self.parent.Hide()
        pass

    def OnIdle(self, event):
        """Event handler for idle time."""
        self._updateStatus()
        if hasattr(self, 'notebook'):
            self._updateTabText()
        self._updateTitle()
        event.Skip()

    def _updateStatus(self):
        """Show current status information."""
        if self.editor and hasattr(self.editor, 'getStatus'):
            status = self.editor.getStatus()
            text = 'File: %s  |  Line: %d  |  Column: %d' % status
        else:
            text = self._defaultText
        if text != self._statusText:
            self.parent.SetStatusText(text)
            self._statusText = text

    def _updateTabText(self):
        """Show current buffer information on notebook tab."""
##         suffix = ' **'
##         notebook = self.notebook
##         selection = notebook.GetSelection()
##         if selection == -1:
##             return
##         text = notebook.GetPageText(selection)
##         window = notebook.GetPage(selection)
##         if window.editor and window.editor.buffer.hasChanged():
##             if text.endswith(suffix):
##                 pass
##             else:
##                 notebook.SetPageText(selection, text + suffix)
##         else:
##             if text.endswith(suffix):
##                 notebook.SetPageText(selection, text[:len(suffix)])

    def _updateTitle(self):
        """Show current title information."""
        title = self.GetTitle()
        if self.bufferHasChanged():
            if title.startswith('* '):
                pass
            else:
                self.SetTitle('* ' + title)
        else:
            if title.startswith('* '):
                self.SetTitle(title[2:])

    def hasBuffer(self):
        """Return True if there is a current buffer."""
        if self.buffer:
            return True
        else:
            return False

    def bufferClose(self):
        """Close buffer."""
        if self.bufferHasChanged():
            cancel = self.bufferSuggestSave()
            if cancel:
                return cancel
        self.bufferDestroy()
        cancel = False
        return cancel

    def bufferCreate(self, filename=None):
        """Create new buffer."""
        self.bufferDestroy()
        buffer = Buffer()
        self.panel = panel = wx.Panel(parent=self, id=-1)
        panel.Bind (wx.EVT_ERASE_BACKGROUND, lambda x: x)
        editor = Editor(parent=panel)
        panel.editor = editor
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(editor.window, 1, wx.EXPAND)
        panel.SetSizer(sizer)
        panel.SetAutoLayout(True)
        sizer.Layout()
        buffer.addEditor(editor)
        buffer.open(filename)
        self.setEditor(editor)
        self.editor.setFocus()
        self.SendSizeEvent()


    def bufferDestroy(self):
        """Destroy the current buffer."""
        if self.buffer:
            for editor in self.buffer.editors.values():
                editor.destroy()
            self.editor = None
            del self.buffers[self.buffer.id]
            self.buffer = None
            self.panel.Destroy()


    def bufferHasChanged(self):
        """Return True if buffer has changed since last save."""
        if self.buffer:
            return self.buffer.hasChanged()
        else:
            return False

    def bufferNew(self):
        """Create new buffer."""
        if self.bufferHasChanged():
            cancel = self.bufferSuggestSave()
            if cancel:
                return cancel
        self.bufferCreate()
        cancel = False
        return cancel

    def bufferOpen(self):
        """Open file in buffer."""
        if self.bufferHasChanged():
            cancel = self.bufferSuggestSave()
            if cancel:
                return cancel
        filedir = ''
        if self.buffer and self.buffer.doc.filedir:
            filedir = self.buffer.doc.filedir
        result = openSingle(directory=filedir)
        if result.path:
            self.bufferCreate(result.path)
        cancel = False
        return cancel

    def bufferSave(self):
        """Save buffer to its file."""
        if self.buffer.doc.filepath:
            self.buffer.save()
            cancel = False
        else:
            cancel = self.bufferSaveAs()
        return cancel

    def bufferSaveAs(self):
        """Save buffer to a new filename."""
        if self.bufferHasChanged() and self.buffer.doc.filepath:
            cancel = self.bufferSuggestSave()
            if cancel:
                return cancel
        filedir = ''
        if self.buffer and self.buffer.doc.filedir:
            filedir = self.buffer.doc.filedir
        result = saveSingle(directory=filedir)
        if result.path:
            self.buffer.saveAs(result.path)
            cancel = False
        else:
            cancel = True
        return cancel

    def bufferSuggestSave(self):
        """Suggest saving changes.  Return True if user selected Cancel."""
        result = messageDialog(parent=None,
                               message='%s has changed.\n'
                                       'Would you like to save it first'
                                       '?' % self.buffer.name,
                               title='Save current file?')
        if result.positive:
            cancel = self.bufferSave()
        else:
            cancel = result.text == 'Cancel'
        return cancel

    def updateNamespace(self):
        """Update the buffer namespace for autocompletion and calltips."""
        if self.buffer.updateNamespace():
            self.parent.SetStatusText('Namespace updated')
        else:
            self.parent.SetStatusText('Error executing, unable to update namespace')


class KiCadEditorNotebookFrame(KiCadEditorFrame):
    def __init__(self, parent):
        """Create EditorNotebookFrame instance."""

        self.notebook = None
        KiCadEditorFrame.__init__(self, parent)

        if self.notebook:
            """Keep pydoc output on stdout instead of pager and
                place the stdout into the editor window """
            import pydoc, sys
            self._keep_stdin = sys.stdin

            """getline will crash unexpectedly, so we don't support it
                and bold fonts wreak havoc on our output, so strip them as well"""
            pydoc.getpager = lambda: pydoc.plainpager
            pydoc.Helper.getline = lambda self, prompt: None
            pydoc.TextDoc.use_bold = lambda self, text: text

            dispatcher.connect(receiver=self._editorChange,
                               signal='EditorChange', sender=self.notebook)

    def _setup(self):
        """Setup prior to first buffer creation.

        Called automatically by base class during init."""
        self.notebook = EditorNotebook(parent=self)
        intro = 'Py %s' % version.VERSION
        import imp
        module = imp.new_module('__main__')
        module.__dict__['__builtins__'] = __builtins__
        namespace = module.__dict__.copy()
        self.crust = crust.Crust(parent=self.notebook, intro=intro, locals=namespace)
        self.shell = self.crust.shell
        # Override the filling so that status messages go to the status bar.
        self.crust.filling.tree.setStatusText = self.SetStatusText
        # Override the shell so that status messages go to the status bar.
        self.shell.setStatusText = self.SetStatusText
        # Fix a problem with the sash shrinking to nothing.
        self.crust.filling.SetSashPosition(200)
        self.notebook.AddPage(page=self.crust, text='*Shell*', select=True)
        self.setEditor(self.crust.editor)
        self.crust.editor.SetFocus()


    def _editorChange(self, editor):
        """Editor change signal receiver."""
        if not self:
            dispatcher.disconnect(receiver=self._editorChange,
                                  signal='EditorChange', sender=self.notebook)
            return
        self.setEditor(editor)

    def _updateTitle(self):
        """Show current title information."""
        pass


    def bufferCreate(self, filename=None):
        """Create new buffer."""
        buffer = Buffer()
        panel = wx.Panel(parent=self.notebook, id=-1)
        panel.Bind(wx.EVT_ERASE_BACKGROUND, lambda x: x)
        editor = Editor(parent=panel)
        panel.editor = editor
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(editor.window, 1, wx.EXPAND)
        panel.SetSizer(sizer)
        panel.SetAutoLayout(True)
        sizer.Layout()
        buffer.addEditor(editor)
        buffer.open(filename)
        self.setEditor(editor)
        self.notebook.AddPage(page=panel, text=self.buffer.name, select=True)
        self.editor.setFocus()

    def bufferDestroy(self):
        """Destroy the current buffer."""
        selection = self.notebook.GetSelection()
##         print("Destroy Selection:", selection)
        if selection > 0:  # Don't destroy the PyCrust tab.
            if self.buffer:
                del self.buffers[self.buffer.id]
                self.buffer = None  # Do this before DeletePage().
            self.notebook.DeletePage(selection)

    def bufferNew(self):
        """Create new buffer."""
        self.bufferCreate()
        cancel = False
        return cancel

    def bufferOpen(self):
        """Open file in buffer."""
        filedir = ''
        if self.buffer and self.buffer.doc.filedir:
            filedir = self.buffer.doc.filedir
        result = openMultiple(directory=filedir)
        for path in result.paths:
            self.bufferCreate(path)
        cancel = False
        return cancel


class KiCadEditorNotebook(wx.Notebook):
    """A notebook containing a page for each editor."""

    def __init__(self, parent):
        """Create EditorNotebook instance."""
        wx.Notebook.__init__(self, parent, id=-1, style=wx.CLIP_CHILDREN)
        self.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGING, self.OnPageChanging, id=self.GetId())
        self.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChanged, id=self.GetId())
        self.Bind(wx.EVT_IDLE, self.OnIdle)

    def OnIdle(self, event):
        """Event handler for idle time."""
        self._updateTabText()
        event.Skip()

    def _updateTabText(self):
        """Show current buffer display name on all but first tab."""
        size = 3
        changed = ' **'
        unchanged = ' --'
        selection = self.GetSelection()
        if selection < 1:
            return
        text = self.GetPageText(selection)
        window = self.GetPage(selection)
        if not window.editor:
            return
        if text.endswith(changed) or text.endswith(unchanged):
            name = text[:-size]
        else:
            name = text
        if name != window.editor.buffer.name:
            text = window.editor.buffer.name
        if window.editor.buffer.hasChanged():
            if text.endswith(changed):
                text = None
            elif text.endswith(unchanged):
                text = text[:-size] + changed
            else:
                text += changed
        else:
            if text.endswith(changed):
                text = text[:-size] + unchanged
            elif text.endswith(unchanged):
                text = None
            else:
                text += unchanged
        if text is not None:
            self.SetPageText(selection, text)
            self.Refresh()  # Needed on Win98.

    def OnPageChanging(self, event):
        """Page changing event handler."""
        event.Skip()

    def OnPageChanged(self, event):
        """Page changed event handler."""
        new = event.GetSelection()
        window = self.GetPage(new)
        dispatcher.send(signal='EditorChange', sender=self,
                        editor=window.editor)
        window.SetFocus()
        event.Skip()


def is_using_dark_background():
    """Backport of wxSystemAppearance::IsUsingDarkBackground() from wxWidgets 3.1."""
    bg = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
    fg = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT)

    return get_luminance(fg) - get_luminance(bg) > 0.2


def get_luminance(color):
    """Backport of wxColour::GetLuminance() from wxWidgets 3.1."""
    return (0.299*color.Red() + 0.587*color.Green() + 0.114*color.Blue()) / 255.0


editwindow_old_init = editwindow.EditWindow.__init__

calltip_dark_bg_color = wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND).ChangeLightness(108)
calltip_dark_fg_color = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT).ChangeLightness(108)

def editwindow_new_init(self, *args, **kwargs):
    editwindow_old_init(self, *args, **kwargs)

    # Cannot be used until we switch to wxWidgets >=3.1.3.
    #if wx.SystemSettings.GetAppearance().IsUsingDarkBackground():

    # If a dark theme is detected, we set the default system colors and add our own Python
    # styles (which are not otherwise supplied by the system).
    if is_using_dark_background():
        faces = editwindow.FACES

        # Override the background in all styles by setting the default style, then copying this
        # to all styles with StyleClearAll(). Iterating over all styles would be slower.
        self.StyleSetForeground(stc.STC_STYLE_DEFAULT,
            wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT))
        self.StyleSetBackground(stc.STC_STYLE_DEFAULT,
            wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND))
        self.StyleClearAll()

        self.SetSelForeground(True, wx.SystemSettings.GetColour(wx.SYS_COLOUR_HIGHLIGHTTEXT))
        self.SetSelBackground(True, wx.SystemSettings.GetColour(wx.SYS_COLOUR_HIGHLIGHT))

        # Override the colors set in EditWindow.setStyles(). We go in the same order as in that
        # function. Most of the foreground colors are just inverses of the default. The background
        # colors default system background color with slightly adjusted brightness.

        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER,
                "back:#3F3F3F,face:%(mono)s,size:%(lnsize)d".format(faces))
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "face:%(mono)s".format(faces))

        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT, "fore:#FFFF00")
        self.StyleSetBackground(stc.STC_STYLE_BRACELIGHT,
                wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND).ChangeLightness(120))

        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD, "fore:#FF0000")
        self.StyleSetBackground(stc.STC_STYLE_BRACEBAD,
                wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND).ChangeLightness(120))

        # Python styles
        self.StyleSetSpec(stc.STC_P_DEFAULT, "face:%(mono)s".format(faces))
        self.StyleSetSpec(stc.STC_P_COMMENTLINE,
                "fore:#FF80FF,face:%(mono)s".format(faces))
        self.StyleSetSpec(stc.STC_P_NUMBER, "")
        self.StyleSetSpec(stc.STC_P_STRING, "fore:#80FF80,face:%(mono)s".format(faces))
        self.StyleSetSpec(stc.STC_P_CHARACTER,
                "fore:#80FF80,face:%(mono)s".format(faces))
        self.StyleSetSpec(stc.STC_P_WORD, "fore:#FFFF80,bold")
        self.StyleSetSpec(stc.STC_P_TRIPLE, "fore:#80FFFF")

        self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:#FFFFCC")
        self.StyleSetBackground(stc.STC_P_TRIPLEDOUBLE,
                wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND).ChangeLightness(105))

        self.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:#FFFF00,bold")
        self.StyleSetSpec(stc.STC_P_DEFNAME, "fore:#FF8080,bold")
        self.StyleSetSpec(stc.STC_P_OPERATOR, "")
        self.StyleSetSpec(stc.STC_P_IDENTIFIER, "")
        self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:#808080")
        self.StyleSetSpec(stc.STC_P_STRINGEOL,
                "fore:#FFFFFF,face:%(mono)s,back:#1F3F1F,eolfilled".format(faces))

        # Yellow calltip background is too contrasting.
        self.CallTipSetBackground(calltip_dark_bg_color)
        self.CallTipSetForeground(calltip_dark_fg_color)

        # Caret is black by default, override this.
        self.SetCaretForeground(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT))


# HACK: Monkey-patch EditWindow to support dark themes.
editwindow.EditWindow.__init__ = editwindow_new_init


calltip_old_init = crust.Calltip.__init__

def calltip_new_init(self, *args, **kwargs):
    calltip_old_init(self, *args, **kwargs)

    if is_using_dark_background():
        self.SetBackgroundColour(calltip_dark_bg_color)
        self.SetForegroundColour(calltip_dark_fg_color)

# HACK: Monkey-patch Calltip to support dark themes.
crust.Calltip.__init__ = calltip_new_init
