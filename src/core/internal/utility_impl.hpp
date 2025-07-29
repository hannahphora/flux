#include <core/subsystems/utility.hpp>

void utility::flushDeinitStack(DeinitStack * deinitStack) {
    while (!deinitStack->empty()) {
        deinitStack->back()();
        deinitStack->pop_back();
    }
}