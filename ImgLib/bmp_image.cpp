#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

    PACKED_STRUCT_BEGIN BitmapFileHeader{
        // поля заголовка Bitmap File Header
        char sign1 = 'B';
        char sign2 = 'M';

        uint32_t header_size = 0;

        uint32_t reserved_space = 0;
        uint32_t indention = 54;
    }PACKED_STRUCT_END

    PACKED_STRUCT_BEGIN BitmapInfoHeader{
        // поля заголовка Bitmap Info Header
        uint32_t header_size = 40;

        uint32_t width = 0;
        uint32_t height = 0;

        uint16_t layers = 1;
        uint16_t bit_per_pixel = 24;

        uint32_t compression_type = 0;
        uint32_t bytes_at_data = 0;

        int32_t vertical_pixel_per_meter = 11811;
        int32_t horizontal_pixel_per_meter = 11811;

        int32_t colors_in_use = 0;
        int32_t colors = 0x1000000;
    }PACKED_STRUCT_END

    // функция вычисления отступа по ширине
    static int GetBMPStride(int w) {
        return 4 * ((w * 3 + 3) / 4);
    }

    // напишите эту функцию
    bool SaveBMP(const Path& file, const Image& image) {
        ofstream out(file, ios::binary);

        int width = image.GetWidth();
        int height = image.GetHeight();

        const int bmp_stride = GetBMPStride(width);

        BitmapFileHeader file_header;
        BitmapInfoHeader info_header;

        file_header.header_size = bmp_stride * height + file_header.indention;

        info_header.width = width;
        info_header.height = height;
        info_header.bytes_at_data = height * bmp_stride;

        out.write(reinterpret_cast<const char*>(&file_header), sizeof(BitmapFileHeader));
        out.write(reinterpret_cast<const char*>(&info_header), sizeof(BitmapInfoHeader));

        std::vector<char> buff(bmp_stride);

        for (int i = height - 1; i >= 0; --i)
        {
            const auto* line = image.GetLine(i);

            for (int j = 0; j < width; ++j) {

                buff[3 * j + 0] = static_cast<char>(line[j].b);
                buff[3 * j + 1] = static_cast<char>(line[j].g);
                buff[3 * j + 2] = static_cast<char>(line[j].r);
            }

            out.write(buff.data(), bmp_stride);
        }

        return out.good();
    }

    // напишите эту функцию
    Image LoadBMP(const Path& file) {
        ifstream input(file, ios::binary);

        BitmapFileHeader file_header;
        BitmapInfoHeader info_header;

        input.read(reinterpret_cast<char*>(&file_header), sizeof(BitmapFileHeader));
        input.read(reinterpret_cast<char*>(&info_header), sizeof(BitmapInfoHeader));

        int width = info_header.width;
        int height = info_header.height;
        auto color = Color::Black();

        const int bmp_stride = GetBMPStride(width);

        std::vector<char> buff(bmp_stride);
        Image result(width, height, color);

        for (int i = height - 1; i >= 0; --i)
        {
            input.read(buff.data(), bmp_stride);
            Color* line = result.GetLine(i);

            for (int j = 0; j < width; ++j) {
                line[j].b = static_cast<byte>(buff[j * 3 + 0]);
                line[j].g = static_cast<byte>(buff[j * 3 + 1]);
                line[j].r = static_cast<byte>(buff[j * 3 + 2]);
            }
        }

        return result;
    }
}  // namespace img_lib