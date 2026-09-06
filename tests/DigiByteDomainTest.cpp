//
// Tests for src/DigiByteDomain.cpp
// Only isDomain is testable without a database populated with domain assets.
//

#include "DigiByteDomain.h"
#include "gtest/gtest.h"

TEST(DigiByteDomain, isDomain) {
    //valid domains end in .dgb
    EXPECT_TRUE(DigiByteDomain::isDomain("test.dgb"));
    EXPECT_TRUE(DigiByteDomain::isDomain("a.dgb"));
    EXPECT_TRUE(DigiByteDomain::isDomain("sub.domain.dgb"));

    //not domains
    EXPECT_FALSE(DigiByteDomain::isDomain("dgb1qtqt4vrsjfnncr7wjvdvhw7evgzsyj39kaxhg6z"));
    EXPECT_FALSE(DigiByteDomain::isDomain("D7hrf5D21PV24ksEGRdYCZ6JqgsboSVKKC"));
    EXPECT_FALSE(DigiByteDomain::isDomain("test.com"));
    EXPECT_FALSE(DigiByteDomain::isDomain(""));
    EXPECT_FALSE(DigiByteDomain::isDomain("dgb"));     //too short
    EXPECT_FALSE(DigiByteDomain::isDomain("testdgb")); //doesn't end in .dgb
    EXPECT_TRUE(DigiByteDomain::isDomain(".dgb"));     //degenerate but currently counts(name lookup will fail later)
}
