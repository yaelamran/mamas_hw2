#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

struct Line
{
	unsigned int tag = 0;
	unsigned int LRU = 0;
	bool dirty = false;
	bool free = true;
};

int ConvertHexStringToInt(string hexString)
{
	// Convert the address string from hex to an integer using std::stoul
	return static_cast<int>(std::stoul(hexString, nullptr, 16));
}

int getTag(unsigned int address, unsigned int numSetBits) 
{
	return address >> numSetBits;
}

int getSet(unsigned int address, unsigned int numSetBits)
{
	unsigned int setMask = (1 << numSetBits) - 1;
	unsigned int set = address & setMask;
	return set;
}

void updateLRUs(std::vector<Line>& set, int hitWay)
{
	unsigned int hitLRU = set[hitWay].LRU;
	for (int i = 0; i < set.size(); ++i) 
	{
		if (set[i].LRU <= hitLRU) 
		{
			set[i].LRU++;
		}
	}
	set[hitWay].LRU = 0;
}


int checkHit(std::vector<std::vector<Line>>& cache, unsigned int address, 
			unsigned int tag, unsigned int set, unsigned int numOfWays)
{
	for (int way = 0; way < numOfWays; ++way) 
	{
		if (cache[set][way].tag == tag && cache[set][way].free == false)
		{
			return way; // Hit - return way number
		}
	}
	return -1; // Miss
}

void updateDirtyBit(std::vector<std::vector<Line>>& cache, unsigned int set,
					unsigned int way, char operation)
{
	if (operation == 'W') 
	{
		cache[set][way].dirty = true;
	}
}

int findFreeWay(std::vector<Line>& set)
{
	for (int way = 0; way < set.size(); ++way)
	{
		if (set[way].free)
		{
			return way; // Return the index of the free way
		}
	}
	return -1; // No free way found
}

Line bringToLayer(std::vector<std::vector<Line>>& L, unsigned int Set, unsigned int Tag,
				unsigned int NumOfWays, char operation) //returns evicted line info if any
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
	L[Set][lruWay].dirty = (operation == 'W' || operation == 'w');
	return evictedLine; // Return evicted line info
}

void handleEvictedLineFromL1(std::vector<std::vector<Line>>& L2, Line& evictedLine,
							unsigned int L1NumBlocksPerWay, unsigned int L1Set, 
							unsigned int L2NumBlocksPerWay, int L2NumOfWays) 
{	
	if (evictedLine.free)
	{
		// no line was evicted
		return; // No further action needed
	}

	// If evicted line is dirty, update L2 (LRUs, dirty bit)
	if (evictedLine.dirty)
	{
		// Get evicted line's address to find its L2 set and tag
		unsigned int evictedAddress = (evictedLine.tag << L1NumBlocksPerWay) | L1Set;
		unsigned int evictedL2Tag = getTag(evictedAddress, L2NumBlocksPerWay);
		unsigned int evictedL2Set = getSet(evictedAddress, L2NumBlocksPerWay);
		int evictedL2Way = checkHit(L2, evictedAddress, evictedL2Tag, evictedL2Set, L2NumOfWays);
		// Should always be a hit
		updateDirtyBit(L2, evictedL2Set, evictedL2Way, 'W');
		updateLRUs(L2[evictedL2Set], evictedL2Way);
	}
}

/* Debug print function
void LiPrint(const std::vector<std::vector<Line>>& L, int i)
{
	cout << "L" << i<< " Cache State:" << endl;
	for (int set = 0; set < L.size(); ++set)
	{
		cout << "Set " << set << ": ";
		for (int way = 0; way < L[set].size(); ++way)
		{
			const Line& line = L[set][way];
			cout << "[Way " << way << ": Tag=" << line.tag
				 << ", LRU=" << line.LRU
				 << ", Dirty=" << line.dirty
				 << ", Free=" << line.free << "] ";
		}
		cout << endl;
	}
}
*/

int main(int argc, char **argv)
{
	if (argc < 19) 
	{
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) 
	{
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
	unsigned int L1TotalBlocks = L1Size - BSize;	
	unsigned int L1NumBlocksPerWay = L1TotalBlocks - L1Assoc;
	unsigned int L1NumOfWays = 1 << L1Assoc;
	unsigned int L1NumOfSets = 1 << L1NumBlocksPerWay;

	// Calculate L2 sizes
	unsigned int L2TotalBlocks = L2Size - BSize;
	unsigned int L2NumBlocksPerWay = L2TotalBlocks - L2Assoc;
	unsigned int L2NumOfWays = 1 << L2Assoc;
	unsigned int L2NumOfSets = 1 << L2NumBlocksPerWay;;

	// init ways / times ++++
	int L1AccessCount = 0; // counts how many times L1 was accesed;
	int L2AccessCount = 0; // counts how many times L2 was accesed;
	int L1MissCount = 0; // counts how many times L1 missed;
	int L2MissCount = 0; // counts how many times L2 missed;
	int memAccessCount = 0; // counts how many times mem was accesed;
	
	std::vector<std::vector<Line>> L1(L1NumOfSets, std::vector<Line>(L1NumOfWays));
	std::vector<std::vector<Line>> L2(L2NumOfSets, std::vector<Line>(L2NumOfWays));
	
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

		operation = toupper(operation);
		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// parse the address:
		unsigned int addressInt = ConvertHexStringToInt(address);
		unsigned int addressSetAndTag = addressInt >> BSize;
		unsigned int L1Tag = getTag(addressSetAndTag, L1NumBlocksPerWay);
		unsigned int L1Set = getSet(addressSetAndTag, L1NumBlocksPerWay);
		unsigned int L2Tag = getTag(addressSetAndTag, L2NumBlocksPerWay);
		unsigned int L2Set = getSet(addressSetAndTag, L2NumBlocksPerWay);

		//check L1
		L1AccessCount++;
		int L1Hit = checkHit(L1, addressInt, L1Tag, L1Set, L1NumOfWays); //returns the way number if hit, -1 if miss
		if (L1Hit >= 0)
		{
			// L1 Hit
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
		if (L2Hit >= 0)
		{
			// L2 Hit
			updateLRUs(L2[L2Set], L2Hit);
			updateDirtyBit(L2, L2Set, L2Hit, operation);
			if (WrAlloc==1 || operation =='R') // bring to l1
			{
				Line evictedLine = bringToLayer(L1, L1Set, L1Tag, L1NumOfWays, operation);
				handleEvictedLineFromL1(L2, evictedLine, L1NumBlocksPerWay, L1Set, L2NumBlocksPerWay, L2NumOfWays);
			}
			continue; // go to next address
		}
		
		// L2 miss -> Memory Access
		L2MissCount++;
		memAccessCount++;

		if (WrAlloc==1 || operation=='R') // bring to l2
		{
			// bring to L2
			Line evictedLine = bringToLayer(L2, L2Set, L2Tag, L2NumOfWays, operation);
			if (!evictedLine.free)
			{
				int evictedAddress = (evictedLine.tag << L2NumBlocksPerWay) | L2Set;
				int evictedL1Tag = getTag(evictedAddress, L1NumBlocksPerWay);
				int evictedL1Set = getSet(evictedAddress, L1NumBlocksPerWay);
				int evictedL1Way = checkHit(L1, evictedAddress, evictedL1Tag, evictedL1Set, L1NumOfWays);
				if (evictedL1Way >= 0)
				{
					L1[evictedL1Set][evictedL1Way].free = true; // free the line for this block in L1
				}
			}

			// bring to L1
			Line evictedLineL1 = bringToLayer(L1, L1Set, L1Tag, L1NumOfWays, operation);
			handleEvictedLineFromL1(L2, evictedLineL1, L1NumBlocksPerWay, L1Set, L2NumBlocksPerWay, L2NumOfWays);
		}

	}

	double L1MissRate = (double)L1MissCount / L1AccessCount;
	double L2MissRate = (double)L2MissCount / L2AccessCount;
	double avgAccTime = ((double)L1AccessCount * L1Cyc +
		(double)L2AccessCount * L2Cyc +
		(double)memAccessCount * MemCyc) / L1AccessCount;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
