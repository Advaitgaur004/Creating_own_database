// Table.cpp
#include "Table.hpp"
#include <sstream>
#include <algorithm>
#include <map>

// Initialize DATA_DIR as a constant
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
}

void Table::select(const std::vector<std::string>& select_columns, 
                  const std::vector<std::pair<std::string, std::string>>& aggregates,
                  const std::string& where_column, 
                  const std::string& where_value,
                  const std::vector<std::pair<std::string, std::string>>& order_by,
                  const std::vector<std::string>& group_by) {
    // Determine columns to display
    std::vector<int> col_indices;
    for (const auto& col : select_columns) {
        auto it = std::find(columns.begin(), columns.end(), col);
        if (it != columns.end()) {
            col_indices.push_back(std::distance(columns.begin(), it));
        } else {
            std::cerr << "Error: Column " << col << " does not exist.\n";
            return;
        }
    }

    // Handle GROUP BY
    if (!group_by.empty()) {
        // Ensure all group_by columns exist
        std::vector<int> group_indices;
        for (const auto& gb_col : group_by) {
            auto it = std::find(columns.begin(), columns.end(), gb_col);
            if (it != columns.end()) {
                group_indices.push_back(std::distance(columns.begin(), it));
            } else {
                std::cerr << "Error: GROUP BY column " << gb_col << " does not exist.\n";
                return;
            }
        }

        // Prepare aggregate functions
        std::vector<std::pair<std::string, int>> agg_functions; // function name and column index (-1 for COUNT(*))
        for (const auto& agg : aggregates) {
            std::string func = agg.first;
            std::string target = agg.second;
            if (func == "COUNT") {
                if (target == "*") {
                    agg_functions.emplace_back(func, -1);
                } else {
                    auto it = std::find(columns.begin(), columns.end(), target);
                    if (it != columns.end()) {
                        agg_functions.emplace_back(func, std::distance(columns.begin(), it));
                    } else {
                        std::cerr << "Error: COUNT target column " << target << " does not exist.\n";
                        return;
                    }
                }
            }
            else {
                std::cerr << "Error: Unsupported aggregate function '" << func << "'.\n";
                return;
            }
        }

        // Group records
        std::map<std::string, std::vector<Record>> grouped_records;
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
                std::string key;
                for (const auto& idx : group_indices) {
                    key += record.fields[idx] + "_";
                }
                grouped_records[key].emplace_back(record);
            }
        }

        // Print header
        for (size_t i = 0; i < group_by.size(); ++i) {
            std::cout << group_by[i];
            if (i != group_by.size() - 1 || !agg_functions.empty()) std::cout << " | ";
        }
        for (size_t i = 0; i < agg_functions.size(); ++i) {
            std::cout << agg_functions[i].first << "(" << (agg_functions[i].second == -1 ? "*" : columns[agg_functions[i].second]) << ")";
            if (i != agg_functions.size() - 1) std::cout << " | ";
        }
        std::cout << "\n";

        // Print separator
        for (size_t i = 0; i < group_by.size(); ++i) {
            std::cout << "--------";
            if (i != group_by.size() - 1 || !agg_functions.empty()) std::cout << "+";
        }
        for (size_t i = 0; i < agg_functions.size(); ++i) {
            std::cout << "--------";
            if (i != agg_functions.size() - 1) std::cout << "+";
        }
        std::cout << "\n";

        // Print grouped records with aggregates
        for (const auto& pair : grouped_records) {
            std::stringstream ss(pair.first);
            std::string value;
            size_t idx = 0;
            while (std::getline(ss, value, '_')) {
                if (idx < group_by.size()) {
                    std::cout << value;
                    if (idx != group_by.size() - 1 || !agg_functions.empty()) std::cout << " | ";
                }
                idx++;
            }
            for (size_t i = 0; i < agg_functions.size(); ++i) {
                if (agg_functions[i].first == "COUNT") {
                    if (agg_functions[i].second == -1) {
                        std::cout << pair.second.size();
                    }
                    else {
                        // Count non-empty values in the specified column
                        int count = 0;
                        for (const auto& rec : pair.second) {
                            if (!rec.fields[agg_functions[i].second].empty()) {
                                count++;
                            }
                        }
                        std::cout << count;
                    }
                }
                // Future aggregate functions can be handled here
                if (i != agg_functions.size() - 1) std::cout << " | ";
            }
            std::cout << "\n";
        }
        return;
    }

    // Filter records based on WHERE clause
    std::vector<Record> filtered_records;
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
            filtered_records.emplace_back(record);
        }
    }

    // Handle ORDER BY
    if (!order_by.empty()) {
        // Check if order_by columns exist
        for (const auto& ob : order_by) {
            if (std::find(columns.begin(), columns.end(), ob.first) == columns.end()) {
                std::cerr << "Error: ORDER BY column " << ob.first << " does not exist.\n";
                return;
            }
        }
        // Sort the filtered_records
        std::sort(filtered_records.begin(), filtered_records.end(),
            [&](const Record& a, const Record& b) -> bool {
                for (const auto& ob : order_by) {
                    int idx = std::distance(columns.begin(), std::find(columns.begin(), columns.end(), ob.first));
                    if (a.fields[idx] < b.fields[idx]) {
                        return ob.second == "ASC";
                    }
                    else if (a.fields[idx] > b.fields[idx]) {
                        return ob.second == "DESC";
                    }
                    // If equal, continue to next ordering
                }
                return false;
            }
        );
    }

    // Print header
    for (size_t i = 0; i < select_columns.size(); ++i) {
        std::cout << select_columns[i];
        if (i != select_columns.size() - 1 || !aggregates.empty()) std::cout << " | ";
    }
    for (size_t i = 0; i < aggregates.size(); ++i) {
        std::cout << aggregates[i].first << "(" << aggregates[i].second << ")";
        if (i != aggregates.size() - 1) std::cout << " | ";
    }
    std::cout << "\n";

    // Print separator
    for (size_t i = 0; i < select_columns.size(); ++i) {
        std::cout << "--------";
        if (i != select_columns.size() - 1 || !aggregates.empty()) std::cout << "+";
    }
    for (size_t i = 0; i < aggregates.size(); ++i) {
        std::cout << "--------";
        if (i != aggregates.size() - 1) std::cout << "+";
    }
    std::cout << "\n";

    // Print records
    for (const auto& record : filtered_records) {
        for (size_t i = 0; i < col_indices.size(); ++i) {
            std::cout << record.fields[col_indices[i]];
            if (i != col_indices.size() - 1 || !aggregates.empty()) std::cout << " | ";
        }
        // Handle aggregates (if any without GROUP BY, typically SUM, COUNT, etc. would require aggregation over all records)
        for (size_t i = 0; i < aggregates.size(); ++i) {
            if (aggregates[i].first == "COUNT") {
                if (aggregates[i].second == "*") {
                    std::cout << "1"; // Each record counts as 1
                }
                else {
                    // Count non-empty values in the specified column
                    int count = 0;
                    for (const auto& rec : filtered_records) {
                        auto it = std::find(columns.begin(), columns.end(), aggregates[i].second);
                        if (it != columns.end()) {
                            int idx = std::distance(columns.begin(), it);
                            if (!rec.fields[idx].empty()) {
                                count++;
                            }
                        }
                    }
                    std::cout << count;
                }
            }
            // Future aggregate functions can be handled here
            if (i != aggregates.size() - 1) std::cout << " | ";
        }
        std::cout << "\n";
    }
}

void Table::update(const std::string& set_column, const std::string& set_value, 
                  const std::string& where_column, 
                  const std::string& where_value) {
    auto it = std::find(columns.begin(), columns.end(), set_column);
    if (it == columns.end()) {
        std::cerr << "Error: SET column " << set_column << " does not exist.\n";
        return;
    }
    int set_idx = std::distance(columns.begin(), it);
    int updated_count = 0;

    for (auto& record : records) {
        bool match = true;
        if (!where_column.empty()) {
            auto where_it = std::find(columns.begin(), columns.end(), where_column);
            if (where_it != columns.end()) {
                int where_idx = std::distance(columns.begin(), where_it);
                if (record.fields[where_idx] != where_value) {
                    match = false;
                }
            }
            else {
                std::cerr << "Error: WHERE column " << where_column << " does not exist.\n";
                return;
            }
        }
        if (match) {
            record.fields[set_idx] = set_value;
            updated_count++;
        }
    }
    std::cout << "Updated " << updated_count << " record(s) in " << name << ".\n";
}

void Table::deleteRecords(const std::string& where_column, const std::string& where_value) {
    auto initial_size = records.size();
    records.erase(
        std::remove_if(records.begin(), records.end(),
            [&](const Record& record) -> bool {
                if (where_column.empty()) {
                    return true; // Delete all
                }
                auto it = std::find(columns.begin(), columns.end(), where_column);
                if (it != columns.end()) {
                    int idx = std::distance(columns.begin(), it);
                    return record.fields[idx] == where_value;
                }
                else {
                    std::cerr << "Error: WHERE column " << where_column << " does not exist.\n";
                    return false;
                }
            }),
        records.end()
    );
    auto deleted_count = initial_size - records.size();
    std::cout << "Deleted " << deleted_count << " record(s) from " << name << ".\n";
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