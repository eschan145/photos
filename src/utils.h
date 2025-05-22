#include "pch.h"

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

}  // namespace Utils
