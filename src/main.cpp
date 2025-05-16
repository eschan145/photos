#include <iostream>
#include <map>

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QFont>
#include <QPushButton>
#include <QMap>
#include <QString>
#include <QFileDialog>
#include <QLayout>
#include <QPair>
#include <QList>
#include <QTextEdit>
#include <QPalette>
#include <QScrollArea>

#include <Exiv2/exiv2.hpp>

struct FieldData {
    QString readable_name;
};

struct MetadataRow {
    QLabel* label;
    QWidget* widget;
};

QMap<QString, MetadataRow> metadata_rows;


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
        this->setFixedSize(500, 400);
        central_widget = new QWidget(this);

        this->main_layout = new QHBoxLayout(central_widget);
        central_widget->setLayout(main_layout);
        setCentralWidget(central_widget);

        this->image_layout = new QVBoxLayout;
        this->main_layout->addLayout(this->image_layout);

        QPushButton* open_button = new QPushButton("Open image");
        this->image_layout->addWidget(open_button);

        QWidget* metadata_layoutw = new QWidget;
        this->metadata_layout = new QVBoxLayout(metadata_layoutw);
        metadata_layoutw->setLayout(this->metadata_layout);

        QScrollArea* scroll_area = new QScrollArea;
        scroll_area->setWidgetResizable(true);
        scroll_area->setWidget(metadata_layoutw);

        this->main_layout->addWidget(scroll_area);

        connect(open_button, &QPushButton::clicked, this, &MainWindow::simulateMetadataUpdate);

        QList<QPair<QString, QString>> metadata = {};

    }

private:
    QHBoxLayout* main_layout;
    QVBoxLayout* image_layout;
    QVBoxLayout* metadata_layout;
    QWidget* central_widget;

    void create_widgets(const QString& title, const QList<QString>& values, int max_rows = 1) {
        QWidget* layoutw = new QWidget;
        layoutw->setFixedWidth(300);
        
        QVBoxLayout* layout = new QVBoxLayout(layoutw);

        QString text;
        for (int i = 0; i < values.size(); ++i) {
            text += values[i];
            if (i != values.size() - 1) {
                text += "\t";
            }
        }

        QWidget* text_layoutw = new QWidget;
        QGridLayout* text_layout = new QGridLayout;
        text_layoutw->setLayout(text_layout);
        int columns = 4;
        for (int i = 0; i < values.size(); ++i) {
            QLineEdit* edit = new QLineEdit;
            edit->setText(values[i]);
            
            connect(edit, &QLineEdit::textChanged, [edit]() {
                QFontMetrics metrics(edit->font());
                int width = metrics.horizontalAdvance(edit->text()) + 10;
                edit->setFixedWidth(width);
            });
            int row = i / columns;
            int col = i % columns;
            text_layout->addWidget(edit, row, col);
        }
        

        
        
        // QFontMetrics font_metrics(text_edit->font());
        // int line_height = font_metrics.lineSpacing();
        // int max_height = line_height * max_rows + 6;
        // text_edit->setMaximumHeight(max_height);
        // QPalette palette = text_edit->palette();
        // palette.setColor(QPalette::Base, Qt::transparent);
        // text_edit->setPalette(palette);
        // text_edit->setAttribute(Qt::WA_TranslucentBackground);
        // text_edit->setFrameStyle(QFrame::NoFrame);

        QLabel* label = new QLabel(title);
        label->setFont(QFont("Segoe UI", 11));
        label->setFrameStyle(QFrame::Box | QFrame::Plain);

        label->setLineWidth(1);

        layout->addWidget(label);
        layout->addWidget(text_layoutw);
        this->metadata_layout->addWidget(layoutw, 0, Qt::AlignTop);
    }


    float parse_fraction(const QString& fraction) {
        QStringList parts = fraction.split('/');
        if (parts.size() != 2) {
            throw std::invalid_argument(
                "Invalid fraction format: " + fraction.toStdString() + "!"
            );
        }

        bool ok1, ok2;
        float numerator = parts[0].toFloat(&ok1);
        float denominator = parts[1].toFloat(&ok2);

        if (!ok1 || !ok2) {
            throw std::invalid_argument(
                "Non-numeric fraction part: " + fraction.toStdString() + "!"
            );
        }
        if (denominator == 0)
            throw std::invalid_argument(
                "Division by zero in fraction: " + fraction.toStdString()
            );

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

    QList<QPair<QString, QString>> process_metadata(Exiv2::ExifData& exif_data) {
        QList<QPair<QString, QString>> metadata;
        std::map<std::string, std::string> exifdata;

        for (auto it = exif_data.begin(); it != exif_data.end(); ++it) {
            exifdata[it->key()] = it->value().toString();
            std::cout << it->key() << "\n";
        }

        for (const auto& pair : exif_data) {
           std::string key = pair.key();
            QString value = qs(pair.toString());
            if (key == "Exif.Image.ImageWidth") {
                auto rational = [&](const std::string& tag) -> float {
                    return parse_rational(exifdata[tag]);
                };
                
                double x_resolution = rational("Exif.Image.XResolution");
                double y_resolution = rational("Exif.Image.YResolution");
                int resolution_unit = std::stoi(exifdata["Exif.Image.ResolutionUnit"]);
                
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
                
                this->create_widgets("Dimensions",
                    {
                        qs(exifdata.at("Exif.Image.ImageWidth")) + " x " +
                        qs(exifdata.at("Exif.Image.ImageLength")),
                        QString::number(dpi)
                    }
                );
            }
            if (key == "Exif.Image.GPSTag") {
                QStringList lat_dms = QString::fromStdString(
                    exifdata.at("Exif.GPSInfo.GPSLatitude")
                ).split(' ');
                QStringList lon_dms = QString::fromStdString(
                    exifdata.at("Exif.GPSInfo.GPSLongitude")
                ).split(' ');

                float latitude = to_decimal(
                    lat_dms,
                    exifdata.at("Exif.GPSInfo.GPSLatitudeRef")
                );
                float longitude = to_decimal(
                    lon_dms,
                    exifdata.at("Exif.GPSInfo.GPSLongitudeRef")
                );

                int altitude_ref = std::stoi(exifdata["Exif.GPSInfo.GPSAltitudeRef"]);

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
                        QString::number(latitude),
                        QString::number(longitude),
                        QString::number(altitude)
                    }
                );
            }
        }

        return metadata;
    }

    void simulateMetadataUpdate() {
        QString filename = QFileDialog::getOpenFileName(
            this,
            "Open file",
            "",
            "Images (*.png; *.xpm; *.jpg; *.heic)"
        );
         std::unique_ptr<Exiv2::Image> image = Exiv2::ImageFactory::open(
            filename.toStdString()
        );
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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
