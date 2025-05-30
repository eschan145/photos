#include "pch.h"

#undef assert

#ifdef NDEBUG
    #define assert(...) ((void)0)
#else
    #define _ASSERT_MSG(cond, message) \
        do { \
            if (!(cond)) { \
                std::ostringstream ossmessage; \
                ossmessage << "Assertion failed: " #cond \
                        << " (" << (cond) \
                        << ") at " << __FILE__ << ":" \
                        << std::to_string(__LINE__) \
                        << " in function " << __func__ \
                        << "\n" << message << "\n"; \
                std::abort(); \
            } \
        } while (0)

    #define _ASSERT_NO_MSG(cond) \
        do { \
            if (!(cond)) { \
                std::ostringstream ossmessage; \
                ossmessage << "Assertion failed: " #cond \
                        << " (" << (cond) \
                        << ") at " << __FILE__ << ":" \
                        << std::to_string(__LINE__) \
                        << " in function " << __func__; \
                std::abort(); \
            } \
        } while (0)

    #define _GET_MACRO(_1, _2, NAME, ...) NAME
    #define assert(...) \
        _GET_MACRO(__VA_ARGS__, _ASSERT_MSG, _ASSERT_NO_MSG)(__VA_ARGS__)
#endif

namespace Utils {

float parse_fraction(const QString& fraction);

float to_decimal(const QStringList& list, const std::string& ref);

float parse_rational(const std::string& string);

/*
Read the contents of a string. This function simply returns the string if is in
a regular format or a string converted from a decimal string representing bytes
otherwise.
*/
std::string read_bytes(std::string input);


bool is_base64(const QString& string);

std::string base64(const std::string& input);

std::string from_base64(const std::string& input);

}  // namespace Utils
