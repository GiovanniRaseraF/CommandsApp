#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <string>
#include <vector>
#include <cstdint>

struct CommandItem {
    int64_t id;
    std::string text;
};

struct GroupItem {
    int64_t id;
    std::string name;
};

class CommandManager {
public:
    CommandManager();
    ~CommandManager() = default;

    // Load commands from SQLite. Creates table and populates defaults if empty.
    bool Load();

    // CRUD operations for Commands
    void AddCommand(const std::string& commandText);
    bool EditCommand(size_t index, const std::string& newCommandText);
    bool DeleteCommand(size_t index);

    // Group Management Operations
    std::vector<GroupItem> GetGroups() const;
    bool AddGroup(const std::string& name);
    bool DeleteGroup(int64_t groupId);
    std::vector<CommandItem> GetCommandsInGroup(int64_t groupId) const;
    std::vector<CommandItem> GetCommandsNotInGroup(int64_t groupId) const;
    bool AddCommandToGroup(int64_t groupId, int64_t commandId);
    bool RemoveCommandFromGroup(int64_t groupId, int64_t commandId);

    // Getters
    const std::vector<CommandItem>& GetCommands() const;
    std::string GetFilePath() const;

private:
    std::vector<CommandItem> m_commands;
    std::string m_filePath;

    // Helper to initialize default commands in database if empty
    void PopulateDefaults();
};

#endif // COMMAND_MANAGER_H
