// Database.hpp
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "Table.hpp"
#include <unordered_map>
#include <memory>

class Database {
private:
    std::unordered_map<std::string, std::unique_ptr<Table>> tables;

public:
    Database() = default;

    void createTable(const std::string& name, const std::vector<std::string>& columns);
    void loadTable(const std::string& name);
    Table* getTable(const std::string& name);

    void run();
};

#endif // DATABASE_HPP