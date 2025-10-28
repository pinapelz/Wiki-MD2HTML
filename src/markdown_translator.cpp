#include "markdown_translator.hpp"
#include <regex>
#include <iostream>
#include <algorithm>

MarkdownTranslator::MarkdownTranslator() : title("Carbon") {

}

MarkdownTranslator::~MarkdownTranslator() {
}

void MarkdownTranslator::processMetadata(const std::vector<std::string>& lines){
    // format of keys -> key: value
    for(std::string currentLine : lines){
        size_t colonPos = currentLine.find(':');
        if (colonPos != std::string::npos) {
            std::string key = currentLine.substr(0, colonPos);
            std::string value = currentLine.substr(colonPos + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            if (key == "title") {
                title = value;
            }
        }
    }
}

std::string MarkdownTranslator::translate(const std::string& markdownContent, const std::string& cssPath) {
    std::stringstream htmlOutput;
    std::stringstream markdownStream(markdownContent);
    std::string line;
    std::vector<std::string> headers;

    std::string currentLine;

    bool metadataExists{false};
    std::vector<std::string> metadataLines;
    while (std::getline(markdownStream, currentLine)) {
        if(currentLine == "---"){
            if(metadataExists){
                break;
            } else {
                metadataExists = true;
                continue;
            }
        }
        if(!metadataExists){
            break;
        }
        metadataLines.push_back(currentLine);
    }
    processMetadata(metadataLines);

    while (std::getline(markdownStream, currentLine)) {
        std::regex headerRegex("^(#{1,3})\\s+(.*)$");
        std::smatch matches;
        if (std::regex_match(currentLine, matches, headerRegex)) {
            int level = matches[1].length();
            std::string content = matches[2];
            if (level <= 3) {
                headers.push_back(std::to_string(level) + ":" + content);
            }
        }
    }
    markdownStream.clear();
    markdownStream.str(markdownContent);

    htmlOutput << buildHTMLHeader(title, cssPath);
    // Add navigation sidebar
    generateSideBar(htmlOutput, headers);

    // Main content container
    htmlOutput << "    <div class=\"main-content\">\n";
    htmlOutput << "        <div class=\"container\">\n";

    // Process each line of markdown

    // State of current parse
    bool inFigureBlock{false};
    std::vector<std::string> figureLines;

    while (std::getline(markdownStream, line)) {
        if(line.find(":::figure") == 0){
            inFigureBlock = true;
            figureLines.clear();
            continue;
        }

        if(inFigureBlock && line == ":::"){
            inFigureBlock = false;
            htmlOutput << "            " << processFigureBlock(figureLines) << "\n";
            continue;
        }
        else if(inFigureBlock){
            figureLines.push_back(line);
            continue;
        }

        htmlOutput << "            " << processLine(line);
    }


    return htmlOutput.str();
}

void MarkdownTranslator::generateSideBar(std::stringstream& output, const std::vector<std::string>& headers) {
    output << "    <div class=\"nav-sidebar\">\n";
    output << "        <div class=\"nav-logo\">\n";
    output << "            <h3>" + title + "</h3>\n";
    output << "        </div>\n";
    output << "        <ul class=\"nav-menu\">\n";

    output << "            <h4>Table of Contents</h4>\n";
    output << "            <li><a href=\"index.html\">Home</a></li>\n";
    output << "            <li><a href=\"#\">Recent Changes</a></li>\n";
    output << "            <li><a href=\"#\">Random Page</a></li>\n";
    output << "            <ul class=\"nav-submenu\">\n";

    // Generate navigation from headers
    for (const auto& header : headers) {
        size_t separatorPos = header.find(':');
        if (separatorPos != std::string::npos) {
            int level = std::stoi(header.substr(0, separatorPos));
            std::string content = header.substr(separatorPos + 1);

            // Create anchor ID from header content
            std::string anchorId = createAnchorId(content);

            // Add indentation based on header level
            std::string indentation = "";
            for (int i = 1; i < level; ++i) {
                indentation += "    ";
            }

            output << "                " << indentation << "<li><a href=\"#" << anchorId << "\">" << content << "</a></li>\n";
        }
    }

    output << "            </ul>\n";
    output << "        </ul>\n";
    output << "    </div>\n";
}

std::string MarkdownTranslator::createAnchorId(const std::string& text) {
    std::string id = text;
    // Convert to lowercase
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);

    // Replace spaces with hyphens
    std::replace(id.begin(), id.end(), ' ', '-');

    // Remove any non-alphanumeric characters except hyphens
    id.erase(
        std::remove_if(
            id.begin(),
            id.end(),
            [](char c) { return !(std::isalnum(c) || c == '-'); }
        ),
        id.end()
    );

    return id;
}

std::string MarkdownTranslator::processLine(const std::string& line) {
    std::string processed = line;
    processed = processHeaders(processed);
    if (processed == line) {
        processed = processBold(processed);
        processed = processItalic(processed);
        processed = processLinks(processed);
        if (!processed.empty() && processed[0] != '<') {
            processed = processParagraph(processed);
        }
    }

    return processed + "\n";
}

std::string MarkdownTranslator::processHeaders(const std::string& line) {
    // Check for H1-H6
    std::regex headerRegex(headerRegexStr);
    std::smatch matches;
    if (std::regex_match(line, matches, headerRegex)) {
        int level = matches[1].length();
        std::string content = matches[2];
        std::string anchorId = createAnchorId(content);
        return "<h" + std::to_string(level) + " id=\"" + anchorId + "\">" + content + "</h" + std::to_string(level) + ">";
    }

    return line;
}

std::string MarkdownTranslator::processBold(const std::string& text) {
     // Replace **text** or __text__ with <strong>text</strong>
    std::string result = text;
    std::regex boldRegex(boldRegexStr);
    result = std::regex_replace(result, boldRegex, "<strong>$1$2</strong>");
    return result;
}

std::string MarkdownTranslator::processItalic(const std::string& text) {
    // Replace *text* or _text_ with <em>text</em>
    std::string result = text;
    std::regex italicRegex(italicRegexStr);
    result = std::regex_replace(result, italicRegex, "<em>$1$2</em>");
    return result;
}

std::string MarkdownTranslator::processLinks(const std::string& text) {
    // Replace [text](url) with <a href="url">text</a>
    std::string result = text;
    std::regex linkRegex(linkRegexStr);
    result = std::regex_replace(result, linkRegex, "<a href=\"$2\">$1</a>");
    return result;
}

std::string MarkdownTranslator::processParagraph(const std::string& text) {
    if (text.empty()) return "";
    return "<p>" + text + "</p>";
}

std::string MarkdownTranslator::processSingleFigure(const std::string& text) {
    // Extract image details using regex
    std::regex imageRegex(imageRegexStr);
    std::smatch matches;

    if (std::regex_search(text, matches, imageRegex)) {
        std::string alt = matches[1].str();
        std::string src = matches[2].str();
        std::string title = matches.size() > 4 && matches[4].matched ? " title=\"" + matches[4].str() + "\"" : "";
        return "<img src=\"" + src + "\" alt=\"" + alt + "\"" + title + ">";
    }
    return text;
}

std::string MarkdownTranslator::processFigureBlock(const std::vector<std::string>& lines){
    std::stringstream html;
    std::string imageHtml;
    std::string caption;
    html << "<figure class=\"custom-figure\">\n";

    // Process each line in the figure block
    for (const auto& line : lines) {
        if (line.find("![") == 0) {
            // Process the image
            imageHtml = processSingleFigure(line);
            html << "    " << imageHtml << "\n";
        } else if (line.find("Caption:") == 0) {
            // Extract caption
            caption = line.substr(8);
            html << "    <figcaption>" << caption << "</figcaption>\n";
        } else if (!line.empty()) {
            // Process any other content
            html << "    <p>" << line << "</p>\n";
        }
    }
    html << "</figure>";
    return html.str();
}
