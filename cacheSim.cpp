#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
//#include "cache.h"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using namespace cache;
using cache::Way;

const int ADDRESS_MISSING = 0xFFFFFFFF;

struct Line
{
	int tag;
	int LRU;
	bool dirty;
	bool free = true;
};

int ConvertHexStringToInt(string hexString)
{
	// Convert the address string from hex to an integer using std::stoul
	return static_cast<int>(std::stoul(hexString, nullptr, 16));
}

int getTag(int address, int numSetBits) 
{
	return address >> numSetBits;
}

int getSet(int address, int numSetBits)
{
	int setMask = (1 << numSetBits) - 1;
	int set = address & setMask;
	return set;
}

void updateLRUs(std::vector<Line>& set, int hitWay)
{
	int hitLRU = set[hitWay].LRU;
	for (int i = 0; i < set.size(); ++i) {
		if (set[i].LRU < hitLRU) {
			set[i].LRU++;
		}
	}
	set[hitWay].LRU = 0;
}


int checkHit(std::vector<std::vector<Line>>& cache, int address, int tag, int set, int numOfWays)
{
	for (int way = 0; way < numOfWays; ++way) {
		if (cache[set][way].tag == tag && !cache[set][way].free) {
			return way; // Hit - return way number
		}
	}
	return -1; // Miss
}

void updateDirtyBit(std::vector<std::vector<Line>>& cache, int set, int way, char operation)
{
	if (operation == 'W') {
		cache[set][way].dirty = true;
	}
}

Line bringToLayer(std::vector<std::vector<Line>>& L, int Set, int Tag, int NumOfWays, char operation) //returns evicted line info if any
{
	//find a free way for this set if one exists
	int way = findFreeWay(L[Set]);
	if (way >= 0)
	{
		// Use this free way
		L[Set][way].tag = Tag;
		L[Set][way].free = false;
		L[Set][way].dirty = (operation == 'W');
		// update LRUs:
		updateLRUs(L[Set], way);
		return Line(); // No eviction
	}
	//if no way is free, evict LRU and update LRUs
	int lruWay = 0;
	for (int way = 1; way < NumOfWays; ++way)
	{
		if (L[Set][way].LRU > L[Set][lruWay].LRU)
		{
			lruWay = way;
		}
	}
	//update the LRUs before eviction
	updateLRUs(L[Set], lruWay);
	Line evictedLine = L[Set][lruWay];
	// Replace the line
	L[Set][lruWay].tag = Tag;
	L[Set][lruWay].free = false;
	L[Set][lruWay].dirty = (operation == 'W');
	return evictedLine; // Return evicted line info
}


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	
	// calc Way size, set size etc.
	// Calculate L1 sizes
	int L1TotalBlocks = L1Size - BSize;	
	int L1NumBlocksPerWay = L1TotalBlocks - L1Assoc;
	int L1SetSize = L1NumBlocksPerWay - L1Assoc;
	int L1NumOfWays = 1 << L1Assoc;
	// Calculate L2 sizes
	int L2TotalBlocks = L2Size - BSize;
	int L2NumBlocksPerWay = L2TotalBlocks - L2Assoc;
	int L2SetSize = L2NumBlocksPerWay - L2Assoc;
	int L2NumOfWays = 1 << L2Assoc;

	// init ways / times ++++
	int L1AccessCount = 0; // counts how many times L1 was accesed;
	int L2AccessCount = 0; // counts how many times L2 was accesed;
	int L1MissCount = 0; // counts how many times L1 missed;
	int L2MissCount = 0; // counts how many times L2 missed;
	int memAccessCount = 0; // counts how many times mem was accesed;

	std::vector<std::vector<Line>> L1; // fix sizes
	std::vector<std::vector<Line>> L2; 
	
	while (getline(file, line))
	{
		//get the adrress:
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address))
		{
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;


		// parse the address:
		int addressInt = ConvertHexStringToInt(address);
		int addressSetAndTag = addressInt >> BSize;
		int L1Tag = getTag(addressInt, L1SetSize);
		int L1Set = getSet(addressInt, L1SetSize);
		
		int L2Tag = getTag(addressInt, L2SetSize);
		int L2Set = getSet(addressInt, L2SetSize);


		//check L1
		L1AccessCount++;
		int L1Hit = checkHit(L1, addressInt, L1Tag, L1Set, L1NumOfWays); //returns the way number if hit, -1 if miss
		if (L1Hit>=0)
		{
			updateLRUs(L1[L1Set], L1Hit);
			updateDirtyBit(L1, L1Set, L1Hit, operation);
			continue; // go to next address
		}

		// L1 miss
		L1MissCount++;

		//check L2
		L2AccessCount++;
		int L2Hit = checkHit(L2, addressInt, L2Tag, L2Set, L2NumOfWays); //returns the way number if hit, -1 if miss

		//L2 hit
		if (L2Hit>=0)
		{
			updateLRUs(L2[L2Set], L2Hit);
			updateDirtyBit(L2, L2Set, L2Hit, operation);
			if (WrAlloc==1 || operation=='R') // bring to l1
			{
				Line evictedLine = bringToLayer(L1, L1Set, L1Tag, L1NumOfWays, operation);
				// if no line was evicted, continue
				if (evictedLine.free)
				{
					continue; // go to next address
				}	
				// else - TODO!

			}
			continue; // go to next address
		}
		
		// L2 miss
		L2MissCount++;
		memAccessCount++;
		if (WrAlloc==1 || operation=='R') // bring to l2
		{
			Line evictedLine = bringToLayer(L2, L2Set, L2Tag, L2NumOfWays, operation);
			
			//TODO - free this line from L1 if needed
		}

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}


//leftovers:


//check L1 cache for hit/miss
bool checkL1(int address, int tag, int set, std::vector<std::vector<Line>>& L1, int numOfWays, int WrAlloc, char operation) 
{
	// Iterate through the ways in the set to check for a hit
	for (int way = 0; way < numOfWays; ++way) {
		if (L1[set][way].tag == tag && !L1[set][way].free) {
			// Hit
			// Update LRU and dirty bit if it's a write operation
			if (operation == 'W') {
				L1[set][way].dirty = true;
			}
			// update LRUs:
			updateLRUs(L1[set], way);
			return true; // Hit
		}
	}
	if (!WrAlloc && operation == 'W') {
		return false; // Miss, no write allocate
	}

	// Miss and write allocate or read operation
	// Find a free line if one exists
	for (int way = 0; way < numOfWays; ++way) {
		if (L1[set][way].free) {
			// Use this free line
			L1[set][way].tag = tag;
			L1[set][way].free = false;
			L1[set][way].dirty = (operation == 'W');
			// update LRUs:
			updateLRUs(L1[set], way);
			return false; // Miss
		}
	}

	// No free line found, need to evict the LRU line
	int lruWay = 0;
	for (int way = 1; way < numOfWays; ++way) {
		if (L1[set][way].LRU > L1[set][lruWay].LRU) {
			lruWay = way;
		}
	}

	//if the line is dirty, update L2



	// Replace the line
	L1[set][lruWay].tag = tag;
	L1[set][lruWay].free = false;
	L1[set][lruWay].dirty = (operation == 'W');
	// update LRUs:
	updateLRUs(L1[set], lruWay);
	return false; // Miss
}



			if (WrAlloc==1 || operation=='R') // bring to l1
			{
				//should be implemented differently - via a function that is given an address, brings it to L1 and returns the evicted line info (if a line was evicted)
				




				//find a free way for this set if one exists
				int way = findFreeWay(L1[L1Set]);
				if (way >= 0)
				{
					// Use this free way
					L1[L1Set][way].tag = L1Tag;
					L1[L1Set][way].free = false;
					L1[L1Set][way].dirty = (operation == 'W');
					// update LRUs:
					updateLRUs(L1[L1Set], way);
					continue; // go to next address
				}
				//if no way is free, evict LRU and update LRUs
				int lruWay = 0;
				for (int way = 1; way < L1NumOfWays; ++way)
				{
					if (L1[L1Set][way].LRU > L1[L1Set][lruWay].LRU)
					{
						lruWay = way;
					}
				}
				//update the LRUs before eviction
				updateLRUs(L1[L1Set], lruWay);
				//if evicted.dirty update L2 (LRUs, dirty bit))
				if (L1[L1Set][lruWay].dirty)
				{
					//TODO
				}
					 
			}
			


		if (WrAlloc==1 || operation=='R') // bring to l2
		{
			//find a free way for this set if one exists
			int way = findFreeWay(L2[L2Set]);
			if (way >= 0)
			//if no line is free, evict LRU and update LRUs
				//free this line from L1 if needed
			//bring to L1
				//find a free line if one exists
				//if no line is free, evict LRU and update LRUs
				//if evicted.dirty update L2 (LRUs, dirty bit))
		}
