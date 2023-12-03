#include <gtest/gtest.h>
#include <Btree.hpp>


static int range_query(const std::set<int> &set, int begin, int end) {
    using it = std::set<int>::iterator;
    it start = set.lower_bound(begin);
    it stop = set.upper_bound(end);
    return std::distance(start, stop);
}


TEST(BTree, DistanceCheck) {
    const int max_load = 120;
    BTree<int, 5> tree;

    for (int i = 0; i < max_load; ++i) {
        tree.insert(i);
    }

    for (int i = 0; i < max_load; ++i) {
        for (int j = i; j < max_load; ++j) {
            int right_distance = j - i + 1;
            if (i == j) {
                right_distance = 0;
            }
            int distance = tree.distance(i, j);
            EXPECT_EQ(right_distance, distance);
        }
    }
}


TEST(BTree, SetCOmpare) {
    const int max_load = 120;
    BTree<int, 5> tree;
    std::set<int> set;

    for (int i = 0; i < max_load; ++i) {
        tree.insert(i);
        set.insert(i);
    }

    for (int i = 0; i < max_load; ++i) {
        for (int j = i; j < max_load; ++j) {
            if (i == j) {
                continue;
            }
            int set_distance = range_query(set, i, j);
            int distance = tree.distance(i, j);
            EXPECT_EQ(set_distance, distance);
        }
    }
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
