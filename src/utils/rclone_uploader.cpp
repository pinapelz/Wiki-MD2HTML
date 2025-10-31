#include <stdexcept>
#include "rclone_uploader.hpp"

RcloneUploader::~RcloneUploader() = default;

RcloneUploader::RcloneUploader(const std::string& cdnEndpoint,
                               const std::string& sourceName)
    : cdnEndpoint(cdnEndpoint), sourceName(sourceName)
{
    std::string rcloneInstalledCommand;
    #ifdef _WIN32
        rcloneInstalledCommand = "where rclone >nul 2>&1";
    #else
        rcloneInstalledCommand = "which rclone >/dev/null 2>&1";
    #endif

    int result = std::system(rcloneInstalledCommand.c_str());
    if (result != 0) {
        throw std::runtime_error("rclone is not installed or not in PATH");
    }
}

bool RcloneUploader::uploadFile(const std::string& filepath, const std::string& remotePath=""){
    std::string command = "rclone copy \"" + filepath + "\" " + sourceName + ":" + remotePath + " 2>/dev/null 1>/dev/null";
    int result = std::system(command.c_str());
    return result == 0;
}

bool RcloneUploader::testConnection(){
    std::string command = "rclone ls " + sourceName + ": 2>/dev/null 1>/dev/null";
    int result = std::system(command.c_str());
    return result == 0;
}
