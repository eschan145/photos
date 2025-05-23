#include "pch.h"

void start_exiftool();

void stop_exiftool();

namespace Image {

QPixmap load_heic(const QString& path);

QPixmap load_image(const QString& path);

void write_heic(
    const std::string& filepath,
    const std::map<std::string, std::string>& metadata
);

std::unique_ptr<Exiv2::Image> write_image(
    const QString& filepath,
    const std::map<std::string, std::string>& metadata,
    std::unique_ptr<Exiv2::Image> image
);

}  // namespace Image