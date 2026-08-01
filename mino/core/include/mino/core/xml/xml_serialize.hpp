#pragma once

#include <string>

#include "mino/core/xml/xml_common.hpp"

namespace mino::core::xml {

    // Serialize an xml_node tree to an UTF-8 XML string.
    // If include_declaration is true, prepend <?xml version="1.0" encoding="UTF-8"?>.
     std::string serialize_xml(const xml_node& node, bool include_declaration = true);

}


