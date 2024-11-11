// Database.cpp (Updated)
#include "Database.hpp"
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

void Database::createTable(const std::string& name, const std::vector<std::string>& columns) {
    if (tables.find(name) != tables.end()) {
        std::cerr << "Error: Table " << name << " already exists.\n";
        return;
    }
    tables[name] = std::make_unique<Table>(name, columns);
    std::cout << "Table " << name << " created successfully.\n";
}

void Database::loadTable(const std::string& name) {
    if (tables.find(name) != tables.end()) {
        std::cerr << "Error: Table " << name << " is already loaded.\n";
        return;
    }
    // Check if file exists
    std::string filepath = "data/" + name + ".tbl";
    if (!fs::exists(filepath)) {
        std::cerr << "Error: Table " << name << " does not exist.\n";
        return;
    }
    tables[name] = std::make_unique<Table>(name);
    std::cout << "Table " << name << " loaded successfully.\n";
}

Table* Database::getTable(const std::string& name) {
    auto it = tables.find(name);
    if (it != tables.end()) {
        return it->second.get();
    }
    std::cerr << "Error: Table " << name << " not found.\n";
    return nullptr;
}

void Database::run() {
    std::string input;
    std::cout << "Welcome to MiniDB! Enter SQL commands or 'exit' to quit.\n";
    while (true) {
        std::cout << "MiniDB> ";
        std::getline(std::cin, input);
        if (input.empty()) continue;

        // Exit condition
        if (input == "exit") break;

        // Simple parsing
        std::stringstream ss(input);
        std::string command;
        ss >> command;
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "CREATE") {
            std::string table_keyword, table_name;
            ss >> table_keyword >> table_name;
            std::transform(table_keyword.begin(), table_keyword.end(), table_keyword.begin(), ::toupper);
            if (table_keyword != "TABLE") {
                std::cerr << "Error: Invalid syntax. Did you mean 'CREATE TABLE'? \n";
                continue;
            }
            // Parse columns
            std::string cols;
            std::getline(ss, cols, '(');
            std::getline(ss, cols, ')');
            std::vector<std::string> columns;
            std::stringstream cols_ss(cols);
            std::string col;
            while (std::getline(cols_ss, col, ',')) {
                // Trim whitespace
                col.erase(col.begin(), std::find_if(col.begin(), col.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                col.erase(std::find_if(col.rbegin(), col.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), col.end());
                columns.push_back(col);
            }
            createTable(table_name, columns);
        }
        else if (command == "INSERT") {
            std::string into_keyword, table_name, values_keyword;
            ss >> into_keyword >> table_name >> values_keyword;
            std::transform(into_keyword.begin(), into_keyword.end(), into_keyword.begin(), ::toupper);
            std::transform(values_keyword.begin(), values_keyword.end(), values_keyword.begin(), ::toupper);
            if (into_keyword != "INTO" || values_keyword != "VALUES") {
                std::cerr << "Error: Invalid syntax. Use 'INSERT INTO table_name VALUES (...)'\n";
                continue;
            }
            std::string vals;
            std::getline(ss, vals, '(');
            std::getline(ss, vals, ')');
            std::vector<std::string> values;
            std::stringstream vals_ss(vals);
            std::string val;
            while (std::getline(vals_ss, val, ',')) {
                // Trim whitespace
                val.erase(val.begin(), std::find_if(val.begin(), val.end(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }));
                val.erase(std::find_if(val.rbegin(), val.rend(), [](unsigned char ch) {
                    return !std::isspace(ch);
                }).base(), val.end());
                values.push_back(val);
            }
            Table* table = getTable(table_name);
            if (table) {
                table->insert(values);
                std::cout << "Record inserted into " << table_name << ".\n";
            }
        }
        else if (command == "SELECT") {
            std::vector<std::string> selected_columns;
            std::string token;

            // Extract selected columns until 'FROM' is found
            while (ss >> token && 
                   token != "FROM" && 
                   token != "from" && 
                   token != "From") {
                // Handle cases where columns are separated by commas
                if (token.back() == ',') {
                    token.pop_back(); // Remove trailing comma
                }
                selected_columns.push_back(token);
            }

            // Check if 'FROM' keyword was found
            if (!(token == "FROM" || token == "from" || token == "From")) {
                std::cerr << "Error: Invalid syntax. Missing 'FROM'.\n";
                continue;
            }

            // Extract table name
            std::string table_name;
            ss >> table_name;
            if (table_name.empty()) {
                std::cerr << "Error: Missing table name after 'FROM'.\n";
                continue;
            }

            // Initialize variables for WHERE clause
            std::string where_keyword, where_column, where_value;

            // Check for 'WHERE' clause
            ss >> where_keyword;
            std::transform(where_keyword.begin(), where_keyword.end(), where_keyword.begin(), ::toupper);
            if (where_keyword == "WHERE") {
                ss >> where_column >> where_value;
                // Remove potential semicolon at the end of where_value
                if (!where_value.empty() && where_value.back() == ';') {
                    where_value.pop_back();
                }
            } else if (!where_keyword.empty()) {
                // If the keyword is not 'WHERE' and not the end, it's a syntax error
                std::cerr << "Error: Unrecognized keyword '" << where_keyword << "'.\n";
                continue;
            }

            // Handle '*' to select all columns
            if (selected_columns.size() == 1 && selected_columns[0] == "*") {
                selected_columns.clear(); // Passing an empty vector will indicate selecting all columns
            }

            // Retrieve the table and perform the select operation
            Table* table = getTable(table_name);
            if (table) {
                table->select(selected_columns, where_column, where_value);
            }
        }
        else {
            std::cerr << "Error: Unrecognized command.\n";
        }
    }
}