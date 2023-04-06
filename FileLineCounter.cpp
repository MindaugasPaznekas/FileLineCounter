// FileLineCounter.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Usage example: >FileLineCounter.exe C:\Folder

#include <iostream>
#include <filesystem>
#include <thread>
#include <algorithm>
#include <fstream>
#include <queue>
#include <list>
#include <future>
#include <chrono>
#include <atomic>
#include <string>

//@brief Result of all lines counted will be stored in this variable
std::atomic<std::uint64_t> LineCount{ 0 };

/**
 * @brief Handles file reading of directory,
 * starting and managing threads that count the lines in files
 */
class FileLineCounter
{
public:
    FileLineCounter(const FileLineCounter&) = delete;
    FileLineCounter(FileLineCounter&&) = delete;
    FileLineCounter& operator=(const FileLineCounter&) = delete;
    FileLineCounter& operator==(const FileLineCounter&) = delete;
    /**
     * @brief Constructor: will start a new task to gather all files that need to be counted 
     * @param mainDir : starting point of the program
     * @param maxThreadCount: Maximum number of threads allowed to run simultaneosly
    */
    FileLineCounter(const std::filesystem::directory_entry& mainDir, 
        const unsigned int maxThreadCount) :
        m_fileQueue(),
        m_fileQueueMutex(),
        m_threadList(),
        MainDir(mainDir),
        MaxThreadCount(maxThreadCount)
    {
        startFileSearchTask();
    }
    /**
     * @brief To be called continuosly for all files to be processed
     * @return : true if everything is processed, false if needs more time
    */
    bool processQueue()
    {
        while ((m_threadList.size() < MaxThreadCount) && addLineCountingTask())
        {
            //create new threads
        }

        if (m_threadList.empty())
        {
            return true;
        }

        for (std::list<std::future<void>>::iterator task = m_threadList.begin(); task != m_threadList.end(); task++)
        {
            if (task->valid() && task->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                try
                {
                    task->get();
                    task = m_threadList.erase(task);
                }
                catch (std::exception& e)
                {
                    std::cerr << "exception caught: " << e.what();
                }
            }
        }
        
        return false;
    }

    ~FileLineCounter() = default;
private:
    void addToQueue(const std::filesystem::path& filePath)
    {
        std::lock_guard lock{ m_fileQueueMutex };

        m_fileQueue.push(filePath);
    }
    /**
     * @brief Adds single task for single file to be counted
     * @return: true when task is added, false if currently there are no files to be processed
    */
    bool addLineCountingTask()
    {
        std::filesystem::path filePath{};
        {
            std::lock_guard lock{ m_fileQueueMutex };
            if (m_fileQueue.empty())
            {
                return false;
            }

            filePath = m_fileQueue.front();
            m_fileQueue.pop();
        }

        auto func = [](const std::filesystem::path filePath)
        {
            std::ifstream inFile{ filePath, std::ios::binary };
            //+1 for the last line
            const auto linesInFile = std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n') + 1;
            LineCount.fetch_add(linesInFile);
        };

        m_threadList.emplace_back(std::async(std::launch::async, func, filePath));
        return true;
    }
    /**
     * @brief: Recursively looks through given directory and adds all found files to queue for counting
     * @param dir: directory to be searched
    */
    void searchForFilesInDirectory(const std::filesystem::directory_entry& dir)
    {
        for (const auto& entryToProcess : std::filesystem::recursive_directory_iterator(dir))
        {
            if (entryToProcess.is_directory())
            {
                continue;
            }
            else if (entryToProcess.is_regular_file())
            {
                addToQueue(entryToProcess.path());
            }
            else
            {
                std::cout << entryToProcess.path().string() << "Is neither file nor a directory it will not be counted." << std::endl;
            }
        }
    }

    void startFileSearchTask()
    {
        m_threadList.emplace_back(std::async(std::launch::async, &FileLineCounter::searchForFilesInDirectory, this, MainDir));
    }

    std::queue<std::filesystem::path> m_fileQueue;
    std::mutex m_fileQueueMutex;
    std::list<std::future<void>> m_threadList;
    const std::filesystem::directory_entry MainDir;
    const unsigned int MaxThreadCount;
};

/**
 * @brief checks provided arguments, if good to go 
 * continously executes 
 * @param argc: number of arguments
 * @param argv: arguments, second argument is for directory to be counted
 * @return: 0 on success
*/
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Please enter path for directory containing files to be counted.\n";
        return -1;
    }

    const std::filesystem::directory_entry mainDir{ argv[1] };

    if (!mainDir.exists() || !mainDir.is_directory())
    {
        std::cout << mainDir.path().string() << " - is not a directory. Please enter a valid directory.\n";
        return -1;
    }

    //if there was an issue reading processor count (returned 0) run only 2 tasks. Otherwise utilize all cores -1 for main thread
    const auto processorCount = std::thread::hardware_concurrency() == 0 ? 1 : (std::thread::hardware_concurrency() - 1);

    FileLineCounter fileQ(mainDir, processorCount);

    while (fileQ.processQueue() == false)
    {
        //Waiting for all files to be process
    }

    std::cout <<"TOTAL number of lines in files: " << std::to_string(LineCount) << std::endl;

    return 0;
}