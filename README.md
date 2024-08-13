
# DATABASE MANAGEMENT SYSTEMS LINEAR HASHING

**The goal of this project is to demonstrate how to implement index structures on data stored in externam memories.**<br/>
**The project requirements are listed below.**

1. The Input File: The input relation is stored in a CSV file, i.e., each tuple is in a separate
line and fields of each record are separated by commas. Your program must assume that the
input CSV file is in the current working directory, i.e., the one from which your program is
running, and its name is Employee.csv. We have included an input CSV file with this
assignment as a sample test case for your program. Your program must create and search
hash indexes correctly for other CSV files with the same fields as the sample file.
2. Index File Creation: Your program must read the input Employee relation and build a
linear hash index for the relation using attribute id. Your program must store the hash
index in a file with the name EmployeeIndex on the current working directory. You may use
one of the methods explained for storing variable-length records and the method described
on storing pages of variable-length records in our lectures on storage management to store
records and blocks in the index file. They are also explained in Sections 9.7.2 and 9.6.2 of
Cow Book, respectively. You can reuse your code for Assignment 2 to store pages in the
index file. Your index file must be a binary data file and not a text or CSV file.
3. Index Parameters: You must use hash function h = idmod216. Your program must
increment the value of n if the average number of records per each page exceeds 70% of the
page capacity.
4. Main Memory Limitation: During the index creation, your program can keep up to
three pages plus the directory of the hash index in main memory at any time. The
submitted solutions that use more main memory will not get any points.
5. Searching the Index File: After finishing the index creation, your program must accept
an Employee id in its command line and search the index file for all records of the given id.
Like index creation, your program may use up to three pages plus the directory of the hash
index in main memory at any time. The submitted solutions that use more main memory
will not get any points for implementing lookup operation. The user of your program may
search for records of multiple ids, one id at a time.


**Setup**


## Use a database to power your API


  * Businesses
  * Reviews
  * Photos


## Database initialization


### MongoDB




USEFULL NOTES: 
In order to see the flow of the program find "DEBUG" and uncomment 
the cout call, his will print the page as it is being updated

debug1 will print the pages in main memory as it is being updated
debug2 will print the buckets in employeeIndex as it is being updated
if you want to see the overflow buckets, use a hex editor

HOW TO RUN:
g++ -std=c++11 main.cpp -o main.out
./main.out

USER FLOW:
Enter ID of desired record
Enter "quit" or "QUIT" to exit the program

PROGRAM FLOW:
a. Loading the data
    1. Open the csv file
    2. Enter records one at a time from csv file into page 1 of main memory
    3. When a page is full, it calls a function that pushes the records 
       from page 1 of main memory into the correct bucket, then checks if we 
       should split.
    4. If a records is about to be entered into a bucket that is at capacity, 
       and the bucket does not have a pre-existing overflow page, create a new
       overflow page. 
    5. When the number of records exceed 70% of total space in the buckets, move
       record from "next" into the loading area in the file (this is so that we can 
       split buckets regardless if the bucket size is larger than 3 pages)
    6. This process is repeated until you reach the end of the csv file.

b. Findind the data
    1. Search pages from the last search
    2. If the data is not found, use the id to find the correct bucket
    3. Insert records into page 3
    4. Search the page in main memory when it is full and when you are at the end of the bucket