#include "CommandManager.h"
#include <wx/wx.h>
#include <wx/filefn.h>
#include <sqlite3.h>

CommandManager::CommandManager() {
    // Resolve home directory SQLite database using wxWidgets cross-platform path helper
    wxString filepath = wxGetHomeDir() + wxFILE_SEP_PATH + ".commands_app.db";
    m_filePath = std::string(filepath.utf8_str());
}

bool CommandManager::Load() {
    m_commands.clear();
    
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    // Enable foreign keys cascading support
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    // 1. Create tables if they don't exist
    const char* createCommandsTableSql = "CREATE TABLE IF NOT EXISTS commands ("
                                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                         "text TEXT NOT NULL);";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createCommandsTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    const char* createGroupsTableSql = "CREATE TABLE IF NOT EXISTS groups ("
                                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                       "name TEXT NOT NULL UNIQUE);";
    if (sqlite3_exec(db, createGroupsTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    const char* createGroupCommandsTableSql = "CREATE TABLE IF NOT EXISTS group_commands ("
                                              "group_id INTEGER, "
                                              "command_id INTEGER, "
                                              "PRIMARY KEY (group_id, command_id), "
                                              "FOREIGN KEY (group_id) REFERENCES groups(id) ON DELETE CASCADE, "
                                              "FOREIGN KEY (command_id) REFERENCES commands(id) ON DELETE CASCADE);";
    if (sqlite3_exec(db, createGroupCommandsTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    // 2. Query count to check if empty
    bool dbIsEmpty = true;
    sqlite3_stmt* countStmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM commands;", -1, &countStmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(countStmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(countStmt, 0);
            if (count > 0) {
                dbIsEmpty = false;
            }
        }
        sqlite3_finalize(countStmt);
    }

    // 3. Populate defaults if database is empty
    if (dbIsEmpty) {
        PopulateDefaults();
        // Insert defaults transactionally
        const char* insertSql = "INSERT INTO commands (text) VALUES (?);";
        sqlite3_stmt* insertStmt = nullptr;
        if (sqlite3_prepare_v2(db, insertSql, -1, &insertStmt, nullptr) == SQLITE_OK) {
            for (const auto& cmd : m_commands) {
                sqlite3_bind_text(insertStmt, 1, cmd.text.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(insertStmt);
                sqlite3_reset(insertStmt);
            }
            sqlite3_finalize(insertStmt);
        }
    }

    // 4. Query all commands
    m_commands.clear();
    const char* selectSql = "SELECT id, text FROM commands ORDER BY id ASC;";
    sqlite3_stmt* selectStmt = nullptr;
    if (sqlite3_prepare_v2(db, selectSql, -1, &selectStmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(selectStmt) == SQLITE_ROW) {
            int64_t id = sqlite3_column_int64(selectStmt, 0);
            const unsigned char* textPtr = sqlite3_column_text(selectStmt, 1);
            std::string text = textPtr ? reinterpret_cast<const char*>(textPtr) : "";
            m_commands.push_back({id, text});
        }
        sqlite3_finalize(selectStmt);
    }

    sqlite3_close(db);
    return true;
}

void CommandManager::AddCommand(const std::string& commandText) {
    if (commandText.empty()) return;

    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return;
    }

    const char* insertSql = "INSERT INTO commands (text) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, commandText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    
    // Reload internal collection
    Load();
}

bool CommandManager::EditCommand(size_t index, const std::string& newCommandText) {
    if (index >= m_commands.size() || newCommandText.empty()) return false;
    int64_t dbId = m_commands[index].id;

    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    const char* updateSql = "UPDATE commands SET text = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, newCommandText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, dbId);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    
    // Reload internal collection
    Load();
    return success;
}

bool CommandManager::DeleteCommand(size_t index) {
    if (index >= m_commands.size()) return false;
    int64_t dbId = m_commands[index].id;

    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    // Enable foreign keys to trigger cascade deletes in group_commands
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    const char* deleteSql = "DELETE FROM commands WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, deleteSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, dbId);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    
    // Reload internal collection
    Load();
    return success;
}

const std::vector<CommandItem>& CommandManager::GetCommands() const {
    return m_commands;
}

std::string CommandManager::GetFilePath() const {
    return m_filePath;
}

void CommandManager::PopulateDefaults() {
    m_commands = { };
}

std::vector<GroupItem> CommandManager::GetGroups() const {
    std::vector<GroupItem> groups;
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return groups;
    }

    const char* selectSql = "SELECT id, name FROM groups ORDER BY name ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t id = sqlite3_column_int64(stmt, 0);
            const unsigned char* namePtr = sqlite3_column_text(stmt, 1);
            std::string name = namePtr ? reinterpret_cast<const char*>(namePtr) : "";
            groups.push_back({id, name});
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return groups;
}

bool CommandManager::AddGroup(const std::string& name) {
    if (name.empty()) return false;
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    const char* insertSql = "INSERT OR IGNORE INTO groups (name) VALUES (?);";
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        // INSERT OR IGNORE won't fail if duplicate, but check if row actually inserted
        if (sqlite3_changes(db) > 0) {
            success = true;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return success;
}

bool CommandManager::DeleteGroup(int64_t groupId) {
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    const char* deleteSql = "DELETE FROM groups WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, deleteSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, groupId);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return success;
}

std::vector<CommandItem> CommandManager::GetCommandsInGroup(int64_t groupId) const {
    std::vector<CommandItem> commands;
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return commands;
    }

    const char* selectSql = "SELECT c.id, c.text FROM commands c "
                            "JOIN group_commands gc ON c.id = gc.command_id "
                            "WHERE gc.group_id = ? ORDER BY c.id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, groupId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t id = sqlite3_column_int64(stmt, 0);
            const unsigned char* textPtr = sqlite3_column_text(stmt, 1);
            std::string text = textPtr ? reinterpret_cast<const char*>(textPtr) : "";
            commands.push_back({id, text});
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return commands;
}

std::vector<CommandItem> CommandManager::GetCommandsNotInGroup(int64_t groupId) const {
    std::vector<CommandItem> commands;
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return commands;
    }

    const char* selectSql = "SELECT c.id, c.text FROM commands c "
                            "WHERE c.id NOT IN (SELECT command_id FROM group_commands WHERE group_id = ?) "
                            "ORDER BY c.id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, groupId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int64_t id = sqlite3_column_int64(stmt, 0);
            const unsigned char* textPtr = sqlite3_column_text(stmt, 1);
            std::string text = textPtr ? reinterpret_cast<const char*>(textPtr) : "";
            commands.push_back({id, text});
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return commands;
}

bool CommandManager::AddCommandToGroup(int64_t groupId, int64_t commandId) {
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    const char* insertSql = "INSERT OR IGNORE INTO group_commands (group_id, command_id) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, groupId);
        sqlite3_bind_int64(stmt, 2, commandId);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return success;
}

bool CommandManager::RemoveCommandFromGroup(int64_t groupId, int64_t commandId) {
    sqlite3* db = nullptr;
    if (sqlite3_open(m_filePath.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        return false;
    }

    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    const char* deleteSql = "DELETE FROM group_commands WHERE group_id = ? AND command_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool success = false;
    if (sqlite3_prepare_v2(db, deleteSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, groupId);
        sqlite3_bind_int64(stmt, 2, commandId);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            success = true;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return success;
}
