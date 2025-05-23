#include <QDebug>
#include "loader.h"


static QProcess exiftool;

void start_exiftool() {
    if (exiftool.state() == QProcess::Running) return;

    exiftool.start("exiftool", {"-stay_open", "True", "-@", "-"});
    exiftool.waitForStarted();

    if (exiftool.state() != QProcess::Running) {
        throw std::runtime_error("ExifTool did not start!");
        return;
    }
}

void stop_exiftool() {
    if (exiftool.state() == QProcess::NotRunning) return;

    exiftool.write("-stay_open\nFalse\n");
    exiftool.waitForBytesWritten();
    exiftool.closeWriteChannel();
    exiftool.waitForFinished();
}

namespace Image {

QPixmap load_heic(const QString& path) {
    heif_context* ctx = heif_context_alloc();

    heif_error err = heif_context_read_from_file(
        ctx,
        path.toUtf8().constData(),
        nullptr
    );
    if (err.code != heif_error_Ok) {
        heif_context_free(ctx);
        throw std::runtime_error(
            "heif_context_read_from_file failed: " + std::string(err.message) + "!"
        );
    }

    heif_image_handle* handle = nullptr;
    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok) {
        heif_context_free(ctx);
        throw std::runtime_error(
            "heif_context_get_primary_image_handle failed: " + std::string(err.message) + "!"
        );
    }

    heif_image* image = nullptr;
    err = heif_decode_image(
        handle,
        &image,
        heif_colorspace_RGB,
        heif_chroma_interleaved_RGB,
        nullptr
    );
    if (err.code != heif_error_Ok) {
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        throw std::runtime_error(
            "heif_decode_image failed: " + std::string(err.message) + "!"
        );
    }

    int width = heif_image_get_width(
        image,
        heif_channel_interleaved
    );
    int height = heif_image_get_height(
        image,
        heif_channel_interleaved
    );
    int stride;
    const uint8_t* data = heif_image_get_plane_readonly(
        image,
        heif_channel_interleaved,
        &stride
    );

    if (!data || width <= 0 || height <= 0) {
        heif_image_release(image);
        heif_image_handle_release(handle);
        heif_context_free(ctx);
        throw std::runtime_error("Invalid HEIC data!");
    }

    QImage qimg(data, width, height, stride, QImage::Format_RGB888);
    QImage final_image = qimg.copy();  // Must detach from libheif memory before freeing

    heif_image_release(image);
    heif_image_handle_release(handle);
    heif_context_free(ctx);

    QPixmap pixmap = QPixmap::fromImage(final_image);
    if (pixmap.isNull()) {
        throw std::runtime_error("Null HEIC pixmap!");
    }

    return pixmap;
}

QPixmap load_image(const QString& path) {
    if (path.endsWith(".heic")) {
        std::cout << "load";
        return load_heic(path);
    }
    else {
        return QPixmap(path);
    }
}

void write_heic(
    const std::string& filepath,
    const std::map<std::string, std::string>& metadata
) {
    start_exiftool();

    QByteArray command;
    command.append("-overwrite_original\n");

    for (const auto& [key, value] : metadata) {
        auto pos = key.find_last_of('.');
        std::string tag = (pos != std::string::npos) ? key.substr(pos + 1) : key;
        command.append(
            '-' + QByteArray::fromStdString(tag) +
            '=' + QByteArray::fromStdString(value) +
            '\n'
        );
    }

    command.append(QByteArray::fromStdString(filepath) + '\n');
    command.append("-execute65536\n");

    exiftool.write(command);
    exiftool.waitForBytesWritten();

    while (!exiftool.canReadLine())
        exiftool.waitForReadyRead();

    while (true) {
        QByteArray line = exiftool.readLine();
        if (line.contains("{ready65536}"))
            break;
        // if (!line.trimmed().isEmpty())
        //     qDebug() << "ExifTool:" << line.trimmed();
    }

    QString std_out = exiftool.readAllStandardOutput();
    QString std_err = exiftool.readAllStandardError();

    if (!std_err.isEmpty()) {
        throw std::runtime_error("ExifTool error:" + std_err.toStdString());
    }
}

std::unique_ptr<Exiv2::Image> write_image(
    const QString& filepath,
    const std::map<std::string, std::string>& metadata,
    std::unique_ptr<Exiv2::Image> image
) {
    if (filepath.endsWith(".heic")) {
        write_heic(filepath.toStdString(), metadata);
    }
    else {
        Exiv2::ExifData exif_data = image->exifData();
        for (auto& [key, value] : metadata) {
            exif_data[key] = value;
        }

        image->setExifData(exif_data);
        image->writeMetadata();
    }
    return image;
}

}  // namespace Image