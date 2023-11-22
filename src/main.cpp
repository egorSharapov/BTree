#include "Btree.hpp"


template <typename Key, size_t N>
void dump(const BTree<Key, N> &tree) {
    std::ofstream dump("temp.dot");
    dump << tree;
    dump.close();
    int error = system("dot -Tpng -o out.png temp.dot");
    if (error) {
        std::cerr << "Can't dump tree\n";
        exit(error);
    }
}


int main() {
    char key = 0;
    BTree<int, 3> tree = {1, 2, 3, 4};

    for (int i = 0; i < 100; ++i) {
        tree.insert(i);
    }

    for (auto value : tree) {
        std::cout <<  value << " ";
    }
    std::cout << "\n";

    while (key != 'c') {
        std::cout << ">> ";
        std::cin >> key;
        int arg1 = 0;
        int arg2 = 0;
        switch (key) {
        case 'k':
            std::cin >> arg1;
            tree.insert(arg1);
            // dump(tree);
            break;
        case 'd':
            std::cin >> arg1;
            tree.erase(arg1);
            dump(tree);
            break;
        case 'p':
            dump(tree);
            break;
        case 'q':
            std::cin >> arg1;
            std::cin >> arg2;
            if (arg1 <= arg2) {
                std::cout << "distance: " << tree.distance(arg1, arg2) << "\n";
            }
            break;
        default:
            std::cout << "wrong key passed [" << key << "]\ntry 'h' for help\n";
            break;
        }
    }

    return 0;
}