//
// Created by lars on 08.06.15.
//

#include <sys/time.h>
#include "SetCover3.h"
#include "Util.h"
#include "Debug.h"
#include "AffinityClustering.h"
#include "AlignmentSymmetry.h"

SetCover3::SetCover3(DBReader * seqDbr, DBReader * alnDbr, float seqIdThr, float coverage, int threads){
    this->seqDbr=seqDbr;
    this->alnDbr=alnDbr;
    this->seqIdThr=seqIdThr;
    this->coverage=coverage;
    this->dbSize=alnDbr->getSize();
    this->threads=threads;
}

std::list<set *>  SetCover3::execute() {
    //time
    struct timeval start, end;
    gettimeofday(&start, NULL);
    ///time
    clustersizes=new int[dbSize];
    memset(clustersizes,0,dbSize);

    const char * data = alnDbr->getData();
    size_t dataSize = alnDbr->getDataSize();
    size_t elementCount = Util::count_lines(data, dataSize);
    unsigned int * elements = new unsigned int[elementCount];
    unsigned int ** elementLookupTable = new unsigned int*[dbSize];
    size_t *elementOffsets = new size_t[dbSize + 1];
    unsigned int* maxClustersizes=new unsigned int [threads];
    memset(maxClustersizes,0,threads);
    unsigned int * seqDbrIdToalnDBrId= new unsigned int[dbSize];
#pragma omp parallel
    {
    int thread_idx = 0;
#ifdef OPENMP
    thread_idx = omp_get_thread_num();
#endif
        maxClustersizes[thread_idx]=0;
#pragma omp for schedule(dynamic, 1000)
    for(size_t i = 0; i < dbSize; i++) {
        char *clusterId = seqDbr->getDbKey(i);
        const size_t alnId = alnDbr->getId(clusterId);
        seqDbrIdToalnDBrId[i]=alnId;
        char *data = alnDbr->getData(alnId);
        size_t dataSize = alnDbr->getSeqLens()[alnId];
        size_t elementCount = Util::count_lines(data, dataSize);
        elementOffsets[i] = elementCount;
        maxClustersizes[thread_idx]=std::max((unsigned int)elementCount,maxClustersizes[thread_idx]);
        clustersizes[i]=elementCount;
    }

    }
    //time
    gettimeofday(&end, NULL);
    int sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for Parallel read in: " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time
    maxClustersize=0;
    for (int j = 0; j < threads; ++j) {
        maxClustersize=std::max(maxClustersize,maxClustersizes[j]);
    }
    SetCover3::initClustersizes();
    //time
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for Maximum determination: " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time
    // make offset table
    AlignmentSymmetry::computeOffsetTable(elementOffsets, dbSize);
    //time
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for offset table: " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time
    // set element edge pointers by using the offset table
    AlignmentSymmetry::setupElementLookupPointer(elements, elementLookupTable, elementOffsets, dbSize);
    //time
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for lookuppointer: " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time
    // fill elements
    AlignmentSymmetry::readInData(alnDbr, seqDbr, elementLookupTable);
    // set element edge pointers by using the offset table
    AlignmentSymmetry::setupElementLookupPointer(elements, elementLookupTable, elementOffsets, dbSize);
    //time
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for Read in: " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time

    std::list<set *> result;
    size_t n = seqDbr->getSize();
    int* assignedcluster=new int[n];
    memset(assignedcluster, -1, sizeof(int)*(n));
    float* bestscore=new float[n];
    memset(bestscore, -10, sizeof(float)*(n));
    char *similarity = new char[255+1];




    //time
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for Clustersize insertion " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time
    //delete from beginning
    for (int cl_size = dbSize-1; cl_size >= 0; cl_size--) {
            int representative =sorted_clustersizes[cl_size];
            if(representative<0){
                continue;
            }
//          Debug(Debug::INFO)<<alnDbr->getDbKey(representative)<<"\n";
            removeClustersize(representative);
            assignedcluster[representative]=representative;
            char *data = alnDbr->getData(seqDbrIdToalnDBrId[representative]);
            //delete clusters of members;
            size_t elementSize = (elementOffsets[representative +1] - elementOffsets[representative]);
            for(size_t elementId = 0; elementId < elementSize; elementId++) {

                const unsigned int elementtodelete = elementLookupTable[representative][elementId];
                const unsigned int currElementSize = (elementOffsets[elementtodelete + 1] -
                                                      elementOffsets[elementtodelete]);


                //


                Util::parseByColumnNumber(data, similarity, 4); //column 4 = sequence identity
                data = Util::skipLine(data);
                float seqId = atof(similarity);
                if (seqId > bestscore[elementtodelete]) {
                    assignedcluster[elementtodelete] = representative;
                    bestscore[elementtodelete] = seqId;
                }
                if (elementtodelete == representative) {
                    continue;
                }
                if (clustersizes[elementtodelete] < 1) {
                    continue;
                }

                removeClustersize(elementtodelete);
            }

                for(size_t elementId = 0; elementId < elementSize; elementId++) {
                    bool representativefound = false;
                    const unsigned int elementtodelete = elementLookupTable[representative][elementId];
                    const unsigned int currElementSize = (elementOffsets[elementtodelete +1] - elementOffsets[elementtodelete]);
                    if (elementtodelete == representative) {
                        clustersizes[elementtodelete] = -1;
                        continue;
                    }
                    if (clustersizes[elementtodelete] < 0) {
                        continue;
                    }
                    clustersizes[elementtodelete] = -1;
                //decrease clustersize of sets that contain the element
                for(size_t elementId2 = 0; elementId2 < currElementSize; elementId2++) {
                    const unsigned int elementtodecrease = elementLookupTable[elementtodelete][elementId2];
                    if(representative == elementtodecrease){
                        representativefound=true;
                    }
                    if(clustersizes[elementtodecrease]==1) {
                        Debug(Debug::ERROR)<<"there must be an error: "<<alnDbr->getDbKey(elementtodelete)<<" deleted from "<<alnDbr->getDbKey(elementtodecrease)<<" that now is empty, but not assigned to a cluster\n";
                    }else if (clustersizes[elementtodecrease]>0) {
                        decreaseClustersize(elementtodecrease);
                    }
                }
                if(!representativefound){
                    Debug(Debug::ERROR)<<"error with cluster:\t"<<alnDbr->getDbKey(representative)<<"\tis not contained in set:\t"<<alnDbr->getDbKey(elementtodelete)<<".\n";
                }

            }




    }
    //delete unnecessary datastructures
    delete [] elementLookupTable;
    delete [] elements;
    delete [] elementOffsets;
    delete [] maxClustersizes;
    delete [] seqDbrIdToalnDBrId;
    delete [] bestscore;
    delete [] sorted_clustersizes;
    delete [] clusterid_to_arrayposition;
    delete [] borders_of_set;

//time
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for Cluster computation: " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time
    set* sets = new set[n];
    memset(sets, 0, sizeof(set *)*(n));
    for(size_t i = 0; i < n; i++) {
        AffinityClustering::add_to_set(i,&sets[assignedcluster[i]],assignedcluster[i]);
    }

    for(size_t i = 0; i < n; i++) {
        set * max_set = &sets[i];
        if (max_set->elements == NULL)
            continue;
        result.push_back(max_set); // O(1)
    }
    //time
    gettimeofday(&end, NULL);
    sec = end.tv_sec - start.tv_sec;
    Debug(Debug::INFO) << "\nTime for preparing cluster sets: " << (sec / 60) << " m " << (sec % 60) << "s\n\n";
    gettimeofday(&start, NULL);
    //time
    return result;
}




void SetCover3::initClustersizes(){
    int * setsize_abundance=new int[maxClustersize+1];
    memset(setsize_abundance,0,sizeof(int)*(maxClustersize+1));
    //count how often a set size occurs
    for (int i = 0; i < dbSize; ++i) {
        setsize_abundance[clustersizes[i]]++;
    }
    //compute offsets
    borders_of_set= new int [maxClustersize+1];
    borders_of_set[0]=0;
    for (int i = 1; i < maxClustersize+1; ++i) {
        borders_of_set[i]=borders_of_set[i-1]+setsize_abundance[i-1];
    }
    //fill array
    sorted_clustersizes =new int [dbSize];
    memset(sorted_clustersizes,0,sizeof(int)*dbSize);
    clusterid_to_arrayposition=new int[dbSize];
    memset(clusterid_to_arrayposition,0,sizeof(int)*dbSize);
    //reuse setsize_abundance as offset counter
    memset(setsize_abundance,0,sizeof(int)*(maxClustersize+1));
    for (int i = 0; i < dbSize; ++i) {
        int position=borders_of_set[clustersizes[i]]+setsize_abundance[clustersizes[i]];
        sorted_clustersizes[position]=i;
        clusterid_to_arrayposition[i]=position;
        setsize_abundance[clustersizes[i]]++;
    }
}


void SetCover3::removeClustersize(int clusterid){
    clustersizes[clusterid]=0;
    sorted_clustersizes[clusterid_to_arrayposition[clusterid]]=-1;
    clusterid_to_arrayposition[clusterid]=-1;
}

void SetCover3::decreaseClustersize(int clusterid){
    int oldposition=clusterid_to_arrayposition[clusterid];
    int newposition=borders_of_set[clustersizes[clusterid]];
    int swapid=sorted_clustersizes[newposition];
    if(swapid!=-1){
        clusterid_to_arrayposition[swapid]=oldposition;
    }
    sorted_clustersizes[oldposition]=swapid;

    sorted_clustersizes[newposition]=clusterid;
    clusterid_to_arrayposition[clusterid]=newposition;
    borders_of_set[clustersizes[clusterid]]++;
    clustersizes[clusterid]--;
}