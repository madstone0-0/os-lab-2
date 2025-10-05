// Description: Simulates Demand Paging with page replacement policies (FIFO &
// LRU)
//
// Compile: g++ demand.cpp -std=c++11 -o demand

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

using namespace std;

// Represent a job with id and size
struct Job {
  int id{};
  int size{};
};

// Represent a page frame in main memory
struct PageFrame {
  int id{};
  int startingAddr{};
  int size{};
};

// Represent a page with id and size
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
  bool inMemory{};
  uint8_t referenced{};
};

// Page Map Table
using PageMapTable = map<int, PageMapTableRow>;

// Job Table Row
struct JobTableRow {
  int id{};
  int size{};
  PageMapTable PMT;
};

// Job Table
using JobTable = map<int, JobTableRow>;

// Memory Map Table Row
struct MemoryMapTableRow {
  int jobId{};
  int pageFrameNumber{};
  int pageNumber{};
  bool busy{};
};

// Memory Map Table
using MemoryMapTable = map<int, MemoryMapTableRow>;

// Divides a job into pages of given page size
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
    PMT[page.id].pageFrameId = -1;
    PMT[page.id].inMemory = false;
    PMT[page.id].referenced = 0;
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
  printf("Page Number\tPage Frame ID\tReference Bit\n");
  for (const auto &kv : PMT) {
    printf("%d\t\t%d\t\t0b%08b\n", kv.second.pageNumber, kv.second.pageFrameId,
           kv.second.referenced);
  }
  printf("\n");
}

// FIFO Replacement Algorithm
int FIFO(JobTable &JT, MemoryMapTable &MMT, queue<int> &fifoQueue, int jobId,
         int pageNum, int pageSize) {

  // Scenario where we have free frame: no need for replacing
  for (auto &kv : MMT) {
    if (!kv.second.busy) {
      int frameNum = kv.second.pageFrameNumber;

      JT[jobId].PMT[pageNum].pageFrameId = frameNum;
      JT[jobId].PMT[pageNum].inMemory = true;

      kv.second.pageNumber = pageNum;
      kv.second.busy = true;
      kv.second.jobId = jobId;

      fifoQueue.push(frameNum);

      printf(" Loaded into free Frame %d\n", frameNum);
      return frameNum;
    }
  }

  // If we don't have a free frame replace
  if (fifoQueue.empty()) {
    throw runtime_error(
        "FIFO queue is empty — memory not initialized correctly!");
  }

  int replacedFrame = fifoQueue.front();
  fifoQueue.pop();

  int oldJobId = MMT[replacedFrame].jobId;
  int oldPageNum = MMT[replacedFrame].pageNumber;

  // mark oldest out of memory
  JT[oldJobId].PMT[oldPageNum].inMemory = false;
  JT[oldJobId].PMT[oldPageNum].pageFrameId = -1;

  printf("\tReplacing P%d J%d (F%d) with P%d of J%d (FIFO)\n", oldPageNum,
         oldJobId, replacedFrame, pageNum, jobId);

  // Load new page into replaced frame
  JT[jobId].PMT[pageNum].pageFrameId = replacedFrame;
  JT[jobId].PMT[pageNum].inMemory = true;

  MMT[replacedFrame].pageNumber = pageNum;
  MMT[replacedFrame].jobId = jobId;
  MMT[replacedFrame].busy = true;

  fifoQueue.push(replacedFrame);

  return replacedFrame;
}

// LRU Replacement Algorithm
int LRU(JobTable &JT, MemoryMapTable &MMT, int jobId, int pageNum,
        int pageSize) {
  for (auto &kv : MMT) {
    if (!kv.second.busy) {
      int frameNum = kv.second.pageFrameNumber;

      JT[jobId].PMT[pageNum].pageFrameId = frameNum;
      JT[jobId].PMT[pageNum].inMemory = true;
      JT[jobId].PMT[pageNum].referenced = 0x80; // Set MSB on reference

      kv.second.pageNumber = pageNum;
      kv.second.jobId = jobId;
      kv.second.busy = true;

      printf(" Loaded into free Frame %d\n", frameNum);
      return frameNum;
    }
  }

  // Find the least recently used frame
  int lruFrame = -1;
  uint8_t smallestRef = 0xFF;

  for (const auto &kv : MMT) {
    if (kv.second.busy) {
      int residentJobId = kv.second.jobId;
      int residentPageNumber = kv.second.pageNumber;
      auto ref = JT[residentJobId].PMT[residentPageNumber].referenced;
      if (ref < smallestRef) {
        smallestRef = JT[residentJobId].PMT[residentPageNumber].referenced;
        lruFrame = kv.second.pageFrameNumber;
      }
    }
  }

  if (lruFrame == -1) {
    throw runtime_error("LRU: No frame found for replacement!");
  }

  int oldJobId = MMT[lruFrame].jobId;
  int oldPageNum = MMT[lruFrame].pageNumber;

  // Mark old page out of memory
  JT[oldJobId].PMT[oldPageNum].inMemory = false;
  JT[oldJobId].PMT[oldPageNum].pageFrameId = -1;
  JT[oldJobId].PMT[oldPageNum].referenced = 0; // Clear reference

  printf("\tReplacing P%d of J%d (F%d) with P%d of J%d (LRU)\n", oldPageNum,
         oldJobId, lruFrame, pageNum, jobId);

  // Load new page into replaced frame
  JT[jobId].PMT[pageNum].pageFrameId = lruFrame;
  JT[jobId].PMT[pageNum].inMemory = true;
  JT[jobId].PMT[pageNum].referenced = 0x80; // Set MSB on reference

  MMT[lruFrame].pageNumber = pageNum;
  MMT[lruFrame].jobId = jobId;
  MMT[lruFrame].busy = true;

  return lruFrame;
}

struct Stats {
  int pageFrames{};
  double failRatio{};
  double successRatio{};
  int numAccesses{};
  int pageFaults{};
  int pageHits{};
};

// Demand Paging Simulation
Stats simulateDemandPaging(int numJobs, int numFrames, int pageSize,
                           int numAccesses, vector<Job> jobs,
                           bool replacement) {
  // Divide all jobs into pages
  JobTable JT;
  int totalPages = 0;

  printf("\n--- Dividing Jobs into Pages ---\n");
  for (const auto &job : jobs) {
    auto divRes = divideIntoPages(job, pageSize);
    auto &pages = divRes.first;
    auto &pmt = divRes.second;
    JT[job.id].id = job.id;
    JT[job.id].size = job.size;
    JT[job.id].PMT = pmt;

    printf("\nJob %d divided into %zu pages:\n", job.id, pages.size());
    for (const auto &page : pages) {
      printf(" Page %d: %d K\n", page.id, page.size);
      totalPages++;
    }

    int internalFrag = pageSize - pages.back().size;
    if (internalFrag > 0) {
      printf(" Internal Fragmentation in last page: %d K\n", internalFrag);
    }
  }

  printf("\nTotal pages across all jobs: %d\n", totalPages);
  printf("Available memory frames: %d\n", numFrames);

  // Initialize memory
  MainMemory ram(numFrames);
  MemoryMapTable MMT;

  for (int i = 0; i < numFrames; i++) {
    ram[i].id = i;
    ram[i].size = pageSize;
    ram[i].startingAddr = i * pageSize;

    MMT[i].pageFrameNumber = i;
    MMT[i].pageNumber = -1;
    MMT[i].jobId = -1;
    MMT[i].busy = false;
  }

  printMMT(MMT);

  // Simulate page requests (demand paging)
  printf("\n--- Simulating Demand Paging ---\n");
  printf("Pages are loaded into memory only when accessed.\n\n");

  int pageFaults = 0;
  queue<int> fifoQueue;
  int pageHits = 0;
  random_device rnd;
  mt19937 gen(rnd());
  uniform_int_distribution<> jobDist(0, numJobs - 1);

  for (int access = 0; access < numAccesses; access++) {
    // Randomly select a job and page
    int jobId = jobDist(gen);
    auto &job = JT[jobId];
    vector<int> pageKeys;
    for (const auto &kv : job.PMT)
      pageKeys.push_back(kv.first);
    if (pageKeys.empty())
      continue;
    uniform_int_distribution<> pageDist(0, (int)pageKeys.size() - 1);
    int pageNum = pageKeys[pageDist(gen)];

    printf("Access %d: J%d, P%d : ", access + 1, jobId, pageNum);

    auto &PMT = JT[jobId].PMT;
    auto &page = PMT[pageNum];

    // Age all pages' referenced bits (for LRU)
    for (auto &kv : JT)
      for (auto &pkv : kv.second.PMT)
        if (pkv.second.inMemory)
          pkv.second.referenced >>= 1; // Shift right one bit

    // Check if page is in memory
    if (page.inMemory) {
      printf("HIT\n");
      pageHits++;
      page.referenced |= 0x80; // Set MSB on reference

    } else {
      printf("FAULT\n");
      pageFaults++;

      // Find an empty frame
      int emptyFrame = -1;
      for (int i = 0; i < numFrames; i++) {
        if (!MMT[i].busy) {
          emptyFrame = i;
          break;
        }
      }

      if (emptyFrame != -1) {
        // Load page into empty frame
        page.pageFrameId = emptyFrame;
        page.inMemory = true;
        page.referenced = 0x80; // Set MSB on reference

        MMT[emptyFrame].pageNumber = pageNum;
        MMT[emptyFrame].jobId = jobId;
        MMT[emptyFrame].busy = true;

        fifoQueue.push(emptyFrame);
        printf("\tLoaded F%d\n", emptyFrame);
      } else {
        if (replacement) {
          FIFO(JT, MMT, fifoQueue, jobId, pageNum, pageSize);
        } else {
          LRU(JT, MMT, jobId, pageNum, pageSize);
        }
      }
    }
  }

  // Print final state
  printMMT(MMT);
  for (const auto &kv : JT) {
    printf("Final PMT for Job %d:\n", kv.first);
    printPMT(kv.second.PMT);
  }

  double failRatio = (double)pageFaults / numAccesses;
  double successRatio = (double)(numAccesses - pageFaults) / numAccesses;
  Stats s;
  s.pageFrames = numFrames;
  s.failRatio = failRatio;
  s.successRatio = successRatio;
  s.numAccesses = numAccesses;
  s.pageFaults = pageFaults;
  s.pageHits = pageHits;

  return s;
}

int main() {
  try {
    printf("Demand Paged Memory Allocation\n");

    int pageSize, numJobs, numFrames;

    cout << "Enter Page Size: ";
    cin >> pageSize;

    cout << "Enter number of jobs: ";
    cin >> numJobs;

    cout << "Enter number of available memory frames: ";
    cin >> numFrames;

    // Generate some random page accesses
    int numAccesses;
    cout << "Enter number of page accesses to simulate: ";
    cin >> numAccesses;

    if (pageSize <= 0 || numJobs <= 0 || numFrames <= 0 || numAccesses <= 0) {
      throw runtime_error("All inputs must be positive integers!");
    }

    // Accept jobs
    vector<Job> jobs;
    for (int i = 0; i < numJobs; i++) {
      Job j;
      j.id = i;
      cout << "Enter size of Job " << j.id << " : ";
      cin >> j.size;
      if (j.size <= 0) {
        throw runtime_error("Job size must be a positive integer!");
      }
      jobs.push_back(j);
    }

    printf("\n--- Jobs Summary ---\n");
    for (const auto &job : jobs) {
      printf("Job %d: %d K\n", job.id, job.size);
    }

    printf("\n--- FIFO Page Replacement ---\n");
    auto fifoStats = simulateDemandPaging(numJobs, numFrames, pageSize,
                                          numAccesses, jobs, true);
    printf("Total Accesses: %d\n", fifoStats.numAccesses);
    printf("Page Faults: %d\n", fifoStats.pageFaults);
    printf("Page Hits: %d\n", fifoStats.pageHits);
    printf("Failure Ratio: %.2f\n", fifoStats.failRatio);
    printf("Success Ratio: %.2f\n", fifoStats.successRatio);

    printf("\n--- LRU Page Replacement ---\n");
    auto lruStats = simulateDemandPaging(numJobs, numFrames, pageSize,
                                         numAccesses, jobs, false);
    printf("Total Accesses: %d\n", lruStats.numAccesses);
    printf("Page Faults: %d\n", lruStats.pageFaults);
    printf("Page Hits: %d\n", lruStats.pageHits);
    printf("Failure Ratio: %.2f\n", lruStats.failRatio);
    printf("Success Ratio: %.2f\n", lruStats.successRatio);
  } catch (const exception &e) {
    printf("Error: %s\n", e.what());
    return 1;
  }
}
