#include "CommandDialog.h"

wxBEGIN_EVENT_TABLE(CommandDialog, wxDialog)
    EVT_BUTTON(wxID_OK, CommandDialog::OnOK)
wxEND_EVENT_TABLE()

CommandDialog::CommandDialog(wxWindow* parent, const wxString& title, const wxString& initialValue)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(450, 250), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) 
{
    // Primary vertical layout manager
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Prompt Label
    wxStaticText* label = new wxStaticText(this, wxID_ANY, "Enter your command below:");
    label->SetFont(wxFont(wxFontInfo(11).Bold()));
    mainSizer->Add(label, 0, wxALL | wxALIGN_LEFT, 10);

    // Multi-line Text Box for the command string
    // wxTE_MULTILINE allows multiple lines, wxTE_DONTWRAP prevents automatic wrapping for command precision.
    // wxTE_PROCESS_ENTER captures the Return/Enter key so the dialog submits automatically.
    m_commandInput = new wxTextCtrl(this, wxID_ANY, initialValue, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_PROCESS_ENTER);
    m_commandInput->SetFont(wxFont(wxFontInfo(11).FaceName("Courier New"))); // Monospaced font for code styling
    
    // Bind Enter key press to OnOK to commit changes automatically
    m_commandInput->Bind(wxEVT_TEXT_ENTER, &CommandDialog::OnOK, this);

    mainSizer->Add(m_commandInput, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // Standard dialog buttons (OK and Cancel) in platform-specific order
    wxSizer* buttonSizer = CreateButtonSizer(wxOK | wxCANCEL);
    if (buttonSizer) {
        mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    }

    SetSizer(mainSizer);
    Layout();
    
    // Position dialog in the center of its parent window
    Centre(wxBOTH);

    // Direct keyboard focus to the input box
    m_commandInput->SetFocus();
}

void CommandDialog::OnOK(wxCommandEvent& event) {
    std::string text = GetCommandText();
    
    // Trim spaces
    size_t start = text.find_first_not_of(" \t\r\n");
    size_t end = text.find_last_not_of(" \t\r\n");
    bool isEmpty = (start == std::string::npos);

    if (isEmpty) {
        wxMessageBox("The command cannot be empty. Please enter a valid command.", 
                     "Invalid Input", 
                     wxOK | wxICON_WARNING, 
                     this);
        // Do not call EndModal, keeping the dialog open
    } else {
        EndModal(wxID_OK);
    }
}

std::string CommandDialog::GetCommandText() const {
    return std::string(m_commandInput->GetValue().utf8_str());
}
