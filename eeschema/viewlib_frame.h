#ifndef __LIBVIEWFRM_H__
#define __LIBVIEWFRM_H__


class WinEDAChoiceBox;
class SCH_SCREEN;
class CMP_LIBRARY;


/**
 * Component library viewer main window.
 */
class WinEDA_ViewlibFrame : public WinEDA_DrawFrame
{
private:
    WinEDAChoiceBox*    SelpartBox;

    // List of libraries (for selection )
    wxSashLayoutWindow* m_LibListWindow;
    wxListBox*          m_LibList;          // The list of libs
    wxSize              m_LibListSize;      // size of the window

    // List of components in the selected library
    wxSashLayoutWindow* m_CmpListWindow;
    wxListBox*          m_CmpList;          // The list of components
    wxSize              m_CmpListSize;      // size of the window

    // Flags
    wxSemaphore*        m_Semaphore;        // != NULL if the frame must emulate a modal dialog
    wxString            m_ConfigPath;       // subpath for configuration

protected:
    static wxString m_libraryName;
    static wxString m_entryName;
    static int      m_unit;
    static int      m_convert;
    static wxSize   m_clientSize;

public:
    WinEDA_ViewlibFrame( wxWindow*    father,
                         CMP_LIBRARY* Library = NULL,
                         wxSemaphore* semaphore = NULL );

    ~WinEDA_ViewlibFrame();

    void OnSize( wxSizeEvent& event );
    void OnSashDrag( wxSashEvent& event );
    void ReCreateListLib();
    void ReCreateListCmp();
    void Process_Special_Functions( wxCommandEvent& event );
    void DisplayLibInfos();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    int  BestZoom();
    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnCmpList( wxCommandEvent& event );
    void OnSetRelativeOffset( wxCommandEvent& event );

    SCH_SCREEN* GetScreen() { return (SCH_SCREEN*) GetBaseScreen(); }

    void GeneralControle( wxDC* DC, wxPoint MousePositionInPixels );

    void LoadSettings();
    void SaveSettings();

    wxString& GetEntryName( void ) const { return m_entryName; }

    int  GetUnit( void ) { return m_unit; }
    int  GetConvert( void ) { return m_convert; }

private:
    /** OnActivate event funtion( virtual )
     */
    virtual void     OnActivate( wxActivateEvent& event );

    void SelectCurrentLibrary();
    void SelectAndViewLibraryPart( int option );
    void ExportToSchematicLibraryPart( wxCommandEvent& event );
    void ViewOneLibraryContent( CMP_LIBRARY* Lib, int Flag );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );

    DECLARE_EVENT_TABLE()
};

#endif  /* __LIBVIEWFRM_H__ */
