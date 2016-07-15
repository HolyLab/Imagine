#ifndef MEMCPY_G_H
#define MEMCPY_G_H

//based on: http://www.gamedev.net/topic/502313-special-case---faster-than-memcpy/

// If the memory source and destination addresses line up on a 16 byte boundary, or are both equally
// unaligned on the 16 bytes boundary, we can do a nice super fast memcpy that even beats
// Microsoft's heavily UV optimized memcpy... this is a highly special case though, which is
// very useful if we can force it to happen :)
const bool __sse2_available=true;
__forceinline void memcpy_g(void *Dest, const void *Src, size_t Size)
{
   if (!__sse2_available || Size < 16 || ((size_t)Src & 15) != ((size_t)Dest & 15)) {
      __debugbreak();
      memcpy(Dest, Src, Size);
   }
   else {
      char *pSrc = (char *)Src, *pDest = (char *)Dest;

      size_t Count = (16 - ((size_t)pSrc & 15)) & 15;
      for (size_t Index = Count; Index > 0; --Index) {
         *pDest++ = *pSrc++;
      }	
      Size -= Count;

      __m128i Reg0, Reg1, Reg2, Reg3, Reg4, Reg5, Reg6, Reg7;
      for (size_t Index = Size >> 7; Index > 0; --Index) {
         _mm_prefetch(pSrc + 256, _MM_HINT_NTA);
         _mm_prefetch(pSrc + 256 + 64, _MM_HINT_NTA);
         Reg0 = _mm_load_si128((__m128i *)(pSrc));
         Reg1 = _mm_load_si128((__m128i *)(pSrc + 16));
         Reg2 = _mm_load_si128((__m128i *)(pSrc + 32));
         Reg3 = _mm_load_si128((__m128i *)(pSrc + 48));
         Reg4 = _mm_load_si128((__m128i *)(pSrc + 64));
         Reg5 = _mm_load_si128((__m128i *)(pSrc + 80));
         Reg6 = _mm_load_si128((__m128i *)(pSrc + 96));
         Reg7 = _mm_load_si128((__m128i *)(pSrc + 112));
         _mm_stream_si128((__m128i *)(pDest), Reg0);
         _mm_stream_si128((__m128i *)(pDest + 16), Reg1);
         _mm_stream_si128((__m128i *)(pDest + 32), Reg2);
         _mm_stream_si128((__m128i *)(pDest + 48), Reg3);
         _mm_stream_si128((__m128i *)(pDest + 64), Reg4);
         _mm_stream_si128((__m128i *)(pDest + 80), Reg5);
         _mm_stream_si128((__m128i *)(pDest + 96), Reg6);
         _mm_stream_si128((__m128i *)(pDest + 112), Reg7);
         pSrc += 128;
         pDest += 128;
      }
      const size_t SizeOn16 = Size & 0x70;
      switch (SizeOn16 >> 4) {
      case 7: Reg7 = _mm_load_si128((__m128i *)(pSrc + 96));
      case 6:	Reg6 = _mm_load_si128((__m128i *)(pSrc + 80));
      case 5:	Reg5 = _mm_load_si128((__m128i *)(pSrc + 64));
      case 4:	Reg4 = _mm_load_si128((__m128i *)(pSrc + 48));
      case 3:	Reg3 = _mm_load_si128((__m128i *)(pSrc + 32));
      case 2:	Reg2 = _mm_load_si128((__m128i *)(pSrc + 16));
      case 1:	Reg1 = _mm_load_si128((__m128i *)(pSrc));
      }
      switch (SizeOn16 >> 4) {
      case 7: _mm_stream_si128((__m128i *)(pDest + 96), Reg7);
      case 6: _mm_stream_si128((__m128i *)(pDest + 80), Reg6);
      case 5: _mm_stream_si128((__m128i *)(pDest + 64), Reg5);
      case 4: _mm_stream_si128((__m128i *)(pDest + 48), Reg4);
      case 3: _mm_stream_si128((__m128i *)(pDest + 32), Reg3);
      case 2: _mm_stream_si128((__m128i *)(pDest + 16), Reg2);
      case 1: _mm_stream_si128((__m128i *)(pDest), Reg1);
      }
      pSrc += SizeOn16;
      pDest += SizeOn16;
      for (size_t Index = Size & 15; Index > 0; --Index) {
         *pDest++ = *pSrc++;
      }	
   }
}


#endif
