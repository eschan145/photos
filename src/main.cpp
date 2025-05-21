#include <Exiv2/exiv2.hpp>
#include <QApplication>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QFont>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QPair>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <QtSvgWidgets/QSvgWidget>
#include <iostream>
#include <map>

const int DATAPANEL_WIDTH = 350;

struct FieldData {
    QString readable_name;
};

enum class DataType { STRING, DATE };

struct MetadataField {
    QString name;
    QString value;
    DataType type = DataType::STRING;
};

std::map<QString, FieldData> supported_fields = {
    {"Exif.Image.DateTime", {"Date/Time"}},
    {"Exif.Image.ImageLength", {"Height"}},
    {"Exif.Image.ImageWidth", {"Width"}},
    {"Exif.Image.Model", {"Model"}},
    {"Exif.Photo.ExposureTime", {"Exposure time"}},
    {"Exif.Photo.ApertureValue", {"Aperture"}},
    {"Exif.Photo.ISOSpeedRatings", {"ISO"}},
    {"Exif.Photo.FNumber", {"F-stop"}},
    {"Exif.Photo.Flash", {"Flash"}},

};


class AssetManager {
 public:
    AssetManager() = default;

    QString operator[](const QString& key) {
        return "assets/" + key + ".svg";
    }
};

AssetManager icons;

void clear_layout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while (true) {
        item = layout->takeAt(0);
        if (!item) break;

        QWidget* widget = item->widget();
        QLayout* child_layout = item->layout();

        if (widget) widget->deleteLater();
        if (child_layout) clear_layout(child_layout);

        delete item;
    }
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
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

inline QString qs(const std::string& string) {
    return QString::fromStdString(string);
}

class MainWindow : public QMainWindow {
   public:
    MainWindow() {
        this->setFixedSize(700, 500);
        central_widget = new QWidget(this);

        this->main_layout = new QHBoxLayout(central_widget);
        central_widget->setLayout(main_layout);
        setCentralWidget(central_widget);

        this->image_layout = new QVBoxLayout;
        this->main_layout->addLayout(this->image_layout);

        QPushButton* open_button = new QPushButton("Open image");
        this->image_layout->addWidget(open_button);

        image_label = new QLabel;
        this->image_layout->addWidget(image_label);

        QWidget* metadata_layoutw = new QWidget;
        this->metadata_layout = new QVBoxLayout(metadata_layoutw);
        metadata_layoutw->setLayout(this->metadata_layout);

        this->field_layoutw = new QWidget;
        this->field_layoutw->setMinimumWidth(DATAPANEL_WIDTH);
        this->field_layoutw->setMaximumWidth(DATAPANEL_WIDTH);
        this->field_layout = new QVBoxLayout(this->field_layoutw);
        this->field_layoutw->setLayout(this->field_layout);

        QScrollArea* scroll_area = new QScrollArea;
        scroll_area->setWidgetResizable(true);
        scroll_area->setWidget(metadata_layoutw);
        scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        this->main_layout->addWidget(scroll_area);

        connect(open_button, &QPushButton::clicked, this,
                &MainWindow::load_image);

        QList<QPair<QString, QString>> metadata = {};
    }

   private:
    QHBoxLayout* main_layout;
    QVBoxLayout* image_layout;
    QVBoxLayout* metadata_layout;
    QWidget* central_widget;
    QWidget* layoutw;
    QVBoxLayout* field_layout;
    QWidget* field_layoutw;
    QLabel* image_label;

    void create_widgets(
        const QString& title,
        const QList<MetadataField>& values,
        const QString& icon = "",
        DataType type = DataType::STRING,
        int max_rows = 1
    ) {
        QWidget* container = new QWidget;
        QHBoxLayout* container_layout = new QHBoxLayout;
        container->setLayout(container_layout);
        container_layout->setContentsMargins(0, 0, 0, 0);

        QSvgWidget* icon_widget = new QSvgWidget(icon);
        icon_widget->setFixedSize(32, 32);
        container_layout->addWidget(icon_widget, 0, Qt::AlignTop);

        // Right side: title and data
        QWidget* right_side = new QWidget;
        QVBoxLayout* right_layout = new QVBoxLayout;
        right_side->setLayout(right_layout);
        right_layout->setContentsMargins(0, 0, 0, 0);

        QLabel* label = new QLabel(title);
        label->setFont(QFont("Segoe UI", 11));
        label->setFrameStyle(QFrame::Box | QFrame::Plain);
        label->setLineWidth(0);

        right_layout->addWidget(label);

        QWidget* data_layoutw = new QWidget;

        if (type == DataType::STRING) {
            QVBoxLayout* data_layout = new QVBoxLayout;
            data_layout->setContentsMargins(0, 0, 0, 0);
            data_layoutw->setLayout(data_layout);

            int total = values.size();
            int columns = 5;
            int rows = (total + columns - 1) / columns;

            int counter = 0;
            for (int row = 0; row < rows; ++row) {
                QWidget* row_layoutw = new QWidget;
                QHBoxLayout* row_layout = new QHBoxLayout;
                row_layout->setContentsMargins(0, 0, 5, 0);
                row_layoutw->setLayout(row_layout);

                for (int col = 0; col < columns; ++col) {
                    int index = row * columns + col;
                    if (index >= total) break;

                    QLineEdit* edit = new QLineEdit;
                    edit->setText(values[counter].value);
                    edit->setToolTip(values[counter].name);
                    edit->setStyleSheet(
                        "QLineEdit { background: transparent; border: none; }");

                    auto resize = [edit]() {
                        QFontMetrics metrics(edit->font());
                        int width =
                            metrics.horizontalAdvance(edit->text()) + 10;
                        edit->setFixedWidth(width);
                    };
                    resize();
                    connect(edit, &QLineEdit::textChanged, resize);

                    row_layout->addWidget(edit, 0, Qt::AlignLeft);
                    counter++;
                }

                row_layout->addStretch();
                data_layout->addWidget(row_layoutw, 0, Qt::AlignTop);
            }
            data_layout->addStretch();

        } else if (type == DataType::DATE) {
            QHBoxLayout* data_layout = new QHBoxLayout;
            data_layout->setContentsMargins(0, 0, 0, 0);
            data_layoutw->setLayout(data_layout);

            QDateTimeEdit* dt_edit = new QDateTimeEdit;
            dt_edit->setFixedWidth(290);
            dt_edit->setDisplayFormat("MMMM d, h:mm:ss AP");
            dt_edit->setCalendarPopup(true);
            dt_edit->setDateTime(
                QDateTime::fromString(values[0].value, "yyyy:MM:dd HH:mm:ss"));
            data_layout->addWidget(dt_edit, 0, Qt::AlignLeft);
            data_layout->addStretch();
        }

        right_layout->addWidget(data_layoutw);
        container_layout->addWidget(right_side);

        this->field_layout->addItem(
            new QSpacerItem(0, 15, QSizePolicy::Minimum, QSizePolicy::Fixed));
        this->field_layout->addWidget(container);
        this->metadata_layout->addWidget(this->field_layoutw, 0, Qt::AlignTop);
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

    QList<QPair<QString, QString>> process_metadata(
        Exiv2::ExifData& exif_data) {
        QList<QPair<QString, QString>> metadata;
        std::map<std::string, std::string> exifdata;

        for (auto it = exif_data.begin(); it != exif_data.end(); ++it) {
            exifdata[it->key()] = it->value().toString();
            std::cout << it->key() << ": " << it->value().toString() << "\n";
        }

        for (const auto& pair : exif_data) {
            std::string key = pair.key();
            QString value = qs(pair.toString());
            if (key == "Exif.Photo.DateTimeOriginal") {
                this->create_widgets(
                    "Date",
                    {
                        {
                            "Date",
                            qs(exifdata["Exif.Photo.DateTimeOriginal"]),
                            DataType::DATE
                        }
                    },
                    icons["date"],
                    DataType::DATE
                );
            }
            if (key == "Exif.Image.ImageWidth") {
                auto rational = [&](const std::string& tag) -> float {
                    return parse_rational(exifdata[tag]);
                };

                double x_resolution = rational("Exif.Image.XResolution");
                double y_resolution = rational("Exif.Image.YResolution");
                int resolution_unit =
                    std::stoi(exifdata["Exif.Image.ResolutionUnit"]);

                float multiplier;
                if (resolution_unit == 2) {
                    multiplier = 1;
                } else if (resolution_unit == 3) {
                    multiplier = 2.54;
                } else {
                    throw std::invalid_argument("Unknown resolution unit!");
                }

                x_resolution *= multiplier;
                y_resolution *= multiplier;

                float dpi = std::sqrt(x_resolution * y_resolution);

                this->create_widgets(
                    "Dimensions",
                    {
                        {
                            "Dimensions",
                            qs(exifdata["Exif.Image.ImageWidth"]) +
                            " x " +
                            qs(exifdata["Exif.Image.ImageLength"])
                        },
                        {"DPI", QString::number(dpi)}
                    },
                    icons["dimension"]
                );
            }
            if (key == "Exif.Photo.ExposureTime") {
                QString focal_length;
                {
                    std::string raw = exifdata["Exif.Photo.FocalLength"];
                    float result = 0.f;
                    if (raw.find('/') != std::string::npos) {
                        size_t slash = raw.find('/');
                        double num = std::stod(raw.substr(0, slash));
                        double denom = std::stod(raw.substr(slash + 1));
                        result = num / denom;
                    } else {
                        result = std::stod(raw);
                    }
                    focal_length = QString::number(result, 'f', 1) + " mm";
                }
                QString aperture;
                {
                    std::string raw = exifdata["Exif.Photo.FNumber"];
                    float value = 0.f;
                    if (raw.find('/') != std::string::npos) {
                        size_t slash = raw.find('/');
                        double num = std::stod(raw.substr(0, slash));
                        double denom = std::stod(raw.substr(slash + 1));
                        value = num / denom;
                    } else {
                        value = std::stod(raw);
                    }
                    aperture = "f/" + QString::number(value, 'f', 1);
                }
                QString exposure_program;
                {
                    switch (std::stoi(exifdata["Exif.Photo.ExposureProgram"])) {
                        case 1:
                            exposure_program = "Manual";
                            break;
                        case 2:
                            exposure_program = "Normal";
                            break;
                        case 3:
                            exposure_program = "Aperture";
                            break;
                        case 4:
                            exposure_program = "Shutter";
                            break;
                        case 5:
                            exposure_program = "Creative";
                            break;
                        case 6:
                            exposure_program = "Action";
                            break;
                        case 7:
                            exposure_program = "Portrait";
                            break;
                        case 8:
                            exposure_program = "Landscape";
                            break;
                        default:
                            exposure_program = "Unknown";
                    }
                }

                QString exposure_time =
                    qs(exifdata["Exif.Photo.ExposureTime"]) + " sec";
                QString iso =
                    "ISO " + qs(exifdata["Exif.Photo.ISOSpeedRatings"]);
                QString flash = (std::stoi(exifdata["Exif.Photo.Flash"]) & 0x1)
                                    ? "Flash"
                                    : "No flash";
                QString wb =
                    (std::stoi(exifdata["Exif.Photo.WhiteBalance"]) == 0)
                        ? "Auto"
                        : "Manual";

                this->create_widgets(
                    "Camera", {{"Make", qs(exifdata["Exif.Image.Make"])},
                               {"Model", qs(exifdata["Exif.Image.Model"])},
                               {"Focal length", focal_length},
                               {"F-stop", aperture},
                               {"Exposure time", exposure_time},
                               {"ISO speed", iso},
                               {"Flash", flash},
                               {"Exposure program", exposure_program},
                               {"White Balance", wb},
                               {"Zoom ratio",
                                qs(exifdata["Exif.Photo.DigitalZoomRatio"])}}, icons["camera"]);
            }
            if ((key == "Exif.Image.GPSTag")) {
                if (!(exifdata.contains("Exif.GPSInfo.GPSLatitude") &&
                      exifdata.contains("Exif.GPSInfo.GPSLatitudeRef") &&
                      exifdata.contains("Exif.GPSInfo.GPSLongitude") &&
                      exifdata.contains("Exif.GPSInfo.GPSLongitudeRef") &&
                      exifdata.contains("Exif.GPSInfo.GPSAltitude") &&
                      exifdata.contains("Exif.GPSInfo.GPSAltitudeRef"))) {
                    std::cout << "Missing GPS metadata.";
                    continue;
                }

                QStringList lat_dms =
                    QString::fromStdString(
                        exifdata["Exif.GPSInfo.GPSLatitude"]
                    ).split(' ');
                QStringList lon_dms =
                    QString::fromStdString(
                        exifdata["Exif.GPSInfo.GPSLongitude"]
                    ).split(' ');

                float latitude = to_decimal(
                    lat_dms, exifdata["Exif.GPSInfo.GPSLatitudeRef"]
                );
                float longitude = to_decimal(
                    lon_dms, exifdata["Exif.GPSInfo.GPSLongitudeRef"]
                );

                int altitude_ref =
                    std::stoi(exifdata["Exif.GPSInfo.GPSAltitudeRef"]);

                float altitude = parse_fraction(
                    QString::fromStdString(
                        exifdata["Exif.GPSInfo.GPSAltitude"]
                    )
                );

                if (altitude_ref == 1) {
                    altitude = -altitude;
                }

                this->create_widgets(
                    "GPS",
                    {
                        {"Latitude", QString::number(latitude)},
                        {"Longitude", QString::number(longitude)},
                        {"Altitude", QString::number(altitude)}
                    },
                    icons["location"]
                );
            }
        }

        return metadata;
    }

    void load_image() {
        QString filename = QFileDialog::getOpenFileName(
            this, "Open file", "", "Images (*.png; *.xpm; *.jpg; *.heic)");

        QPixmap pixmap(filename);
        if (pixmap.isNull()) {
            std::cerr << "Failed to load image: " << filename.toStdString()
                      << std::endl;
            return;
        }
        clear_layout(this->metadata_layout);

        // Prevent dangling pointer when its uninitialized
        this->field_layoutw = new QWidget;
        this->field_layout = new QVBoxLayout;
        this->field_layoutw->setLayout(this->field_layout);

        QSize max_size(this->width() - DATAPANEL_WIDTH, this->height());
        QPixmap scaled_pixmap = pixmap.scaled(max_size, Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation);
        this->image_label->setPixmap(scaled_pixmap);
        this->image_label->show();

        std::unique_ptr<Exiv2::Image> image =
            Exiv2::ImageFactory::open(filename.toStdString());
        image->readMetadata();

        Exiv2::ExifData& exif_data = image->exifData();

        QList<QPair<QString, QString>> metadata;

        if (exif_data.empty()) {
            throw std::runtime_error("Empty exif data!");
        } else {
            metadata = process_metadata(exif_data);
        }
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
