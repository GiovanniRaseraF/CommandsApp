#ifndef COMMAND_DIALOG_H
#define COMMAND_DIALOG_H

#include <wx/wx.h>
#include <string>

class CommandDialog : public wxDialog {
public:
    CommandDialog(wxWindow* parent, const wxString& title, const wxString& initialValue = "");
    ~CommandDialog() = default;

    // Retrieve the final command text entered by the user
    std::string GetCommandText() const;

private:
    wxTextCtrl* m_commandInput;

    // Event handler for validation before closing the dialog
    void OnOK(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

#endif // COMMAND_DIALOG_H
