#include "stdafx.h"
#include "FileIO.h"

#include <ctime>
#include <chrono>

FileIO::FileIO(std::string aPath) {
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

   std::string cachePath = aPath + ".cache";
   struct _stat stat {};
   struct _stat statCache {};
   mHasFile = !_stat(aPath.c_str(), &stat);
   mHasCache = !_stat(cachePath.c_str(), &statCache);

   mFileModifyTime = stat.st_mtime;

   PrepareCache();
}

std::string FileIO::Read() {
   std::ifstream fileStream(mFilePath);
   std::stringstream dataStream;
   dataStream << fileStream.rdbuf();
   return dataStream.str();
}

void FileIO::StoreCache(std::size_t aHash, char* aInput, std::size_t aSize) {
   StoreCache(aHash, aInput, aSize, std::time(0));
}

void FileIO::StoreCache(std::size_t aHash, char* aInput, std::size_t aSize, time_t atime) {
   NewCache cache;
   cache.mData = new char[aSize];
   memcpy(cache.mData, aInput, aSize);
   cache.mSize = aSize;
   cache.mTime = atime;
   mNewCacheMap[aHash] = cache;
}

template<typename T>
void WriteData(std::ofstream& stream, const T& data) {
   stream.write(reinterpret_cast<const char*>(&data), sizeof T);
}

template<typename T>
void ReadData(std::ifstream& stream, T& data) {
   stream.read(reinterpret_cast<char*>(&data), sizeof T);
}

void FileIO::Save() {
   std::string cachePath = mFilePath + ".cache";
   //save out the old stuff
   {
      std::ifstream fileStream(cachePath, std::ios::binary);
      if (fileStream.good()) {
         for (auto& element : mNewCacheMap) {
            mOldCacheMap.erase(element.first);
         }
         for (auto& element : mOldCacheMap) {
            fileStream.seekg(element.second.mOffset);
            std::vector<char> data;
            data.resize(element.second.mSize);
            fileStream.read((char*)&data[0], element.second.mSize);
            StoreCache(element.first, &data[0], element.second.mSize, element.second.mTime);
         }
         mOldCacheMap.clear();
      }
      fileStream.close();
   }

   std::ofstream fileStream(cachePath, std::ios::binary);
   if (fileStream.fail()) {
      return;
   }
   WriteData(fileStream, 'C');
   WriteData(fileStream, 'A');
   WriteData(fileStream, 'C');
   WriteData(fileStream, 'H');
   WriteData(fileStream, 'E');
   WriteData(fileStream, '1');
   WriteData(fileStream, '.');
   WriteData(fileStream, '0');

   size_t numCache = mNewCacheMap.size();

   WriteData(fileStream, numCache);
   size_t offset = (sizeof(size_t) * 4) * numCache + (sizeof(size_t) + 8);
   for (auto& element : mNewCacheMap) {
      WriteData(fileStream, element.first);
      WriteData(fileStream, element.second.mSize);
      WriteData(fileStream, element.second.mTime);
      WriteData(fileStream, offset);
      offset += element.second.mSize;
   }
   for (auto& element : mOldCacheMap) {
      WriteData(fileStream, element.first);
      WriteData(fileStream, element.second.mSize);
      WriteData(fileStream, element.second.mTime);
      WriteData(fileStream, offset);
      offset += element.second.mSize;
   }
   for (auto& element : mNewCacheMap) {
      fileStream.write(element.second.mData, element.second.mSize);
      delete[] element.second.mData;
   }
   fileStream.close();

}

const void FileIO::GetHashFileData(std::size_t aHash, char* aOutput) {
   auto cache = mOldCacheMap.find(aHash);
   if (cache == mOldCacheMap.end()) {
      return;
   }
   std::string cachePath = mFilePath + ".cache";
   std::ifstream fileStream(cachePath, std::ios::binary);
   if (fileStream.good()) {
      fileStream.seekg(cache->second.mOffset);
      fileStream.read(aOutput, cache->second.mSize);
   }
   fileStream.close();
}

void FileIO::PrepareCache() {
   std::string cachePath = mFilePath + ".cache";
   std::ifstream fileStream(cachePath, std::ios::binary);
   if (fileStream.good()) {
      fileStream.seekg(8);
      size_t numberElements = 0;
      ReadData(fileStream, numberElements);
      for (int i = 0; i < numberElements; i++) {
         size_t hash;
         OldCache cache;
         ReadData(fileStream, hash);
         ReadData(fileStream, cache.mSize);
         ReadData(fileStream, cache.mTime);
         ReadData(fileStream, cache.mOffset);
         mOldCacheMap[hash] = cache;
      }
   }
   fileStream.close();
}