#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {
/**
 * Количество бит в пикселе
 */
static constexpr uint8_t BITS_PER_PIXEL = 24;
/**
 * Округление до следующей 32-разрядной границы
 */
static constexpr uint8_t ROUNDING = 31;
/**
 * Количество DWORD(double words) в строке
 */
static constexpr uint8_t DWORDS_PER_ROW = 32;
/**
 * Количество байт в строке
 */
static constexpr uint8_t BYTES_PER_ROW = 4;
/**
 * Подпись заголовка
 */
static constexpr char HEADER_SIGNATURE[2] = {'B', 'M'};
/**
 * Размер BitmapFileHeader в байтах
 */
static constexpr uint32_t FILE_HEADER_SIZE = 14;
/**
 * Размер BitmapInfoHeader в байтах
 */
static constexpr uint32_t INFO_HEADER_SIZE = 40;
/**
 * Функция вычисления отступа по ширине
 */
static int GetBMPStride(int width) {
    return BYTES_PER_ROW * ((width * BITS_PER_PIXEL + ROUNDING) / DWORDS_PER_ROW);
}
/**
 * Заголовок растрового файла Bitmap
 */
PACKED_STRUCT_BEGIN BitmapFileHeader {
    /**
     * Конструктор по-умолчанию
     */
    BitmapFileHeader() = default;
    /**
     * Конструктор
     */
    BitmapFileHeader(int width, int height) {
        size_ = FILE_HEADER_SIZE + INFO_HEADER_SIZE + GetBMPStride(width) * height;
    }
    /**
     * Подпись. Символы BM.
     */
    char header_sign_[2] = {'B', 'M'};
    /**
     * Суммарный размер заголовка и данных.
     * Размер данных определяется как отступ, умноженный на высоту изображения.
     */
    uint32_t size_ = 0;
    /**
     * Зарезервированное пространство.
     */
    uint32_t reserved_space_ = 0x0000;
    /**
     * Отступ данных от начала файла.
     */
    uint32_t offset_ = FILE_HEADER_SIZE + INFO_HEADER_SIZE;
}
PACKED_STRUCT_END
/**
 * Информационный заголовок растрового изображения Bitmap
 */
PACKED_STRUCT_BEGIN BitmapInfoHeader {
    /**
     * Конструктор по-умолчанию
     */
    BitmapInfoHeader() = default;
    /**
     * Конструктор
     */
    BitmapInfoHeader(int width, int height) :
        width_(width),
        height_(height),
        bytes_at_data_(GetBMPStride(width) * height) {}
    /**
     * Размер заголовка.
     * Учитывается только размер второй части заголовка.
     */
    uint32_t size_ = INFO_HEADER_SIZE;
    /**
     * Ширина изображения в пикселях
     */
    int32_t width_;
    /**
     * Высота изображения в пикселях
     */
    int32_t height_;
    /**
     * Количество плоскостей.
     */
    uint16_t layers_ = 1;
    /**
     * Количество бит на пиксель
     */
    uint16_t bit_per_pixel_ = BITS_PER_PIXEL;
    /**
     * Тип сжатия
     */
    uint32_t compression_type_ = 0;
    /**
     * Количество байт в данных.
     * Произведение отступа на высоту.
     */
    uint32_t bytes_at_data_ = 0;
    /**
     * Горизонтальное разрешение, пикселей на метр
     */
    int32_t x_pix_per_met_ = 11811;
    /**
     * Вертикальное разрешение, пикселей на метр
     */
    int32_t y_pix_per_met_ = 11811;
    /**
     * Количество использованных цветов
     */
    int32_t colors_in_use_ = 0;
    /**
     * Количество значимых цветов
     */
    int32_t colors_ = 0x1000000;
}
PACKED_STRUCT_END

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out;
    out.open(file, std::ios::out | std::ios::binary);
    if (out.fail()) {
        return false;
    }
    BitmapFileHeader file_header { image.GetWidth(), image.GetHeight() };
    BitmapInfoHeader info_header { image.GetWidth(), image.GetHeight() };
    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    const int bmp_stride = GetBMPStride(image.GetWidth());
    vector<char> buff(bmp_stride);

    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < image.GetWidth(); ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(reinterpret_cast<const char*>(buff.data()), buff.size());
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream input;
    // пробуем открыть файл
    input.open(file, std::ios::in | std::ios::binary);
    if (input.fail()) {
        return {};
    }
    // проверяем размер
    input.seekg(0, std::ios::end);
    uint64_t file_size = input.tellg();
    if (file_size == 0 || file_size < sizeof (BitmapFileHeader) + sizeof(BitmapInfoHeader)) {
        return {};
    }
    input.seekg(std::ios::beg);
    // читаем заголовок
    BitmapFileHeader file_header;
    input.read(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader));
    // проверяем заголовок на корректность
    if (file_header.header_sign_[0] != HEADER_SIGNATURE[0] ||
            file_header.header_sign_[1] != HEADER_SIGNATURE[1]) {
        return {};
    }
    // читаем инфо
    BitmapInfoHeader info_header;
    input.read(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader));
    // инициализируем изображение
    Image image(info_header.width_, info_header.height_, Color::Black());
    // вычисляем отступ
    const int bmp_stride = GetBMPStride(info_header.width_);
    std::vector<char> buff(bmp_stride);
    // загружаем цвета
    for (int i = image.GetHeight() - 1; i >= 0; --i) {
        input.read(buff.data(), bmp_stride);
        Color* line = image.GetLine(i);
        for (int j = 0; j < image.GetWidth(); ++j) {
            line[j].b = static_cast<byte>(buff[j * 3 + 0]);
            line[j].g = static_cast<byte>(buff[j * 3 + 1]);
            line[j].r = static_cast<byte>(buff[j * 3 + 2]);
        }
    }
    return image;
}
}  // namespace img_lib
