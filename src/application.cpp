#include "application.h"

QStringList IMAGE_EXTENSIONS = {
    "png",
    "jpg",
    "heic"
};

QString AssetManager::operator[](const QString& key) {
    return "./assets/" + key + ".svg";
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

Application::Application(const char* folder) {
    this->resize(900, 600);
    central_widget = new QWidget(this);

    this->main_layout = new QHBoxLayout(central_widget);
    central_widget->setLayout(main_layout);
    setCentralWidget(central_widget);

    this->image_layout = new QVBoxLayout;
    this->main_layout->addLayout(this->image_layout);

    QPushButton* open_button = new QPushButton("Open image");
    this->image_layout->addWidget(open_button);

    this->image_label = new QLabel;
    this->image_label->setAlignment(Qt::AlignCenter);

    this->image_scroll_area = new QScrollArea;
    image_scroll_area->setWidget(this->image_label);
    image_scroll_area->setWidgetResizable(true);

    this->image_layout->addWidget(image_scroll_area);

    this->left_button = new QPushButton;
    this->left_button->setIcon(QIcon(icons["arrow_left"]));
    this->left_button->setFixedSize(ARROW_SIZE, ARROW_SIZE);
    this->left_button->setIconSize({ARROW_SIZE, ARROW_SIZE});
    this->left_button->setParent(this->image_label);
    this->left_button->setToolTip("Previous");

    auto* left_effect = new QGraphicsOpacityEffect(this->left_button);
    this->left_button->setGraphicsEffect(left_effect);
    left_effect->setOpacity(0.0);

    this->left_opacity_anim = new QPropertyAnimation(left_effect, "opacity", this);
    this->left_opacity_anim->setDuration(100);

    this->right_button = new QPushButton;
    this->right_button->setIcon(QIcon(icons["arrow_right"]));
    this->right_button->setFixedSize(ARROW_SIZE, ARROW_SIZE);
    this->right_button->setIconSize({ARROW_SIZE, ARROW_SIZE});
    this->right_button->setParent(this->image_label);
    this->right_button->setToolTip("Next");

    auto* right_effect = new QGraphicsOpacityEffect(this->right_button);
    this->right_button->setGraphicsEffect(right_effect);
    right_effect->setOpacity(0.0);

    this->right_opacity_anim = new QPropertyAnimation(right_effect, "opacity", this);
    this->right_opacity_anim->setDuration(100);

    connect(
        this->left_button,
        &QPushButton::pressed,
        this,
        [this] {
            this->left_button->setIconSize(
                {ARROW_SIZE - 3, ARROW_SIZE - 3}
            );
        }
    );
    connect(
        this->left_button,
        &QPushButton::released,
        this,
        [this] {
            this->left_button->setIconSize(
                {ARROW_SIZE, ARROW_SIZE}
            );
            this->previous();
        }
    );
    connect(
        this->right_button,
        &QPushButton::pressed,
        this,
        [this] {
            this->right_button->setIconSize({ARROW_SIZE - 3, ARROW_SIZE - 3});
        }
    );
    connect(
        this->right_button,
        &QPushButton::released,
        this,
        [this] {
            this->right_button->setIconSize({ARROW_SIZE, ARROW_SIZE});
            this->next();
        }
    );

    // connect(right_button, &QPushButton::pressed, this, {

    // });

    // this->image_layout->addWidget(image_container);

    QWidget* metadata_layoutw = new QWidget;
    this->metadata_layout = new QVBoxLayout(metadata_layoutw);
    metadata_layoutw->setMaximumWidth(DATAPANEL_WIDTH);
    metadata_layoutw->setLayout(this->metadata_layout);

    QScrollArea* scroll_area = new QScrollArea;
    scroll_area->setWidgetResizable(true);
    scroll_area->setWidget(metadata_layoutw);
    scroll_area->setMinimumWidth(DATAPANEL_WIDTH);
    scroll_area->setMaximumWidth(DATAPANEL_WIDTH);
    scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    metadata_layout->setSpacing(0);
    metadata_layout->setContentsMargins(0, 0, 0, 0);

    this->setMouseTracking(true);
    this->centralWidget()->setMouseTracking(true);
    this->image_label->setMouseTracking(true);
    this->image_scroll_area->setMouseTracking(true);
    scroll_area->setMouseTracking(true);

    this->main_layout->addWidget(scroll_area);

    connect(
        open_button,
        &QPushButton::clicked,
        this,
        &Application::open_directory
    );

    QDir("cacheDir").removeRecursively();
    auto cache = new QNetworkDiskCache(this);
    cache->setCacheDirectory("cacheDir");
    auto manager = new QNetworkAccessManager(this);
    manager->setCache(cache);
    QGV::setNetworkManager(manager);

    // Ensure the cache dir is created and initialized before!
    std::cout << "Opening folder " << folder << "\n";
    if (folder) {
        this->current_folder = QString::fromUtf8(folder ? folder : "");
        this->reload_files();
    }

    QTimer* refresh_timer = new QTimer;
    connect(refresh_timer, &QTimer::timeout, this, &Application::reload_files);
    refresh_timer->start(5000);

    qApp->installEventFilter(this);

    this->is_initialized = true;

    // Let layout initialize first
    QTimer::singleShot(0, [this]() {
        this->resize_buttons();
    });
}

bool Application::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Left) {
            this->previous();
            return true;
        } else if (key_event->key() == Qt::Key_Right) {
            this->next();
            return true;
        }
    }
    return QMainWindow::eventFilter(object, event);
}

void Application::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    if (!this->pixmap.isNull()) {
        QPixmap scaled_pixmap = this->pixmap.scaled(
            this->image_scroll_area->viewport()->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        this->image_label->setPixmap(scaled_pixmap);
    }

    this->resize_buttons();
}

void Application::mouseMoveEvent(QMouseEvent* event) {
    const int fade_start = 45;
    const int fade_end = 35;

    QPoint global_mouse = event->globalPosition().toPoint();
    QPoint left_button_pos = this->left_button->mapToGlobal(QPoint(0, 0));
    QPoint right_button_pos = this->right_button->mapToGlobal(QPoint(0, 0));

    int mouse_x = global_mouse.x();
    int left_x = left_button_pos.x() + this->left_button->width() / 2;
    int right_x = right_button_pos.x() + this->right_button->width() / 2;

    auto fade_calc = [&](int button_x) -> qreal {
        int dist = std::abs(mouse_x - button_x);
        if (dist > fade_start) return 0.0;
        if (dist < fade_end) return 1.0;
        return 1.0 - (dist - fade_end) / double(fade_start - fade_end);
    };

    qreal left_opacity = fade_calc(left_x);
    qreal right_opacity = fade_calc(right_x);

    this->left_opacity_anim->stop();
    this->left_opacity_anim->setEndValue(left_opacity);
    this->left_opacity_anim->start();

    this->right_opacity_anim->stop();
    this->right_opacity_anim->setEndValue(right_opacity);
    this->right_opacity_anim->start();
}

void Application::resize_buttons() {
    if (!this->is_initialized) return;

    const int offset = 10;
    int y_center = this->image_scroll_area->y() + (this->image_scroll_area->height() / 2) - ARROW_SIZE * 1.5;

    this->left_button->move(
        this->image_label->x() + offset,
        y_center
    );
    this->right_button->move(
        this->image_label->x() + this->image_scroll_area->viewport()->width() - ARROW_SIZE - offset,
        y_center
    );
}

void Application::next() {
    if (this->files.isEmpty()) return;
    this->refresh_metadata();
    this->image_index = (this->image_index + 1) % this->files.size();
    this->filepath = files[this->image_index];
    this->show_image(this->files[this->image_index]);
}

void Application::previous() {
    if (this->files.isEmpty()) return;
    this->refresh_metadata();
    this->image_index = (this->image_index + this->files.size() - 1) % this->files.size();
    // std::cout << this->files[this->image_index].toStdString() << "\n";
    // assert(this->files[this->image_index]);
    this->filepath = files[this->image_index];
    this->show_image(this->filepath);
}

void Application::reload_files() {
    if (this->current_folder == "") return;

    QStringList new_files;
    QStringList filters = {"*.jpg", "*.jpeg", "*.heic", "*.png", "*.bmp", "*.gif"};
    QDirIterator diriterator(
        this->current_folder,
        filters,
        QDir::Files,
        QDirIterator::Subdirectories
    );

    while (diriterator.hasNext()) {
        new_files.append(diriterator.next());
    }

    if (new_files != this->files) {
        this->files = new_files;
        if (this->files.isEmpty()) {
            this->image_label->setText("No image files found.");
            this->image_index = 0;
        }
        else {
            this->image_index = qMin(this->image_index, this->files.size() - 1);
            this->filepath = this->files[this->image_index];
            this->show_image(this->filepath);
        }
    }
}

void Application::create_widgets(
    const QString& title,
    const QList<MetadataField>& values,
    const QString& icon,
    DataType type,
    std::string key,
    bool is_binary,
    int max_rows,
    double latitude,
    double longitude
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
        QString value = values[0].value;
        value.replace("\\n", "\n");
        text_edit->setText(value);
        
        right_layout->addWidget(text_edit);

        auto resize = [this, text_edit, key]() {
            int height = static_cast<int>(text_edit->document()->size().height());
            int margin = text_edit->contentsMargins().top() +
                text_edit->contentsMargins().bottom();
            text_edit->setFixedHeight(std::max(30, height + margin));
        };
        auto content_resize = [this, text_edit, key, resize]() {
            resize();
            this->metadata[key] = text_edit->toMarkdown().toStdString();
            this->refresh_metadata();
        };
        if (!key.empty()) {
            connect(
                text_edit,
                &QTextEdit::textChanged,
                this,
                content_resize
            );
        }

        // Resize text edit to fit new content after text is updated by Qt
        QTimer::singleShot(0, this, resize);
    }
    else if (type == DataType::LOCATION) {
        QGVMap* map = new QGVMap(this);
        auto osm_layer = new QGVLayerOSM();
        map->addItem(osm_layer);
        auto target = map->getProjection()->boundaryGeoRect();
        assert(latitude != -1 && longitude != -1);
        auto action = QGVCameraActions(map)
            .moveTo(QGV::GeoPos{latitude, longitude})
            .scaleBy(0.2);
        map->cameraTo(action);
        right_layout->addWidget(map);
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
    this->edit_filepath = this->filepath;

    QList<QPair<QString, QString>> metadata;
    std::map<std::string, std::string> exifdata;

    for (auto it = exif_data.begin(); it != exif_data.end(); ++it) {
        exifdata[it->key()] = it->value().toString();
        // std::cout << it->key() << ": " << it->value().toString() << "\n";
    }

    for (const auto& pair : exif_data) {
        std::string key = pair.key();
        QString value = qs(pair.toString());
    }

    // if (exifdata.contains("Exif.Image.XPTitle")) {

    std::string key = "Exif.Image.XPTitle";
    if (exifdata["Exif.Image.XPTitle"].empty()) {
        key = "Exif.Image.XPSubject";
    } else if (exifdata["Exif.Image.XPSubject"].empty()) {
        key = "Exif.Image.XPTitle";
    }
        this->create_widgets(
            "Title",
            {
                {
                    "Title",
                    qs(Utils::read_bytes(exifdata[key]))
                }
            },
            icons["title"],
            DataType::MULTISTRING,
            key,
            this->filepath.endsWith(".heic") ? false : true
        );
        // std::cout << exifdata["Exif.Image.XPTitle"] << " to " << Utils::read_bytes(exifdata["Exif.Image.XPTitle"]) << "\n";
    // }
    // if (exifdata.contains("Exif.Image.XPTitle")) {
    std::string description_key = "Exif.Image.ImageDescription";
    if (exifdata["Exif.Image.ImageDescription"].empty()) {
        description_key = "Exif.Image.XPComment";
    } else if (exifdata["Exif.Image.XPComment"].empty()) {
        description_key = "Exif.Image.ImageDescription";
    }
    this->create_widgets(
        "Description",
        {
            {
                "Description",
                qs(Utils::read_bytes(exifdata[description_key]))
            }
        },
        icons["description"],
        DataType::MULTILINE,
        description_key
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

    QImage image = pixmap.toImage();

    double x_resolution = image.dotsPerMeterX() / INCH_TO_METER;
    double y_resolution = image.dotsPerMeterY() / INCH_TO_METER;

    QFileInfo fileinfo(this->filepath);

    int dpi = std::sqrt(x_resolution * y_resolution);

    this->create_widgets(
        "Size info",
        {
            {
                "Dimensions",
                QString::number(image.width()) +
                " x " +
                QString::number(image.height())
            },
            { "File size", Utils::format_size(fileinfo.size()) },
            { "DPI", QString::number(dpi) }
        },
        icons["dimension"]
    );

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

        this->create_widgets(
            "Location",
            {
                {
                    "Location",
                    qs(Utils::read_bytes(exifdata[description_key]))
                }
            },
            icons["description"],
            DataType::LOCATION,
            description_key,
            false,
            0,
            latitude,
            longitude
        );
    }
    assert(!this->filepath.isEmpty());
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

void Application::open_directory() {
    this->current_folder = QFileDialog::getExistingDirectory(
        this,
        "Open folder",
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    this->reload_files();
}

void Application::show_image(const QString& filepath) {
    this->metadata.clear();
    this->pixmap = Image::load_image(filepath);
    if (this->pixmap.isNull()) {
        std::cerr << "Failed to load image: " << filepath.toStdString()
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

    QSize max_size(
        this->image_scroll_area->viewport()->width(),
        this->image_scroll_area->viewport()->height()
    );
    
    QPixmap scaled_pixmap = pixmap.scaled(
        max_size,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    this->image_label->setPixmap(scaled_pixmap);

    this->image = Exiv2::ImageFactory::open(filepath.toStdString());

    this->image->readMetadata();
    this->exif_data = this->image->exifData();

    // if (this->exif_data.empty()) {
        // return;// this->image_label->setText("")
    // } else {
    process_metadata(this->exif_data);
    // }
}

void Application::refresh_metadata() {
    if (this->metadata.empty()) return;

    qDebug() << "Writing metadata to " << this->edit_filepath << "\n";

    for (auto& [key, value] : this->metadata) {
        // std::cout << key << " was changed to "
        //           << value << " from " << this->exif_data[key]; 
        this->exif_data[key] = value;
        // std::cout << key << ": " << value << "\n";
    }

    this->image = Image::write_image(
        this->edit_filepath,
        this->metadata,
        std::move(this->image)
    );

    // this->metadata.clear();
}
