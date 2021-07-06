/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2019 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with no express
 * or implied warranties, other than those that are expressly stated in the
 * License.
 *
 ******************************************************************************/

#include "CrashdumpSections/UncoreRegs.hpp"

#include "gtest/gtest.h"

TEST(UncoreRegs, sizeCheck)
{
    // Check Num of PCI registers
    int expected = 2702;
    int val = sizeof(sUncoreStatusPciICX1) / sizeof(SUncoreStatusRegPci);
    EXPECT_EQ(val, expected);

    // Check Num of MMIO registers
    val =
        sizeof(sUncoreStatusPciMmioICX1) / sizeof(SUncoreStatusRegPciMmioICX1);
    expected = 670;
    EXPECT_EQ(val, expected);
}
