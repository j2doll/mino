#pragma once

#include "mino/core/reflect/reflect.hpp"

struct point {
    double x;
    double y;

    MINO_REFLECT(x, y)
};


