#pragma once

#include "Core/Assert.h"

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

class Archive
{
public:
   Archive()
      : offset(0)
   {
   }

   Archive(std::size_t numBytes)
      : data(numBytes)
      , offset(0)
   {
   }

   Archive(const std::vector<uint8_t>& inData)
      : data(inData)
      , offset(0)
   {
   }

   Archive(const Archive& other)
      : data(other.data)
      , offset(other.offset)
   {
   }

   Archive(Archive&& other)
      : data(std::move(other.data))
      , offset(std::move(other.offset))
   {
      other.offset = 0;
   }

   ~Archive()
   {
      ASSERT(isAtEnd());
   }

   Archive& operator=(const Archive& other)
   {
      data = other.data;
      offset = other.offset;

      return *this;
   }

   Archive& operator=(Archive&& other)
   {
      data = std::move(other.data);
      offset = std::move(other.offset);
      other.offset = 0;

      return *this;
   }

   const std::vector<uint8_t>& getData() const
   {
      return data;
   }

   bool isAtEnd() const
   {
      return offset == data.size();
   }

   void reserve(std::size_t numBytes)
   {
      data.resize(numBytes);
   }

   template<typename T>
   bool read(T& outVal)
   {
      std::size_t numBytes = sizeof(T);
      if (offset + numBytes > data.size())
      {
         return false;
      }

      std::memcpy(&outVal, &data[offset], numBytes);
      offset += numBytes;

      return true;
   }

   template<typename T>
   void write(const T& inVal)
   {
      std::size_t numBytes = sizeof(T);
      if (offset + numBytes > data.size())
      {
         data.resize(offset + numBytes);
      }

      std::memcpy(&data[offset], &inVal, numBytes);
      offset += numBytes;
   }

private:
   std::vector<uint8_t> data;
   std::size_t offset;
};
