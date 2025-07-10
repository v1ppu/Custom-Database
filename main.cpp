#include "table.h"
#include <iostream>
#include <getopt.h>
using namespace std;

void printHelp(){
    cout << "Usage: ./lite [--help] [--quiet]" << endl;
}

int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin >> std::boolalpha;
    cout << std::boolalpha;

    bool quiet = false;
    int opt;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"quiet", no_argument, 0, 'q'},
        {nullptr, 0, nullptr, 0}
    };

    while((opt = getopt_long(argc, argv, "hq", long_options, nullptr)) != -1){
        if(opt == 'h'){
            printHelp();
            return 0;
        } else if (opt == 'q'){
            quiet = true;
        }
    }

    SQLlite db(quiet);
    string command;
    do {
        if(cin.fail()){
            if(cin.eof()){
                break;
            }
            cout << "Error reading from input" << endl;
            return 1;
        }
        cout << "% ";
        
        cin >> command;
        db.processCommand(command);
    } while (command != "QUIT");
    return 0;
}