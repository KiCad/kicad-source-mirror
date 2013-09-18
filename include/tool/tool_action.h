#ifndef __TOOL_ACTION_H
#define __TOOL_ACTION_H

#include <string>
#include <wx/wx.h>

#include <tool/tool_base.h>

///> Scope of tool actions
enum TOOL_ActionScope {
	SCOPE_CONTEXT = 1,  ///> Action belongs to a particular tool (i.e. a part of a pop-up menu)
	SCOPE_GLOBAL		///> Global action (toolbar/main menu event, global shortcut)
};

// TOOL_ACTION - represents a single action. For instance:
// - changing layer to top by pressing PgUp
// - running the DRC from the menu
// and so on, and so forth....
class TOOL_ACTION 
{
	public:
	

		TOOL_ACTION
		(
			const std::string& name,
			TOOL_ActionScope scope = SCOPE_GLOBAL,
			int aDefaultHotKey = 0,
			const wxString& menuItem = wxT(""), 
			const wxString& menuDesc = wxT("")
		) :
			m_name(name),
			m_scope(scope),
			m_defaultHotKey(aDefaultHotKey),
			m_currentHotKey(aDefaultHotKey),
			m_menuItem(menuItem),
			m_menuDescription(menuDesc) {}

		bool operator == ( const TOOL_ACTION& rhs ) const
		{
			return m_id	== rhs.m_id;
		}

		bool operator != ( const TOOL_ACTION& rhs ) const
		{
			return m_id	!= rhs.m_id;
		}

		bool hasHotKey() const
		{
			return m_currentHotKey > 0;
		}

	private:
		friend class TOOL_MANAGER;

		void setId ( int aId )
		{
			m_id = aId;
		}


		// name of the action (convention is: app.[tool.]action.name)
		std::string m_name;
		TOOL_ActionScope m_scope;
		int m_defaultHotKey;
		int m_currentHotKey;
		
		// Menu item text
		wxString m_menuItem;
		// Pop-up help
		wxString m_menuDescription;
		
		//KiBitmap m_bitmap;

		// Unique ID for fast matching. Assigned by TOOL_MANAGER
		int m_id;

		// Origin of the action
		TOOL_BASE *m_origin;

		// Originating UI object
		wxWindow *m_uiOrigin;
};

#endif
