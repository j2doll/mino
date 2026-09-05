#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "mino/external/third-party/pugixml/pugixml.hpp"

struct Price {
    std::string unit;
    int value;

    void deserialize(const pugi::xml_node& node) {
        if (node.empty()) throw std::runtime_error("Node Price is empty or not found.");

        auto attr_unit = node.attribute("unit");
        if (attr_unit.empty()) throw std::runtime_error("Required attribute 'unit' is missing in Price.");
        unit = attr_unit.as_string();

        try { value = std::stoi(node.text().get()); }
        catch (...) { throw std::runtime_error("Failed to parse inner int text in Price."); }

    }

    void serialize(pugi::xml_node& node) const {
        if (node.empty()) throw std::runtime_error("Cannot serialize Price into an empty XML node.");

        node.append_attribute("unit").set_value(unit);
        node.text().set(value);
    }
};

struct Book {
    std::string id;
    std::string category;
    std::string title;
    Price price;
    std::string description;

    void deserialize(const pugi::xml_node& node) {
        if (node.empty()) throw std::runtime_error("Node Book is empty or not found.");

        auto attr_id = node.attribute("id");
        if (attr_id.empty()) throw std::runtime_error("Required attribute 'id' is missing in Book.");
        id = attr_id.as_string();

        auto attr_category = node.attribute("category");
        if (attr_category.empty()) throw std::runtime_error("Required attribute 'category' is missing in Book.");
        category = attr_category.as_string();

        auto child_title = node.child("title");
        if (child_title.empty()) throw std::runtime_error("Required element 'title' is missing in Book.");
        title = child_title.text().as_string();

        auto child_price = node.child("price");
        if (child_price.empty()) throw std::runtime_error("Required element 'price' is missing in Book.");
        price.deserialize(child_price);

        auto child_description = node.child("description");
        if (child_description.empty()) throw std::runtime_error("Required element 'description' is missing in Book.");
        description = child_description.text().as_string();

    }

    void serialize(pugi::xml_node& node) const {
        if (node.empty()) throw std::runtime_error("Cannot serialize Book into an empty XML node.");

        node.append_attribute("id").set_value(id);
        node.append_attribute("category").set_value(category);
        auto child_title = node.append_child("title");
        child_title.text().set(title);
        auto child_price = node.append_child("price");
        price.serialize(child_price);
        auto child_description = node.append_child("description");
        child_description.text().set(description);
    }
};

struct Catalog {
    Book book;

    void deserialize(const pugi::xml_node& node) {
        if (node.empty()) throw std::runtime_error("Node Catalog is empty or not found.");

        auto child_book = node.child("book");
        if (child_book.empty()) throw std::runtime_error("Required element 'book' is missing in Catalog.");
        book.deserialize(child_book);

    }

    void serialize(pugi::xml_node& node) const {
        if (node.empty()) throw std::runtime_error("Cannot serialize Catalog into an empty XML node.");

        auto child_book = node.append_child("book");
        book.serialize(child_book);
    }
};
