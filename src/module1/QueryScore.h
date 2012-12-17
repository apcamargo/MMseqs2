#ifndef QUERY_SCORE_H
#define QUERY_SCORE_H

// Written by Maria Hauser mhauser@genzentrum.lmu.de
// 
// Calculates the overall prefiltering score for the template database sequences and returns all sequences 
// with the prefiltering score >= prefiltering threshold.
//

#include <list>
#include <iostream>
#include <cstring>
#include <algorithm>

typedef struct {
    short seqId;
    short prefScore;
} hit_t;


class QueryScore {
    public:

        QueryScore (size_t dbSize, short prefThreshold);

        ~QueryScore ();

        // add k-mer match score for all DB sequences from the list
        void addScores (size_t* hitList, size_t hitListSize, short score);

        // get the list of the sequences with the score > prefThreshold and the corresponding 
        std::list<hit_t>* getResult ();

        // reset the prefiltering score counter for the next query sequence
        void reset ();

    private:

        // number of sequences in the target DB
        int dbSize;

        // prefiltering threshold
        short prefThreshold;

        // position in the array: sequence id
        // entry in the array: prefiltering score
        short* scores;

        // mapping db id -> position of the last match in the sequence
        short* lastMatchPos;

        // list of all DB sequences with the prefiltering score >= prefThreshold
        std::list<size_t>* hitList;

        // list of all DB sequences with the prefiltering score >= prefThreshold with the corresponding scores
        std::list<hit_t>* resList;

        void addElementToResults(size_t seqId);

};

#endif
