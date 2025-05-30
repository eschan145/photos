#pragma once

#include "pch.h"

#include "loader.h"
#include "utils.h"

const int DATAPANEL_WIDTH = 340;

struct FieldData {
    QString readable_name;
};

extern QStringList IMAGE_EXTENSIONS;

enum class DataType { STRING, DATE, MULTISTRING, MULTILINE };

struct MetadataField {
    QString name;
    QString value;
    DataType type = DataType::STRING;
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
    Application();

   protected:
    bool eventFilter(QObject* object, QEvent* event) override;

   private:
    QHBoxLayout* main_layout;
    QVBoxLayout* image_layout;
    QVBoxLayout* metadata_layout;
    QWidget* central_widget;
    QWidget* layoutw;
    QVBoxLayout* field_layout;
    QWidget* field_layoutw;
    QLabel* image_label;

    QString filepath;
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
        int max_rows = 1
    );

    QList<QPair<QString, QString>> process_metadata(
        Exiv2::ExifData& exif_data
    );

    void open_directory();
    void show_image(const QString& filepath);
    void refresh_metadata();
};

