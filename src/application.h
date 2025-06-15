#pragma once

#include "pch.h"

#include "loader.h"
#include "utils.h"

const int DATAPANEL_WIDTH = 340;
const int ARROW_SIZE = 40;

struct FieldData {
    QString readable_name;
};

extern QStringList IMAGE_EXTENSIONS;

enum class DataType { STRING, DATE, MULTISTRING, MULTILINE, LOCATION };

struct MetadataField {
    QString name;
    QString value;
    DataType type = DataType::STRING;
};

struct EditingContext {
    QString filepath;
    Exiv2::ExifData exif_data;
    std::unique_ptr<Exiv2::Image> image;
};

class AssetManager {
 public:
    AssetManager() = default;

    QString operator[](const QString& key);
};

extern AssetManager icons;

void clear_layout(QLayout* layout);

inline QString qs(const std::string& string);

class Application : public QMainWindow {
   public:
    Application(const char* folder = nullptr);

   protected:
    bool eventFilter(QObject* object, QEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    void resize_buttons();

   private:
    bool is_initialized = false;
    QHBoxLayout* main_layout;
    QVBoxLayout* image_layout;
    QVBoxLayout* metadata_layout;
    QWidget* central_widget;
    QWidget* layoutw;
    QVBoxLayout* field_layout;
    QWidget* field_layoutw;
    QLabel* image_label;
    QScrollArea* image_scroll_area;

    QPushButton* left_button;
    QPushButton* right_button;
    QPropertyAnimation* left_opacity_anim;
    QPropertyAnimation* right_opacity_anim;

    QString filepath;
    QString edit_filepath;
    QPixmap pixmap;
    std::unique_ptr<Exiv2::Image> image;
    Exiv2::ExifData exif_data;

    std::map<std::string, std::string> metadata;

    // QFileSystemWatcher* watcher;
    QStringList files;
    QString current_folder;
    int image_index = 0;

    void next();
    void previous();
    void reload_files();

    void create_widgets(
        const QString& title,
        const QList<MetadataField>& values,
        const QString& icon = "",
        DataType type = DataType::STRING,
        std::string key = "",
        bool binary = false,
        int max_rows = 1,
        double latitude = -1,
        double longitude = -1
    );

    QList<QPair<QString, QString>> process_metadata(
        Exiv2::ExifData& exif_data
    );

    void open_directory();
    void show_image(const QString& filepath);
    void refresh_metadata();
};

