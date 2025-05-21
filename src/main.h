#include "pch.h"

const int DATAPANEL_WIDTH = 370;

struct FieldData {
    QString readable_name;
};

enum class DataType { STRING, DATE };

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

AssetManager icons;

void clear_layout(QLayout* layout);

float parse_rational(const std::string& string);

float parse_fraction(const QString& fraction);

float to_decimal(const QStringList& list, const std::string& ref);

inline QString qs(const std::string& string);

class MainWindow : public QMainWindow {
   public:
    MainWindow();

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
    );

    QList<QPair<QString, QString>> process_metadata(
        Exiv2::ExifData& exif_data
    );

    void load_image();
};

