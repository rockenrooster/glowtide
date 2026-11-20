#pragma once
// Check macros and the simulation's tests (included by tests.cpp only).

#include <cstdio>

namespace gt::test {
extern int g_failures;
extern int g_checks;
void runSimTests();
}  // namespace gt::test

#define GT_CHECK_MSG(cond, msg)                                                                       \
    do {                                                                                              \
        ++::gt::test::g_checks;                                                                       \
        if (!(cond)) {                                                                                \
            ++::gt::test::g_failures;                                                                 \
            std::printf("FAIL %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, msg);                     \
        }                                                                                             \
    } while (0)
#define GT_CHECK(cond) GT_CHECK_MSG(cond, "")
#define GT_CHECK_QUIET(cond)                                                                          \
    do {                                                                                              \
        if (!(cond)) {                                                                                \
            ++::gt::test::g_checks;                                                                   \
            ++::gt::test::g_failures;                                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                               \
        }                                                                                             \
    } while (0)
