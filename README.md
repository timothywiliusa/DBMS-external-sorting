DATABASE MANAGEMENT SYSTEMS LINEAR HASHING

USEFULL NOTE: 
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