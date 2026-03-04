#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <ctime>
#include <cstdio>

struct TodoItem {
    int id;
    std::string title;
    std::string status;
    std::string createdDate;
};

std::string getCurrentDate() {
    time_t now = time(nullptr);
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &now);
#else
    localtime_r(&now, &timeInfo);
#endif
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &timeInfo);
    return std::string(buffer);
}

std::string escapeCSV(const std::string& field) {
    if (field.find(',') != std::string::npos ||
        field.find('"') != std::string::npos ||
        field.find('\n') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') escaped += "\"\"";
            else escaped += c;
        }
        escaped += "\"";
        return escaped;
    }
    return field;
}

std::string unescapeCSV(const std::string& field) {
    if (field.size() >= 2 && field.front() == '"' && field.back() == '"') {
        std::string unescaped;
        for (size_t i = 1; i < field.size() - 1; i++) {
            if (field[i] == '"' && i + 1 < field.size() - 1 && field[i + 1] == '"') {
                unescaped += '"';
                i++;
            } else {
                unescaped += field[i];
            }
        }
        return unescaped;
    }
    return field;
}

std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    i++;
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
    }
    fields.push_back(current);
    return fields;
}

const std::string FILENAME = "todos.csv";

std::vector<TodoItem> loadTodos() {
    std::vector<TodoItem> items;
    std::ifstream file(FILENAME);
    if (!file.is_open()) return items;

    std::string line;
    if (std::getline(file, line)) {}

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> fields = parseCSVLine(line);
        if (fields.size() >= 4) {
            TodoItem item;
            item.id = std::stoi(fields[0]);
            item.title = fields[1];
            item.status = fields[2];
            item.createdDate = fields[3];
            items.push_back(item);
        }
    }
    file.close();
    return items;
}

void saveTodos(const std::vector<TodoItem>& items) {
    std::ofstream file(FILENAME);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << FILENAME << " for writing.\n";
        return;
    }
    file << "ID,Title,Status,Created\n";
    for (const auto& item : items) {
        file << item.id << ","
             << escapeCSV(item.title) << ","
             << escapeCSV(item.status) << ","
             << escapeCSV(item.createdDate) << "\n";
    }
    file.close();
}

int getNextId(const std::vector<TodoItem>& items) {
    int maxId = 0;
    for (const auto& item : items) {
        if (item.id > maxId) maxId = item.id;
    }
    return maxId + 1;
}

void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printSeparator() {
    std::cout << "----------------------------------------\n";
}

void createItem(std::vector<TodoItem>& items) {
    std::cout << "\n";
    printSeparator();
    std::cout << "  CREATE NEW TO-DO ITEM\n";
    printSeparator();
    std::cout << "Enter title (or 'cancel' to go back): ";
    std::string title;
    std::getline(std::cin, title);

    title.erase(std::remove(title.begin(), title.end(), '\n'), title.end());
    title.erase(std::remove(title.begin(), title.end(), '\r'), title.end());

    if (title.empty()) {
        std::cout << "Title cannot be empty.\n";
        return;
    }
    if (title == "cancel") {
        std::cout << "Cancelled.\n";
        return;
    }

    TodoItem item;
    item.id = getNextId(items);
    item.title = title;
    item.status = "Pending";
    item.createdDate = getCurrentDate();
    items.push_back(item);
    saveTodos(items);

    std::cout << "Item #" << item.id << " created successfully.\n";
}

void listItems(const std::vector<TodoItem>& items) {
    std::cout << "\n";
    printSeparator();
    std::cout << "  ALL TO-DO ITEMS\n";
    printSeparator();

    if (items.empty()) {
        std::cout << "No items found. Create one first!\n";
        return;
    }

    std::cout << std::string(70, '-') << "\n";
    printf("%-5s %-30s %-12s %-18s\n", "ID", "Title", "Status", "Created");
    std::cout << std::string(70, '-') << "\n";

    for (const auto& item : items) {
        std::string displayTitle = item.title;
        if (displayTitle.size() > 28) displayTitle = displayTitle.substr(0, 25) + "...";

        printf("%-5d %-30s %-12s %-18s\n",
               item.id,
               displayTitle.c_str(),
               item.status.c_str(),
               item.createdDate.c_str());
    }
    std::cout << std::string(70, '-') << "\n";
    std::cout << "Total: " << items.size() << " item(s)\n";
}

void editItem(std::vector<TodoItem>& items) {
    std::cout << "\n";
    printSeparator();
    std::cout << "  EDIT TO-DO ITEM\n";
    printSeparator();

    if (items.empty()) {
        std::cout << "No items to edit.\n";
        return;
    }

    listItems(items);

    std::cout << "\nEnter the ID of the item to edit (or 0 to cancel): ";
    int id;
    if (!(std::cin >> id)) {
        clearInput();
        std::cout << "Invalid input.\n";
        return;
    }
    clearInput();

    if (id == 0) {
        std::cout << "Cancelled.\n";
        return;
    }

    auto it = std::find_if(items.begin(), items.end(),
        [id](const TodoItem& item) { return item.id == id; });

    if (it == items.end()) {
        std::cout << "Item #" << id << " not found.\n";
        return;
    }

    std::cout << "Current title: " << it->title << "\n";
    std::cout << "Enter new title (or press Enter to keep current): ";
    std::string newTitle;
    std::getline(std::cin, newTitle);

    newTitle.erase(std::remove(newTitle.begin(), newTitle.end(), '\n'), newTitle.end());
    newTitle.erase(std::remove(newTitle.begin(), newTitle.end(), '\r'), newTitle.end());

    if (!newTitle.empty()) {
        it->title = newTitle;
        saveTodos(items);
        std::cout << "Item #" << id << " updated successfully.\n";
    } else {
        std::cout << "No changes made.\n";
    }
}

void deleteItem(std::vector<TodoItem>& items) {
    std::cout << "\n";
    printSeparator();
    std::cout << "  DELETE TO-DO ITEM\n";
    printSeparator();

    if (items.empty()) {
        std::cout << "No items to delete.\n";
        return;
    }

    listItems(items);

    std::cout << "\nEnter the ID of the item to delete (or 0 to cancel): ";
    int id;
    if (!(std::cin >> id)) {
        clearInput();
        std::cout << "Invalid input.\n";
        return;
    }
    clearInput();

    if (id == 0) {
        std::cout << "Cancelled.\n";
        return;
    }

    auto it = std::find_if(items.begin(), items.end(),
        [id](const TodoItem& item) { return item.id == id; });

    if (it == items.end()) {
        std::cout << "Item #" << id << " not found.\n";
        return;
    }

    std::cout << "Are you sure you want to delete \"" << it->title << "\"? (y/n): ";
    char confirm;
    std::cin >> confirm;
    clearInput();

    if (confirm == 'y' || confirm == 'Y') {
        items.erase(it);
        saveTodos(items);
        std::cout << "Item #" << id << " deleted.\n";
    } else {
        std::cout << "Cancelled.\n";
    }
}

void markComplete(std::vector<TodoItem>& items) {
    std::cout << "\n";
    printSeparator();
    std::cout << "  MARK ITEM COMPLETE\n";
    printSeparator();

    std::vector<TodoItem*> pending;
    for (auto& item : items) {
        if (item.status == "Pending") {
            pending.push_back(&item);
        }
    }

    if (pending.empty()) {
        std::cout << "No pending items to mark complete.\n";
        return;
    }

    std::cout << "\nPending items:\n";
    std::cout << std::string(50, '-') << "\n";
    printf("%-5s %-30s %-12s\n", "ID", "Title", "Status");
    std::cout << std::string(50, '-') << "\n";
    for (const auto* item : pending) {
        std::string displayTitle = item->title;
        if (displayTitle.size() > 28) displayTitle = displayTitle.substr(0, 25) + "...";
        printf("%-5d %-30s %-12s\n", item->id, displayTitle.c_str(), item->status.c_str());
    }
    std::cout << std::string(50, '-') << "\n";

    std::cout << "\nEnter the ID of the item to mark complete (or 0 to cancel): ";
    int id;
    if (!(std::cin >> id)) {
        clearInput();
        std::cout << "Invalid input.\n";
        return;
    }
    clearInput();

    if (id == 0) {
        std::cout << "Cancelled.\n";
        return;
    }

    auto it = std::find_if(items.begin(), items.end(),
        [id](const TodoItem& item) { return item.id == id; });

    if (it == items.end()) {
        std::cout << "Item #" << id << " not found.\n";
        return;
    }

    if (it->status == "Complete") {
        std::cout << "Item #" << id << " is already marked complete.\n";
        return;
    }

    it->status = "Complete";
    saveTodos(items);
    std::cout << "Item #" << id << " marked as complete.\n";
}

void showMenu() {
    std::cout << "\n";
    printSeparator();
    std::cout << "  TO-DO LIST APPLICATION\n";
    printSeparator();
    std::cout << "  1. Create new item\n";
    std::cout << "  2. List all items\n";
    std::cout << "  3. Edit an item\n";
    std::cout << "  4. Delete an item\n";
    std::cout << "  5. Mark item complete\n";
    std::cout << "  6. Exit\n";
    printSeparator();
    std::cout << "Choose an option (1-6): ";
}

int main() {
    std::vector<TodoItem> items = loadTodos();

    while (true) {
        showMenu();

        int choice;
        if (!(std::cin >> choice)) {
            clearInput();
            std::cout << "Invalid input. Please enter a number 1-6.\n";
            continue;
        }
        clearInput();

        switch (choice) {
            case 1: createItem(items);    break;
            case 2: listItems(items);     break;
            case 3: editItem(items);      break;
            case 4: deleteItem(items);    break;
            case 5: markComplete(items);  break;
            case 6:
                std::cout << "Goodbye!\n";
                std::cout << "Press Enter to exit...";
                std::cin.get();
                return 0;
            default:
                std::cout << "Invalid option. Please enter 1-6.\n";
                break;
        }
    }

    return 0;
}
