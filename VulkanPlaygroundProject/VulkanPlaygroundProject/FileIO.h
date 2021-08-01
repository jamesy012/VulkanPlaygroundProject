#pragma once

#include <fstream>
#include <sstream>
//cache can hold multiple hashes..

class FileIO {
public:
   FileIO(std::string aPath);

   //void ReadFile(std::stringstream& aOutput, bool aForceOriginal = false, std::size_t aHash = 0);
   //void ReadFileBinary(std::vector<char>& aOutput, bool aForceOriginal = false, std::size_t aHash = 0);
   //void ReadFileBinary(char* aOutput, bool aForceOriginal = false, std::size_t aHash = 0);

   const std::string GetFileExtension() const {
      return mFileExt;
   };

   //temp
   FileIO(std::string aPath, std::size_t aHash);

   void ReadNormal(std::string& aOutput);
   void ReadCache(char* aOutput);
   void WriteCache(char* aInput, std::size_t aSize);

   const bool IsOrginalFileNewer() const {
      return mOrginialNewer;
   };
   const std::size_t GetCacheSize() const {
      return mCacheSize;
   };

private:
   std::string mFilePath;
   std::string mFileName;
   std::string mFileExt;
   std::ifstream mCacheFileStream;
   bool mHasFile;
   bool mHasCache;


   //temp
   std::string mFileCachePath;
   bool mOrginialNewer;
   std::size_t mCacheSize;
};

