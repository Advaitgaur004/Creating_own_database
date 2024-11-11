// Table.cpp
#include "Table.hpp"
#include <sstream>
#include <algorithm>

const std::string DATA_DIR = "data/";

Table::Table(const std::string& name, const std::vector<std::string>& columns) : name(name), columns(columns) {
    filepath = DATA_DIR + name + ".tbl";
    save(); // Save table schema
}

Table::Table(const std::string& name) : name(name) {
    filepath = DATA_DIR + name + ".tbl";
    load();
}

void Table::insert(const std::vector<std::string>& fields) {
    if (fields.size() != columns.size()) {
        std::cerr << "Error: Field count doesn't match column count.\n";
        return;
    }
    records.emplace_back(fields);
    save();
}

void Table::select(const std::vector<std::string>& select_columns, const std::string& where_column, const std::string& where_value) {
    // Determine columns to display
    std::vector<int> col_indices;
    if (select_columns.empty()) { // Select all
        for (size_t i = 0; i < columns.size(); ++i) {
            col_indices.push_back(i);
        }
    } else {
        for (const auto& col : select_columns) {
            auto it = std::find(columns.begin(), columns.end(), col);
            if (it != columns.end()) {
                col_indices.push_back(std::distance(columns.begin(), it));
            } else {
                std::cerr << "Error: Column " << col << " does not exist.\n";
                return;
            }
        }
    }

    // Print header
    for (size_t i = 0; i < col_indices.size(); ++i) {
        std::cout << columns[col_indices[i]];
        if (i != col_indices.size() - 1) std::cout << " | ";
    }
    std::cout << "\n";

    // Print separator
    for (size_t i = 0; i < col_indices.size(); ++i) {
        std::cout << "--------";
        if (i != col_indices.size() - 1) std::cout << "+";
    }
    std::cout << "\n";

    // Print records
    for (const auto& record : records) {
        bool match = true;
        if (!where_column.empty()) {
            auto it = std::find(columns.begin(), columns.end(), where_column);
            if (it != columns.end()) {
                int idx = std::distance(columns.begin(), it);
                if (record.fields[idx] != where_value) {
                    match = false;
                }
            } else {
                std::cerr << "Error: WHERE column " << where_column << " does not exist.\n";
                return;
            }
        }
        if (match) {
            for (size_t i = 0; i < col_indices.size(); ++i) {
                std::cout << record.fields[col_indices[i]];
                if (i != col_indices.size() - 1) std::cout << " | ";
            }
            std::cout << "\n";
        }
    }
}

void Table::save() {
    std::ofstream ofs(filepath, std::ios::trunc);
    if (!ofs) {
        std::cerr << "Error: Unable to open file " << filepath << " for writing.\n";
        return;
    }
    // First line: column headers
    for (size_t i = 0; i < columns.size(); ++i) {
        ofs << columns[i];
        if (i != columns.size() - 1) ofs << ",";
    }
    ofs << "\n";

    // Records
    for (const auto& record : records) {
        for (size_t i = 0; i < record.fields.size(); ++i) {
            ofs << record.fields[i];
            if (i != record.fields.size() - 1) ofs << ",";
        }
        ofs << "\n";
    }
    ofs.close();
}

void Table::load() {
    std::ifstream ifs(filepath);
    if (!ifs) {
        std::cerr << "Error: Unable to open file " << filepath << " for reading.\n";
        return;
    }
    std::string line;
    bool is_header = true;
    while (std::getline(ifs, line)) {
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }
        if (is_header) {
            columns = fields;
            is_header = false;
        } else {
            records.emplace_back(fields);
        }
    }
    ifs.close();
}   