#include "utils.h"

namespace Utils {

Benchmark::Benchmark(const std::string& name) {
    this->name = name;
    this->start = std::chrono::high_resolution_clock::now();
}

Benchmark::~Benchmark() {
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << name << " took " << ms << "us\n";
}

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

double parse_rational(const std::string& string) {
    size_t slash = string.find('/');
    if (slash != std::string::npos) {
        int numerator = std::stoi(string.substr(0, slash));
        int denominator = std::stoi(string.substr(slash + 1));
        if (denominator != 0)
            return static_cast<double>(numerator) / denominator;
    }
    return 0.f;
}

double to_decimal(const QStringList& list, const std::string& ref) {
    if (list.size() != 3) {
        throw std::invalid_argument("DMS called without three elements!");
    }

    double degrees = parse_fraction(list[0]);
    double minutes = parse_fraction(list[1]);
    double seconds = parse_fraction(list[2]);

    double decimal = degrees + minutes / 60.0 + seconds / 3600.0;

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
        if (byte_value != 0) {
            result.push_back(byte_value < 32 || byte_value > 126 ? '?' : static_cast<char>(byte_value));
        }
    }

    if (result.empty()) return input;

    return result;
}

bool is_base64(const QString& string) {
    static QRegularExpression regex("^[A-Za-z0-9+/]*={0,2}$");
    if (string.length() < 8) return false;
    if (!regex.match(string).hasMatch()) return false;
    if (string.length() % 4 != 0) return false;

    QByteArray decoded = QByteArray::fromBase64(string.toUtf8());
    if (decoded.isEmpty() && !string.isEmpty()) return false;

    QByteArray reencoded = decoded.toBase64();
    return reencoded == string.toUtf8();
}

std::string to_base64(const std::string& input) {
    QString qstr = QString::fromStdString(input);
    if (is_base64(qstr)) {
        return input;
    }
    return QString::fromUtf8(qstr.toUtf8().toBase64()).toStdString();
}

std::string from_base64(const std::string& input) {
    QString qstr = QString::fromStdString(input);
    if (!is_base64(qstr)) {
        std::cout << input << " is not base64";
        return input;
    }

    std::cout << input << " is base64";

    QByteArray decoded = QByteArray::fromBase64(qstr.toUtf8());
    if (decoded.isEmpty() && !input.empty()) {
        throw std::runtime_error("Invalid base64 encoding: decoding failed.");
    }

    return std::string(decoded.constData(), static_cast<std::string::size_type>(decoded.size()));
}

QString format_size(qint64 bytes) {
    double size = bytes;
    QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int index = 0;

    while (size >= 1024.0 && index < units.size() - 1) {
        size /= 1024.0;
        ++index;
    }

    return QString::number(size, 'f', (size < 10.0 ? 1 : 0)) + " " + units[index];
}

}  // namespace Utils