#pragma once

struct NonCopyable
{
   NonCopyable() : _markerValue(0) { }
   NonCopyable(const NonCopyable&) = delete;
   NonCopyable(NonCopyable&& other)
   {
      _markerValue = other._markerValue;
      other._markerValue = 0;
   }
   NonCopyable& operator=(const NonCopyable&) = delete;
   NonCopyable& operator=(NonCopyable&& other)
   {
      _markerValue = other._markerValue;
      other._markerValue = 0;
      return *this;
   }

   int _markerValue;
};
