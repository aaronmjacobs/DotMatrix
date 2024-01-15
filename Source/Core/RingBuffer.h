#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <span>

namespace DotMatrix
{

template<typename T, std::size_t Size>
class RingBuffer
{
public:
   RingBuffer()
      : buffer(std::make_unique<std::array<T, Size>>())
   {
   }

   void push(T element)
   {
      (*buffer)[writeOffset] = element;

      ++writeOffset;
      if (writeOffset == Size)
      {
         writeOffset = 0;
      }
   }

   void push(std::span<const T> elements)
   {
      std::size_t elementsSize = elements.size();

      std::size_t copyOffset = 0;
      while (copyOffset < elementsSize)
      {
         std::size_t space = Size - writeOffset;
         std::size_t numToCopy = std::min(elementsSize - copyOffset, space);
         std::copy_n(elements.data() + copyOffset, numToCopy, buffer->data() + writeOffset);

         writeOffset += numToCopy;
         if (writeOffset == Size)
         {
            writeOffset = 0;
         }

         copyOffset += numToCopy;
      }
   }

   T pop()
   {
      std::size_t oldReadOffset = readOffset;
      if (readOffset != writeOffset)
      {
         ++readOffset;
         if (readOffset == Size)
         {
            readOffset = 0;
         }
      }

      return (*buffer)[oldReadOffset];
   }

   std::size_t pop(std::span<T> elements, std::size_t customWriteOffset = Size)
   {
      std::size_t elementsSize = elements.size();
      std::size_t localWriteOffset = customWriteOffset < Size ? customWriteOffset : writeOffset;

      std::size_t copyOffset = 0;
      while (copyOffset < elementsSize && readOffset != localWriteOffset)
      {
         std::size_t space = Size - readOffset;
         if (localWriteOffset > readOffset)
         {
            space = localWriteOffset - readOffset;
         }

         std::size_t numToCopy = std::min(elementsSize - copyOffset, space);
         std::copy_n(buffer->data() + readOffset, numToCopy, elements.data() + copyOffset);

         readOffset += numToCopy;
         if (readOffset == Size)
         {
            readOffset = 0;
         }

         copyOffset += numToCopy;
      }

      return copyOffset;
   }

   std::size_t getWriteOffset() const
   {
      return writeOffset;
   }

   constexpr std::size_t size() const
   {
      return Size;
   }

private:
   std::unique_ptr<std::array<T, Size>> buffer;
   std::size_t writeOffset = 0;
   std::size_t readOffset = 0;
};

}
