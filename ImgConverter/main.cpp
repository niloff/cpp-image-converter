#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>
#include <bmp_image.h>
 
#include <filesystem>
#include <string_view>
#include <iostream>
 
using namespace std;
 
enum class Format {JPEG, PPM, BMP, UNKNOWN};
 
Format GetFormatByExtension(const img_lib::Path& input_file) {
    
    const string ext = input_file.extension().string();
    
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }
 
    if (ext == ".ppm"sv) {
        return Format::PPM;
    }

    if (ext == ".bmp"sv) {
        return Format::BMP;
    }
 
    return Format::UNKNOWN;
} 
 
class ImageFormatInterface {
public:
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};
 
class JPEG : public ImageFormatInterface {
public:
    
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }
 
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};
 
class PPM : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }
 
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadPPM(file);
    }
};

class BMP : public ImageFormatInterface {
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveBMP(file, image);
    }
 
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadBMP(file);
    }
};
 
const ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
 
    static const JPEG jpeg;
    static const PPM ppm;
    static const BMP bmp;
 
    switch (GetFormatByExtension(path)) {
            
        case Format::UNKNOWN:
            return nullptr;
 
        case Format::JPEG:
            return &jpeg;
            
        case Format::PPM:
            return &ppm;

        case Format::BMP:
            return &bmp;
            
        default:
            return nullptr;
    }
}
 
int main(int argc, const char** argv) {
    
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }
 
    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];
    
    const ImageFormatInterface* input_format = GetFormatInterface(in_path);
    
    if (!input_format) {
        cerr << "Unknown format of the input file"sv << endl;
        return 2;
    }
 
    const ImageFormatInterface* output_format = GetFormatInterface(out_path);
    
    if (!output_format) {
        cerr << "Unknown format of the output file"sv << endl;
        return 3;
    }
 
    img_lib::Image image = input_format->LoadImage(in_path);
    
    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }
 
    if (!output_format->SaveImage(out_path, image)) {
        cerr << "Saving failed"sv << endl;
        return 5;
    }
 
    cout << "Successfully converted"sv << endl;
}