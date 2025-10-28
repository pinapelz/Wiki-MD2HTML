#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include "markdown_translator.h"

bool parseArguments(int argc, char* argv[], std::unordered_map<std::string, std::string>& params, std::string& inputFile) {
    if (argc < 2) {
        std::cerr << "Usage: COMMAND <input_file> [-o <output_file>] [-css <css_file_path>] [other options]\n";
        return false;
    }
    params["-o"] = "index.html";
    params["-css"] = "styles/ffxiv-style.css";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            if (i + 1 < argc) {
                params[arg] = argv[i + 1];
                ++i;
            } else {
                std::cerr << "Error: Missing value for option " << arg << "\n";
                return false;
            }
        } else {
            if (inputFile.empty()) {
                inputFile = arg;
            } else {
                std::cerr << "Error: Multiple input files specified: " << inputFile << " and " << arg << "\n";
                return false;
            }
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> params;
    std::string inputFile;

    if (!parseArguments(argc, argv, params, inputFile)) {
        return 1;
    }

    std::ifstream file(inputFile);
    if (!file) {
        std::cerr << "Error: Could not open file " << inputFile << "\n";
        return 1;
    }

    // Read entire file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string markdownContent = buffer.str();
    file.close();

    // Create translator and convert markdown to HTML
    MarkdownTranslator translator;

    // Get CSS path from parameters or use default
    std::string cssPath = "styles/ffxiv-style.css";
    if (params.find("-css") != params.end()) {
        cssPath = params["-css"];
    }

    std::string htmlOutput = translator.translate(markdownContent, cssPath);

    // Write to output file
    std::string outputFile = params["-o"];
    std::ofstream outFile(outputFile);
    if (!outFile) {
        std::cerr << "Error: Could not open output file " << outputFile << "\n";
        return 1;
    }

    outFile << htmlOutput;
    outFile.close();

    std::cout << "HTML output written to " << outputFile << std::endl;
    return 0;
}
