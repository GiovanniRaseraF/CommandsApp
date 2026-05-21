#ifndef MAIN_FRAME_H
#define MAIN_FRAME_H

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/scrolwin.h>
#include <vector>
#include <string>
#include "CommandManager.h"

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame() = default;

private:
    CommandManager m_commandManager;

    // GUI Controls
    wxTextCtrl* m_searchCtrl;
    wxScrolledWindow* m_scrollWin;
    wxBoxSizer* m_listSizer;
    wxButton* m_addButton;
    wxStaticText* m_statusText;

    // Timer for clearing the "Copied!" notification
    wxTimer m_statusTimer;
    static const int TIMER_ID = 10001;

    // UI Updating Helpers
    void PopulateList();
    void CopyToClipboardDirectly(const std::string& commandText);
    void EditCommandDirectly(size_t masterIndex);
    void DeleteCommandDirectly(size_t masterIndex);

    // Event Handlers
    void OnSearchChange(wxCommandEvent& event);
    void OnAdd(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif // MAIN_FRAME_H
