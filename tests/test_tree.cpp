#include <gtest/gtest.h>
#define NOTDEBUG
#include <Btree.hpp>

TEST(BTree, DistanceCheck) {
    const int max_load = 120;
    BTree<int, 5> tree;

    for (int i = 0; i < max_load; ++i) {
        tree.insert(i);
    }

    for (int i = 0; i < max_load; ++i) {
        for (int j = i; j < max_load; ++j) {
            int right_distance = j - i + 1;
            int distance = tree.distance(i, j);
            EXPECT_EQ(right_distance, distance);
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}