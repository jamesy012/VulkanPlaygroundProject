#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <assert.h>

#include <thread>

//use chrome://tracing/ to see result
#define ALLOW_PROFILING 1
#if ALLOW_PROFILING
#define __PROFILE_TEXT_COMBINE(x,y) x##y
#define PROFILE_START(name) Profiler::Get().BeginEvent(name, __FUNCTION__ );
#define __PROFILE_START_SCOPED(varName, counter, name) ScopedEvent __PROFILE_TEXT_COMBINE(varName, counter) (name, __FUNCTION__);
#define PROFILE_START_SCOPED(name) __PROFILE_START_SCOPED(___SCOPEDEVENT_,__COUNTER__,name)
#define PROFILE_END() Profiler::Get().EndEvent();
#define PROFILE_MARKER(name) Profiler::Get().AddMarker(name, __FUNCTION__);
#else
#define PROFILE_START(name) 
#define PROFILE_START_SCOPED(name) 
#define PROFILE_END() 
#define PROFILE_MARKER(name)
#endif

static class Profiler* instance;

class Profiler {
   struct Event {
      std::string name;
      std::string function;
      std::thread::id id;
      std::chrono::time_point<std::chrono::high_resolution_clock> start;
      std::chrono::time_point<std::chrono::high_resolution_clock> end;
      //float duration;
      int stack;
   };
public:

   void StartProfile() {
#if ALLOW_PROFILING
      mEvents.resize(0);
      mMarkers.resize(0);
      mStack = 0;
      mStartProfile = true;
#endif
   }

   void EndProfile() {
#if ALLOW_PROFILING
      mOutputStream.open(GetWorkDir() + "Profile.json");
      mOutputStream << "{\"otherData\": {},\"traceEvents\":[";
      for (int i = 0; i < mEvents.size(); i++) {
         Event& e = mEvents[i];
         bool inStack = false;
         for (int q = 0; q < mStack+1; q++) {
            if (i == mEventStack[q]) {
               inStack = true;
               break;
            }
         }
         if (inStack) {
            continue;
         }
         long long start = std::chrono::time_point_cast<std::chrono::microseconds>(e.start).time_since_epoch().count();
         long long end = std::chrono::time_point_cast<std::chrono::microseconds>(e.end).time_since_epoch().count();
         long long dur = end - start;
         if (i != 0) {
            mOutputStream << ",";
         }
         mOutputStream << "{";
         mOutputStream << "\"cat\":\"function\",";
         mOutputStream << "\"dur\":" << (dur) << ',';
         mOutputStream << "\"name\":\"" << e.name << "\",";
         mOutputStream << "\"ph\":\"X\",";
         mOutputStream << "\"pid\":0,";
         if (mThreads.find(e.id) == mThreads.end()) {
            mThreads[e.id] = std::to_string(std::hash<std::thread::id>{}(e.id));
         }
         mOutputStream << "\"tid\":\"" << mThreads[e.id] << "\",";
         mOutputStream << "\"ts\":" << start << ",";
         mOutputStream << "\"args\": {";
         mOutputStream << "\"Function\":\"" << e.function << "\"";
         mOutputStream << "}";
         /*
               "args": {
                  "someArg": 1,
                   "anotherArg": {
                     "value": "my value"
                     }
                  }
         */
         mOutputStream << "}";
      }
      for (int i = 0; i < mMarkers.size(); i++) {
         if (mEvents.size() != 0 || i != 0) {
            mOutputStream << ",";
         }
         Event& e = mMarkers[i];
         long long start = std::chrono::time_point_cast<std::chrono::microseconds>(e.start).time_since_epoch().count();
         mOutputStream << "{";
         mOutputStream << "\"cat\":\"marker\",";
         mOutputStream << "\"name\":\"" << e.name << "\",";
         mOutputStream << "\"ph\":\"O\",";
         mOutputStream << "\"pid\":0,";
         mOutputStream << "\"id\":"<<i<<",";
         if (mThreads.find(e.id) == mThreads.end()) {
            mThreads[e.id] = std::to_string(std::hash<std::thread::id>{}(e.id));
         }
         mOutputStream << "\"tid\":\"" << mThreads[e.id] << "\",";
         mOutputStream << "\"ts\":" << start << ",";
         mOutputStream << "\"args\": {";
         mOutputStream << "\"Function\":\"" << e.function << "\",";
         mOutputStream << "\"snapshot\":\"" << e.name << "\"";
         mOutputStream << "}";
         /*
               "args": {
                  "someArg": 1,
                   "anotherArg": {
                     "value": "my value"
                     }
                  }
         */
         mOutputStream << "}";
      }
      mOutputStream << "]}";
      mOutputStream.flush();
#endif
   }

   void BeginEvent(std::string aName, std::string aFunction) {
      if (mStartProfile) {
         mRunning = true;
         mStartProfile = false;
         mStartTime = std::chrono::high_resolution_clock::now();
      }
      if (mRunning) {
         Event e;
         e.name = aName;
         e.stack = mStack++;
         if (mStack >= 20) {
            mStack = 19;
         }
         e.function = aFunction;
         e.id = std::this_thread::get_id();
         //e.start = std::chrono::high_resolution_clock::now();
         mEvents.push_back(e);
         
         mEventStack[mStack] = (int)mEvents.size() - 1;
         mEvents[mEvents.size() - 1].start = std::chrono::high_resolution_clock::now();
      }
   }

   void EndEvent() {
      if (mRunning) {
         std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
         Event& e = mEvents[mEventStack[mStack--]];
         assert(e.stack == mStack);

         e.end = end;
         //long long mStartTime = std::chrono::time_point_cast<std::chrono::microseconds>(e.mStartTime).time_since_epoch().count();
         //long long end = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
         //e.name += " (" + std::to_string(e.duration / 1000.0f) + "ms)";
      }
   }

   void AddMarker(std::string aName, std::string aFunction) {
      if (mStartProfile) {
         mRunning = true;
         mStartProfile = false;
      }
      if (mRunning) {
         Event e;
         e.name = aName;
         e.start  = std::chrono::high_resolution_clock::now();         
         e.function = aFunction;
         mMarkers.push_back(e);
      }
   }

   void SetThreadName(std::string aName) {      
      mThreads[std::this_thread::get_id()] = aName + " (" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ")";
   }

   static Profiler& Get() {
      if (instance == nullptr) {
         instance = new Profiler();
      }
      return *instance;
   }
private:
   std::ofstream mOutputStream;
   int mStack = 0;
   bool mRunning = false;
   int mEventStack[20]{ -1 };
   std::vector<Event> mEvents;
   std::vector<Event> mMarkers;
   bool mStartProfile = false;
   std::map<std::thread::id, std::string> mThreads;
   std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime;
};

class ScopedEvent {
public:
   ScopedEvent(std::string aName, std::string aFunction) {
      Profiler::Get().BeginEvent(aName, aFunction);
   }
   ~ScopedEvent() {
      Profiler::Get().EndEvent();
   }
};

