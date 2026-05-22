#include "MainFrame.h"
#include "CommandDialog.h"
#include "GroupAddCommandsDialog.h"
#include <wx/clipbrd.h>
#include <wx/settings.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_TEXT(wxID_ANY, MainFrame::OnSearchChange)
    EVT_TIMER(MainFrame::TIMER_ID, MainFrame::OnTimer)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 600)),
      m_statusTimer(this, TIMER_ID)
{
    // Load existing commands from SQLite
    m_commandManager.Load();

    // Background styling matching native system theme
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));

    // Master Sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create the Tab Control Wrapper
    m_notebook = new wxNotebook(this, wxID_ANY);
    m_allCommandsPanel = new wxPanel(m_notebook, wxID_ANY);
    m_groupsPanel = new wxPanel(m_notebook, wxID_ANY);

    // ----------------------------------------------------
    // TAB 1: All Commands View Layout
    // ----------------------------------------------------
    wxBoxSizer* allCmdsPageSizer = new wxBoxSizer(wxVERTICAL);
    m_allCommandsPanel->SetSizer(allCmdsPageSizer);

    // Search Box with placeholder hint
    m_searchCtrl = new wxTextCtrl(m_allCommandsPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_searchCtrl->SetHint("🔍 Search commands...");
    m_searchCtrl->SetFont(wxFont(wxFontInfo(12)));
    allCmdsPageSizer->Add(m_searchCtrl, 0, wxEXPAND | wxALL, 15);

    // Scrollable Window for Card Buttons
    m_scrollWin = new wxScrolledWindow(m_allCommandsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_scrollWin->SetScrollRate(0, 10); // Scroll vertically, 10px per tick
    
    // Vertical Sizer inside Scroll Window
    m_listSizer = new wxBoxSizer(wxVERTICAL);
    m_scrollWin->SetSizer(m_listSizer);
    allCmdsPageSizer->Add(m_scrollWin, 1, wxEXPAND | wxLEFT | wxRIGHT, 15);

    // Simplified Add Button Footer
    m_addButton = new wxButton(m_allCommandsPanel, wxID_ANY, "➕ Add New Command");
    m_addButton->SetFont(wxFont(wxFontInfo(12).Bold()));
    m_addButton->Bind(wxEVT_BUTTON, &MainFrame::OnAdd, this);
    allCmdsPageSizer->Add(m_addButton, 0, wxEXPAND | wxALL, 15);

    // ----------------------------------------------------
    // TAB 2: Command Groups View Layout
    // ----------------------------------------------------
    wxBoxSizer* groupsPageSizer = new wxBoxSizer(wxVERTICAL);
    m_groupsPanel->SetSizer(groupsPageSizer);

    // Splitter window to create Left (Groups List) and Right (Group Details) Columns
    wxSplitterWindow* splitter = new wxSplitterWindow(m_groupsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    
    wxPanel* leftPanel = new wxPanel(splitter, wxID_ANY);
    wxPanel* rightPanel = new wxPanel(splitter, wxID_ANY);

    // --- Left Panel Layout: Groups List ---
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    leftPanel->SetSizer(leftSizer);

    wxStaticText* groupsLabel = new wxStaticText(leftPanel, wxID_ANY, "📁 Groups");
    groupsLabel->SetFont(wxFont(wxFontInfo(11).Bold()));
    leftSizer->Add(groupsLabel, 0, wxALL | wxALIGN_LEFT, 10);

    m_groupsScrollWin = new wxScrolledWindow(leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_groupsScrollWin->SetScrollRate(0, 10);
    m_groupsListSizer = new wxBoxSizer(wxVERTICAL);
    m_groupsScrollWin->SetSizer(m_groupsListSizer);
    leftSizer->Add(m_groupsScrollWin, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    wxButton* createGroupBtn = new wxButton(leftPanel, wxID_ANY, "➕ Create Group");
    createGroupBtn->SetFont(wxFont(wxFontInfo(10).Bold()));
    createGroupBtn->Bind(wxEVT_BUTTON, &MainFrame::OnCreateGroup, this);
    leftSizer->Add(createGroupBtn, 0, wxEXPAND | wxALL, 10);

    // --- Right Panel Layout: Group Commands List ---
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    rightPanel->SetSizer(rightSizer);

    // Unselected State Placeholder Text
    m_noGroupSelectedText = new wxStaticText(rightPanel, wxID_ANY, "Select a group from the left panel\nto manage its commands.", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    m_noGroupSelectedText->SetFont(wxFont(wxFontInfo(12).Italic()));
    m_noGroupSelectedText->SetForegroundColour(wxColour(120, 120, 120));
    rightSizer->Add(m_noGroupSelectedText, 1, wxALIGN_CENTER | wxALL, 20);

    // Selected Group Header
    m_groupTitleLabel = new wxStaticText(rightPanel, wxID_ANY, "");
    m_groupTitleLabel->SetFont(wxFont(wxFontInfo(13).Bold()));
    rightSizer->Add(m_groupTitleLabel, 0, wxALL | wxALIGN_LEFT, 10);
    m_groupTitleLabel->Hide(); // Hidden initially

    // Selected Group Commands Scrolled List
    m_groupCommandsScrollWin = new wxScrolledWindow(rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_groupCommandsScrollWin->SetScrollRate(0, 10);
    m_groupCommandsListSizer = new wxBoxSizer(wxVERTICAL);
    m_groupCommandsScrollWin->SetSizer(m_groupCommandsListSizer);
    rightSizer->Add(m_groupCommandsScrollWin, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    m_groupCommandsScrollWin->Hide(); // Hidden initially

    // Selected Group "Add Commands to Group" Action button
    m_addCommandsToGroupBtn = new wxButton(rightPanel, wxID_ANY, "➕ Add Commands to Group");
    m_addCommandsToGroupBtn->SetFont(wxFont(wxFontInfo(11).Bold()));
    m_addCommandsToGroupBtn->Bind(wxEVT_BUTTON, &MainFrame::OnAddCommandsToGroup, this);
    rightSizer->Add(m_addCommandsToGroupBtn, 0, wxEXPAND | wxALL, 10);
    m_addCommandsToGroupBtn->Hide(); // Hidden initially

    // Split and add splitter to panel sizer
    splitter->SplitVertically(leftPanel, rightPanel, 200);
    splitter->SetMinimumPaneSize(140);
    groupsPageSizer->Add(splitter, 1, wxEXPAND);

    // ----------------------------------------------------
    // ASSEMBLY
    // ----------------------------------------------------
    m_notebook->AddPage(m_allCommandsPanel, "📋 All Commands");
    m_notebook->AddPage(m_groupsPanel, "📁 Command Groups");
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);

    // Premium Visual Status Label
    m_statusText = new wxStaticText(this, wxID_ANY, "💡 Click any command card to copy it instantly to your clipboard");
    m_statusText->SetFont(wxFont(wxFontInfo(10).Italic()));
    m_statusText->SetForegroundColour(wxColour(120, 120, 120));
    mainSizer->Add(m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    SetSizer(mainSizer);
    Layout();
    
    // Center app on screen
    Centre();

    // Bind page change to trigger panel list updates
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& event) {
        int sel = event.GetSelection();
        if (sel == 0) {
            this->PopulateList();
        } else if (sel == 1) {
            this->PopulateGroupsList();
            this->PopulateGroupCommandsList();
        }
    });

    // Populate initial views
    PopulateList();
    PopulateGroupsList();
    PopulateGroupCommandsList();
}

static bool FuzzyMatch(const std::string& query, const std::string& target) {
    if (query.empty()) return true;
    size_t queryIdx = 0;
    size_t targetIdx = 0;
    while (queryIdx < query.length() && targetIdx < target.length()) {
        if (std::tolower(query[queryIdx]) == std::tolower(target[targetIdx])) {
            queryIdx++;
        }
        targetIdx++;
    }
    return queryIdx == query.length();
}

void MainFrame::PopulateList() {
    m_scrollWin->Freeze(); // Prevent rendering flickering during list rebuild

    // Destroy all current child controls in the list sizer recursively
    m_listSizer->Clear(true);

    std::string searchQuery = std::string(m_searchCtrl->GetValue().utf8_str());
    const auto& allCommands = m_commandManager.GetCommands();

    // Load modern monospaced font
    wxFont monoFont(wxFontInfo(12).FaceName("Courier New"));
    if (!monoFont.IsOk()) {
        monoFont = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
    }

    for (size_t i = 0; i < allCommands.size(); ++i) {
        const auto& cmd = allCommands[i];

        // Search filtering matching using FuzzyMatch
        if (!FuzzyMatch(searchQuery, cmd.text)) {
            continue;
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

void MainFrame::PopulateGroupsList() {
    m_groupsScrollWin->Freeze();
    m_groupsListSizer->Clear(true);

    std::vector<GroupItem> groups = m_commandManager.GetGroups();

    for (const auto& group : groups) {
        wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);

        // Group Selection Button (displays folder emoji and name)
        wxString labelText = "📁 " + wxString::FromUTF8(group.name);
        wxButton* groupBtn = new wxButton(m_groupsScrollWin, wxID_ANY, labelText, wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
        groupBtn->SetToolTip("Select this group");

        // If this group is currently selected, highlight it visually
        if (group.id == m_selectedGroupId) {
            groupBtn->SetBackgroundColour(wxColour(0, 122, 255)); // Apple Blue Accent
            groupBtn->SetForegroundColour(*wxWHITE);
            groupBtn->SetFont(wxFont(wxFontInfo(10).Bold()));
        } else {
            groupBtn->SetFont(wxFont(wxFontInfo(10)));
        }

        // Delete group button
        wxButton* deleteBtn = new wxButton(m_groupsScrollWin, wxID_ANY, "❌", wxDefaultPosition, wxSize(30, 30));
        deleteBtn->SetToolTip("Delete group");

        // Bindings
        groupBtn->Bind(wxEVT_BUTTON, [this, group](wxCommandEvent& event) {
            this->m_selectedGroupId = group.id;
            this->m_selectedGroupName = group.name;
            this->PopulateGroupsList(); // Redraw to update selection highlighting
            this->PopulateGroupCommandsList();
        });

        deleteBtn->Bind(wxEVT_BUTTON, [this, group](wxCommandEvent& event) {
            this->DeleteGroupDirectly(group.id, group.name);
        });

        rowSizer->Add(groupBtn, 1, wxEXPAND | wxRIGHT, 4);
        rowSizer->Add(deleteBtn, 0, wxALIGN_CENTER_VERTICAL);

        m_groupsListSizer->Add(rowSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    }

    m_groupsListSizer->Layout();
    m_groupsScrollWin->FitInside();
    m_groupsScrollWin->Thaw();
}

void MainFrame::PopulateGroupCommandsList() {
    m_groupCommandsScrollWin->Freeze();
    m_groupCommandsListSizer->Clear(true);

    if (m_selectedGroupId == -1) {
        m_noGroupSelectedText->Show();
        m_groupTitleLabel->Hide();
        m_groupCommandsScrollWin->Hide();
        m_addCommandsToGroupBtn->Hide();
    } else {
        m_noGroupSelectedText->Hide();
        
        m_groupTitleLabel->SetLabel("📁 " + wxString::FromUTF8(m_selectedGroupName));
        m_groupTitleLabel->Show();
        m_groupCommandsScrollWin->Show();
        m_addCommandsToGroupBtn->Show();

        std::vector<CommandItem> commands = m_commandManager.GetCommandsInGroup(m_selectedGroupId);

        wxFont monoFont(wxFontInfo(11).FaceName("Courier New"));
        if (!monoFont.IsOk()) {
            monoFont = wxFont(wxFontInfo(11).Family(wxFONTFAMILY_TELETYPE));
        }

        for (const auto& cmd : commands) {
            wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);

            // Wide command card (copies on click)
            wxButton* cmdBtn = new wxButton(m_groupCommandsScrollWin, wxID_ANY, wxString::FromUTF8(cmd.text), wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
            cmdBtn->SetFont(monoFont);
            cmdBtn->SetToolTip("Click to copy to clipboard");

            // Remove from group button
            wxButton* removeBtn = new wxButton(m_groupCommandsScrollWin, wxID_ANY, "❌", wxDefaultPosition, wxSize(32, 32));
            removeBtn->SetToolTip("Remove from this group");

            // Bindings
            cmdBtn->Bind(wxEVT_BUTTON, [this, cmdText = cmd.text](wxCommandEvent& event) {
                this->CopyToClipboardDirectly(cmdText);
            });

            removeBtn->Bind(wxEVT_BUTTON, [this, cmd](wxCommandEvent& event) {
                this->RemoveCommandFromGroupDirectly(cmd.id, cmd.text);
            });

            rowSizer->Add(cmdBtn, 1, wxEXPAND | wxRIGHT, 6);
            rowSizer->Add(removeBtn, 0, wxALIGN_CENTER_VERTICAL);

            m_groupCommandsListSizer->Add(rowSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
        }
    }

    m_groupCommandsListSizer->Layout();
    m_groupCommandsScrollWin->FitInside();
    
    // Call layout on the right panel to ensure resizing is correct
    m_groupCommandsScrollWin->GetParent()->Layout();
    
    m_groupCommandsScrollWin->Thaw();
}

void MainFrame::OnCreateGroup(wxCommandEvent& event) {
    wxTextEntryDialog dialog(this, "Enter name for the new command group:", "Create Group");
    if (dialog.ShowModal() == wxID_OK) {
        wxString rawName = dialog.GetValue();
        std::string name = std::string(rawName.utf8_str());
        
        // Trim whitespace
        size_t first = name.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            wxMessageBox("Group name cannot be empty!", "Error", wxOK | wxICON_ERROR, this);
            return;
        }
        size_t last = name.find_last_not_of(" \t\r\n");
        std::string trimmedName = name.substr(first, last - first + 1);

        if (m_commandManager.AddGroup(trimmedName)) {
            PopulateGroupsList();
            m_statusText->SetLabel("📁 Group \"" + wxString::FromUTF8(trimmedName) + "\" created successfully!");
            m_statusText->SetForegroundColour(wxColour(52, 199, 89)); // Green
            m_statusTimer.StartOnce(2000);
        } else {
            wxMessageBox("Failed to create group. Group might already exist with this name.", "Error", wxOK | wxICON_ERROR, this);
        }
    }
}

void MainFrame::DeleteGroupDirectly(int64_t groupId, const std::string& groupName) {
    wxMessageDialog confirm(this,
                            "Are you sure you want to delete the group \"" + wxString::FromUTF8(groupName) + "\"?\n\nThe commands in this group will not be deleted from the database.",
                            "Confirm Group Deletion",
                            wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);

    if (confirm.ShowModal() == wxID_YES) {
        if (m_commandManager.DeleteGroup(groupId)) {
            if (groupId == m_selectedGroupId) {
                m_selectedGroupId = -1;
                m_selectedGroupName = "";
            }
            PopulateGroupsList();
            PopulateGroupCommandsList();

            m_statusText->SetLabel("❌ Group \"" + wxString::FromUTF8(groupName) + "\" deleted!");
            m_statusText->SetForegroundColour(wxColour(255, 59, 48)); // Alert Red
            m_statusTimer.StartOnce(2000);
        }
    }
}

void MainFrame::OnAddCommandsToGroup(wxCommandEvent& event) {
    if (m_selectedGroupId == -1) return;

    std::vector<CommandItem> eligibleCommands = m_commandManager.GetCommandsNotInGroup(m_selectedGroupId);

    if (eligibleCommands.empty()) {
        wxMessageBox("All existing commands are already in this group!", 
                     "Already Populated", 
                     wxOK | wxICON_INFORMATION, 
                     this);
        return;
    }

    GroupAddCommandsDialog dialog(this, "Add Commands to " + wxString::FromUTF8(m_selectedGroupName), eligibleCommands);
    if (dialog.ShowModal() == wxID_OK) {
        std::vector<int64_t> selectedIds = dialog.GetSelectedCommandIds();
        int addedCount = 0;
        for (int64_t cmdId : selectedIds) {
            if (m_commandManager.AddCommandToGroup(m_selectedGroupId, cmdId)) {
                addedCount++;
            }
        }

        PopulateGroupCommandsList();
        
        m_statusText->SetLabel("➕ Added " + wxString::Format("%d", addedCount) + " command(s) to group!");
        m_statusText->SetForegroundColour(wxColour(52, 199, 89)); // Green
        m_statusTimer.StartOnce(2000);
    }
}

void MainFrame::RemoveCommandFromGroupDirectly(int64_t commandId, const std::string& commandText) {
    wxString shortCmd = wxString::FromUTF8(commandText);
    if (shortCmd.Length() > 30) {
        shortCmd = shortCmd.Left(27) + "...";
    }

    wxMessageDialog confirm(this,
                            "Are you sure you want to remove \"" + shortCmd + "\" from this group?\n\nThe command will not be deleted from your main list.",
                            "Confirm Removal",
                            wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);

    if (confirm.ShowModal() == wxID_YES) {
        if (m_commandManager.RemoveCommandFromGroup(m_selectedGroupId, commandId)) {
            PopulateGroupCommandsList();

            m_statusText->SetLabel("❌ Command removed from group!");
            m_statusText->SetForegroundColour(wxColour(255, 59, 48)); // Alert Red
            m_statusTimer.StartOnce(2000);
        }
    }
}
