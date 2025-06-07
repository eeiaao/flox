#include "flox/util/base/decimal.h"

#include <gtest/gtest.h>
#include <math.h>

using namespace flox;

namespace
{

struct PriceTag
{
};
using Price = Decimal<PriceTag, 1000000, 10>;

TEST(DecimalTest, FromDoubleAndToDouble)
{
  Price p = Price::fromDouble(123.456789);
  EXPECT_NEAR(p.toDouble(), 123.456789, 1e-6);
}

TEST(DecimalTest, FromRawAndRawAccess)
{
  Price p = Price::fromRaw(123456789);
  EXPECT_EQ(p.raw(), 123456789);
  EXPECT_NEAR(p.toDouble(), 123.456789, 1e-6);
}

TEST(DecimalTest, ArithmeticOperations)
{
  Price a = Price::fromDouble(100.0);
  Price b = Price::fromDouble(25.0);
  Price c = a + b;
  Price d = a - b;

  EXPECT_NEAR(c.toDouble(), 125.0, 1e-6);
  EXPECT_NEAR(d.toDouble(), 75.0, 1e-6);
}

TEST(DecimalTest, ComparisonOperators)
{
  Price a = Price::fromDouble(10.0);
  Price b = Price::fromDouble(20.0);

  EXPECT_TRUE(a < b);
  EXPECT_TRUE(b > a);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(b >= a);
  EXPECT_TRUE(a == a);
  EXPECT_FALSE(a == b);
}

TEST(DecimalTest, RoundToTick)
{
  Price p = Price::fromDouble(103.27);
  Price rounded = p.roundToTick();
  EXPECT_EQ(rounded.raw() % Price::TickSize, 0);
  EXPECT_NEAR(rounded.toDouble(),
              103.27 - fmod(103.27 * Price::Scale, Price::TickSize) / Price::Scale, 1e-6);
}

TEST(DecimalTest, IsZero)
{
  Price zero = Price::fromRaw(0);
  Price nonZero = Price::fromDouble(0.000001);

  EXPECT_TRUE(zero.isZero());
  EXPECT_FALSE(nonZero.isZero());
}

TEST(DecimalTest, FromDoubleNegativeRoundCorrectly)
{
  EXPECT_EQ(Price::fromDouble(-0.25).raw(), -250000);
  EXPECT_EQ(Price::fromDouble(-1.0).raw(), -1000000);
  EXPECT_EQ(Price::fromDouble(-0.000001).raw(), -1);
  EXPECT_EQ(Price::fromDouble(-0.0000001).raw(), 0);
}

TEST(DecimalTest, FromDoublePositiveRoundCorrectly)
{
  EXPECT_EQ(Price::fromDouble(0.25).raw(), 250000);
  EXPECT_EQ(Price::fromDouble(1.0).raw(), 1000000);
  EXPECT_EQ(Price::fromDouble(0.000001).raw(), 1);
  EXPECT_EQ(Price::fromDouble(0.0000001).raw(), 0);
}

}  // namespace
