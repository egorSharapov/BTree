#include "Btree.hpp"
#include <sstream>
#include <iostream>

int main() {
    std::string res;
    BTree<int, 7> tree;

    std::string input;
    std::getline(std::cin, input);
    std::stringstream in(input);

    while (!in.eof()) {
        char key = 0;
        in >> key;

        int arg1 = 0;
        int arg2 = 0;
        switch (key) {
        case 'k':
            in >> arg1;
            tree.insert(arg1);
            break;
        case 'q':
            in >> arg1;
            in >> arg2;
            if (arg1 <= arg2) {
                std::cout << tree.distance(arg1, arg2) << " ";
                break;
            }
        default:
            return -1;
        }
    }
    std::cout << std::endl;
    return 0;
}