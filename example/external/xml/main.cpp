#include <iostream>
#include <filesystem>

#define PUGIXML_HEADER_ONLY
#include "mino/external/third-party/pugixml/pugixml.hpp"

#include "catalog.hpp" // xml2xpp.py로 생성된 헤더 파일

int main() {
    std::string cmake_dir = CMAKE_SOURCE_DIR_PATH;
    auto xml_file_path = std::filesystem::path(cmake_dir) / "catalog.xml";

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file( xml_file_path.string().c_str() );

    // 1. XML 문법 무결성 검증
    if (!result) {
        std::cerr << "[Error] Failed to load catalog.xml: "
            << result.description() << std::endl;
        return -1;
    }

    Catalog catalog;

    // 2. 역직렬화 및 예외 처리 검증
    try {
        catalog.deserialize(doc.child("catalog"));
        std::cout << "[Success] Deserialization completed.\n";

        // 파싱된 데이터 조회
        std::cout << "Book ID: " << catalog.book.id << "\n";
        std::cout << "Category: " << catalog.book.category << "\n";
        std::cout << "Title: " << catalog.book.title << "\n";
        std::cout << "Price: " << catalog.book.price.value << " "
            << catalog.book.price.unit << "\n";
        std::cout << "Description: " << catalog.book.description << "\n";

        // 데이터 수정
        catalog.book.price.value += 5000;
        catalog.book.category = "non-fiction";

    }
    catch (const std::runtime_error& e) {
        std::cerr << "[Exception Caught] " << e.what() << std::endl;
        return -1;
    }

    // 3. 수정된 객체를 새 XML 파일로 직렬화 (Export)
    try {
        pugi::xml_document out_doc;

        // XML 헤더 선언 추가
        auto decl = out_doc.prepend_child(pugi::node_declaration);
        decl.append_attribute("version") = "1.0";
        decl.append_attribute("encoding") = "UTF-8";

        auto root = out_doc.append_child("catalog");
        catalog.serialize(root);

        if (out_doc.save_file("catalog_updated.xml", "    ")) {
            std::cout << "[Success] Saved updated data to catalog_updated.xml\n";
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << "[Serialization Error] " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
