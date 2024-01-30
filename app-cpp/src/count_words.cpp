#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <utility>
#include "./matplotlib-cpp/matplotlibcpp.h"  // Include the matplotlibcpp.h header

namespace plt = matplotlibcpp;
namespace fs = std::filesystem;
using namespace std;

// Function to process each line and extract word counts
pair<string, int> extractWordCount(const string& line) {
    istringstream iss(line);
    string word;
    int count;

    // Attempt to extract a word and its count from the line
    if (iss >> word >> count) {
        return {word, count};
    } else {
        // Return an empty pair if extraction fails
        return {};
    }
}

// Function to read word counts from a file
vector<pair<string, int>> readWordCountsFromFile(const fs::path& filePath) {
    ifstream file(filePath);
    vector<pair<string, int>> wordCounts;
    string line;

    // Process each line of the file and extract word counts
    while (getline(file, line)) {
        pair<string, int> wordCount = extractWordCount(line);

        // Check if the extraction was successful
        if (!wordCount.first.empty()) {
            wordCounts.push_back(wordCount);
        }
    }

    return wordCounts;
}

// Function to sort word counts by frequency in descending order
void sortWordCountsDescending(vector<pair<string, int>>& wordCounts) {
    sort(wordCounts.begin(), wordCounts.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
}

// Function to write sorted word counts to a file
void writeSortedWordCountsToFile(const vector<pair<string, int>>& wordCounts, const fs::path& outputPath) {
    ofstream outFile(outputPath);

    for (const auto& wc : wordCounts) {
        outFile << wc.first << " " << wc.second << "\n";
    }
}

// Function to process each file in a directory
void processDirectoryAndCalculateThroughput(const fs::path& Input_Directory,
                                            const fs::path& Output_Directory,
                                            long long& totalWords,
                                            double& totalTime,
                                            vector<double>& File_Sizes,
                                            vector<double>& throughputValues) {
    for (const auto& entry : fs::recursive_directory_iterator(Input_Directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            fs::path relativePath = fs::relative(entry.path(), Input_Directory);
            fs::path outputPath = Output_Directory / relativePath;
            fs::create_directories(outputPath.parent_path());

            auto start = chrono::high_resolution_clock::now();

            // Read word counts from the file
            auto wordCounts = readWordCountsFromFile(entry.path());

            double fileSize = fs::file_size(entry.path()) / 1024.0 / 1024.0; // Size in MiB

            // Sort word counts by frequency in descending order
            sortWordCountsDescending(wordCounts);

            // Write sorted word counts to the output file
            writeSortedWordCountsToFile(wordCounts, outputPath);

            auto end = chrono::high_resolution_clock::now();

            double timeTaken = chrono::duration<double>(end - start).count();
            totalWords += wordCounts.size();
            totalTime += timeTaken;

            File_Sizes.push_back(fileSize);
            throughputValues.push_back(wordCounts.size() / timeTaken); // Calculate throughput
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <Input_Directory> <Output_Directory>\n";
        return 1;
    }

    filesystem::path Input_Directory(argv[1]);
    filesystem::path Output_Directory(argv[2]);
    long long totalWords = 0;
    double totalTime = 0.0;
    vector<double> File_Sizes, throughputValues;

    processDirectoryAndCalculateThroughput(Input_Directory, Output_Directory, totalWords, totalTime, File_Sizes, throughputValues);

    // Plotting
    plt::plot(File_Sizes, throughputValues, "bo"); // "bo" stands for "blue circle" markers
    plt::xlabel("MiB");
    plt::ylabel("words/second");
    plt::title("Throughput vs. File Size");
    plt::show();

    return 0;
}
