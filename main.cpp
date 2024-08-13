/*
Skeleton code for linear hash indexing
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;
/*
Skeleton code for storage and buffer management
*/

bool all_digits(string& s){
    for (char c : s) {
        int v = c;
        if (!(c >= 48 && c <= 57)) {
            return false;
        }
    }
    return true;
}

int main(int argc, char* const argv[]) {

    // Create the EmployeeRelation file from Employee.csv
    cout << "Linear Hashing\n";
    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");
    
    // Loop to lookup IDs until user is ready to quit

    bool quit = false;
    auto is_digit_check = [] (unsigned char c) { return std::isdigit(c); };
    while(1){
        cout << "Lookup ID: ";
        string input;
        cin >> input;
        
        if(input == "quit" || input == "QUIT"){
            cout << "exiting program\n";
            break;
        }
        else if(input.size() != 8 || !all_digits(input)){
            cout << "You did not enter a valid ID!\n";
        }
        else{
            Record r = emp_index.findRecordById(atoi(input.data()));
            if(r.id == 0){
                cout << "\n*****RECORD WITH ID " << input << " DOES NOT EXIST IN THIS DATABASE*****\n";
            }
            else{
                r.print();
            } 
        }
    }
    emp_index.freeDynamicallyAllocatedMemory();
    return 0;
}







