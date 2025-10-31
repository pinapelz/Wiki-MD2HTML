#ifndef RCLONE_UPLOADER_H
#define RCLONE_UPLOADER_H

#include "file_uploader.hpp"
#include <string>

class RcloneUploader : public FileUploader {

public:
    ~RcloneUploader();
    RcloneUploader(const std::string& cdnEndpoint, const std::string& sourceName);
    bool uploadFile(const std::string& filepath, const std::string& remotePath) override;
    bool testConnection() override;

private:
    std::string cdnEndpoint;
    std::string sourceName;
};

#endif // RCLONE_UPLOADER_H
