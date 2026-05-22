#ifndef GROUP_ADD_COMMANDS_DIALOG_H
#define GROUP_ADD_COMMANDS_DIALOG_H

#include <wx/wx.h>
#include <vector>
#include <string>
#include <cstdint>
#include "CommandManager.h"

class GroupAddCommandsDialog : public wxDialog {
public:
    GroupAddCommandsDialog(wxWindow* parent, const wxString& title, const std::vector<CommandItem>& commands);
    ~GroupAddCommandsDialog() = default;

    // Retrieve the indices of the selected commands
    std::vector<int64_t> GetSelectedCommandIds() const;

private:
    std::vector<CommandItem> m_commands;
    wxListBox* m_listBox;

    void OnOK(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif // GROUP_ADD_COMMANDS_DIALOG_H
