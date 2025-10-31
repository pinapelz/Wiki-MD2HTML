#pragma once
#ifndef FILE_UPLOADER_H
#define FILE_UPLOADER_H

#include <string>

class FileUploader {
public:
    virtual ~FileUploader() = default;
    virtual bool uploadFile(const std::string& filepath, const std::string& remotePath) = 0;
    virtual bool testConnection() = 0;
};

#endif // FILE_UPLOADER_H
