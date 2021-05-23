/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 * ddci3   - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

class cFileList {
private:
  std::vector<std::string> l;
public:
  cFileList(std::string aDirectory, std::string Filter = "") {
     for(DIR* dirp = opendir(aDirectory.c_str()); dirp; ) {
        errno = 0;
        struct dirent* d = readdir(dirp);

        if (d != nullptr) {
           std::string name(d->d_name);
           if (name.find(Filter) != std::string::npos)
              l.push_back(name);
           }
        else {
           closedir(dirp);
           dirp = nullptr;
           }
        }
     }
  size_t size(void) { return l.size(); }
  std::vector<std::string> List(void) { return l; }
};

class File {
public:
  static bool Exists(std::string FileName) {
    return (access(FileName.c_str(), R_OK) == 0);
    }
};
