#include "SkBitmap.h"
#include "SkColorTable.h"
#include "SkStream.h"
#include "SkImageEncoder.h"
#include <media/stagefright/NuMediaExtractor.h>

extern "C" {
	 void _ZN14SkImageDecoder10DecodeFileEPKcP8SkBitmap11SkColorTypeNS_4ModeEPNS_6FormatE() {}
     void _ZN14SkImageEncoder10EncodeFileEPKcRK8SkBitmapNS_4TypeEi() {}
     void _ZN15SkMemoryWStreamC1EPvj() {}
}

extern "C" void _ZN8SkBitmap14tryAllocPixelsEPNS_9AllocatorE(SkBitmap::Allocator* allocator);

extern "C" void _ZN8SkBitmap14tryAllocPixelsEPNS_9AllocatorEP12SkColorTable(SkBitmap::Allocator* allocator, SkColorTable* ctable) {
    _ZN8SkBitmap14tryAllocPixelsEPNS_9AllocatorE(allocator);
}

extern "C" bool _ZN14SkImageEncoder12EncodeStreamEP9SkWStreamRK8SkBitmapNS_4TypeEi(SkWStream* stream, const SkBitmap& bm, int quality) {
	return SkMin32(100, SkMax32(0, quality));
}

extern "C" {
	extern void _ZN7android14MediaExtractor23RegisterDefaultSniffersEv();
	void _ZN7android10DataSource23RegisterDefaultSniffersEv() {
		_ZN7android14MediaExtractor23RegisterDefaultSniffersEv();
	}
}