#include "stdafx.h"
#include "FileIO.h"

bool ReadFile(std::string aPath, std::stringstream& aOutput) {
   std::ifstream fileStream(aPath);
   if (fileStream.fail()) {
      return false;
   }
   aOutput = std::stringstream();
   aOutput << fileStream.rdbuf();
   return true;
}

bool ReadFileBinary(std::string aPath, char* aOutput, uint32_t aSize) {
   std::ifstream fileStream(aPath, std::ios::binary);
   if (fileStream.fail()) {
      return false;
   }
   fileStream.read(aOutput, aSize);
   return true;
}

bool WriteFile(std::string aPath, std::string aInput) {
   std::ofstream fileStream(aPath);
   if (fileStream.fail()) {
      return false;
   }
   fileStream << aInput;
   fileStream.close();
   return true;
}

bool WriteFileBinary(std::string aPath, const char* aInput, uint32_t aSize) {
   std::ofstream fileStream(aPath, std::ios::binary);
   if (fileStream.fail()) {
      return false;
   }
   fileStream.write(aInput, aSize);
   fileStream.close();
   return true;
}

FileIO::FileIO(std::string aPath) {

}

FileIO::FileIO(std::string aPath, std::size_t aHash) {
   mFilePath = aPath;
   size_t fileExtIndex = aPath.find_last_of('.');
   if (fileExtIndex != std::string::npos) {
      mFileExt = aPath.substr(fileExtIndex + 1);
      for (auto& c : mFileExt) {
         c = tolower(c);
      }
   } else {
      ASSERT_RET("Invalid file name?");
   }

   size_t fileNameIndex = aPath.find_last_of('\\');
   if (fileNameIndex == std::string::npos) {
      fileNameIndex = aPath.find_last_of('/');
   }
   if (fileNameIndex != std::string::npos) {
      mFileName = aPath.substr(fileNameIndex + 1, aPath.size() - (aPath.size() - fileExtIndex) - fileNameIndex - 1);
   }
   mFileCachePath = aPath;
   mFileCachePath.insert(fileExtIndex, "-" + std::to_string(aHash));
   mFileCachePath += ".cache";

   struct _stat stat {};
   struct _stat statCache {};
   mHasFile = !_stat(aPath.c_str(), &stat);
   mHasCache = !_stat(mFileCachePath.c_str(), &statCache);

   mCacheSize = statCache.st_size;

   mOrginialNewer = (stat.st_mtime > statCache.st_mtime);
}

void FileIO::ReadNormal(std::string& aOutput) {
   std::ifstream fileStream(mFilePath);
   std::stringstream dataStream;
   dataStream << fileStream.rdbuf();
   aOutput = dataStream.str();
}

void FileIO::ReadCache(char* aOutput) {
   ReadFileBinary(mFileCachePath, aOutput, mCacheSize);
}

void FileIO::WriteCache(char* aInput, std::size_t aSize) {
   WriteFileBinary(mFileCachePath, aInput, aSize);
}
