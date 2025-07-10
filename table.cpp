#include "table.h"
#include <sstream>
#include <algorithm>
#include <variant>

using namespace std;

//main loop function call
void SQLlite::processCommand(const string& cmd){
    vector<string> tokens;
    string token;
    string line;

    getline(cin, line);
    stringstream ss(line);

    while(ss >> token){
        if(token == ";"){
            break;
        }
        tokens.push_back(token);
    }

    if(cmd[0] == '#'){
        return;
    } else if(cmd == "QUIT"){
        cout << "Thanks for using!" << endl;
        return;
    } else if (cmd == "CREATE"){
        createTable(tokens);
    } else if(cmd == "REMOVE"){
        if(tokens.empty()){
            cout << "Error during REMOVE: Missing table name" << endl;
            return;
        }
        removeTable(tokens[0]);
    } else if (cmd == "INSERT"){
        if(tokens.size() >= 1 && tokens[0] == "INTO"){
            insertInto(vector<string>(tokens.begin() + 1, tokens.end()));
        } else {
            cout << "Error during INSERT: Expected format 'INSERT INTO <table> <numRows>'" << endl;
        }
    } else if (cmd == "PRINT"){
        if(tokens.size() >= 1 && tokens[0] == "FROM"){
            
            //PRINT FROM <tableName> <numCols> <col1> <col2> ... WHERE <colname> <op> <value>
            string tableName = tokens[1];
            auto tableIt = tables.find(tableName);
            if(tableIt == tables.end()){
                cout << "Error during PRINT: " << tableName << " does not name a table in the database" << endl;
                return;
            }

            auto whereIt = find(tokens.begin(), tokens.end(), "WHERE");
            if(whereIt != tokens.end() && distance(whereIt, tokens.end()) >= 4){
                int numCols;
                try{
                    numCols = stoi(tokens[2]);
                } catch (...){
                    cout << "Error during PRINT: Invalid number of columns" << endl;
                    return;
                }

                vector<string> selectedColumns;
                for(int i = 0; i < numCols && i + 3 < whereIt - tokens.begin(); ++ i){
                    selectedColumns.push_back(tokens[3 + i]);
                }

                string whereCol = *(whereIt + 1);
                string op = *(whereIt + 2);
                string valueStr = *(whereIt + 3);

                if(op != "<" && op != ">" && op != "="){
                    cout << "Error during PRINT: Invalid comparison operator '" << op << "'" << endl;
                    return;
                }

                auto colIt = find(tableIt->second.columnNames.begin(), tableIt->second.columnNames.end(), whereCol);
                if(colIt == tableIt->second.columnNames.end()){
                    cout << "Error during PRINT: " << whereCol << " does not name a column in " << tableName << endl;
                    return;
                }

                int colIndex = static_cast<int>(distance(tableIt->second.columnNames.begin(), colIt));
                Field value = parseValue(valueStr, tableIt->second.columnTypes[colIndex]);
                tableIt->second.printWhere(selectedColumns, whereCol, op, value, quiet, tableName);
            } else {
                printTable(vector<string>(tokens.begin() + 1, tokens.end()), quiet);
            }
        } else {
            cout << "Error during PRINT: Expected format 'PRINT FROM <table> <numCols> <col1> <col2> ... ALL'" << endl;
        }
    } else if (cmd == "DELETE"){ // DELETE FROM <tablename> WHERE <colname> <OP> <value>
        if(tokens.size() >= 4 && tokens[0] == "FROM" && tokens[2] == "WHERE"){
            deleteFromTable(tokens);
        } else {
            cout << "Error during DELETE: Expected format 'DELETE FROM <table> WHERE <column> <op> <value>'" << endl;    
        }
    } else if(cmd == "GENERATE"){ // GENERTE FOR <tablename> <indextype> INDEX ON <colname>
        if(tokens.size() == 6 && tokens[0] == "FOR" && tokens[3] == "INDEX" && tokens[4] == "ON"){
            string tableName = tokens[1];
            string indexType = tokens[2];
            string colName = tokens[5];
            auto tableIt = tables.find(tableName);
            if(tableIt == tables.end()){
                cout << "Error during GENERATE: " << tableName << " does not name a table in the database" << endl;
                return;
            }

            tableIt->second.generateIndex(colName, indexType, tableName);
        }
    } else if (cmd == "JOIN"){
        joinTables(tokens);
    } else {
        cout << "Error: unrecognized command" << endl;
    }
}

//create table function
void SQLlite::createTable(const vector<string>& tokens){
    string tableName = tokens[0];

    if(tables.find(tableName) != tables.end()){
        cout << "Error during CREATE: Cannot create already existing table " << tableName << endl;
        return;
    }

    int numCols = stoi(tokens[1]);
    vector<ColumnType> columnTypes;
    vector<string> columnNames;

    for(int i = 0; i < numCols; ++i){
        string type = tokens[2+i];
        if(type == "string"){
            columnTypes.push_back(ColumnType::String);
        } else if (type == "double"){
            columnTypes.push_back(ColumnType::Double);
        } else if (type == "int"){
            columnTypes.push_back(ColumnType::Int);
        } else if(type == "bool"){
            columnTypes.push_back(ColumnType::Bool);
        }
    }

    for(int i = 0; i < numCols; ++i){
        columnNames.push_back(tokens[2+numCols+i]);
    }

    tables.emplace(tableName, Table(columnNames, columnTypes));

    cout << "New table " << tableName << " with column(s)";
    for(const auto& name : columnNames){
        cout << " " << name;
    }
    cout << " created" << endl;
}


void SQLlite::removeTable(const string& tableName){
    auto it = tables.find(tableName);
    if(it == tables.end()){
        cout << "Error during REMOVE: " << tableName << " does not name a table in the database" << endl;
        return;
    }

    tables.erase(it);
    cout << "Table " << tableName << " removed" << endl;
}


void SQLlite::insertInto(const vector<string>& tokens){
    if(tokens.size() < 3 || tokens[2] != "ROWS"){
        cout << "Error during INSERT: Expected format 'INSERT INTO <table> <numRows>'" << endl;
        return;
    }

    string tableName = tokens[0];
    auto it = tables.find(tableName);

    if(it == tables.end()){
        cout << "Error during INSERT: " << tableName << " does not name a table in the database" << endl;
        return;
    }

    Table& table = it->second;
    size_t numCols = table.columnNames.size();
    size_t startIndex = table.rows.size();

    int numRows;
    try{
        numRows = stoi(tokens[1]);
        if(numRows <= 0){
            cout << "Error during INSERT: Number of rows inserted must be positive" << endl;
            return;
        }
    } catch (...){
        cout << "Error during INSERT: Invalid number of rows" << endl;
        return;
    }

    for(int row = 0; row < numRows; ++row){
        vector<Field> newRow;
        string line;
        string value;
        getline(cin, line);
        stringstream ss(line);
        vector<string> rowValues;

        while(ss >> value){
            rowValues.push_back(value);
        }

        if(rowValues.size() != numCols){
            cout << "Error during INSERT: Expected " << numCols << " values, but got " << rowValues.size() << " on row " << row + 1 << endl;
            return;
        }

        for(size_t i = 0; i < numCols; ++i){
            ColumnType colType = table.columnTypes[i];

            try {
                if(colType == ColumnType::Int){
                    newRow.emplace_back(stoi(rowValues[i]));
                } else if (colType == ColumnType::Double){
                    newRow.emplace_back(stod(rowValues[i]));
                } else if (colType == ColumnType::String){
                    if(rowValues[i].find(' ') != string::npos){
                        cout << "Error during INSERT: String values must be a single word" << endl;
                        return;
                    }
                    newRow.emplace_back(rowValues[i]);
                } else if (colType == ColumnType::Bool){
                    if(rowValues[i] == "true" || rowValues[i] == "1"){
                        newRow.emplace_back(true);
                    } else if(rowValues[i] == "false" || rowValues[i] == "0"){
                        newRow.emplace_back(false);
                    } else {
                        cout << "Error during INSERT: Invalid boolean value" << endl;
                        return;
                    }
                } else {
                    cout << "Error during INSERT: Invalid boolean value in row" << row + 1 << endl;
                    return;
                }
            } catch (const exception&){
                cout << "Error during INSERT: Invalid value for column " << table.columnNames[i] << " in row " << row + 1 << endl;
                return;
            }
        } 
        table.rows.push_back(newRow);
    }

    size_t endIndex = table.rows.size() - 1;

    for(size_t colIdx = 0; colIdx < table.columnNames.size(); ++colIdx){
        const string& colName = table.columnNames[colIdx];
        if(table.hashIndex.find(colName) != table.hashIndex.end() && !table.hashIndex[colName].empty()){
            for(size_t rowIdx = startIndex; rowIdx <= endIndex; ++rowIdx){
                table.hashIndex[colName][table.rows[rowIdx][colIdx]].push_back(rowIdx);
            }
        }

        if(table.bstIndex.find(colName) != table.bstIndex.end() && !table.bstIndex[colName].empty()){
            for(size_t rowIdx = startIndex; rowIdx <= endIndex; ++rowIdx){
                table.bstIndex[colName][table.rows[rowIdx][colIdx]].push_back(rowIdx);
            }
        }
            
    }

    cout << "Added " << numRows << " rows to " << tableName << " from position " << startIndex << " to " << endIndex << endl;
}

void SQLlite::printTable(const vector<string>& tokens, bool quiet){
    if(tokens.size() < 3){
        cout << "Error durring PRINT: Missing table name or column selection" << endl;
        return;
    }

    string tableName = tokens[0];
    auto it = tables.find(tableName);
    if(it == tables.end()){
        cout << "Error during PRINT: " << tableName << " does not name a table in the database" << endl;
        return;
    }


    Table& table = it->second;
    int numCols;

    try{
        numCols = stoi(tokens[1]);
    } catch (...){
        cout << "Error during PRINT: Invalid number of columns" << endl;
        return;
    }

    if(tokens.size() < static_cast<std::size_t>(2 + numCols + 1) || tokens[2 + numCols] != "ALL"){
        cout << "Error during PRINT: Invalid command format" << endl;
    }

    // column names
    vector<string> selectedColumns(tokens.begin() + 2, tokens.begin() + 2 + numCols);
    
    
    //correct selected columns
    vector<int> colIndices;

    for(const string& col : selectedColumns){
        auto it = find(table.columnNames.begin(), table.columnNames.end(), col);
        if(it == table.columnNames.end()){
            cout << "Error during PRINT: " << col << " does not name a column in " << tableName << endl;
            return;
        }
        colIndices.push_back(static_cast<int>(std::distance(table.columnNames.begin(), it)));
    }

    if(!quiet){
        //column names
        for(const string& col : selectedColumns){
            cout << col << " ";
        }
        cout << endl;

        //rows from selected columns
        for(const auto& row: table.rows){
            for(int index : colIndices){
                cout << row[index] << " ";
            }
            cout << endl;
        }
    }
    //summary
    cout << "Printed " << table.rows.size() << " matching rows from " << tableName << endl;
}


//helper function
Field SQLlite::parseValue(const string& value, ColumnType type){
    try{
        if(type == ColumnType::Int){
            return Field(stoi(value));
        }
        if(type == ColumnType::Double){
            return Field(stod(value));
        }
        if(type == ColumnType::String){
            return Field(value);
        } 
        if(type == ColumnType::Bool){
            if(value == "true" || value == "1"){
                return Field(true);
            } else if(value == "false" || value == "0"){
                return Field(false);
            } else {
                cout << "Error during DELETE: Invalid boolean value" << endl;
                throw runtime_error("Invalid boolean value");
            }
        }
    } catch (...){
        cout << "Error during DELETE: Invalid value for column " << value << endl;
        throw;
    }
    return Field("");
}

//use STL to delete rows fror faster time

// FROM <tablename> WHERE <colname> <OP> <value>
void SQLlite::deleteFromTable(const vector<string>& tokens){
    if(tokens.size() < 6) {
        cout << "Error 1 during DELETE: Expected format 'DELETE FROM <table> WHERE <column> <op> <value>'" << endl;
        return;
    }

    string tableName = tokens[1];
    string colName = tokens[3];
    string op = tokens[4];
    string valueStr = tokens[5];   
    
    if(op != "<" && op != ">" && op != "=") {
        cout << "Error during DELETE: Invalid comparison operator '" << op << "'" << endl;
        return;
    }

    auto it = tables.find(tableName);
    if(it == tables.end()){
        cout << "Error during DELETE: " << tableName << " does not name a table in the database" << endl;
        return;
    }

    Table& table = it->second;
    auto colIt = find(table.columnNames.begin(), table.columnNames.end(), colName);
    if(colIt == table.columnNames.end()){
        cout << "Error during DELETE: " << colName << " does not name a column in " << tableName << endl;
        return;
    }

    int colIndex = static_cast<int>(std::distance(table.columnNames.begin(), colIt));
    try {
        Field value = parseValue(valueStr, table.columnTypes[colIndex]);

        vector<size_t> rowsToDelete;
        for(size_t i = 0; i < table.rows.size(); ++i){
            if((op == "<" && table.rows[i][colIndex] < value) ||
                (op == ">" && table.rows[i][colIndex] > value) || 
                (op == "=" && table.rows[i][colIndex] == value)){
                rowsToDelete.push_back(i);
            }
        }


        auto newEnd = remove_if(table.rows.begin(), table.rows.end(), Table::RowMatch(colIndex, op, value));
        size_t numDeleted = distance(newEnd, table.rows.end());
        table.rows.erase(newEnd, table.rows.end());

        // Update the hash and BST indices
        for(size_t colIdx = 0; colIdx < table.columnNames.size(); ++colIdx){
            const string& currentColName = table.columnNames[colIdx];

            if(table.hashIndex.find(currentColName) != table.hashIndex.end() && !table.hashIndex[currentColName].empty()){
                unordered_map<Field, vector<size_t>> newIndex;
                for(size_t i = 0; i < table.rows.size(); ++i){
                    newIndex[table.rows[i][colIdx]].push_back(i);
                }
                table.hashIndex[currentColName] = move(newIndex);
            }

            if(table.bstIndex.find(currentColName) != table.bstIndex.end() && !table.bstIndex[currentColName].empty()){
                map<Field, vector<size_t>> newIndex;
                for(size_t i = 0; i < table.rows.size(); ++i){
                    newIndex[table.rows[i][colIdx]].push_back(i);
                }
                table.bstIndex[currentColName] = move(newIndex);
            }
        }
        

        cout << "Deleted " << numDeleted << " rows from " << tableName << endl;
    } catch (...) {
        return;
    }
}


void SQLlite::Table::generateIndex(const string& col, const string& type, const string& tableName){
    auto it = find(columnNames.begin(), columnNames.end(), col);
    if(it == columnNames.end()){
        cout << "Error during GENERATE: " << col << " does not name a column in " << tableName << endl;
        return;
    }

    size_t colIndex = distance(columnNames.begin(), it);
    hashIndex[col].clear();
    bstIndex[col].clear();

    if(type == "hash"){
        unordered_map<Field, vector<size_t>> newIndex;
        for(size_t i = 0; i < rows.size(); ++i){
            newIndex[rows[i][colIndex]].push_back(i);
        }
        hashIndex[col] = move(newIndex);
        cout << "Generated hash index for table " << tableName << " on column " << col << ", with " << hashIndex[col].size() << " distinct keys" << endl;
    } else if(type == "bst"){
        map<Field, vector<size_t>> newIndex;
        for(size_t i = 0; i < rows.size(); ++i){
            newIndex[rows[i][colIndex]].push_back(i);
        }
        bstIndex[col] = move(newIndex);
        cout << "Generated bst index for table " << tableName << " on column " << col << ", with " << bstIndex[col].size() << " distinct keys" << endl;
    } else {
        cout << "Error during GENERATE: Invalid index type '" << type << "'" << endl;
    }
}

void SQLlite::Table::printWhere(const vector<string>& selectedColumns, const string& whereCol, const string& op, const Field& val, bool quiet, const string& tableName){
    vector<int> colIndices;
    for(const auto& col : selectedColumns){
        auto it = find(columnNames.begin(), columnNames.end(), col);
        if(it == columnNames.end()){
            cout << "Error during PRINT: " << col << " does not name a column in " << tableName << endl;
            return;
        }
        colIndices.push_back(static_cast<int>(distance(columnNames.begin(), it)));
    }

    auto whereIt = find(columnNames.begin(), columnNames.end(), whereCol);
    if(whereIt == columnNames.end()){
        cout << "Error during PRINT: " << whereCol << " does not name column in " << tableName << endl;
        return;
    }

    size_t whereColIndex = distance(columnNames.begin(), whereIt);
    vector<size_t> matchingRows;
    bool foundWithIndex = false;

    if(op == "=" && hashIndex.count(whereCol) > 0){
        auto valueIt = hashIndex[whereCol].find(val);
        if(valueIt != hashIndex[whereCol].end()){
            matchingRows = valueIt->second;
            foundWithIndex = true;
        }
    } else if(bstIndex.count(whereCol) > 0){
        if(op == "="){
            auto valueIt = bstIndex[whereCol].find(val);
            if(valueIt != bstIndex[whereCol].end()){
                matchingRows = valueIt->second;
                foundWithIndex = true;
            }
        } else if (op == "<"){
            for(auto it = bstIndex[whereCol].begin(); it != bstIndex[whereCol].end() && it->first < val; ++it){
                matchingRows.insert(matchingRows.end(), it->second.begin(), it->second.end());
                foundWithIndex = true;
            }
        } else if(op == ">"){
            for(auto it = bstIndex[whereCol].upper_bound(val); it != bstIndex[whereCol].end(); ++it){
                matchingRows.insert(matchingRows.end(), it->second.begin(), it->second.end());
                foundWithIndex = true;
            }
        } 
    } 
    
    if(!foundWithIndex){
        for(size_t i = 0; i < rows.size(); ++i){
            if((op == "<" && rows[i][whereColIndex] < val) ||
                (op == ">" && rows[i][whereColIndex] > val) || 
                (op == "=" && rows[i][whereColIndex] == val)){   
                matchingRows.push_back(i);
            }
        }
    }
        
    if(!quiet){
        for(const auto& colIdx : colIndices){
            cout << columnNames[colIdx] << " ";
        }
        cout << endl;

        for(size_t rowIndex : matchingRows){
            for(int colIdx : colIndices){
                cout << rows[rowIndex][colIdx] << " ";
            }
            cout << endl;
        }
    }

    cout << "Printed " << matchingRows.size() << " matching rows from " << tableName << endl;
}

// JOIN <table1> AND <table2> WHERE <col1> = <col2> AND PRINT <N> <printcol1> ... <printcoln>
void SQLlite::joinTables(const vector<string>& tokens){
    if(tokens.size() < 9){
        cout << "Error during JOIN: Invalid command format" << endl;
        return;
    }

    string table1Name = tokens[0];

    if(tokens[1] != "AND"){
        cout << "Error during JOIN: Expected 'AND' after first table name" << endl;
        return;
    }

    string table2Name = tokens[2];

    auto table1It = tables.find(table1Name);
    auto table2It = tables.find(table2Name);

    if(table1It == tables.end()){
        cout << "Error during JOIN: " << table1Name << " does not name a table in the database" << endl;
        return;
    }

    if(table2It == tables.end()){
        cout << "Error during JOIN: " << table2Name << " does not name a table in the database" << endl;
        return;
    }

    Table& table1 = table1It->second;
    Table& table2 = table2It->second;
    
    //where token and extract join columns
    auto whereIt = find(tokens.begin(), tokens.end(), "WHERE");
    if(whereIt == tokens.end() || distance(whereIt, tokens.end()) < 4){
        cout << "Error during JOIN: Missing or Incomplete WHERE clause" << endl;
        return;
    }

    string column1 = *(whereIt + 1);
    if(*(whereIt + 2) != "="){
        cout << "Error during JOIN: Invalid comparison operator" << endl;
        return;
    }

    string column2 = *(whereIt + 3);

    //verify join columns exist in tables
    auto col1It = find(table1.columnNames.begin(), table1.columnNames.end(), column1);
    auto col2It = find(table2.columnNames.begin(), table2.columnNames.end(), column2);
    if(col1It == table1.columnNames.end()){
        cout << "Error during JOIN: " << column1 << " does not name a column in " << table1Name << endl;
        return;
    }
    if(col2It == table2.columnNames.end()){
        cout << "Error during JOIN: " << column2 << " does not name a column in " << table2Name << endl;
        return;
    }

    int col1Index = static_cast<int>(distance(table1.columnNames.begin(), col1It));
    int col2Index = static_cast<int>(distance(table2.columnNames.begin(), col2It));

    //ensure matching column types
    if(table1.columnTypes[col1Index] != table2.columnTypes[col2Index]){
        cout << "Error during JOIN: Column types do not match for join columns" << endl;
        return;
    }

    //print token and extract print columns
    auto andPrintIt = find(tokens.begin() + 3, tokens.end(), "AND");
    if(andPrintIt == tokens.end() || distance(andPrintIt, tokens.end()) < 2 || *(andPrintIt + 1) != "PRINT"){
        cout << "Error during JOIN: Missing or incomplete PRINT clause" << endl;
        return;
    }


    int numPrintCols;
    try {
        numPrintCols = stoi(*(andPrintIt + 2));
        if(numPrintCols <= 0){
            cout << "Error during JOIN: Number of columns to print must be positive" << endl;
            return;
        }
    } catch (...) {
        cout << "Error during JOIN: Invalid number of columns to print" << endl;
        return;
    }

    if(distance(andPrintIt + 3, tokens.end()) < 2 * numPrintCols){
        cout << "Error during JOIN: Not enough columns to print" << endl;
        return;
    }


    //parse print columns
    vector<pair<string, int>> printColumnInfo; // (col name, table num)


    for(int i = 0; i < numPrintCols; ++i){
        string colName = *(andPrintIt + 3 + 2 * i);
        string tableNumStr = *(andPrintIt + 3 + 2 * i + 1);

        int tableNum;
        try {
            tableNum = stoi(tableNumStr);
            if(tableNum != 1 && tableNum != 2){
                cout << "Error during JOIN: Table indicator must be 1 or 2" << endl;
                return;
            }
        } catch (...){
            cout << "Error during JOIN: Invalid table indicator" << endl;
            return;
        }
        printColumnInfo.emplace_back(colName, tableNum);
    }

    vector<pair<int, int>> printColIndices; //table num, col indx

    for(const auto& [colName, tableNum] : printColumnInfo){
        const Table& table = (tableNum == 1) ? table1 : table2;
        const string& tableName = (tableNum == 1) ? table1Name : table2Name;

        auto colIt = find(table.columnNames.begin(), table.columnNames.end(), colName);
        if(colIt == table.columnNames.end()){
            cout << "Error during JOIN: " << colName << " does not name a column in " << tableName << endl;
            return;
        }
        
        int colIndex = static_cast<int>(distance(table.columnNames.begin(), colIt));
        printColIndices.emplace_back(tableNum, colIndex);
    }

    vector<pair<size_t, size_t>> joinedRows;

    bool table1HasHashIndex = table1.hashIndex.count(column1) > 0 && !table1.hashIndex[column1].empty();
    bool table2HasHashIndex = table2.hashIndex.count(column2) > 0 && !table2.hashIndex[column2].empty();
    bool table1HasBSTIndex = table1.bstIndex.count(column1) > 0 && !table1.bstIndex[column1].empty();
    bool table2HasBSTIndex = table2.bstIndex.count(column2) > 0 && !table2.bstIndex[column2].empty();

    if (table1HasHashIndex) {
        for (size_t rowIdx1 = 0; rowIdx1 < table1.rows.size(); ++rowIdx1) {
            const Field& joinValue1 = table1.rows[rowIdx1][col1Index];

            if (table2HasHashIndex) {
                // both have hash indices
                auto it = table2.hashIndex[column2].find(joinValue1);
                if (it != table2.hashIndex[column2].end()) {
                    for (size_t rowIdx2 : it->second) {
                        joinedRows.emplace_back(rowIdx1, rowIdx2);
                    }
                }
            } else {
                // table1 has hash, table2 doesn't
                for (size_t rowIdx2 = 0; rowIdx2 < table2.rows.size(); ++rowIdx2) {
                    const Field& joinValue2 = table2.rows[rowIdx2][col2Index];
                    if (joinValue1 == joinValue2) {
                        joinedRows.emplace_back(rowIdx1, rowIdx2);
                    }
                }
            }
        }
    } else if (table1HasBSTIndex) {
        // table1 has BST index
        for (size_t rowIdx1 = 0; rowIdx1 < table1.rows.size(); ++rowIdx1) {
            const Field& joinValue1 = table1.rows[rowIdx1][col1Index];

            if (table2HasBSTIndex) {
                // Both have BST indices
                auto it = table2.bstIndex[column2].find(joinValue1);
                if (it != table2.bstIndex[column2].end()) {
                    for (size_t rowIdx2 : it->second) {
                        joinedRows.emplace_back(rowIdx1, rowIdx2);
                    }
                }
            } else {
                // table1 has BST, table2 doesn't
                for (size_t rowIdx2 = 0; rowIdx2 < table2.rows.size(); ++rowIdx2) {
                    const Field& joinValue2 = table2.rows[rowIdx2][col2Index];
                    if (joinValue1 == joinValue2) {
                        joinedRows.emplace_back(rowIdx1, rowIdx2);
                    }
                }
            }
        }
    } else {
        // no index on table1, iterate through all rows
        for (size_t rowIdx1 = 0; rowIdx1 < table1.rows.size(); ++rowIdx1) {
            const Field& joinValue1 = table1.rows[rowIdx1][col1Index];

            // use index on table2 if available
            if (table2HasHashIndex) {
                auto it = table2.hashIndex[column2].find(joinValue1);
                if (it != table2.hashIndex[column2].end()) {
                    // add matching rows from table2 in insertion order
                    for (size_t rowIdx2 : it->second) {
                        joinedRows.emplace_back(rowIdx1, rowIdx2);
                    }
                }
            } else if (table2HasBSTIndex) {
                auto it = table2.bstIndex[column2].find(joinValue1);
                if (it != table2.bstIndex[column2].end()) {
                    // add matching rows from table2 in insertion order
                    for (size_t rowIdx2 : it->second) {
                        joinedRows.emplace_back(rowIdx1, rowIdx2);
                    }
                }
            } else {
                // No index on table2, iterate through all rows
                for (size_t rowIdx2 = 0; rowIdx2 < table2.rows.size(); ++rowIdx2) {
                    const Field& joinValue2 = table2.rows[rowIdx2][col2Index];
                    if (joinValue1 == joinValue2) {
                        joinedRows.emplace_back(rowIdx1, rowIdx2);
                    }
                }
            }
        }
    }
    
    //print join results
    if(!quiet){
        for(const auto& [colName, tableNum]: printColumnInfo){
            cout << colName << " ";
        }
        cout << endl;
    

        for(const auto& [idx1, idx2] : joinedRows){
            for(const auto& [tableNum, colIdx] : printColIndices){
                if(tableNum == 1){
                    cout << table1.rows[idx1][colIdx] << " ";
                } else {
                    cout << table2.rows[idx2][colIdx] << " ";
                }
            }
            cout << endl;
        }
    }

    cout << "Printed " << joinedRows.size() << " rows from joining " << table1Name << " to " << table2Name << endl;
}