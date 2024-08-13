#include<cmath>

#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
using namespace std;

class Record {
public:
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }
};


class LinearHashIndex {

private:
    const int BLOCK_SIZE = 4096;
    const int MAX_RECORD_SIZE = 720; // 8 + 200 + 500 + 8 + 3 comas + \n
    const int MAX_FIELD_SIZE = 500; // max bio length
    const int NUM_RECORDS = 50; // number of records to be inserted into File 
    const int MAX_PAGE_NUM = 256;
    const int MINUS = -1;
    const int TWOONE = 21;
   

    vector<int>* blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;  // The number of indexes in blockDirectory currently being used
    int b;	// The number of least-significant-bits of h(id) to check. Will need to increase b once n > 2^b
    int numRecords;    // Records currently in index. Used to test whether to increase n
    int nextFreeBlock; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;      // Name of index file

    int next; // next bucket to split
    int recordSize; // size of the record currently being manipulated
    int recordsProcessed; // tracks the number of records inserted into a page since last action
    int pagesCreated; // tracks the number of pages created since last action
    int recordsInBucket[256]= { 0 };
    

    int flags[5]; 
    // flags[0] = flag to tell which page to use for that process
    // flags[1] = force reset the page
    // flags[3] = flag to tell the program to split




    // buffers
    char* recordBuff; 
    char* fieldBuff;
    int* intBuff;
    FILE* csvFile;
    FILE* indexFile;
    vector<std::string>* vectorFields;
    vector<int>* vectorOffsets;
    vector<int>* vectorSizes;
    Record* recordPtr;


    // memory management and slot directory
    vector<vector<Record>*>* pages; 
    vector<Record>* pagePtr; 
    int recordsIn[3]; // number of entries in the page
    int bytesIn[3]; // number of bytes in the page
    
    int hashKey(int id){

        int key = id % 216;
        int pos = key % (int)(pow(2,b));

        if(pos >= n){
            // if bucket dont exist yet
            pos = pos % (int)(pow(2,b-1));
        }

        return pos;
    }

    // Add record to the index in the correct block, creating a overflow block if necessary
    void appendRecordAtEndOf(Record record, int index){

        // serialize the record
        vectorFields = new vector<std::string>;
        vectorSizes = new vector<int>;
        vectorOffsets = new vector<int>;
        memset(fieldBuff, 0x00, MAX_FIELD_SIZE);

        sprintf(fieldBuff, "%d", record.id);
        vectorFields->push_back(fieldBuff);
        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
        vectorOffsets->push_back(5 * sizeof(int));
        vectorOffsets->push_back(vectorSizes->at(0) + vectorOffsets->at(0));

        sprintf(fieldBuff, "%s", record.name.data());
        vectorFields->push_back(fieldBuff);
        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
        vectorOffsets->push_back(vectorSizes->at(1) + vectorOffsets->at(1));

        sprintf(fieldBuff, "%s", record.bio.data());
        vectorFields->push_back(fieldBuff);
        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
        vectorOffsets->push_back(vectorSizes->at(2) + vectorOffsets->at(2));
        
        sprintf(fieldBuff, "%d", record.manager_id);
        vectorFields->push_back(fieldBuff);
        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
        vectorOffsets->push_back(vectorSizes->at(3) + vectorOffsets->at(3));

        // go to start of bucket
        fseek(indexFile, blockDirectory->at(index), SEEK_SET);
        int currentBlockStart = ftell(indexFile);
        
        // go to the end of the bucket by finding the -1
        int* offsets = new int[5*sizeof(int)];
        memset(offsets, 0x00, 5*sizeof(int));
        while(offsets[0] != -1 ){
            
            fread(offsets, sizeof(int), 5, indexFile);
            fseek(indexFile, -20, SEEK_CUR);
            
            if(offsets[0] == 20){
                // go to the next record in the bucket
                fseek(indexFile, offsets[4], SEEK_CUR);
            
            }
            else if(offsets[0] == 21){
                // go to overflow page
                
                fseek(indexFile, blockDirectory->at(index)+BLOCK_SIZE-5, SEEK_SET);

                fread(offsets, sizeof(int), 1, indexFile);

                fseek(indexFile, offsets[0], SEEK_SET);
                currentBlockStart = ftell(indexFile);
                offsets[0] = 0;
            }
            

            // if block is full, create an overflow page or go to the pre-existing page
            if(offsets[0] == -1 && (4+ vectorOffsets->at(4)) > (currentBlockStart + BLOCK_SIZE - ftell(indexFile))){

                // write the indicator that the bucket now full
                fwrite(&TWOONE, sizeof(int), 1, indexFile);

                // check if this bucket has a pre-existing overflow
                fseek(indexFile, blockDirectory->at(index)+BLOCK_SIZE-5, SEEK_SET);
                int* offsets2 = new int[5*sizeof(int)];
                memset(offsets2, 0x00, 5*sizeof(int));
                fread(offsets2, sizeof(int), 1, indexFile);
                if(offsets2[0] == 0){
                    // if no pre-existing overflow exist, create one
                    fseek(indexFile, blockDirectory->at(index)+BLOCK_SIZE-5, SEEK_SET);
                    fwrite(&nextFreeBlock, sizeof(int), 1, indexFile);
                    fseek(indexFile, nextFreeBlock, SEEK_SET);
                    cout << "Creating overflow page for bucket #" << index << " at page " << nextFreeBlock / BLOCK_SIZE +1 << " index " << nextFreeBlock << "\n";
                    nextFreeBlock += BLOCK_SIZE;
                    offsets2[4] = 0;
                }
                else{
                    // otherwise, go to pre-existing overflow for that bucket
                    fseek(indexFile, offsets2[0], SEEK_SET);
                }
            }
        }
        
        
       
        for(int i=0;i<5;i++) {
            fwrite(&(vectorOffsets->at(i)),sizeof(int), 1,indexFile);
        }
        // write fields into the file
        for(int i=0;i<4;i++) {
            fwrite(vectorFields->at(i).c_str(), sizeof(char),vectorSizes->at(i), indexFile);
        }
        fwrite(&MINUS, sizeof(int), 1, indexFile);
        fclose(indexFile);
        indexFile = fopen("EmployeeIndex.dat", "r+b");
        
        recordsInBucket[index]++;

        // debug2 code that prints bucket info everytime it updates 
        // printf("Inserting %s | #%d | %d\n", record.name.data() , index+1,  ftell(indexFile));
        // cout << "Printing buckets: ";
        // int total = 0;
        // for(int i = 0; i < blockDirectory->size(); i++){
        //     printf(" #%d[%d]", i+1,recordsInBucket[i]);
        // }
        // cout << endl;
    }

    

    // Insert new record into index
    void insertRecord(Record record) {
        // No records written to index yet
        if (numRecords == 0) {
            // Initialize index with first blocks (start with 4)
            blockDirectory = new vector<int>;
            for(int i = 0; i < 4 ;i++){
                
                cout << "Creating new bucket #" << i+1 << " at page " << blockDirectory->size() << " index " << nextFreeBlock << endl;

                fseek(indexFile, nextFreeBlock, SEEK_SET);
                fwrite(&MINUS, sizeof(int), 1, indexFile);
                fseek(indexFile, BLOCK_SIZE - 4, SEEK_CUR);
                fwrite(&MINUS, sizeof(int), 1, indexFile);
                blockDirectory->push_back(nextFreeBlock);
                nextFreeBlock += BLOCK_SIZE;
            }
            fseek(indexFile, (MAX_PAGE_NUM)*BLOCK_SIZE, SEEK_SET);
            // Initialize loading dock at the footer
            for(int i = 0; i < 10 ;i++){
                fwrite(&MINUS, sizeof(int), 1, indexFile);
                fseek(indexFile, BLOCK_SIZE-4, SEEK_CUR);
            }
            fputc('\0', indexFile);
            fclose(indexFile);
            indexFile = fopen("EmployeeIndex.dat", "r+b");
        }

       // Add record to the index in the correct block, creating a overflow block if necessary
        appendRecordAtEndOf(record, hashKey(record.id));

        numRecords++;
        // cout << "numRecords is " << numRecords << ", checking split...\n";
        
        // check for splitting
        if((float)numRecords/(float)(blockDirectory->size()*8) > 0.7){
            
            
            
            n++;

            if(n > pow(2,b)){
                b++;
                next = 0;
            }
            
            next++;
            
           

            // create new bucket at the next free block
            fseek(indexFile, nextFreeBlock , SEEK_SET);
            fwrite(&MINUS, sizeof(int), 1, indexFile);
            blockDirectory->push_back(nextFreeBlock);
            cout << "SPLITITNG BUCKET #" << next << " | Creating new bucket #" << blockDirectory->size() << " at page " << nextFreeBlock / BLOCK_SIZE + 1 << " index " << nextFreeBlock << "\n";
            nextFreeBlock += BLOCK_SIZE;

            
            
            


            // read record from split bucket into loading area


            fseek(indexFile, (blockDirectory->at(next-1)), SEEK_SET);
            int *intBuff2;
            intBuff2 = new int[5*sizeof(int)];
            memset(intBuff2, 0x00, 5*sizeof(int));
            
            flags[1] = 1; //2. 
            while(intBuff2[0] != -1 ){
                fread(intBuff2, sizeof(int), 5, indexFile);
                // cout << "intbuff is: " << intBuff2[0] << endl;
                int remember2 = intBuff2[0];
                if(intBuff2[0] == 20 ){
                    
                    // reading id
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[1] - intBuff2[0]), indexFile);
                    vectorFields = new vector<string>;
                    vectorFields->push_back(fieldBuff);

                    // reading name
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[2] - intBuff2[1]), indexFile);
                    vectorFields->push_back(fieldBuff);
                    // cout << "reading in " << fieldBuff << endl;
                    
                    // reading bio
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[3] - intBuff2[2]), indexFile);
                    vectorFields->push_back(fieldBuff);

                    // reading manager_id
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[4] - intBuff2[3]), indexFile);
                    vectorFields->push_back(fieldBuff);

                    // get record size
                    recordSize = intBuff2[4]-intBuff2[0];
                    flags[3] = 0;
                    flags[0] = 2; // use page 2 for splitting
                    recordPtr = new Record(*vectorFields);
                    processRecord(*recordPtr);

                }
                else if(intBuff2[0] == 21){
                    fseek(indexFile, blockDirectory->at(next-1)+BLOCK_SIZE-5, SEEK_SET);
                    fread(intBuff2, sizeof(int), 1, indexFile);
                    fseek(indexFile, intBuff2[0], SEEK_SET);
                    
                    
                }

                if(recordsIn[1] == 8 && intBuff2[0] == -1){
                    break;
                }
            
                if((recordsIn[1] == 8 || intBuff2[0] == -1) && remember2 != 21){
                    int remember = ftell(indexFile);

                    for( int i = 0; i < recordsIn[1]; i++){

                        fseek(indexFile, (MAX_PAGE_NUM*BLOCK_SIZE), SEEK_SET);

                        int* intBuff3 = new int[5*sizeof(int)];
                        memset(intBuff3, 0x00, 5*sizeof(int));
                        while(intBuff3[0] != -1 ){
                            fread(intBuff3, sizeof(int), 5, indexFile);
                            fseek(indexFile, -20, SEEK_CUR);
                    
                            if(intBuff3[0] == 20){
                                // check the next space
                                fseek(indexFile, intBuff3[4], SEEK_CUR);
                            }
                        

                        }

                        vectorFields = new vector<std::string>;
                        vectorSizes = new vector<int>;
                        vectorOffsets = new vector<int>;
                        memset(fieldBuff, 0x00, MAX_FIELD_SIZE);

                        // parse data from the page into vectorFields, vectorSizes, and generate offsets
                        sprintf(fieldBuff, "%d", pages->at(1)->at(i).id);
                        vectorFields->push_back(fieldBuff);
                        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
                        vectorOffsets->push_back(5 * sizeof(int));
                        vectorOffsets->push_back(vectorSizes->at(0) + vectorOffsets->at(0));

                        sprintf(fieldBuff, "%s", pages->at(1)->at(i).name.data());
                        vectorFields->push_back(fieldBuff);
                        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
                        vectorOffsets->push_back(vectorSizes->at(1) + vectorOffsets->at(1));

                        sprintf(fieldBuff, "%s", pages->at(1)->at(i).bio.data());
                        vectorFields->push_back(fieldBuff);
                        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
                        vectorOffsets->push_back(vectorSizes->at(2) + vectorOffsets->at(2));
                        
                        sprintf(fieldBuff, "%d", pages->at(1)->at(i).manager_id);
                        vectorFields->push_back(fieldBuff);
                        vectorSizes->push_back((strlen(fieldBuff)) * sizeof(char));
                        vectorOffsets->push_back(vectorSizes->at(3) + vectorOffsets->at(3));
                        
                        // go to loading aread
                        // fseek(indexFile, (MAX_PAGE_NUM)*BLOCK_SIZE, SEEK_SET);
                        
                        // printf("writing %s into index #%d\n", pages->at(1)->at(i).name.data() , ftell(indexFile));
                        for(int i=0;i<5;i++) {
                            fwrite(&(vectorOffsets->at(i)),sizeof(int), 1,indexFile);
                        }
                        // write fields into the file
                        for(int i=0;i<4;i++) {
                            fwrite(vectorFields->at(i).c_str(), sizeof(char),vectorSizes->at(i), indexFile);
                        }
                        fwrite(&MINUS, sizeof(int), 1, indexFile);
                        fclose(indexFile);
                        indexFile = fopen("EmployeeIndex.dat", "r+b");

                    }
                    fseek(indexFile, remember, SEEK_SET);
                }
            }

            // free the split block
            fseek (indexFile, (long int)(blockDirectory->at(next-1)), SEEK_SET);
            fwrite(&MINUS, sizeof(int), 1, indexFile);
            fclose(indexFile);
            indexFile = fopen("EmployeeIndex.dat", "r+b");

            // reset counter
            recordsInBucket[next-1] = 0;
            
            // read record from loading area into split buckets
            fseek(indexFile, (MAX_PAGE_NUM*BLOCK_SIZE), SEEK_SET);
            intBuff2 = new int[5*sizeof(int)];
            memset(intBuff2, 0x00, 5*sizeof(int));
            flags[1] = 2; // clear page 2
            while(intBuff2[0] != -1){
                fread(intBuff2, sizeof(int), 5, indexFile);
                if(intBuff2[0] == 20){
                    // reading id
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[1] - intBuff2[0]), indexFile);
                    vectorFields = new vector<string>;
                    vectorFields->push_back(fieldBuff);

                    // reading name
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[2] - intBuff2[1]), indexFile);
                    vectorFields->push_back(fieldBuff);
                    
                    // reading bio
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[3] - intBuff2[2]), indexFile);
                    vectorFields->push_back(fieldBuff);

                    // reading manager_id
                    memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                    fread(fieldBuff, sizeof(char), (intBuff2[4] - intBuff2[3]), indexFile);
                    vectorFields->push_back(fieldBuff);

                    // get record size
                    recordSize = intBuff2[4]-intBuff2[0];
                    flags[3] = 0;
                    flags[0] = 2; // use page 2 for splitting
                    recordPtr = new Record(*vectorFields);
                    // recordPtr->print();

                    // cout << "reading from load " << recordPtr->name << endl;
                    processRecord(*recordPtr);
                }

                // push records into correct blocks

                if(recordsIn[1] == 8 && intBuff2[0] == -1){
                    break;
                }
                
                // runs twice when splitting a bucket size 8
                if(recordsIn[1] == 8 || intBuff2[0] == -1){
                    int remember = ftell(indexFile);
                    int i = 0;
                    
                    
                    for(int i = 0; i < recordsIn[1]; i++){
                        appendRecordAtEndOf(pages->at(1)->at(i), hashKey(pages->at(1)->at(i).id) );

                    }
                    fseek(indexFile, remember, SEEK_SET);

                }

            }
            // free the loading area
            fseek(indexFile, (MAX_PAGE_NUM)*BLOCK_SIZE, SEEK_SET);
            fwrite(&MINUS, sizeof(int), 1, indexFile);
            fclose(indexFile);
            indexFile = fopen("EmployeeIndex.dat", "r+b");

        }

    }

    // Process record in main memory
    void processRecord(Record record){

        if (recordsProcessed == 0) {
            // Initialize first block
            pages = new vector<vector<Record>*>;
            for(int i = 0; i < 3; i++){
                pagePtr = new vector<Record>;
                pages->push_back(pagePtr);
                recordsIn[i] = 0;
                bytesIn[i] = 0;
            }
            
        }
        
        flags[0]--;
        // create a page if there is no space
        if(bytesIn[flags[0]] + recordSize >= BLOCK_SIZE || flags[1] != 0){
            // cout << "creating new page at " << flags[0] << endl;
            pages->at(flags[0]) = new vector<Record>;

            // initialize bytes and reset
            recordsIn[flags[0]] = 0;
            bytesIn[flags[0]] = 0;
            flags[1] = 0;
        }

        // enter record into block when there is space
        pages->at(flags[0])->push_back(record);
        recordsIn[flags[0]]++;
        bytesIn[flags[0]] += recordSize;
        recordsProcessed++;

        // debug1 code that prints records in main memory everytime it updates 
        // cout << "Records processed #" << recordsProcessed << "\nPage x info: bytes | entries \nPage 1 info: " << bytesIn[0] << " | " << recordsIn[0] << "\nPage 2 info: " << bytesIn[1] << " | " << recordsIn[1] << "\nPage 3 info: " << bytesIn[2] << " | " << recordsIn[2] << "\n";
    }

public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        b = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        
        fName = indexFileName;

        recordsProcessed = 0;
        recordSize = 0;
        flags[0] = 0;
        flags[1] = 0;
        flags[0] = 0;
        flags[3] = 0;
        flags[1] = 0;
        bytesIn[0] = 0;
        bytesIn[1] = 0;
        bytesIn[2] = 0;
        recordsIn[0] = 0;
        recordsIn[1] = 0;
        recordsIn[2] = 0;
        next = 0;
        

        // Create your EmployeeIndex file and write out the initial 4 buckets
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately

        indexFile = fopen(indexFileName.append(".dat").data(), "wb");
        nextFreeBlock = 0;
      
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {

        // initialize buffers
        recordBuff = (char*)malloc(sizeof(char) * MAX_RECORD_SIZE);
        memset(recordBuff, 0x00, MAX_RECORD_SIZE);

        fieldBuff = (char*)malloc(sizeof(char) * MAX_FIELD_SIZE);
        memset(fieldBuff, 0x00, MAX_FIELD_SIZE);


        // open file
        csvFile = fopen(csvFName.data(), "r");


        // create records;
        for(int i = 0; i < NUM_RECORDS; i++){
            
            fgets(recordBuff,MAX_RECORD_SIZE, csvFile);
            
            recordSize = strlen(recordBuff) - 5; // [,,,\n]
            vectorFields = new vector<std::string>;
            
            // get id
            // fieldBuff = strtok(recordBuff, ",");
            vectorFields->push_back(strtok(recordBuff, ","));

            // get name and bio
            for(int i = 0; i < 3; i++){
                // fieldBuff = strtok(NULL, ",");
                vectorFields->push_back(strtok(NULL, ","));
            }
            
            // get manager_id
            // fieldBuff = strtok(NULL, ",");
            // fieldBuff[9] = 0; //remove the "\n" so that strlen works properly
            // vectorFields->push_back(fieldBuff);
            vectorFields->at(3)[9] = 0;
            
            // create record and insert it into main memory
            recordPtr = new Record(*vectorFields);
            flags[0] = 1; // use page 1
            processRecord(*recordPtr);
            
            // when a page is full, load the page from main memory into employee index
            if(recordsIn[0] == 8 || i == NUM_RECORDS-1 ){ 
                for(int i = 0;i < recordsIn[0]; i++){
                    insertRecord(pages->at(0)->at(i));
                }
                
            }

            
        }
        
        fclose(csvFile);
        // fclose(indexFile);
        // indexFile = fopen("EmployeeRelation.dat", "rb");
        // cout << pagesCreated << " new pages required to create the data file.\n";
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
         int bucket = blockDirectory->at(hashKey(id)); // Find bucket address at blockDirectory

        // Grab records from bucket
        intBuff = new int[5 * sizeof(int)];           // Create an empty 20bytes memory called intBuff
        memset(intBuff, 0x00, 5 * sizeof(int));       // Initialize intBuff with zeros
        indexFile = fopen("EmployeeIndex.dat", "rb"); // Read as binary file
        // vectorFields->clear();                        // Clean vectorFields Contents

        fseek(indexFile, blockDirectory->at(hashKey(id)), SEEK_SET); // Seek to desired bucket

        // Keep reading until you find -1, which means the last record.
        while (intBuff[0] != -1){
            fread(intBuff, sizeof(int), 5, indexFile); // Read the first 20 bytes of the page

            if (intBuff[0] == 20){

                vectorFields = new vector<string>;  

                memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                fread(fieldBuff, sizeof(char), (intBuff[1] - intBuff[0]), indexFile); 
                vectorFields->push_back(fieldBuff);

                // reading name
                memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                fread(fieldBuff, sizeof(char), (intBuff[2] - intBuff[1]), indexFile);
                vectorFields->push_back(fieldBuff);

                // reading bio
                memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                fread(fieldBuff, sizeof(char), (intBuff[3] - intBuff[2]), indexFile);
                vectorFields->push_back(fieldBuff);


                
                // reading manager_id
                memset(fieldBuff, 0x00, MAX_FIELD_SIZE);
                fread(fieldBuff, sizeof(char), (intBuff[4] - intBuff[3]), indexFile);
                vectorFields->push_back(fieldBuff);

                flags[0] = 3; // Use page 3 of the reserved memory
                recordPtr = new Record(*vectorFields);
                processRecord(*recordPtr);
            }
            else if (intBuff[0] > 21){ // That means we got the address to the overflow page
                // Seek to overflow page
                fseek(indexFile, blockDirectory->at(hashKey(id))+BLOCK_SIZE-5, SEEK_SET);

                fread(fieldBuff, sizeof(int), 1, indexFile);

                fseek(indexFile, fieldBuff[0], SEEK_SET);
                fieldBuff[0] = 0;
            }


            if(intBuff[0] != 21 && (intBuff[0] == -1 || recordsIn[2] == 8)){
                for(int i = 0; i < recordsIn[2]; i++){
                    if(pages->at(2)->at(i).id == id){
                        
                        cout << "*****RECORD FOUND IN BUCKET # " << hashKey(id) + 1 << " *****\n";
                        return pages->at(2)->at(i);
                    }
                }
            }

           
            
            // If not found, seek to the next record, (minus 28 because the pointer is at 28 after checking the id)
        }
        recordPtr->id = 0;
        return *recordPtr;
    }






    


    void freeDynamicallyAllocatedMemory(){

        fclose(indexFile);
        delete vectorFields;
        delete vectorOffsets;
        delete vectorSizes;
        delete intBuff;
        delete recordPtr;
        delete pagePtr;
        delete pages; 
    }
};
