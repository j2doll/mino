#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

#include "mino/core/container/container.hpp"

void test_red_black_tree() {

    auto print_separator = [](std::string_view title) {
        std::cout << "\n========================================\n";
        std::cout << " [TEST] " << title << "\n";
        std::cout << "========================================\n";
        };

    using namespace mino::core::container;

    // -------------------------------------------------------------
    // Case 1. Empty Tree & Boundary Test
    // -------------------------------------------------------------
    print_separator("1. Empty Tree & Boundary Test");

    red_black_tree<int> empty_tree;

    std::cout
        << "Empty Tree size: " << empty_tree.size()
        << " (empty: " << std::boolalpha << empty_tree.empty() << ")\n";
    std::cout
        << "get_root() on empty tree: "
        << (empty_tree.get_root() == nullptr ? "nullptr (Passed)" : "Failed") << '\n';
    std::cout << "ASCII Dump:\n";

    empty_tree.print_tree_structure();

    // -------------------------------------------------------------
    // Case 2. Insertions & Duplicate Handling
    // -------------------------------------------------------------
    print_separator("2. Insertions & Duplicate Handling");
    red_black_tree<int> tree_a;
    std::vector<int> dataset = { 50, 30, 70, 20, 40, 60, 80, 15, 25, 35, 45 };

    for (int v : dataset) {
        bool inserted = tree_a.insert(v);
        std::cout << "Insert " << v << " -> " << (inserted ? "OK" : "DUP") << '\n';
    }

    std::cout
        << "\nRe-inserting duplicate key 30: "
        << (tree_a.insert(30) ? "Success" : "Failed (Duplicate correctly rejected)") << '\n';
    std::cout << "Current Tree A size: " << tree_a.size() << '\n';

    // -------------------------------------------------------------
    // Case 3. ASCII Tree Structure Dump
    // -------------------------------------------------------------
    print_separator("3. ASCII Tree Structure Dump");
    tree_a.print_tree_structure();

    // -------------------------------------------------------------
    // Case 4. Root Node Inspection & Black Root Invariant
    // -------------------------------------------------------------
    print_separator("4. Root Node Inspection");
    if (const auto* root = tree_a.get_root(); root != nullptr) {
        std::cout << "Root Key: " << root->key << '\n';
        std::cout
            << "Root is BLACK: "
            << (tree_a.is_black(root) ? "TRUE (Invariant Satisfied)" : "FALSE") << '\n';
    }

    // -------------------------------------------------------------
    // Case 5. Path Navigation ("L", "R", "LR", etc.) & Color Check
    // -------------------------------------------------------------
    print_separator("5. Path Navigation & Color Inspection");
    constexpr std::string_view test_paths[] = {
        "", "L", "R", "LL", "LR", "RL", "RR", "LLL", "LLR", "LLLL"
    };

    for (auto path : test_paths) {
        const auto* node_ptr = path.empty() ? tree_a.get_root() : tree_a.get_node_at_path(path);
        std::string_view label = path.empty() ? "[ROOT]" : path;

        if (tree_a.is_nil(node_ptr)) {
            std::cout << "Path \"" << label << "\": -> [NIL / Leaf Node]\n";
        }
        else {
            std::cout
                << "Path \"" << label << "\": -> Key = " << node_ptr->key
                << " | Color = " << (tree_a.is_red(node_ptr) ? "RED" : "BLACK")
                << " | Enum Direct = " << (node_ptr->color == node_color::red ? "RED" : "BLACK") << '\n';
        }
    }

    // -------------------------------------------------------------
    // Case 6. Key Search & Pointer Comparisons
    // -------------------------------------------------------------
    print_separator("6. Key Search & Pointer Comparisons");
    for (int search_key : {40, 15, 999}) {
        const auto* found = tree_a.find_node(search_key);
        if (found) {
            std::cout
                << "Found Key [" << found->key << "]:\n";
            std::cout
                << "  - Parent: "
                << (!tree_a.is_nil(found->parent) ? std::to_string(found->parent->key) : "NONE (Root)") << '\n';
            std::cout
                << "  - Left Child:  "
                << (!tree_a.is_nil(found->left) ? std::to_string(found->left->key) : "NIL") << '\n';
            std::cout
                << "  - Right Child: "
                << (!tree_a.is_nil(found->right) ? std::to_string(found->right->key) : "NIL") << '\n';
            std::cout
                << "  - Left == get_nil(): "
                << (found->left == tree_a.get_nil() ? "true" : "false") << '\n';
        }
        else {
            std::cout << "Key [" << search_key << "] not found in tree.\n";
        }
    }

    // -------------------------------------------------------------
    // Case 7. Tree Merge (Tree A + Tree B)
    // -------------------------------------------------------------
    print_separator("7. Tree Merge (Tree A + Tree B)");
    red_black_tree<int> tree_b;
    for (int v : {5, 25, 45, 90, 100}) { // 25, 45 overlap with Tree A
        tree_b.insert(v);
    }

    std::cout << "--- Tree B Structure ---\n";
    tree_b.print_tree_structure();

    std::cout << "\nTree A size before merge: " << tree_a.size() << '\n';
    tree_a.merge(tree_b);
    std::cout << "Tree A size after merge: " << tree_a.size() << '\n';

    std::cout << "\n--- Tree A Structure After Merge ---\n";
    tree_a.print_tree_structure();

    // -------------------------------------------------------------
    // Case 8. C++17 Move Semantics Verification
    // -------------------------------------------------------------
    print_separator("8. C++17 Move Semantics Verification");
    std::cout << "Move constructing Tree C from Tree A...\n";
    red_black_tree<int> tree_c = std::move(tree_a);

    std::cout << "Tree C (moved-to) size: " << tree_c.size() << '\n';
    std::cout << "Tree A (moved-from) size: " << tree_a.size() << '\n';
    std::cout << "Tree A empty(): " << tree_a.empty() << '\n';
}
