// Description: Simulates paging memory allocation scheme
// Compile: g++ paged.cpp -std=c++11 -o paged

#include <algorithm>
#include <cstdio>
#include <exception>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// Represents a job with id and size
struct Job {
  int id{};
  int size{};
};

// Represents a page frame in main memory
struct PageFrame {
  int id{};
  int startingAddr{};
  int size{};
};

// Represents a page with id and size
struct Page {
  int id{};
  int size{};
};

// Represents the main memory as a vector of page frames
using MainMemory = vector<PageFrame>;

// Page Map Table Row
struct PageMapTableRow {
  int pageNumber{};
  int pageFrameId{};
};

// Page Map Table
using PageMapTable = map<int, PageMapTableRow>;

// Memory Map Table Row
struct MemoryMapTableRow {
  int pageFrameNumber{};
  int pageNumber{};
  bool busy{};
};

// Memory Map Table
using MemoryMapTable = map<int, MemoryMapTableRow>;

// Divides a job into pages of given page size and returns the pages and PMT
pair<vector<Page>, PageMapTable> divideIntoPages(const Job &j, int pageSize) {
  auto size = j.size;
  vector<Page> res;
  PageMapTable PMT;

  int i{0};
  while (size >= pageSize) {
    Page p;
    p.id = i;
    p.size = pageSize;
    res.push_back(p);
    size -= pageSize;
    i++;
  }
  if (size != 0) {
    Page p;
    p.id = i;
    p.size = size;
    res.push_back(p);
  }

  // Build PMT
  for (const auto &page : res) {
    PMT[page.id].pageNumber = page.id;
    PMT[page.id].pageFrameId = -1; // Not assigned yet
  }

  return {res, PMT};
}

// Prints the Memory Map Table
void printMMT(const MemoryMapTable &MMT) {
  printf("MMT:\n");
  printf("Page Frame Number\tPage Number\tBusy\n");
  for (const auto kv : MMT) {
    printf("%d\t\t\t%d\t\t%d\n", kv.second.pageFrameNumber,
           kv.second.pageNumber, kv.second.busy);
  }
  printf("\n");
}

// Prints the Page Map Table
void printPMT(const PageMapTable &PMT) {
  printf("PMT:\n");
  printf("Page Number\tPage Frame ID\n");
  for (const auto &kv : PMT) {
    printf("%d\t\t%d\n", kv.second.pageNumber, kv.second.pageFrameId);
  }
  printf("\n");
}

int main() {
  // Input page size and job size
  string pageSizeStr;
  int pageSize{};
  string jobSizeStr;
  int jobSize{};

  cout << "Enter page size -> ";
  cin >> pageSizeStr;
  cout << "Enter job size -> ";
  cin >> jobSizeStr;
  try {
    pageSize = std::stoi(pageSizeStr);
    if (pageSize <= 0)
      throw invalid_argument{"Page size must be positive and greater than 0"};
    jobSize = std::stoi(jobSizeStr);
    if (jobSize <= 0)
      throw invalid_argument{"Job size must be positive and greater than 0"};
  } catch (const exception &e) {
    printf("Error -> %s\n", e.what());
    return 1;
  }
  printf("\n");
  printf("Job Size -> %d\nPage Size -> %d\n", jobSize, pageSize);
  printf("\n");

  // Divide job into pages
  Job j;
  j.id = 1;
  j.size = jobSize;
  auto divRes = divideIntoPages(j, pageSize);
  auto &pages = divRes.first;
  auto &PMT = divRes.second;
  printf("Pages:\n");
  for (const auto &page : pages) {
    printf("Page %d -> %d\n", page.id, page.size);
  }
  printf("\n");
  printPMT(PMT);

  // Create main memory and memory map table
  MainMemory ram;
  MemoryMapTable MMT;
  ram.resize(pages.size() + 1);

  int ramSize{pageSize * (int)ram.size()};
  for (int i{}, j{}; i < ram.size(); ++i, j += pageSize) {
    ram[i].id = i;
    ram[i].size = pageSize;
    ram[i].startingAddr = j;
    MMT[i].pageFrameNumber = i;
    MMT[i].pageNumber = -1;
    MMT[i].busy = false;
  }
  printMMT(MMT);

  // Calculate internal fragmentation if any
  int internalFragmentation = pageSize - pages.back().size;
  if (internalFragmentation > 0)
    printf("Internal Fragmentation In Page (%zu) -> %d\n", pages.size(),
           internalFragmentation);

  // Assign pages to page frames randomly
  printf("Assigning pages to page frames randomly...\n");
  vector<int> ids(pages.size());
  iota(ids.begin(), ids.end(), 0);
  random_device rnd;
  shuffle(ids.begin(), ids.end(), std::mt19937{rnd()});
  int i{};
  while (!ids.empty()) {
    auto id = ids.back();
    ids.pop_back();

    PMT[id].pageFrameId = MMT[i].pageFrameNumber;
    MMT[i].pageNumber = id;
    MMT[i].busy = true;
    i++;
  }
  printMMT(MMT);
  printPMT(PMT);

  // Perform address translation for 3 random addresses
  printf("Resolve 3 random address\n");
  vector<int> addresses(3);
  for (auto &addr : addresses) {
    addr = rnd() % jobSize;
    printf("Address -> %d\n", addr);

    int pageNumber = addr / pageSize;
    int offset = addr % pageSize;
    int pageFrameId = PMT[pageNumber].pageFrameId;
    int physicalAddr = ram[pageFrameId].startingAddr + offset;
    printf("Page Number -> %d\nOffset -> %d\nPhysical Address -> %d\n",
           pageNumber, offset, physicalAddr);
    printf("\n");
  }
}
