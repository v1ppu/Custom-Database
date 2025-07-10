# SQLlite-Database

A lightweight, in-memory SQL-like database engine implemented in C++. This project supports basic table operations, row insertion, deletion, indexing, and simple join queries, all via a custom command-line interface.

## Features

- Create and remove tables with custom column types (`string`, `int`, `double`, `bool`)
- Insert and delete rows with flexible conditions
- Print selected columns with filtering (WHERE clause)
- Generate hash and BST indexes for fast lookups
- Perform simple equi-joins between tables
- Quiet mode for minimal output
- Command-driven interface (see example below)

## Example Usage

Given the following input file ([example.txt](example.txt)):

```
CREATE classroom 3 string string bool name school Male
INSERT INTO classroom 5 ROWS
Jack Yale true
Jessica Harvard false
Adit MIT true
Emma MIT false
Hamilton Stanford true
DELETE FROM classroom WHERE person = Adit
GENERATE FOR classroom hash INDEX ON school
PRINT FROM classroom 2 name school WHERE Male = false
CREATE universities 3 string bool bool Name private large
INSERT INTO universities 2 ROWS
Berkley false true
Stanford true true
JOIN universities AND classroom WHERE Name = school AND PRINT 3 Name 1 school 2 Male 1
REMOVE universities
REMOVE classroom
QUIT
```

The output ([example_out.txt](example_out.txt)) will be:

```
% % New table classroom with column(s) name school Male created
% Added 5 rows to classroom from position 0 to 4
% Deleted 1 rows from classroom
% Generated hash index for table classroom on column school, with 4 distinct keys
% person emotion 
name school 
Jessica Harvard
Emma MIT
Printed 2 matching rows from classroom
% New table universities with column(s) Name private large created
% Added 2 rows to pets from position 0 to 1
% Name school Male
Stanford Stanford false 
Printed 1 rows from joining universities to classroom
% Table universities removed
% Table classroom removed
% Thanks for using!
```

## Building

This project uses a `Makefile` for building. To compile:

```sh
make lite
```

This will produce an executable (e.g., `lite.exe` on Windows).

## Running

You can run the program and provide commands interactively or via input redirection:

```sh
./lite < example.txt
```

### Command-Line Options

- `--help` : Show usage information
- `--quiet` : Suppress detailed output, only show essential information

## File Structure

- `main.cpp` — Entry point, command parsing
- `table.h` / `table.cpp` — Table and database logic
- `field.h` / `field.cpp` — Field and type handling
- `example.txt` — Example input file/commands
- `example_out.txt` — Example output
 - `Makefile` — Compiles project

Note: Due to being part of a school project, and under copyright, crucial files for `Field.cpp`, and `Field.h` will never be avaliable, and thus running this code under this GitHub will not work. The code provided is entirely my own, and provides the main logic behind the database design. 