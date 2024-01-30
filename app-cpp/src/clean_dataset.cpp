#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <regex>
#include <algorithm>
#include "matplotlib-cpp/matplotlibcpp.h"  // Include the matplotlibcpp header

// Namespace aliases for brevity
namespace plot = matplotlibcpp;
namespace fs = std::filesystem;
using namespace std;

// Function to open input and output files
bool openFiles(const fs::path& inputFile, const fs::path& outputFile, ifstream& inFile, ofstream& outFile_Main) {
    // Open input and output files
    inFile.open(inputFile);
    outFile_Main.open(outputFile);

    // Check if files were opened successfully
    if (!inFile.is_open() || !outFile_Main.is_open()) {
        cerr << "Error opening file(s).\n";
        return false;
    }

    return true;
}

// Function to cleanse each line
string cleanseLine(const string& inputLine) {
    // Remove '\r' characters
    string cleanedLine = inputLine;
    cleanedLine.erase(remove(cleanedLine.begin(), cleanedLine.end(), '\r'), cleanedLine.end());

    // Replace non-alphanumeric characters (excluding delimiters) with nothing (remove them)
    cleanedLine = regex_replace(cleanedLine, regex("[^0-9a-zA-Z \\t\\n\\r]+"), "");

    // Replace multiple delimiters with the last one in the sequence
    cleanedLine = regex_replace(cleanedLine, regex("([ \\t\\n\\r])\\1+"), "$1");

    return cleanedLine;
}

// Function to process each file
void processFile(const fs::path& inputFile, const fs::path& outputFile) {
    ifstream inFile;
    ofstream outFile_Main;

    // Open input and output files
    if (!openFiles(inputFile, outputFile, inFile, outFile_Main)) {
        return; // Abort processing if files cannot be opened
    }

    string currentLine;
    // Process each line in the input file
    while (getline(inFile, currentLine)) {
        outFile_Main << cleanseLine(currentLine) << "\n";
    }
}

// Function to process a single file and measure time
void processSingleFile(const fs::path& inputFile, const fs::path& outputFile,
                       double& total_Size, vector<double>& sizes_, vector<double>& times_) {
    // Calculate file size in MiB
    double file_Size = fs::file_size(inputFile) / 1024.0 / 1024.0;

    // Start measuring wall time using high_resolution_clock
    auto startTime = chrono::high_resolution_clock::now();

    // Process the file
    processFile(inputFile, outputFile);

    // Stop measuring wall time
    auto endTime = chrono::high_resolution_clock::now();

    // Calculate the elapsed_different time (wall time) for processing the file
    chrono::duration<double> elapsed_different = endTime - startTime;
    double timeTaken = elapsed_different.count();

    // Print the wall time for the current file
    cout << "Time taken for " << inputFile.filename() << ": " << timeTaken << " seconds.\n";

    // Update total size, sizes_, and times_ vectors
    total_Size += file_Size;
    sizes_.push_back(file_Size);
    times_.push_back(timeTaken);
}

// Function to process all files in a directory and measure time
void processAllFilesInDirectory(const fs::path& inputDirectory, const fs::path& outputDirectory,
                                double& total_Size, vector<double>& sizes_, vector<double>& times_) {
    // Iterate through all files in the input directory
    for (const auto& fileEntry : fs::recursive_directory_iterator(inputDirectory)) {
        // Process only regular files with ".txt" extension
        if (fileEntry.is_regular_file() && fileEntry.path().extension() == ".txt") {
            fs::path Relative_Path = fs::relative(fileEntry.path(), inputDirectory);
            fs::path Output_Path = outputDirectory / Relative_Path;
            fs::create_directories(Output_Path.parent_path());

            // Process a single file and measure time
            processSingleFile(fileEntry.path(), Output_Path, total_Size, sizes_, times_);
        }
    }
}

// Function to process all files in a directory without measuring time
void processAllFilesInDirectory(const fs::path& inputDirectory, const fs::path& outputDirectory) {
    // Iterate through all files in the input directory
    for (const auto& fileEntry : fs::recursive_directory_iterator(inputDirectory)) {
        // Process only regular files with ".txt" extension
        if (fileEntry.is_regular_file() && fileEntry.path().extension() == ".txt") {
            fs::path Relative_Path = fs::relative(fileEntry.path(), inputDirectory);
            fs::path Output_Path = outputDirectory / Relative_Path;
            fs::create_directories(Output_Path.parent_path());

            // Process a single file without measuring time
            processFile(fileEntry.path(), Output_Path);
        }
    }
}

// Function to process a single file without measuring time
void processSingleFile(const fs::path& inputFile, const fs::path& outputFile) {
    // Process a single file without measuring time
    processFile(inputFile, outputFile);
}

// Function to print statistics (total size, average size, average time)
void printStatistics(double total_Size, const vector<double>& sizes_, const vector<double>& times_) {
    // Print total size of processed files
    cout << "Total size of processed files: " << total_Size << " MiB\n";

    // Print average size of processed files if sizes_ vector is not empty
    if (!sizes_.empty()) {
        double averageSize = accumulate(sizes_.begin(), sizes_.end(), 0.0) / sizes_.size();
        cout << "Average size of processed files: " << averageSize << " MiB\n";
    }

    // Print average processing time per file if times_ vector is not empty
    if (!times_.empty()) {
        double averageTime = accumulate(times_.begin(), times_.end(), 0.0) / times_.size();
        cout << "Average processing time per file: " << averageTime << " seconds\n";
    }
}

// Main function with command-line arguments
int main(int argc, char* argument[]) {
    // Check if the correct number of command-line arguments is provided
    if (argc != 3) {
        cerr << "Used: " << argument[0] << " <InputDirectory> <OutputDirectory>\n";
        return 1;
    }

    // Parse command-line arguments to get input and output directories
    fs::path InputDirectory(argument[1]);
    fs::path OutputDirectory(argument[2]);

    // Initialize variables for statistics
    double total_Size = 0.0;
    vector<double> sizes_, times_;

    // Process all files in the input directory and measure time
    processAllFilesInDirectory(InputDirectory, OutputDirectory, total_Size, sizes_, times_);

    // Plotting
    vector<double> throughput;
    for (size_t i = 0; i < sizes_.size(); ++i) {
        // Calculate throughput for each dataset
        throughput.push_back(sizes_[i] / times_[i]);
    }

    // Assuming your_plot_library has a plot function with similar parameters
    // Adjust the function call based on the actual plotting library you are using
    plot::plot(sizes_, throughput, "bo"); // "bo" stands for "blue circle" markers
    plot::xlabel("MiB");
    plot::ylabel("MiB/second");
    plot::title("Throughput vs. Dataset Size");
    plot::show();

    // Print statistics
    printStatistics(total_Size, sizes_, times_);

    return 0;
}
