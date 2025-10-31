#pragma once
#ifndef FILE_UPLOADER_H
#define FILE_UPLOADER_H

#include <string>

class FileUploader {
public:
    virtual ~FileUploader() = default;
    virtual std::string uploadFile(const std::string& filepath, const std::string& remotePath) = 0; // expects public url
    virtual bool testConnection() = 0;
};

#endif // FILE_UPLOADER_H
