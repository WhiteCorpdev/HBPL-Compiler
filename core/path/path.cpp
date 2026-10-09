#include "path.hpp"

#include <filesystem>
#include <string>
#include <cstring>
#include <cstdlib>

namespace fs = std::filesystem;

static char* hbpl_make_string(const std::string& value) {
    char* result =
        static_cast<char*>(std::malloc(value.size() + 1));

    if (!result)
        return nullptr;

    std::memcpy(
        result,
        value.c_str(),
        value.size() + 1
    );

    return result;
}

extern "C" {

char* hbpl_path_current_dir() {
    return hbpl_make_string(
        fs::current_path().string()
    );
}

char* hbpl_path_join(
    const char* a,
    const char* b
) {
    if (!a || !b)
        return nullptr;

    fs::path result = fs::path(a) / fs::path(b);

    return hbpl_make_string(
        result.lexically_normal().string()
    );
}

bool hbpl_path_exists(const char* path) {
    if (!path)
        return false;

    return fs::exists(path);
}

bool hbpl_path_is_file(const char* path) {
    if (!path)
        return false;

    return fs::is_regular_file(path);
}

bool hbpl_path_is_dir(const char* path) {
    if (!path)
        return false;

    return fs::is_directory(path);
}

char* hbpl_path_filename(const char* path) {
    if (!path)
        return nullptr;

    return hbpl_make_string(
        fs::path(path).filename().string()
    );
}

char* hbpl_path_extension(const char* path) {
    if (!path)
        return nullptr;

    return hbpl_make_string(
        fs::path(path).extension().string()
    );
}

char* hbpl_path_parent(const char* path) {
    if (!path)
        return nullptr;

    return hbpl_make_string(
        fs::path(path).parent_path().string()
    );
}

}
