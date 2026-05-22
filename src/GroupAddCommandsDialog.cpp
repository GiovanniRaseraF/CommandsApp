#include "GroupAddCommandsDialog.h"

wxBEGIN_EVENT_TABLE(GroupAddCommandsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, GroupAddCommandsDialog::OnOK)
wxEND_EVENT_TABLE()

GroupAddCommandsDialog::GroupAddCommandsDialog(wxWindow* parent, const wxString& title, const std::vector<CommandItem>& commands)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(450, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_commands(commands)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Label instruction
    wxStaticText* label = new wxStaticText(this, wxID_ANY, "Select one or more commands to add:");
    label->SetFont(wxFont(wxFontInfo(11).Bold()));
    mainSizer->Add(label, 0, wxALL | wxALIGN_LEFT, 12);

    // ListBox with multiple selection style
    wxArrayString items;
    for (const auto& cmd : m_commands) {
        items.Add(wxString::FromUTF8(cmd.text));
    }

    m_listBox = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items, wxLB_MULTIPLE | wxLB_NEEDED_SB);
    
    // Set a premium monospaced font
    wxFont monoFont(wxFontInfo(10).FaceName("Courier New"));
    if (!monoFont.IsOk()) {
        monoFont = wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE));
    }
    m_listBox->SetFont(monoFont);
    
    mainSizer->Add(m_listBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // OK / Cancel Buttons
    wxSizer* buttonSizer = CreateButtonSizer(wxOK | wxCANCEL);
    if (buttonSizer) {
        mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 12);
    }

    SetSizer(mainSizer);
    Layout();
    Centre(wxBOTH);
}

void GroupAddCommandsDialog::OnOK(wxCommandEvent& event) {
    wxArrayInt selections;
    int count = m_listBox->GetSelections(selections);
    if (count == 0) {
        wxMessageBox("Please select at least one command, or click Cancel.", 
                     "No Selection", 
                     wxOK | wxICON_INFORMATION, 
                     this);
        return; // Don't close
    }
    EndModal(wxID_OK);
}

std::vector<int64_t> GroupAddCommandsDialog::GetSelectedCommandIds() const {
    std::vector<int64_t> ids;
    wxArrayInt selections;
    int count = m_listBox->GetSelections(selections);
    for (int i = 0; i < count; ++i) {
        int index = selections[i];
        if (index >= 0 && static_cast<size_t>(index) < m_commands.size()) {
            ids.push_back(m_commands[index].id);
        }
    }
    return ids;
}
