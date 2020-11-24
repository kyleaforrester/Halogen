#pragma once
#include "Move.h"
#include "BitBoardDefine.h"
#include <climits>

const unsigned int HALF_MOVE_MODULO = 16;

enum class EntryType : char {
	EMPTY_ENTRY,
	EXACT,
	LOWERBOUND,
	UPPERBOUND
};

//16 bytes
struct TTEntry
{
public:
	TTEntry();
	TTEntry(Move best, uint64_t ZobristKey, int Score, int Depth, int currentTurnCount, int distanceFromRoot, EntryType Cutoff);
	~TTEntry();

	bool IsAncient(unsigned int currenthalfmove, unsigned int distanceFromRoot) const { return halfmove != static_cast<char>((currenthalfmove - distanceFromRoot) % (HALF_MOVE_MODULO)); }

	void SetHalfMove(int currenthalfmove, int distanceFromRoot) { halfmove = CalculateHalfMove(currenthalfmove, distanceFromRoot); }	//halfmove is from current position, distanceFromRoot adjusts this to get what the halfmove was at the root of the search
	void MateScoreAdjustment(int distanceFromRoot);
	void Reset();

	static uint8_t CalculateHalfMove(int currenthalfmove, int distanceFromRoot) { return (currenthalfmove - distanceFromRoot) % (HALF_MOVE_MODULO); }
	int8_t CalculateHashScore(int8_t currentGeneration);

	uint64_t GetKey() const { return key; }
	int GetScore() const { return score; }
	int GetDepth() const { return depth; }
	EntryType GetCutoff() const { return cutoff; }
	char GetHalfMove() const { return halfmove; }
	Move GetMove() const { return Move(bestMove.data); }

private:
	/*Arranged to minimize padding*/
	uint64_t key;			//8 bytes
	MoveBits bestMove;		//2 bytes 
	short int score;		//2 bytes
	char depth;				//1 bytes
	EntryType cutoff;		//1 bytes
	char halfmove;			//1 bytes		(is stored as the halfmove at the ROOT of this current search, modulo 16)
};

struct TTBucket
{
	void Reset();
	TTEntry entry[4];
};

