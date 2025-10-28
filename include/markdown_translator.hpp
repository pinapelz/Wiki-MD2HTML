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
    std::string translate(const std::string& markdownContent, const std::string& cssPath = "styles/carbon.css");
    std::string processLine(const std::string& line);

private:
    // Regex for various tags
    const std::string headerRegexStr{"^(#{1,6})\\s+(.*)$"};
    const std::string boldRegexStr{"\\*\\*([^\\*]+)\\*\\*|__([^_]+)__"};
    const std::string italicRegexStr{"\\*([^\\*]+)\\*|_([^_]+)_"};
    const std::string linkRegexStr{"\\[([^\\]]+)\\]\\(([^\\)]+)\\)"};
    const std::string imageRegexStr{"!\\[(.*?)\\]\\(([^\\s\"]+)(\\s+\"(.*?)\")?\\)"};
    // Helper functions for different markdown elements
    void processMetadata(const std::vector<std::string>& lines);
    std::string processHeaders(const std::string& line);
    std::string processBold(const std::string& text);
    std::string processItalic(const std::string& text);
    std::string processLinks(const std::string& text);
    std::string processParagraph(const std::string& text);
    std::string processSingleFigure(const std::string& text);
    std::string processFigureBlock(const std::vector<std::string>& lines);
    // Navigation and table of contents
    void generateSideBar(std::stringstream& output, const std::vector<std::string>& headers);
    std::string createAnchorId(const std::string& text);
    // Utility functions

    std::string getCurrentDateTime() {
        std::time_t now = std::time(nullptr);
        std::tm* localTime = std::localtime(&now);

        char buffer[80];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);

        return std::string(buffer);
    }

    // HTML builders
    std::string buildHTMLHeader(const std::string& title, const std::string& cssPath){
        return R"(<!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>)" + title + R"(</title>
            <link rel="stylesheet" href=")" + cssPath + R"(">
        </head>
        <body>
        )";
    }

    std::string buildHTMLFooter(){
                return R"( <div class="article-meta">
                        <p>Last updated: )" + getCurrentDateTime() + R"(</p>
                    </div>
                </div>
            </div>
        </body>
        </html>
        )";
    }

    // Member variables
    std::string title;
};

#endif // MARKDOWN_TRANSLATOR_H
