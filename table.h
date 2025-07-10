#include "field.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <map>

using namespace std;

class SQLlite{
    public:
        explicit SQLlite(bool quietMode = false) : quiet(quietMode) {}
        void processCommand(const string& cmd);`

    private:
        struct Table{
            vector<string> columnNames;
            vector<ColumnType> columnTypes;
            vector<vector<Field>> rows;
            unordered_map<string, unordered_map<Field, vector<size_t>>> hashIndex;
            unordered_map<string, map<Field, vector<size_t>>> bstIndex;

            Table(vector<string> names, vector<ColumnType> types) : columnNames(move(names)), columnTypes(move(types)) {}


            void insertRow(const vector<Field>& row) {rows.push_back(row); }
            void printAll();

            void printWhere(const vector<string>& selectedColumns, const string& whereCol, const string& op, const Field& val, bool quiet, const string& tableName);
            void deleteWhere(const string& col, const string& op, const Field& val);
            void generateIndex(const string& col, const string& type, const string& tableName);

            struct RowMatch{
                size_t colIndex;
                string op;
                Field val;

                RowMatch(size_t idx, string operation, Field value) : colIndex(idx), op(move(operation)), val(move(value)) {}
                bool operator()(const vector<Field>& row) const {
                    if(op == "<")
                        return row[colIndex] < val;
                    else if(op == ">")
                        return row[colIndex] > val;
                    else if(op == "=")
                        return row[colIndex] == val;
                    return false;
                }
            };
        };

        unordered_map<string, Table> tables;
        bool quiet;
        void createTable(const vector<string>& tokens);
        void removeTable(const string& tableName);
        void insertInto(const vector<string>& tokens);
        void printTable(const vector<string>& tokens, bool quiet);
        void deleteFromTable(const vector<string>& tokens);
        void joinTables(const vector<string>& tokens);
        void generateIndex(const vector<string>& tokens);
        Field parseValue(const string& value, ColumnType type);
};