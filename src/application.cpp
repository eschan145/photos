#include "application.h"

QString AssetManager::operator[](const QString& key) {
    return "assets/" + key + ".svg";
}

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

inline QString qs(const std::string& string) {
    return QString::fromStdString(string);
}

Application::Application() {
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

    QScrollArea* scroll_area = new QScrollArea;
    scroll_area->setWidgetResizable(true);
    scroll_area->setWidget(metadata_layoutw);
    scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    metadata_layout->setSpacing(0);
    metadata_layout->setContentsMargins(0, 0, 0, 0);

    this->main_layout->addWidget(scroll_area);

    connect(
        open_button,
        &QPushButton::clicked,
        this,
        &Application::load_image
    );
}

void Application::create_widgets(
    const QString& title,
    const QList<MetadataField>& values,
    const QString& icon,
    DataType type,
    std::string key,
    bool is_binary,
    int max_rows
) {
    QWidget* container = new QWidget;
    QHBoxLayout* container_layout = new QHBoxLayout;
    container->setLayout(container_layout);
    container_layout->setContentsMargins(0, 0, 0, 0);

    QSvgWidget* icon_widget = new QSvgWidget(icon);
    icon_widget->setFixedSize(24, 24);
    icon_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    container_layout->addWidget(icon_widget, 0, Qt::AlignTop);

    // Right side: title and data
    QWidget* right_side = new QWidget;
    QVBoxLayout* right_layout = new QVBoxLayout;
    right_side->setLayout(right_layout);
    right_layout->setContentsMargins(0, 0, 0, 0);

    if (type != DataType::MULTISTRING && type != DataType::MULTILINE) {
        QLabel* label = new QLabel(title);
        label->setFont(QFont("Segoe UI", 11));
        label->setFrameStyle(QFrame::Box | QFrame::Plain);
        label->setLineWidth(0);

        right_layout->addWidget(label);
    }

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
    } else if (type == DataType::MULTISTRING) {
        QLineEdit* line_edit = new QLineEdit;
        line_edit->setFixedWidth(290);
        line_edit->setFixedHeight(30);
        line_edit->setPlaceholderText(title);
        line_edit->setText(values[0].value);
        right_layout->addWidget(line_edit);

        if (!key.empty()) {
            connect(
                line_edit,
                &QLineEdit::textChanged,
                this,
                [this, is_binary, key](const QString& text) {
                    if (is_binary) {
                        std::u16string utf16 = text.toStdU16String();
                        std::ostringstream oss;

                        for (char16_t ch : utf16) {
                            oss << static_cast<int>(ch & 0xFF) << ' '
                                << static_cast<int>((ch >> 8) & 0xFF) << ' ';
                        }

                        oss << "0 0";

                        this->metadata[key] = oss.str();
                    }
                    else {
                        this->metadata[key] = text.toStdString();
                    }
                    this->refresh_metadata();
                }
            );

        }
        // UAF: Important to only pass [this, key] for lambda to own full copy
        // instead of [&] to own reference in connect(). Otherwise the key var
        // would be immediately destroyed after lambda and std::string would
        // try to allocate 1.5 million characters from a dangling pointer such
        // as 0x10017ee40, throwing SIGSEGV or std::bad_alloc.
    }
    else if (type == DataType::MULTILINE) {
        QTextEdit* text_edit = new QTextEdit;
        text_edit->setFixedWidth(290);
        text_edit->setMaximumHeight(500);
        text_edit->setPlaceholderText(title);
        text_edit->setText(values[0].value);
        
        right_layout->addWidget(text_edit);

        auto resize = [this, text_edit, key]() {
            int height = static_cast<int>(text_edit->document()->size().height());
            int margin = text_edit->contentsMargins().top() +
                text_edit->contentsMargins().bottom();
            text_edit->setFixedHeight(std::max(30, height + margin));
            this->metadata[key] = text_edit->toMarkdown().toStdString();
            this->refresh_metadata();
        };
        if (!key.empty()) {
            connect(
                text_edit,
                &QTextEdit::textChanged,
                this,
                resize
            );
        }

        // Resize text edit to fit new content after text is updated by Qt
        QTimer::singleShot(0, this, resize);
    }

    right_layout->addWidget(data_layoutw);
    container_layout->addWidget(right_side);

    this->field_layout->addWidget(container, 0, Qt::AlignTop | Qt::AlignLeft);
    this->field_layout->addItem(
        new QSpacerItem(0, 15, QSizePolicy::Minimum, QSizePolicy::Fixed)
    );
    this->metadata_layout->addWidget(this->field_layoutw, 0, Qt::AlignTop);
}

QList<QPair<QString, QString>> Application::process_metadata(
    Exiv2::ExifData& exif_data
) {
    QList<QPair<QString, QString>> metadata;
    std::map<std::string, std::string> exifdata;

    for (auto it = exif_data.begin(); it != exif_data.end(); ++it) {
        exifdata[it->key()] = it->value().toString();
        std::cout << it->key() << ": " << it->value().toString() << "\n";
    }

    for (const auto& pair : exif_data) {
        std::string key = pair.key();
        QString value = qs(pair.toString());
    }

    // if (exifdata.contains("Exif.Image.XPTitle")) {
        this->create_widgets(
            "Title",
            {
                {
                    "Title",
                    qs(Utils::read_bytes(exifdata["Exif.Image.XPTitle"]))
                }
            },
            icons["title"],
            DataType::MULTISTRING,
            "Exif.Image.XPTitle",
            this->filepath.endsWith(".heic") ? false : true
        );
        std::cout << exifdata["Exif.Image.XPTitle"] << " to " << Utils::read_bytes(exifdata["Exif.Image.XPTitle"]) << "\n";
    // }
    // if (exifdata.contains("Exif.Image.XPTitle")) {
        this->create_widgets(
            "Description",
            {
                {
                    "Description",
                    qs(Utils::read_bytes(exifdata["Exif.Image.ImageDescription"]))
                }
            },
            icons["description"],
            DataType::MULTILINE,
            "Exif.Image.ImageDescription"
        );
    // }
    if (exifdata.contains("Exif.Photo.DateTimeOriginal")) {
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
    if (exifdata.contains("Exif.Image.ImageWidth")) {
        auto rational = [&](const std::string& tag) -> float {
            return Utils::parse_rational(exifdata[tag]);
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
    if (exifdata.contains("Exif.Photo.ExposureTime")) {
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
            "Camera",
            {
                {"Make", qs(exifdata["Exif.Image.Make"])},
                {"Model", qs(exifdata["Exif.Image.Model"])},
                {"Focal length", focal_length},
                {"F-stop", aperture},
                {"Exposure time", exposure_time},
                {"ISO speed", iso},
                {"Flash", flash},
                {"Exposure program", exposure_program},
                {"White Balance", wb},
                {
                    "Zoom ratio",
                    qs(exifdata["Exif.Photo.DigitalZoomRatio"])
                }
            },
            icons["camera"]
        );
    }
    if (exifdata.contains("Exif.Image.GPSTag") &&
        exifdata.contains("Exif.GPSInfo.GPSLatitude") &&
        exifdata.contains("Exif.GPSInfo.GPSLatitudeRef") &&
        exifdata.contains("Exif.GPSInfo.GPSLongitude") &&
        exifdata.contains("Exif.GPSInfo.GPSLongitudeRef") &&
        exifdata.contains("Exif.GPSInfo.GPSAltitude") &&
        exifdata.contains("Exif.GPSInfo.GPSAltitudeRef")) {

        QStringList lat_dms =
            QString::fromStdString(
                exifdata["Exif.GPSInfo.GPSLatitude"]
            ).split(' ');
        QStringList lon_dms =
            QString::fromStdString(
                exifdata["Exif.GPSInfo.GPSLongitude"]
            ).split(' ');

        float latitude = Utils::to_decimal(
            lat_dms, exifdata["Exif.GPSInfo.GPSLatitudeRef"]
        );
        float longitude = Utils::to_decimal(
            lon_dms, exifdata["Exif.GPSInfo.GPSLongitudeRef"]
        );

        int altitude_ref =
            std::stoi(exifdata["Exif.GPSInfo.GPSAltitudeRef"]);

        float altitude = Utils::parse_fraction(
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
    this->create_widgets(
        "Source",
        {
            {
                "Source",
                this->filepath
            }
        },
        icons["folder"],
        DataType::MULTISTRING
    );

    return metadata;
}

void Application::load_image() {
    this->filepath = QFileDialog::getOpenFileName(
        this, "Open file", "", "Images (*.png; *.xpm; *.jpg; *.heic)");

    QPixmap pixmap = Image::load_image(this->filepath);
    if (pixmap.isNull()) {
        std::cerr << "Failed to load image: " << this->filepath.toStdString()
                  << std::endl;
        return;
    }
    clear_layout(this->metadata_layout);

    // Prevent dangling pointer when its uninitialized
    this->field_layoutw = new QWidget;
    this->field_layoutw->setMinimumWidth(DATAPANEL_WIDTH);
    this->field_layoutw->setMaximumWidth(DATAPANEL_WIDTH);
    this->field_layout = new QVBoxLayout(this->field_layoutw);
    this->field_layoutw->setLayout(this->field_layout);

    QSize max_size(this->width() - DATAPANEL_WIDTH, this->height());
    QPixmap scaled_pixmap = pixmap.scaled(
        max_size,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    this->image_label->setPixmap(scaled_pixmap);
    this->image_label->show();

    this->image = Exiv2::ImageFactory::open(this->filepath.toStdString());

    this->image->readMetadata();
    this->exif_data = this->image->exifData();

    if (this->exif_data.empty()) {
        throw std::runtime_error("Empty exif data!");
    } else {
        process_metadata(this->exif_data);
    }
}

void Application::refresh_metadata() {
    if (this->metadata.empty()) return;

    for (auto& [key, value] : this->metadata) {
        this->exif_data[key] = value;
        // std::cout << key << ": " << value << "\n";
    }

    this->image = Image::write_image(
        this->filepath,
        this->metadata,
        std::move(this->image)
    );

    // this->metadata.clear();
}
