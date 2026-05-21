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

    // 1. Create table if it doesn't exist
    const char* createTableSql = "CREATE TABLE IF NOT EXISTS commands ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "text TEXT NOT NULL);";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) {
            sqlite3_free(errMsg);
        }
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
