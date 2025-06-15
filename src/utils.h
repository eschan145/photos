#include "pch.h"

#undef assert

#ifndef NDEBUG
    inline void _assert(
        bool condition,
        const char* condition_str,
        const char* file,
        int line,
        const char* func,
        const std::string& message = ""
    ) {
        if (!condition) {
            std::cerr << "\nASSERTION FAILED: \n"
                      << "    Expression: " << condition_str << "\n"
                      << "    Location: " << file << ":" << line
                      << " in function " << func << "\n";
            if (!message.empty())
                std::cerr << "Reason: " << message << "\n";
            std::abort();
        }
    }
    #define assert(...) \
        assert_selector(__VA_ARGS__, assert_with_msg, assert_no_msg)(__VA_ARGS__)
    #define assert_selector(_1, _2, NAME, ...) NAME
    #define assert_no_msg(condition) \
        _assert((condition), #condition, __FILE__, __LINE__, __func__)
    #define assert_with_msg(condition, reason) \
        _assert((condition), #condition, __FILE__, __LINE__, __func__, (reason))
#else
    #define assert(...) ((void)0)
#endif

namespace Utils {

class Benchmark {
    std::string name;
    std::chrono::high_resolution_clock::time_point start;

   public:
    Benchmark(const std::string& name);
    ~Benchmark();
};

float parse_fraction(const QString& fraction);

double to_decimal(const QStringList& list, const std::string& ref);

double parse_rational(const std::string& string);

/*
Read the contents of a string. This function simply returns the string if is in
a regular format or a string converted from a decimal string representing bytes
otherwise.
*/
std::string read_bytes(std::string input);


bool is_base64(const QString& string);

std::string base64(const std::string& input);

std::string from_base64(const std::string& input);

QString format_size(qint64 bytes);

}  // namespace Utils

#if ENABLE_BENCHMARKS
    #define BENCHMARK(name) Utils::Benchmark _timer##__LINE__{name}
#else
    #define BENCHMARK(name)
#endif
