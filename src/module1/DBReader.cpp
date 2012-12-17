#include "DBReader.h"

DBReader::DBReader(char* dataFileName_, char* indexFileName_):
    dataFileName(dataFileName_),
    indexFileName(indexFileName_)
{
    dataSize = 0;
}

void DBReader::open(){
    // open ffindex
    dataFile = fopen(dataFileName, "r");
    indexFile = fopen(indexFileName, "r");

    if( indexFile == NULL) { fferror_print(__FILE__, __LINE__, "run_sw", indexFileName);  exit(EXIT_FAILURE); }
    if( dataFile == NULL) { fferror_print(__FILE__, __LINE__, "run_sw", dataFileName);  exit(EXIT_FAILURE); }

    data = ffindex_mmap_data(dataFile, &dataSize);

    index = ffindex_index_parse(indexFile, 0);
    
    if(index == NULL)
    {
        fferror_print(__FILE__, __LINE__, "ffindex_index_parse", indexFileName);
        exit(EXIT_FAILURE);
    }

    size = index->n_entries;

    // generate id -> ffindex_entry mapping
    id2entry = new ffindex_entry*[size];

    for (int i = 0; i < size; i++){
        id2entry[i] = ffindex_get_entry_by_index(index, i);
    }
}

void DBReader::close(){

    delete[] id2entry;

    fclose(dataFile);
    fclose(indexFile);

}

char* DBReader::getData (int id){
    ffindex_entry_t* entry = id2entry[id];
    return ffindex_get_data_by_entry(data, entry);
}

char* DBReader::getDataByDBKey (char* key){
    return ffindex_get_data_by_name(data, index, key);
}

size_t DBReader::getSize (){
    return size;
}

char* DBReader::getDbKey (int id){
    return &(id2entry[id]->name[0]);
}
