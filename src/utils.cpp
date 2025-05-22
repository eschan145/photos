#include "utils.h"

namespace Utils {

float parse_fraction(const QString& fraction) {
    QStringList parts = fraction.split('/');
    if (parts.size() != 2) {
        throw std::invalid_argument(
            "Invalid fraction format: " + fraction.toStdString() + "!");
    }

    bool ok1, ok2;
    float numerator = parts[0].toFloat(&ok1);
    float denominator = parts[1].toFloat(&ok2);

    if (!ok1 || !ok2) {
        throw std::invalid_argument(
            "Non-numeric fraction part: " + fraction.toStdString() + "!");
    }
    if (denominator == 0)
        throw std::invalid_argument("Division by zero in fraction: " +
                                    fraction.toStdString());

    return numerator / denominator;
}

float parse_rational(const std::string& string) {
    size_t slash = string.find('/');
    if (slash != std::string::npos) {
        int numerator = std::stoi(string.substr(0, slash));
        int denominator = std::stoi(string.substr(slash + 1));
        if (denominator != 0)
            return static_cast<float>(numerator) / denominator;
    }
    return 0.f;
}

float to_decimal(const QStringList& list, const std::string& ref) {
    if (list.size() != 3) {
        throw std::invalid_argument("DMS called without three elements!");
    }

    float degrees = parse_fraction(list[0]);
    float minutes = parse_fraction(list[1]);
    float seconds = parse_fraction(list[2]);

    float decimal = degrees + minutes / 60.0 + seconds / 3600.0;

    if (ref == "S" || ref == "W") {
        decimal = -decimal;
    }
    return decimal;
}

std::string read_bytes(std::string input) {
    size_t start = input.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return input;
    }
    size_t end = input.find_last_not_of(" \t\n\r");
    input = input.substr(start, end - start + 1);

    bool has_digit = false;
    for (char c : input) {
        if (!std::isdigit(c) && !std::isspace(c)) {
            return input;
        }
        if (std::isdigit(c)) has_digit = true;
    }
    if (!has_digit) return input;

    std::istringstream iss(input);
    std::string result;
    int byte_value;

    while (iss >> byte_value) {
        if (byte_value < 0 || byte_value > 255) {
            return input;
        }
        result.push_back(static_cast<char>(byte_value));
    }

    if (result.empty()) return input;

    return result;
}

}  // namespace Utils