#pragma once

#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Minimal header-only test runner. Keeps the repository free of a test
// framework dependency while still giving CTest one executable per module.
namespace testing
{
struct Failure : std::exception
{
    std::string message;

    explicit Failure(std::string text) : message(std::move(text)) {}

    const char* what() const noexcept override { return message.c_str(); }
};

struct TestCase
{
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& Registry()
{
    static std::vector<TestCase> registry;
    return registry;
}

struct Registrar
{
    Registrar(const char* name, std::function<void()> body)
    {
        Registry().push_back({name, std::move(body)});
    }
};

inline int RunAll()
{
    int failures = 0;

    for (const TestCase& test : Registry())
    {
        try
        {
            test.body();
            std::cout << "[PASS] " << test.name << '\n';
        }
        catch (const Failure& failure)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name << '\n' << failure.what() << '\n';
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name
                      << "\n  unexpected exception: " << error.what() << '\n';
        }
    }

    std::cout << (failures == 0 ? "All tests passed" : std::to_string(failures) + " test(s) failed")
              << " (" << Registry().size() << " tests)\n";
    return failures == 0 ? 0 : 1;
}
} // namespace testing

#define TEST(name)                                              \
    static void name();                                         \
    static ::testing::Registrar registrar_##name(#name, name);  \
    static void name()

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            std::ostringstream stream;                                          \
            stream << __FILE__ << ":" << __LINE__                               \
                   << "\n  CHECK failed: " #condition;                          \
            throw ::testing::Failure(stream.str());                             \
        }                                                                       \
    } while (false)

#define CHECK_EQ(actual, expected)                                              \
    do                                                                          \
    {                                                                           \
        const auto& actualValue = (actual);                                     \
        const auto& expectedValue = (expected);                                 \
        if (!(actualValue == expectedValue))                                    \
        {                                                                       \
            std::ostringstream stream;                                          \
            stream << __FILE__ << ":" << __LINE__                               \
                   << "\n  CHECK_EQ failed: " #actual " == " #expected;         \
            throw ::testing::Failure(stream.str());                             \
        }                                                                       \
    } while (false)

#define CHECK_NEAR(actual, expected, epsilon)                                   \
    do                                                                          \
    {                                                                           \
        const double actualValue = static_cast<double>(actual);                 \
        const double expectedValue = static_cast<double>(expected);             \
        if (!(std::fabs(actualValue - expectedValue) <=                          \
              static_cast<double>(epsilon)))                                    \
        {                                                                       \
            std::ostringstream stream;                                          \
            stream << __FILE__ << ":" << __LINE__                               \
                   << "\n  CHECK_NEAR failed: " #actual " ~= " #expected        \
                   << "\n    actual: " << actualValue                           \
                   << "\n    expected: " << expectedValue;                      \
            throw ::testing::Failure(stream.str());                             \
        }                                                                       \
    } while (false)

#define CHECK_THROWS(expression)                                                \
    do                                                                          \
    {                                                                           \
        bool threw = false;                                                     \
        try                                                                     \
        {                                                                       \
            (void)(expression);                                                 \
        }                                                                       \
        catch (...)                                                             \
        {                                                                       \
            threw = true;                                                       \
        }                                                                       \
        if (!threw)                                                             \
        {                                                                       \
            std::ostringstream stream;                                          \
            stream << __FILE__ << ":" << __LINE__                               \
                   << "\n  CHECK_THROWS failed: " #expression " did not throw"; \
            throw ::testing::Failure(stream.str());                             \
        }                                                                       \
    } while (false)

#define CHECK_THROWS_MESSAGE(expression, text)                                  \
    do                                                                          \
    {                                                                           \
        bool threw = false;                                                     \
        try                                                                     \
        {                                                                       \
            (void)(expression);                                                 \
        }                                                                       \
        catch (const std::exception& error)                                     \
        {                                                                       \
            threw = true;                                                       \
            CHECK(std::string(error.what()).find(text) != std::string::npos);   \
        }                                                                       \
        if (!threw)                                                             \
        {                                                                       \
            std::ostringstream stream;                                          \
            stream << __FILE__ << ":" << __LINE__                               \
                   << "\n  CHECK_THROWS_MESSAGE failed: " #expression            \
                   << " did not throw";                                         \
            throw ::testing::Failure(stream.str());                             \
        }                                                                       \
    } while (false)
