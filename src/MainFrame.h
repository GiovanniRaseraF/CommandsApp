#ifndef MAIN_FRAME_H
#define MAIN_FRAME_H

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/scrolwin.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <vector>
#include <string>
#include "CommandManager.h"

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame() = default;

private:
    CommandManager m_commandManager;

    // --- Tab Wrapper ---
    wxNotebook* m_notebook;
    wxPanel* m_allCommandsPanel;
    wxPanel* m_groupsPanel;

    // --- Tab 1: All Commands View Controls ---
    wxTextCtrl* m_searchCtrl;
    wxScrolledWindow* m_scrollWin;
    wxBoxSizer* m_listSizer;
    wxButton* m_addButton;

    // --- Tab 2: Command Groups View Controls ---
    wxScrolledWindow* m_groupsScrollWin;
    wxBoxSizer* m_groupsListSizer;
    wxScrolledWindow* m_groupCommandsScrollWin;
    wxBoxSizer* m_groupCommandsListSizer;
    wxStaticText* m_groupTitleLabel;
    wxButton* m_addCommandsToGroupBtn;
    wxStaticText* m_noGroupSelectedText;

    // --- Common Controls ---
    wxStaticText* m_statusText;

    // Timer for clearing the "Copied!" notification
    wxTimer m_statusTimer;
    static const int TIMER_ID = 10001;

    // --- State ---
    int64_t m_selectedGroupId = -1;
    std::string m_selectedGroupName = "";

    // --- UI Updating Helpers ---
    void PopulateList();
    void PopulateGroupsList();
    void PopulateGroupCommandsList();
    void CopyToClipboardDirectly(const std::string& commandText);
    void EditCommandDirectly(size_t masterIndex);
    void DeleteCommandDirectly(size_t masterIndex);
    void DeleteGroupDirectly(int64_t groupId, const std::string& groupName);
    void RemoveCommandFromGroupDirectly(int64_t commandId, const std::string& commandText);

    // --- Event Handlers ---
    void OnSearchChange(wxCommandEvent& event);
    void OnAdd(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnCreateGroup(wxCommandEvent& event);
    void OnAddCommandsToGroup(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif // MAIN_FRAME_H
