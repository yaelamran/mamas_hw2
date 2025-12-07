#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "Way.h"

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
	// clac Way size, set size etc.
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

	std::vector<Way> l1_ways(L1NumOfWays, Way(L1NumBlocksPerWay));
	std::vector<Way> l2_ways (L2NumOfWays,Way(L2NumBlocksPerWay));

	bool found = false;
	while (getline(file, line))
	{
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address))
		{
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		
		int addressInt = ConvertHexStringToInt(address);
		int addressSetAndTag = addressInt >> BSize;
		int L1Tag = getTag(addressInt, L1SetSize);
		int L1Set = getSet(addressInt, L1SetSize);
		
		int L2Tag = getTag(addressInt, L2SetSize);
		int L2Set = getSet(addressInt, L2SetSize);

		//find which way (if any) has the address
		for (int way = 0; way < L1NumOfWays; way++)
		{
			if (L1Tag == l1_ways[way].tag(L1Set))
			{
				found = true;
				break;
			}
		}

		//if L1 hit - update dirty and LRU

		//else L1 miss - check L2
		




		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		// start checking the address
		// go to L1
		// if not in L1 go to L2 
		// if not in L2 go to mem

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
