#include <gtest/gtest.h>

#include <DRAMSys/common/utils.h>

using sc_core::SC_NS;
using sc_core::sc_time;
using sc_core::SC_US;

TEST(AlignAtNext, FullCycle)
{
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(5, SC_NS), sc_time(1, SC_NS)), sc_time(5, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(10, SC_NS), sc_time(2, SC_NS)), sc_time(10, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(10, SC_NS), sc_time(10, SC_NS)), sc_time(10, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(100, SC_NS), sc_time(10, SC_NS)), sc_time(100, SC_NS));
}

TEST(AlignAtNext, HalfCycle)
{
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(0.5, SC_NS), sc_time(1, SC_NS)), sc_time(1, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(5, SC_NS), sc_time(10, SC_NS)), sc_time(10, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(22.5, SC_NS), sc_time(5, SC_NS)), sc_time(25, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(55, SC_NS), sc_time(5, SC_NS)), sc_time(55, SC_NS));
}

TEST(AlignAtNext, ArbitraryCycle)
{
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(0.37, SC_NS), sc_time(1, SC_NS)), sc_time(1, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(5, SC_NS), sc_time(6.67, SC_NS)), sc_time(6.67, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(4.99, SC_NS), sc_time(5, SC_NS)), sc_time(5, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(0, SC_NS), sc_time(7.77, SC_NS)), sc_time(0, SC_NS));
    EXPECT_EQ(DRAMSys::alignAtNext(sc_time(4.49, SC_US), sc_time(500, SC_NS)), sc_time(4.5, SC_US));
}

TEST(IsFullCycle, IsFullCycle)
{
    EXPECT_TRUE(DRAMSys::isFullCycle(sc_time(0, SC_NS), sc_time(1, SC_NS)));
    EXPECT_TRUE(DRAMSys::isFullCycle(sc_time(0, SC_NS), sc_time(1000, SC_US)));
    EXPECT_TRUE(DRAMSys::isFullCycle(sc_time(5, SC_NS), sc_time(1, SC_NS)));
    EXPECT_FALSE(DRAMSys::isFullCycle(sc_time(0.5, SC_NS), sc_time(1, SC_NS)));
    EXPECT_TRUE(DRAMSys::isFullCycle(sc_time(67, SC_US), sc_time(1, SC_NS)));
    EXPECT_FALSE(DRAMSys::isFullCycle(sc_time(67.05, SC_US), sc_time(100, SC_NS)));
}
