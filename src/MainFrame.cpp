#include "MainFrame.h"
#include "CommandDialog.h"
#include <wx/clipbrd.h>
#include <wx/settings.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_TEXT(wxID_ANY, MainFrame::OnSearchChange)
    EVT_TIMER(MainFrame::TIMER_ID, MainFrame::OnTimer)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(550, 550)),
      m_statusTimer(this, TIMER_ID)
{
    // Load existing commands from SQLite
    m_commandManager.Load();

    // Background styling matching native system theme
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));

    // Master Sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // 1. Search Box with placeholder hint
    m_searchCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_searchCtrl->SetHint("🔍 Search commands...");
    m_searchCtrl->SetFont(wxFont(wxFontInfo(12)));
    mainSizer->Add(m_searchCtrl, 0, wxEXPAND | wxALL, 15);

    // 2. Scrollable Window for Card Buttons
    m_scrollWin = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_scrollWin->SetScrollRate(0, 10); // Scroll vertically, 10px per tick
    
    // Vertical Sizer inside Scroll Window
    m_listSizer = new wxBoxSizer(wxVERTICAL);
    m_scrollWin->SetSizer(m_listSizer);
    
    mainSizer->Add(m_scrollWin, 1, wxEXPAND | wxLEFT | wxRIGHT, 15);

    // 3. Simplified Add Button Footer
    m_addButton = new wxButton(this, wxID_ANY, "➕ Add New Command");
    m_addButton->SetFont(wxFont(wxFontInfo(12).Bold()));
    m_addButton->Bind(wxEVT_BUTTON, &MainFrame::OnAdd, this);
    mainSizer->Add(m_addButton, 0, wxEXPAND | wxALL, 15);

    // 4. Premium Visual Status Label
    m_statusText = new wxStaticText(this, wxID_ANY, "💡 Click any command card to copy it instantly to your clipboard");
    m_statusText->SetFont(wxFont(wxFontInfo(10).Italic()));
    m_statusText->SetForegroundColour(wxColour(120, 120, 120));
    mainSizer->Add(m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    SetSizer(mainSizer);
    Layout();
    
    // Center app on screen
    Centre();

    // Populate scroll list
    PopulateList();
}

void MainFrame::PopulateList() {
    m_scrollWin->Freeze(); // Prevent rendering flickering during list rebuild

    // Destroy all current child controls in the list sizer recursively
    m_listSizer->Clear(true);

    std::string searchQuery = std::string(m_searchCtrl->GetValue().Lower().utf8_str());
    const auto& allCommands = m_commandManager.GetCommands();

    // Load modern monospaced font
    wxFont monoFont(wxFontInfo(12).FaceName("Courier New"));
    if (!monoFont.IsOk()) {
        monoFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
    }

    for (size_t i = 0; i < allCommands.size(); ++i) {
        const auto& cmd = allCommands[i];

        // Search filtering matching
        if (!searchQuery.empty()) {
            std::string cmdLower = cmd.text;
            std::transform(cmdLower.begin(), cmdLower.end(), cmdLower.begin(), ::tolower);
            if (cmdLower.find(searchQuery) == std::string::npos) {
                continue;
            }
        }

        // Horizontal Row sizer for this card
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);

        // A. Wide Command Button (copies on click)
        // wxBU_LEFT aligns command text cleanly to the left of the card
        wxButton* cmdBtn = new wxButton(m_scrollWin, wxID_ANY, wxString::FromUTF8(cmd.text), wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
        cmdBtn->SetFont(monoFont);
        cmdBtn->SetToolTip("Click to copy to clipboard");

        // B. Inline Edit Button (✏️)
        wxButton* editBtn = new wxButton(m_scrollWin, wxID_ANY, "✏️", wxDefaultPosition, wxSize(36, 36));
        editBtn->SetToolTip("Edit this command");

        // C. Inline Delete Button (❌)
        wxButton* deleteBtn = new wxButton(m_scrollWin, wxID_ANY, "❌", wxDefaultPosition, wxSize(36, 36));
        deleteBtn->SetToolTip("Delete this command");

        // C++11 Lambda bindings to handle triggers directly per card
        cmdBtn->Bind(wxEVT_BUTTON, [this, cmdText = cmd.text](wxCommandEvent& event) {
            this->CopyToClipboardDirectly(cmdText);
        });

        editBtn->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& event) {
            this->EditCommandDirectly(i);
        });

        deleteBtn->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& event) {
            this->DeleteCommandDirectly(i);
        });

        // Add to card row sizer
        rowSizer->Add(cmdBtn, 1, wxEXPAND | wxRIGHT, 8);
        rowSizer->Add(editBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        rowSizer->Add(deleteBtn, 0, wxALIGN_CENTER_VERTICAL);

        // Add row sizer to scroll window sizer with margins for neat layout separation
        m_listSizer->Add(rowSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    }

    // Refresh and recalculate scrollbars virtual size
    m_listSizer->Layout();
    m_scrollWin->FitInside();
    m_scrollWin->Thaw();
}

void MainFrame::OnSearchChange(wxCommandEvent& event) {
    PopulateList();
}

void MainFrame::CopyToClipboardDirectly(const std::string& commandText) {
    if (wxTheClipboard->Open()) {
        wxTheClipboard->Clear();
        wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(commandText)));
        wxTheClipboard->Flush(); // Linux persistence
        wxTheClipboard->Close();

        wxString shortCmd = wxString::FromUTF8(commandText);
        if (shortCmd.Length() > 40) {
            shortCmd = shortCmd.Left(37) + "...";
        }
        m_statusText->SetLabel("📋 Copied to clipboard: \"" + shortCmd + "\"");
        m_statusText->SetForegroundColour(wxColour(0, 122, 255)); // Apple Blue Accent
        m_statusTimer.StartOnce(2000);
    } else {
        m_statusText->SetLabel("❌ Error: Failed to open clipboard!");
        m_statusText->SetForegroundColour(wxColour(255, 59, 48)); // Alert Red
    }
}

void MainFrame::EditCommandDirectly(size_t masterIndex) {
    const auto& allCommands = m_commandManager.GetCommands();
    if (masterIndex >= allCommands.size()) return;

    std::string currentCmd = allCommands[masterIndex].text;

    CommandDialog dialog(this, "Edit Command", wxString::FromUTF8(currentCmd));
    if (dialog.ShowModal() == wxID_OK) {
        std::string editedCmd = dialog.GetCommandText();
        m_commandManager.EditCommand(masterIndex, editedCmd);

        // Refresh dynamic button list
        PopulateList();

        m_statusText->SetLabel("✏️ Command edited successfully!");
        m_statusText->SetForegroundColour(wxColour(52, 199, 89)); // Premium Green
        m_statusTimer.StartOnce(2000);
    }
}

void MainFrame::DeleteCommandDirectly(size_t masterIndex) {
    const auto& allCommands = m_commandManager.GetCommands();
    if (masterIndex >= allCommands.size()) return;

    std::string cmdText = allCommands[masterIndex].text;

    wxString shortCmd = wxString::FromUTF8(cmdText);
    if (shortCmd.Length() > 40) {
        shortCmd = shortCmd.Left(37) + "...";
    }

    wxMessageDialog confirm(this, 
                            "Are you sure you want to delete this command?\n\n\"" + shortCmd + "\"", 
                            "Confirm Deletion", 
                            wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION);

    if (confirm.ShowModal() == wxID_YES) {
        m_commandManager.DeleteCommand(masterIndex);
        
        // Refresh dynamic button list
        PopulateList();

        m_statusText->SetLabel("❌ Command deleted!");
        m_statusText->SetForegroundColour(wxColour(255, 59, 48)); // Alert Red
        m_statusTimer.StartOnce(2000);
    }
}

void MainFrame::OnAdd(wxCommandEvent& event) {
    CommandDialog dialog(this, "Add New Command");
    if (dialog.ShowModal() == wxID_OK) {
        std::string newCmd = dialog.GetCommandText();
        m_commandManager.AddCommand(newCmd);
        
        // Refresh dynamic button list
        PopulateList();
        
        m_statusText->SetLabel("➕ Command added successfully!");
        m_statusText->SetForegroundColour(wxColour(52, 199, 89)); // Premium Green
        m_statusTimer.StartOnce(2000);
    }
}

void MainFrame::OnTimer(wxTimerEvent& event) {
    m_statusText->SetLabel("💡 Click any command card to copy it instantly to your clipboard");
    m_statusText->SetForegroundColour(wxColour(120, 120, 120));
}
