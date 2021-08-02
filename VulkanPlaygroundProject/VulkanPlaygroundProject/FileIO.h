#pragma once

#include <map>
#include <fstream>
#include <sstream>
//cache can hold multiple hashes..

class FileIO {
public:
   FileIO(std::string aPath);

   std::string Read();
   void StoreCache(std::size_t aHash, char* aInput, std::size_t aSize);

   void Save();

   const std::string GetFileExtension() const {
      return mFileExt;
   };

   const void GetHashFileData(std::size_t aHash, char* aOutput);

   const std::size_t GetHashFileSize(std::size_t aHash) {
      auto cache = mOldCacheMap.find(aHash);
      if (cache != mOldCacheMap.end()) {
         return cache->second.mSize;
      }
      return 0;
   };

   const bool GetIsHashNewer(std::size_t aHash) {
      auto cache = mOldCacheMap.find(aHash);
      if (cache != mOldCacheMap.end()) {
         return cache->second.mTime > mFileModifyTime;
      }
      return false;
   };


private:
   void StoreCache(std::size_t aHash, char* aInput, std::size_t aSize, time_t aTime);

   void PrepareCache();

   std::string mFilePath;
   std::string mFileName;
   std::string mFileExt;
   std::ifstream mCacheFileStream;
   bool mHasFile;
   bool mHasCache;
   std::size_t mFileModifyTime;


   struct Cache {
      std::size_t mSize;
      std::size_t mTime;
   };
   struct NewCache : public Cache {
      char* mData = nullptr;
   };
   struct OldCache : public Cache {
      std::size_t mOffset;
   };
   //hash, Cache
   std::map<std::size_t, NewCache> mNewCacheMap;
   std::map<std::size_t, OldCache> mOldCacheMap;
};