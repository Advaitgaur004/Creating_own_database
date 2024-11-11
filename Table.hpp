// Table.hpp
#ifndef TABLE_HPP
#define TABLE_HPP

#include "Record.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

class Table {
private:
    std::string name;
    std::vector<std::string> columns;
    std::vector<Record> records;
    std::string filepath;

public:
    Table(const std::string& name, const std::vector<std::string>& columns);
    Table(const std::string& name); // Load existing table

    void insert(const std::vector<std::string>& fields);
    void select(const std::vector<std::string>& select_columns, const std::string& where_column = "", const std::string& where_value = "");

    void save();
    void load();
    const std::string& getName() const { return name; }
};

#endif // TABLE_HPP