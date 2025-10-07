#ifndef MARKDOWN_TRANSLATOR_H
#define MARKDOWN_TRANSLATOR_H

#include <string>
#include <vector>
#include <sstream>
#include <ctime>

class MarkdownTranslator {
public:
    // Constructor
    MarkdownTranslator();
    // Destructor
    ~MarkdownTranslator();
    // Main translation function - takes markdown content and returns HTML
    std::string translate(const std::string& markdownContent, const std::string& cssPath = "styles/ffxiv-style.css", const std::string& title = "Title");
    std::string processLine(const std::string& line);

private:
    // Helper functions for different markdown elements
    std::string processHeaders(const std::string& line);
    std::string processBold(const std::string& text);
    std::string processItalic(const std::string& text);
    std::string processLinks(const std::string& text);
    std::string processParagraph(const std::string& text);
    std::string processSingleFigure(const std::string& text);
    std::string processFigureBlock(const std::vector<std::string>& lines);
    // Navigation and table of contents
    void generateSideBar(std::stringstream& output, const std::vector<std::string>& headers, const std::string& title);
    std::string createAnchorId(const std::string& text);
    // Utility functions
    std::string getCurrentDateTime();
};

#endif // MARKDOWN_TRANSLATOR_H
