
#include "mino/core/broker/object_broker.hpp"

namespace mino::core::broker {

object_broker& object_broker::get_instance() {
    static object_broker instance;
    return instance;
}

void object_broker::clear() {
    std::unique_lock lock(get_instance().mutex_);
    get_instance().storage_.clear();
}

} // namespace mino::core::broker
