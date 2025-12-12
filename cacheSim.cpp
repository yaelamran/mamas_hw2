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
	//cout << "Checking address " << address <<  endl;	
	//cout << "Checking set " << set << " with tag " << tag <<  endl;	
	for (int way = 0; way < numOfWays; ++way) 
	{
		//cout << "Checking way " << way << " with tag " << cache[set][way].tag << " and free " << cache[set][way].free << endl;	
		if (cache[set][way].tag == tag && cache[set][way].free == false)
		{
			//cout << "Hit in way " << way << endl;
			return way; // Hit - return way number
		}
	}
	return -1; // Miss
}

void updateDirtyBit(std::vector<std::vector<Line>>& cache, int set, int way, char operation)
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
		//cout << "L[Set][way] tag: " << L[Set][way].tag << ", dirty: " << L[Set][way].dirty << ", free: " << L[Set][way].free << endl;
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

void handleEvictedLineFromL1(std::vector<std::vector<Line>>& L2, Line& evictedLine, int L1NumBlocksPerWay, 
							 int L1Set, int L2NumBlocksPerWay, int L2NumOfWays) 
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
		int evictedAddress = (evictedLine.tag << L1NumBlocksPerWay) | L1Set;
		int evictedL2Tag = getTag(evictedAddress, L2NumBlocksPerWay);
		int evictedL2Set = getSet(evictedAddress, L2NumBlocksPerWay);
		int evictedL2Way = checkHit(L2, evictedAddress, evictedL2Tag, evictedL2Set, L2NumOfWays);
		// Should always be a hit
		updateDirtyBit(L2, evictedL2Set, evictedL2Way, 'W');
		updateLRUs(L2[evictedL2Set], evictedL2Way);
	}
}


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
	//unsigned int L1SetSize = L1NumBlocksPerWay - L1Assoc;
	unsigned int L1NumOfWays = 1 << L1Assoc;
	unsigned int L1NumOfSets = 1 << L1NumBlocksPerWay;

	// Calculate L2 sizes
	unsigned int L2TotalBlocks = L2Size - BSize;
	unsigned int L2NumBlocksPerWay = L2TotalBlocks - L2Assoc;
	//unsigned int L2SetSize = L2NumBlocksPerWay - L2Assoc;
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
		cout << "------------------------" << endl;
		LiPrint(L1,1);
		cout << "------------------------" << endl;
		LiPrint(L2,2);
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
		//cout << "addressSetAndTag: " << addressSetAndTag << endl;
		int L1Tag = getTag(addressSetAndTag, L1NumBlocksPerWay);
		int L1Set = getSet(addressSetAndTag, L1NumBlocksPerWay);
		cout << "L1 Tag: " << L1Tag << ", L1 Set: " << L1Set << endl;
		int L2Tag = getTag(addressSetAndTag, L2NumBlocksPerWay);
		int L2Set = getSet(addressSetAndTag, L2NumBlocksPerWay);
		cout << "L2 Tag: " << L2Tag << ", L2 Set: " << L2Set << endl;

		//check L1
		L1AccessCount++;
		int L1Hit = checkHit(L1, addressInt, L1Tag, L1Set, L1NumOfWays); //returns the way number if hit, -1 if miss
		if (L1Hit >= 0)
		{
			cout << "L1 Hit!" << endl;
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
			cout << "L2 Hit!" << endl;
			updateLRUs(L2[L2Set], L2Hit);
			updateDirtyBit(L2, L2Set, L2Hit, operation);
			if (WrAlloc==1 || operation =='R') // bring to l1
			{
				Line evictedLine = bringToLayer(L1, L1Set, L1Tag, L1NumOfWays, operation);
				handleEvictedLineFromL1(L2, evictedLine, L1NumBlocksPerWay, L1Set, L2NumBlocksPerWay, L2NumOfWays);
			}
			continue; // go to next address
		}
		
		// L2 miss
		L2MissCount++;
		memAccessCount++;
		if (WrAlloc==1 || operation=='R') // bring to l2
		{
			// bring to L2
			Line evictedLine = bringToLayer(L2, L2Set, L2Tag, L2NumOfWays, operation);
			if (!evictedLine.free)
			{
				//cout << "L2 evicted line tag: " << evictedLine.tag << ", dirty: " << evictedLine.dirty << ", free: " << evictedLine.free << endl;
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
			// TODO - handle evicted line from L1 if any
			Line evictedLineL1 = bringToLayer(L1, L1Set, L1Tag, L1NumOfWays, operation);
			handleEvictedLineFromL1(L2, evictedLineL1, L1NumBlocksPerWay, L1Set, L2NumBlocksPerWay, L2NumOfWays);
		}

	}

	double L1MissRate = (double)L1MissCount / L1AccessCount;
	double L2MissRate = (double)L2MissCount / L2AccessCount;
	double avgAccTime = ((double)L1AccessCount * L1Cyc +
		(double)L2AccessCount * L2Cyc +
		(double)memAccessCount * MemCyc) / L1AccessCount;
	// Print all the values in use
	cout << "L1MissCount: " << L1MissCount << endl;
	cout << "L2MissCount: " << L2MissCount << endl;
	cout << "L1AccessCount: " << L1AccessCount << endl;
	cout << "L2AccessCount: " << L2AccessCount << endl;
	cout << "memAccessCount: " << memAccessCount << endl;
	cout << "avgAccTime: " << avgAccTime << endl;


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}


//leftovers:

/*
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
*/