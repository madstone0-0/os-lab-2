// Discription: Simulates Demand Paging with page replacment policies (FIFO & LRU)
// Compile: g++ demand.cpp -std=c++11 -o demand

#include <algorithm>
#include <cstdio>
#include <exception>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

// Represent a job with id and size
struct Job
{
    int id{};
    int size{};
};

// Represent a page frame in main memory
struct PageFrame
{
    int id{};
    int startingAddr{};
    int size{};
    int jobId{};
};

// Represent a page with id and size
struct Page
{
    int id{};
    int size{};
    int jobId{};
};

// Represensts the main memory as a vector of page frames
using MainMemory = vector<PageFrame>;

// Page Map Table Row
struct PageMapTableRow
{
    int pageNumber{};
    int pageFrameId{};
    bool inMemory{};
    int jobId{};
};

// Page Map Table
using PageMapTable = map<int, PageMapTableRow>;

// Memory Map Table Row
struct MemoryMapTableRow
{
    int pageFrameNumber{};
    int pageNumber{};
    int jobId{};
    bool busy{};
};

// Memory Map Table
using MemoryMapTable = map<int, MemoryMapTableRow>;

// Divides a job into pages of given page size
pair<vector<Page>, PageMapTable> divideIntoPages(const Job &j, int pageSize)
{
    auto size = j.size;
    vector<Page> res;
    PageMapTable PMT;

    int i{0};
    while (size >= pageSize)
    {
        Page p;
        p.id = i;
        p.size = pageSize;
        p.jobId = j.id;
        res.push_back(p);
        size -= pageSize;
        i++;
    }
    if (size != 0)
    {
        Page p;
        p.id = i;
        p.size = size;
        p.jobId = j.id;
        res.push_back(p);
    }

    // Build PMT
    for (const auto &page : res)
    {
        int key = j.id * 1000 + page.id;
        PMT[key].pageNumber = page.id;
        PMT[key].pageFrameId = -1;
        PMT[key].inMemory = false;
        PMT[key].jobId = j.id;
    }

    return {res, PMT};
}

// Prints the Memory Map Table
void printMMT(const MemoryMapTable &MMT)
{
    printf("MMT:\n");
    printf("Page Frame Number\tPage Number\tBusy\n");
    for (const auto kv : MMT)
    {
        printf("%d\t\t\t%d\t\t%d\n", kv.second.pageFrameNumber,
               kv.second.pageNumber, kv.second.busy);
    }
    printf("\n");
}

// Prints the Page Map Table
void printPMT(const PageMapTable &PMT)
{
    printf("PMT:\n");
    printf("Page Number\tPage Frame ID\n");
    for (const auto &kv : PMT)
    {
        printf("%d\t\t%d\n", kv.second.pageNumber, kv.second.pageFrameId);
    }
    printf("\n");
}

// FIFO Replacement Algo
int FIFO(MemoryMapTable &MMT, PageMapTable &PMT, queue<int> &fifoQueue,
         int jobId, int pageNum, int pageSize, int key)
{

    // Scenario where we have free frame: no need for replaceing
    for (auto &kv : MMT)
    {
        if (!kv.second.busy)
        {
            int frameNum = kv.second.pageFrameNumber;

            PMT[key].pageFrameId = frameNum;
            PMT[key].inMemory = true;

            kv.second.pageNumber = pageNum;
            kv.second.jobId = jobId;
            kv.second.busy = true;

            fifoQueue.push(frameNum);

            printf(" Loaded into free Frame %d\n", frameNum);
            return frameNum;
        }
    }

    // If we don't have a free frame replace
    if (fifoQueue.empty())
    {
        throw runtime_error("FIFO queue is empty — memory not initialized correctly!");
    }

    int replacedFrame = fifoQueue.front();
    fifoQueue.pop();

    int oldJobId = MMT[replacedFrame].jobId;
    int oldPageNum = MMT[replacedFrame].pageNumber;
    int oldKey = oldJobId * 1000 + oldPageNum;

    // mark oldest out of memory
    PMT[oldKey].inMemory = false;
    PMT[oldKey].pageFrameId = -1;

    printf(" Replacing Page %d of Job %d (Frame %d) with Page %d of Job %d\n",
           oldPageNum, oldJobId, replacedFrame, pageNum, jobId);

    // Load new page into replaced frame
    PMT[key].pageFrameId = replacedFrame;
    PMT[key].inMemory = true;

    MMT[replacedFrame].pageNumber = pageNum;
    MMT[replacedFrame].jobId = jobId;
    MMT[replacedFrame].busy = true;

    fifoQueue.push(replacedFrame);

    return replacedFrame;
}

// LRU Replacement Algorithm
int LRU(MemoryMapTable &MMT, PageMapTable &PMT, map<int, int> &lruCounter,
        int &accessTime, int jobId, int pageNum, int pageSize, int key)
{
    for (auto &kv : MMT)
    {
        if (!kv.second.busy)
        {
            int frameNum = kv.second.pageFrameNumber;

            PMT[key].pageFrameId = frameNum;
            PMT[key].inMemory = true;
            kv.second.pageNumber = pageNum;
            kv.second.jobId = jobId;
            kv.second.busy = true;

            lruCounter[frameNum] = accessTime++;  // Track access time

            printf(" Loaded into free Frame %d\n", frameNum);
            return frameNum;
        }
    }

    // Find the least recently used frame
    int lruFrame = -1;
    int oldestTime = accessTime;

    for (const auto &kv : MMT)
    {
        if (kv.second.busy)
        {
            int frameNum = kv.second.pageFrameNumber;
            if (lruCounter[frameNum] < oldestTime)
            {
                oldestTime = lruCounter[frameNum];
                lruFrame = frameNum;
            }
        }
    }

    if (lruFrame == -1)
    {
        throw runtime_error("LRU: No frame found for replacement!");
    }

    int oldJobId = MMT[lruFrame].jobId;
    int oldPageNum = MMT[lruFrame].pageNumber;
    int oldKey = oldJobId * 1000 + oldPageNum;

    // Mark old page out of memory
    PMT[oldKey].inMemory = false;
    PMT[oldKey].pageFrameId = -1;

    printf(" Replacing Page %d of Job %d (Frame %d) with Page %d of Job %d (LRU)\n",
           oldPageNum, oldJobId, lruFrame, pageNum, jobId);

    // Load new page into replaced frame
    PMT[key].pageFrameId = lruFrame;
    PMT[key].inMemory = true;

    MMT[lruFrame].pageNumber = pageNum;
    MMT[lruFrame].jobId = jobId;
    MMT[lruFrame].busy = true;

    lruCounter[lruFrame] = accessTime++;  // Update access time

    return lruFrame;
}

// Demand Paging Simulation
void simulateDemandPaging(int numJobs, int numFrames, int pageSize,vector<Job> jobs, vector<vector<Page>> allPages, PageMapTable &globalPMT, bool replacement)
{
    // Initialize memory
    MainMemory ram(numFrames);
    MemoryMapTable MMT;

    for (int i = 0; i < numFrames; i++)
    {
        ram[i].id = i;
        ram[i].size = pageSize;
        ram[i].startingAddr = i * pageSize;
        ram[i].jobId = -1;

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
    map<int, int> lruCounter; //LRU
    int accessTime = 0; //LRU 
    int pageHits = 0;
    random_device rnd;
    mt19937 gen(rnd());

    // Generate some random page acesses
    int numAccesses;
    cout << "Enter number of page accesses to simulate: ";
    cin >> numAccesses;

    for (int access = 0; access < numAccesses; access++)
    {
        // Randomly select a job and page
        int jobIdx = gen() % numJobs;
        int pageIdx = gen() % allPages[jobIdx].size();
        int jobId = jobs[jobIdx].id;
        int pageNum = allPages[jobIdx][pageIdx].id;
        int key = jobId * 1000 + pageNum;

        printf("Access %d: Job %d, Page %d : ", access + 1, jobId, pageNum);

        // Check if page is in memory
        if (globalPMT[key].inMemory)
        {
            printf("HIT (Page already in memory)\n");
            pageHits++;
            // Update LRU counter on hit
            int frameId = globalPMT[key].pageFrameId;
            lruCounter[frameId] = accessTime++;
        }
        else
        {
            printf("Page Fault ..... Loading page into memory\n");
            pageFaults++;

            // Find an empty frame
            int emptyFrame = -1;
            for (int i = 0; i < numFrames; i++)
            {
                if (!MMT[i].busy)
                {
                    emptyFrame = i;
                    break;
                }
            }

            if (emptyFrame != -1)
            {
                // Load page into empty frame
                globalPMT[key].pageFrameId = emptyFrame;
                globalPMT[key].inMemory = true;

                MMT[emptyFrame].pageNumber = pageNum;
                MMT[emptyFrame].jobId = jobId;
                MMT[emptyFrame].busy = true;

                fifoQueue.push(emptyFrame);
                printf(" Loaded into Frame %d\n", emptyFrame);
            }
            else
            {
                if (replacement)
                {
                    FIFO(MMT, globalPMT, fifoQueue, jobId, pageNum, pageSize, key);
                }
                else
                {
                    LRU(MMT, globalPMT, lruCounter, accessTime, jobId, pageNum, pageSize, key);
                }
            }
        }
    }

    // Print final state
    printMMT(MMT);
    printPMT(globalPMT);

    printf("\n--- Demand Paging Results ---\n");
    printf("Total Accesses: %d\n", numAccesses);
    printf("Page Faults: %d\n", pageFaults);
    printf("Page Hits: %d\n", pageHits);
    printf("Page Fault Rate: %.2f%%\n", (double)pageFaults / numAccesses * 100);
}



int main()
{
     printf("Demand Paged Memory Allocation\n");

    int pageSize, numJobs, numFrames;

    cout << "Enter Page Size: ";
    cin >> pageSize;

    cout << "Enter number of jobs: ";
    cin >> numJobs;

    cout << "Enter number of available memory frames: ";
    cin >> numFrames;

    if (pageSize <= 0 || numJobs <= 0 || numFrames <= 0)
    {
        printf("Error: All values must be positive!\n");
        return 1;
    }

    // Accept jobs
    vector<Job> jobs;
    for (int i = 0; i < numJobs; i++)
    {
        Job j;
        j.id = i + 1;
        cout << "Enter size of Job " << j.id << " : ";
        cin >> j.size;
        if (j.size <= 0)
        {
            printf("Error: Job size must be positive!\n");
            return 1;
        }
        jobs.push_back(j);
    }

    printf("\n--- Jobs Summary ---\n");
    for (const auto &job : jobs)
    {
        printf("Job %d: %d KB\n", job.id, job.size);
    }
    // Divide all jobs into pages
    vector<vector<Page>> allPages; // TODO: Replace with job table later
    PageMapTable globalPMT;
    int totalPages = 0;

    printf("\n--- Dividing Jobs into Pages ---\n");
    for (const auto &job : jobs)
    {
        auto divRes = divideIntoPages(job, pageSize);
        auto &pages = divRes.first;
        auto &pmt = divRes.second;
        allPages.push_back(pages);

        // Merge into global PMT
        for (const auto &kv : pmt)
        {
            globalPMT[kv.first] = kv.second;
        }

        printf("\nJob %d divided into %zu pages:\n", job.id, pages.size());
        for (const auto &page : pages)
        {
            printf(" Page %d: %d KB\n", page.id, page.size);
            totalPages++;
        }

        int internalFrag = pageSize - pages.back().size;
        if (internalFrag > 0)
        {
            printf(" Internal Fragmentation in last page: %d KB\n", internalFrag);
        }
    }

    printf("\nTotal pages across all jobs: %d\n", totalPages);
    printf("Available memory frames: %d\n", numFrames);

    printf("\nFIFO Page Replacement\n\n");
    simulateDemandPaging(numJobs, numFrames, pageSize, jobs, allPages, globalPMT, true);
}