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

namespace plot = matplotlibcpp;
namespace fs = std::filesystem;
using namespace std;

// Function to process each line_ and extract word_cnt counts
pair<string, int> extractWordCountFromLine(const string& line_) {
    istringstream iss(line_);
    string word_cnt;
    int cnt;

    // Attempt to extract a word_cnt and its cnt from the line_
    if (iss >> word_cnt >> cnt) {
        return {word_cnt, cnt};
    } else {
        // Return an empty pair if extraction fails
        return {};
    }
}

// Function to read word_cnt counts from a file
vector<pair<string, int>> readWordCountsFromFile(const fs::path& filePath) {
    ifstream file(filePath);
    vector<pair<string, int>> Word_Counts;
    string line_;

    // Process each line_ of the file and extract word_cnt counts
    while (getline(file, line_)) {
        pair<string, int> wordCount = extractWordCountFromLine(line_);

        // Check if the extraction was successful
        if (!wordCount.first.empty()) {
            Word_Counts.push_back(wordCount);
        }
    }

    return Word_Counts;
}

// Function to sort word_cnt counts by frequency in descending order
void sortWordCountsDescending(vector<pair<string, int>>& Word_Counts) {
    sort(Word_Counts.begin(), Word_Counts.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
}

// Function to write sorted word_cnt counts to a file
void writeSortedWordCountsToFile(const vector<pair<string, int>>& Word_Counts, const fs::path& filePath) {
    ofstream outFile(filePath);

    for (const auto& [word_cnt, cnt] : Word_Counts) {
        outFile << word_cnt << " " << cnt << "\n";
    }
}

// Function to process a single file and measure time
void processSingleFile(const fs::path& inputFile, const fs::path& outputFile,
                       long long& Total_Words, double& Total_Time,
                       vector<double>& file_Sizes, vector<double>& Progress_Values) {
    auto Word_Counts = readWordCountsFromFile(inputFile);

    double fileSizeInMiB = fs::file_size(inputFile) / 1024.0 / 1024.0;
    auto startTime = chrono::high_resolution_clock::now();

    // Sort word_cnt counts by frequency in descending order
    sortWordCountsDescending(Word_Counts);

    // Write sorted word_cnt counts to the output file
    writeSortedWordCountsToFile(Word_Counts, outputFile);

    auto endTime = chrono::high_resolution_clock::now();

    double timeTakenInSeconds = chrono::duration<double>(endTime - startTime).count();
    Total_Words += Word_Counts.size();
    Total_Time += timeTakenInSeconds;

    file_Sizes.push_back(fileSizeInMiB);
    Progress_Values.push_back(Word_Counts.size() / timeTakenInSeconds); // Calculate throughput
}

// Function to process all files in a directory and measure time
void processAllFilesInDirectory(const fs::path& inputDirectory, const fs::path& outputDirectory,
                                long long& Total_Words, double& Total_Time,
                                vector<double>& file_Sizes, vector<double>& Progress_Values) {
    for (const auto& fileEntry : fs::recursive_directory_iterator(inputDirectory)) {
        if (fileEntry.is_regular_file() && fileEntry.path().extension() == ".txt") {
            fs::path Relative_Path = fs::relative(fileEntry.path(), inputDirectory);
            fs::path Output_Path = outputDirectory / Relative_Path;
            fs::create_directories(Output_Path.parent_path());

            processSingleFile(fileEntry.path(), Output_Path, Total_Words, Total_Time, file_Sizes, Progress_Values);
        }
    }
}

// Function to process all files in a directory without measuring time
void processAllFilesInDirectoryWithoutTiming(const fs::path& inputDirectory, const fs::path& outputDirectory) {
    for (const auto& fileEntry : fs::recursive_directory_iterator(inputDirectory)) {
        if (fileEntry.is_regular_file() && fileEntry.path().extension() == ".txt") {
            fs::path Relative_Path = fs::relative(fileEntry.path(), inputDirectory);
            fs::path Output_Path = outputDirectory / Relative_Path;
            fs::create_directories(Output_Path.parent_path());

            auto Word_Counts = readWordCountsFromFile(fileEntry.path());

            // Sort word_cnt counts by frequency in descending order
            sortWordCountsDescending(Word_Counts);

            // Write sorted word_cnt counts to the output file
            writeSortedWordCountsToFile(Word_Counts, Output_Path);
        }
    }
}

// Function to print statistics (total words, total time, etc.)
void printProcessingStatistics(double Total_Words, double Total_Time,
                                const vector<double>& file_Sizes, const vector<double>& Progress_Values) {
    cout << "Total words in all files: " << Total_Words << "\n";
    cout << "Total time taken for processing: " << Total_Time << " seconds\n";

    for (size_t i = 0; i < file_Sizes.size(); ++i) {
        cout << "File size: " << file_Sizes[i] << " MiB, Throughput: " << Progress_Values[i] << " words/second\n";
    }
}

int main(int argc, char* Argument[]) {
    if (argc != 3) {
        cerr << "Usage: " << Argument[0] << " <inputDirectory> <outputDirectory>\n";
        return 1;
    }

    fs::path inputDirectory(Argument[1]);
    fs::path outputDirectory(Argument[2]);
    long long Total_Words = 0;
    double Total_Time = 0.0;
    vector<double> file_Sizes, Progress_Values;

    processAllFilesInDirectory(inputDirectory, outputDirectory, Total_Words, Total_Time, file_Sizes, Progress_Values);

    // Plotting
    plot::plot(file_Sizes, Progress_Values, "bo"); // "bo" stands for "blue circle" markers
    plot::xlabel("File Size (MiB)");
    plot::ylabel("(words/second)");
    plot::title("Throughput vs. File Size");
    plot::show();

    return 0;
}
