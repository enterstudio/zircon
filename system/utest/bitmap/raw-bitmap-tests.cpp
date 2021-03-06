// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <bitmap/raw-bitmap.h>
#include <bitmap/storage.h>

#include <fbl/algorithm.h>
#include <fbl/alloc_checker.h>
#include <unittest/unittest.h>

namespace bitmap {
namespace tests {

template <typename RawBitmap>
static bool InitializedEmpty(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(0), ZX_OK);
    EXPECT_EQ(bitmap.size(), 0U, "get size");

    EXPECT_TRUE(bitmap.GetOne(0), "get one bit");
    EXPECT_EQ(bitmap.SetOne(0), ZX_ERR_INVALID_ARGS, "set one bit");
    EXPECT_EQ(bitmap.ClearOne(0), ZX_ERR_INVALID_ARGS, "clear one bit");

    EXPECT_EQ(bitmap.Reset(1), ZX_OK);
    EXPECT_FALSE(bitmap.GetOne(0), "get one bit");
    EXPECT_EQ(bitmap.SetOne(0), ZX_OK, "set one bit");
    EXPECT_EQ(bitmap.ClearOne(0), ZX_OK, "clear one bit");

    END_TEST;
}

template <typename RawBitmap>
static bool SingleBit(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_FALSE(bitmap.GetOne(2), "get bit before setting");

    EXPECT_EQ(bitmap.SetOne(2), ZX_OK, "set bit");
    EXPECT_TRUE(bitmap.GetOne(2), "get bit after setting");

    EXPECT_EQ(bitmap.ClearOne(2), ZX_OK, "clear bit");
    EXPECT_FALSE(bitmap.GetOne(2), "get bit after clearing");

    END_TEST;
}

template <typename RawBitmap>
static bool SetTwice(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_EQ(bitmap.SetOne(2), ZX_OK, "set bit");
    EXPECT_TRUE(bitmap.GetOne(2), "get bit after setting");

    EXPECT_EQ(bitmap.SetOne(2), ZX_OK, "set bit again");
    EXPECT_TRUE(bitmap.GetOne(2), "get bit after setting again");

    END_TEST;
}

template <typename RawBitmap>
static bool ClearTwice(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_EQ(bitmap.SetOne(2), ZX_OK, "set bit");

    EXPECT_EQ(bitmap.ClearOne(2), ZX_OK, "clear bit");
    EXPECT_FALSE(bitmap.GetOne(2), "get bit after clearing");

    EXPECT_EQ(bitmap.ClearOne(2), ZX_OK, "clear bit again");
    EXPECT_FALSE(bitmap.GetOne(2), "get bit after clearing again");

    END_TEST;
}

template <typename RawBitmap>
static bool GetReturnArg(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    size_t first_unset = 0;
    EXPECT_FALSE(bitmap.Get(2, 3, nullptr), "get bit with null");
    EXPECT_FALSE(bitmap.Get(2, 3, &first_unset), "get bit with nonnull");
    EXPECT_EQ(first_unset, 2U, "check returned arg");

    EXPECT_EQ(bitmap.SetOne(2), ZX_OK, "set bit");
    EXPECT_TRUE(bitmap.Get(2, 3, &first_unset), "get bit after setting");
    EXPECT_EQ(first_unset, 3U, "check returned arg");

    first_unset = 0;
    EXPECT_FALSE(bitmap.Get(2, 4, &first_unset), "get larger range after setting");
    EXPECT_EQ(first_unset, 3U, "check returned arg");

    EXPECT_EQ(bitmap.SetOne(3), ZX_OK, "set another bit");
    EXPECT_FALSE(bitmap.Get(2, 5, &first_unset), "get larger range after setting another");
    EXPECT_EQ(first_unset, 4U, "check returned arg");

    END_TEST;
}

template <typename RawBitmap>
static bool SetRange(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_EQ(bitmap.Set(2, 100), ZX_OK, "set range");

    size_t first_unset = 0;
    EXPECT_TRUE(bitmap.Get(2, 3, &first_unset), "get first bit in range");
    EXPECT_EQ(first_unset, 3U, "check returned arg");

    EXPECT_TRUE(bitmap.Get(99, 100, &first_unset), "get last bit in range");
    EXPECT_EQ(first_unset, 100U, "check returned arg");

    EXPECT_FALSE(bitmap.Get(1, 2, &first_unset), "get bit before first in range");
    EXPECT_EQ(first_unset, 1U, "check returned arg");

    EXPECT_FALSE(bitmap.Get(100, 101, &first_unset), "get bit after last in range");
    EXPECT_EQ(first_unset, 100U, "check returned arg");

    EXPECT_TRUE(bitmap.Get(2, 100, &first_unset), "get entire range");
    EXPECT_EQ(first_unset, 100U, "check returned arg");

    EXPECT_TRUE(bitmap.Get(50, 80, &first_unset), "get part of range");
    EXPECT_EQ(first_unset, 80U, "check returned arg");

    EXPECT_EQ(bitmap.Scan(0, 100, true), 0U, "scan set bits out of range");
    EXPECT_EQ(bitmap.Scan(0, 100, false), 2U, "scan cleared bits to start");
    EXPECT_EQ(bitmap.Scan(2, 100, true), 100U, "scan set bits to end");
    EXPECT_EQ(bitmap.Scan(2, 100, false), 2U, "scan cleared bits in set range");
    EXPECT_EQ(bitmap.Scan(50, 80, true), 80U, "scan set bits in subrange");
    EXPECT_EQ(bitmap.Scan(100, 200, false), 128U, "scan past end of bitmap");

    END_TEST;
}

template <typename RawBitmap>
static bool FindSimple(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    size_t bitoff_start;

    // Invalid finds
    EXPECT_EQ(bitmap.Find(false, 0, 0, 1, &bitoff_start), ZX_ERR_INVALID_ARGS, "bad range");
    EXPECT_EQ(bitmap.Find(false, 1, 0, 1, &bitoff_start), ZX_ERR_INVALID_ARGS, "bad range");
    EXPECT_EQ(bitmap.Find(false, 0, 1, 1, nullptr), ZX_ERR_INVALID_ARGS, "bad output");

    // Finds from offset zero
    EXPECT_EQ(bitmap.Find(false, 0, 100, 1, &bitoff_start), ZX_OK, "find unset");
    EXPECT_EQ(bitoff_start, 0, "check returned arg");
    EXPECT_EQ(bitmap.Find(true, 0, 100, 1, &bitoff_start), ZX_ERR_NO_RESOURCES, "find set");
    EXPECT_EQ(bitoff_start, 100, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 0, 100, 5, &bitoff_start), ZX_OK, "find more unset");
    EXPECT_EQ(bitoff_start, 0, "check returned arg");
    EXPECT_EQ(bitmap.Find(true, 0, 100, 5, &bitoff_start), ZX_ERR_NO_RESOURCES, "find more set");
    EXPECT_EQ(bitoff_start, 100, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 0, 100, 100, &bitoff_start), ZX_OK, "find all uset");
    EXPECT_EQ(bitoff_start, 0, "check returned arg");
    EXPECT_EQ(bitmap.Find(true, 0, 100, 100, &bitoff_start), ZX_ERR_NO_RESOURCES, "find all set");
    EXPECT_EQ(bitoff_start, 100, "check returned arg");

    // Finds at an offset
    EXPECT_EQ(bitmap.Find(false, 50, 100, 3, &bitoff_start), ZX_OK, "find at offset");
    EXPECT_EQ(bitoff_start, 50, "check returned arg");
    EXPECT_EQ(bitmap.Find(true, 50, 100, 3, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail at offset");
    EXPECT_EQ(bitoff_start, 100, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 90, 100, 10, &bitoff_start), ZX_OK, "find at offset end");
    EXPECT_EQ(bitoff_start, 90, "check returned arg");

    // Invalid scans
    EXPECT_EQ(bitmap.Find(false, 0, 100, 101, &bitoff_start), ZX_ERR_NO_RESOURCES, "no space");
    EXPECT_EQ(bitoff_start, 100, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 91, 100, 10, &bitoff_start), ZX_ERR_NO_RESOURCES, "no space");
    EXPECT_EQ(bitoff_start, 100, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 90, 100, 11, &bitoff_start), ZX_ERR_NO_RESOURCES, "no space");
    EXPECT_EQ(bitoff_start, 100, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 90, 95, 6, &bitoff_start), ZX_ERR_NO_RESOURCES, "no space");
    EXPECT_EQ(bitoff_start, 95, "check returned arg");

    // Fill the bitmap
    EXPECT_EQ(bitmap.Set(5, 10), ZX_OK, "set range");
    EXPECT_EQ(bitmap.Set(20, 30), ZX_OK, "set range");
    EXPECT_EQ(bitmap.Set(32, 35), ZX_OK, "set range");

    EXPECT_EQ(bitmap.Find(false, 0, 50, 5, &bitoff_start), ZX_OK, "find in first group");
    EXPECT_EQ(bitoff_start, 0, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 0, 50, 10, &bitoff_start), ZX_OK, "find in second group");
    EXPECT_EQ(bitoff_start, 10, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 0, 50, 15, &bitoff_start), ZX_OK, "find in third group");
    EXPECT_EQ(bitoff_start, 35, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 0, 50, 16, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail to find");
    EXPECT_EQ(bitoff_start, 50, "check returned arg");

    EXPECT_EQ(bitmap.Find(false, 5, 20, 10, &bitoff_start), ZX_OK, "find space (offset)");
    EXPECT_EQ(bitoff_start, 10, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 5, 25, 10, &bitoff_start), ZX_OK, "find space (offset)");
    EXPECT_EQ(bitoff_start, 10, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 5, 15, 6, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail to find (offset)");
    EXPECT_EQ(bitoff_start, 15, "check returned arg");

    EXPECT_EQ(bitmap.Find(true, 0, 15, 2, &bitoff_start), ZX_OK, "find set bits");
    EXPECT_EQ(bitoff_start, 5, "check returned arg");
    EXPECT_EQ(bitmap.Find(true, 0, 15, 6, &bitoff_start), ZX_ERR_NO_RESOURCES, "find set bits (fail)");
    EXPECT_EQ(bitoff_start, 15, "check returned arg");

    EXPECT_EQ(bitmap.Find(false, 32, 35, 3, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail to find");
    EXPECT_EQ(bitoff_start, 35, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 32, 35, 4, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail to find");
    EXPECT_EQ(bitoff_start, 35, "check returned arg");
    EXPECT_EQ(bitmap.Find(true, 32, 35, 4, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail to find (set)");
    EXPECT_EQ(bitoff_start, 35, "check returned arg");

    // Fill the whole bitmap
    EXPECT_EQ(bitmap.Set(0, 128), ZX_OK, "set range");

    EXPECT_EQ(bitmap.Find(false, 0, 1, 1, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail to find (small)");
    EXPECT_EQ(bitoff_start, 1, "check returned arg");
    EXPECT_EQ(bitmap.Find(false, 0, 128, 1, &bitoff_start), ZX_ERR_NO_RESOURCES, "fail to find (large)");
    EXPECT_EQ(bitoff_start, 128, "check returned arg");

    END_TEST;
}

template <typename RawBitmap>
static bool ClearAll(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_EQ(bitmap.Set(0, 100), ZX_OK, "set range");

    bitmap.ClearAll();

    size_t first = 0;
    EXPECT_FALSE(bitmap.Get(2, 100, &first), "get range");
    EXPECT_EQ(first, 2U, "all clear");

    EXPECT_EQ(bitmap.Set(0, 99), ZX_OK, "set range");
    EXPECT_FALSE(bitmap.Get(0, 100, &first), "get range");
    EXPECT_EQ(first, 99U, "all clear");

    END_TEST;
}

template <typename RawBitmap>
static bool ClearSubrange(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_EQ(bitmap.Set(2, 100), ZX_OK, "set range");
    EXPECT_EQ(bitmap.Clear(50, 80), ZX_OK, "clear range");

    size_t first_unset = 0;
    EXPECT_FALSE(bitmap.Get(2, 100, &first_unset), "get whole original range");
    EXPECT_EQ(first_unset, 50U, "check returned arg");

    first_unset = 0;
    EXPECT_TRUE(bitmap.Get(2, 50, &first_unset), "get first half range");
    EXPECT_EQ(first_unset, 50U, "check returned arg");

    EXPECT_TRUE(bitmap.Get(80, 100, &first_unset), "get second half range");
    EXPECT_EQ(first_unset, 100U, "check returned arg");

    EXPECT_FALSE(bitmap.Get(50, 80, &first_unset), "get cleared range");
    EXPECT_EQ(first_unset, 50U, "check returned arg");

    END_TEST;
}

template <typename RawBitmap>
static bool BoundaryArguments(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_EQ(bitmap.Set(0, 0), ZX_OK, "range contains no bits");
    EXPECT_EQ(bitmap.Set(5, 4), ZX_ERR_INVALID_ARGS, "max is less than off");
    EXPECT_EQ(bitmap.Set(5, 5), ZX_OK, "range contains no bits");

    EXPECT_EQ(bitmap.Clear(0, 0), ZX_OK, "range contains no bits");
    EXPECT_EQ(bitmap.Clear(5, 4), ZX_ERR_INVALID_ARGS, "max is less than off");
    EXPECT_EQ(bitmap.Clear(5, 5), ZX_OK, "range contains no bits");

    EXPECT_TRUE(bitmap.Get(0, 0), "range contains no bits, so all are true");
    EXPECT_TRUE(bitmap.Get(5, 4), "range contains no bits, so all are true");
    EXPECT_TRUE(bitmap.Get(5, 5), "range contains no bits, so all are true");

    END_TEST;
}

template <typename RawBitmap>
static bool SetOutOfOrder(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128U, "get size");

    EXPECT_EQ(bitmap.SetOne(0x64), ZX_OK, "setting later");
    EXPECT_EQ(bitmap.SetOne(0x60), ZX_OK, "setting earlier");

    EXPECT_TRUE(bitmap.GetOne(0x64), "getting first set");
    EXPECT_TRUE(bitmap.GetOne(0x60), "getting second set");
    END_TEST;
}

template <typename RawBitmap>
static bool GrowAcrossPage(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128u);

    EXPECT_FALSE(bitmap.GetOne(100));
    EXPECT_EQ(bitmap.SetOne(100), ZX_OK);
    EXPECT_TRUE(bitmap.GetOne(100));

    size_t bitoff_start;
    EXPECT_EQ(bitmap.Find(true, 101, 128, 1, &bitoff_start), ZX_ERR_NO_RESOURCES,
              "Expected tail end of bitmap to be unset");

    // We can't set bits out of range
    EXPECT_NE(bitmap.SetOne(16 * PAGE_SIZE - 1), ZX_OK);

    EXPECT_EQ(bitmap.Grow(16 * PAGE_SIZE), ZX_OK);
    EXPECT_EQ(bitmap.Find(true, 101, 16 * PAGE_SIZE, 1, &bitoff_start),
              ZX_ERR_NO_RESOURCES, "Expected tail end of bitmap to be unset");

    // Now we can set the previously inaccessible bits
    EXPECT_FALSE(bitmap.GetOne(16 * PAGE_SIZE - 1));
    EXPECT_EQ(bitmap.SetOne(16 * PAGE_SIZE - 1), ZX_OK);
    EXPECT_TRUE(bitmap.GetOne(16 * PAGE_SIZE - 1));

    // But our orignal 'set bit' is still set
    EXPECT_TRUE(bitmap.GetOne(100), "Growing should not unset bits");

    // If we shrink and re-expand the bitmap, it should
    // have cleared the underlying bits
    EXPECT_EQ(bitmap.Shrink(99), ZX_OK);
    EXPECT_EQ(bitmap.Grow(16 * PAGE_SIZE), ZX_OK);
    EXPECT_FALSE(bitmap.GetOne(100));
    EXPECT_FALSE(bitmap.GetOne(16 * PAGE_SIZE - 1));

    END_TEST;
}

template <typename RawBitmap>
static bool GrowShrink(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128u);

    EXPECT_FALSE(bitmap.GetOne(100));
    EXPECT_EQ(bitmap.SetOne(100), ZX_OK);
    EXPECT_TRUE(bitmap.GetOne(100));

    for (size_t i = 8; i < 16; i++) {
        for (int j = -16; j <= 16; j++) {
            size_t bitmap_size = (1 << i) + j;

            for (size_t shrink_len = 1; shrink_len < 32; shrink_len++) {
                EXPECT_EQ(bitmap.Reset(bitmap_size), ZX_OK);
                EXPECT_EQ(bitmap.size(), bitmap_size);

                // This bit will be eliminated by shrink / grow
                EXPECT_FALSE(bitmap.GetOne(bitmap_size - shrink_len));
                EXPECT_EQ(bitmap.SetOne(bitmap_size - shrink_len), ZX_OK);
                EXPECT_TRUE(bitmap.GetOne(bitmap_size - shrink_len));

                // This bit will stay
                EXPECT_FALSE(bitmap.GetOne(bitmap_size - shrink_len - 1));
                EXPECT_EQ(bitmap.SetOne(bitmap_size - shrink_len - 1), ZX_OK);
                EXPECT_TRUE(bitmap.GetOne(bitmap_size - shrink_len - 1));

                EXPECT_EQ(bitmap.Shrink(bitmap_size - shrink_len), ZX_OK);
                EXPECT_EQ(bitmap.Grow(bitmap_size), ZX_OK);

                EXPECT_FALSE(bitmap.GetOne(bitmap_size - shrink_len),
                             "Expected 'shrunk' bit to be unset");
                EXPECT_TRUE(bitmap.GetOne(bitmap_size - shrink_len - 1),
                            "Expected bit outside shrink range to be set");

                size_t bitoff_start;
                EXPECT_EQ(bitmap.Find(true, bitmap_size - shrink_len,
                                      bitmap_size, 1, &bitoff_start),
                          ZX_ERR_NO_RESOURCES,
                          "Expected tail end of bitmap to be unset");
            }
        }
    }

    END_TEST;
}

template <typename RawBitmap>
static bool GrowFailure(void) {
    BEGIN_TEST;

    RawBitmap bitmap;
    EXPECT_EQ(bitmap.Reset(128), ZX_OK);
    EXPECT_EQ(bitmap.size(), 128u);

    EXPECT_EQ(bitmap.Grow(64), ZX_ERR_NO_RESOURCES);
    EXPECT_EQ(bitmap.Grow(128), ZX_ERR_NO_RESOURCES);
    EXPECT_EQ(bitmap.Grow(128 + 1), ZX_ERR_NO_RESOURCES);
    EXPECT_EQ(bitmap.Grow(8 * PAGE_SIZE), ZX_ERR_NO_RESOURCES);
    END_TEST;
}

#define RUN_TEMPLATIZED_TEST(test, specialization) RUN_TEST(test<specialization>)
#define ALL_TESTS(specialization)                           \
    RUN_TEMPLATIZED_TEST(InitializedEmpty, specialization)  \
    RUN_TEMPLATIZED_TEST(SingleBit, specialization)         \
    RUN_TEMPLATIZED_TEST(SetTwice, specialization)          \
    RUN_TEMPLATIZED_TEST(ClearTwice, specialization)        \
    RUN_TEMPLATIZED_TEST(GetReturnArg, specialization)      \
    RUN_TEMPLATIZED_TEST(SetRange, specialization)          \
    RUN_TEMPLATIZED_TEST(FindSimple, specialization)        \
    RUN_TEMPLATIZED_TEST(ClearSubrange, specialization)     \
    RUN_TEMPLATIZED_TEST(BoundaryArguments, specialization) \
    RUN_TEMPLATIZED_TEST(ClearAll, specialization)          \
    RUN_TEMPLATIZED_TEST(SetOutOfOrder, specialization)

BEGIN_TEST_CASE(raw_bitmap_tests)
ALL_TESTS(RawBitmapGeneric<DefaultStorage>)
ALL_TESTS(RawBitmapGeneric<VmoStorage>)
RUN_TEST(GrowAcrossPage<RawBitmapGeneric<VmoStorage>>)
RUN_TEST(GrowShrink<RawBitmapGeneric<VmoStorage>>)
RUN_TEST(GrowFailure<RawBitmapGeneric<DefaultStorage>>)
END_TEST_CASE(raw_bitmap_tests);

} // namespace tests
} // namespace bitmap
