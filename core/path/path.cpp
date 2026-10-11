#include "path.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
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
// ============================================================
// ARCHIVOS
// ============================================================

bool hbpl_path_file_create(const char* path) {
    if (!path)
        return false;

    std::ofstream file(path, std::ios::binary | std::ios::app);
    return static_cast<bool>(file);
}

char* hbpl_path_file_read(const char* path) {
    if (!path)
        return nullptr;

    std::ifstream file(path, std::ios::binary);

    if (!file)
        return nullptr;

    std::ostringstream content;
    content << file.rdbuf();

    return hbpl_make_string(content.str());
}

bool hbpl_path_file_write(const char* path, const char* content) {
    if (!path)
        return false;

    std::ofstream file(
        path,
        std::ios::binary | std::ios::trunc
    );

    if (!file)
        return false;

    file << (content ? content : "");

    return static_cast<bool>(file);
}

bool hbpl_path_file_append(const char* path, const char* content) {
    if (!path)
        return false;

    std::ofstream file(
        path,
        std::ios::binary | std::ios::app
    );

    if (!file)
        return false;

    file << (content ? content : "");

    return static_cast<bool>(file);
}

bool hbpl_path_file_delete(const char* path) {
    if (!path)
        return false;

    std::error_code ec;

    if (!fs::is_regular_file(path, ec) || ec)
        return false;

    return fs::remove(path, ec) && !ec;
}

bool hbpl_path_file_copy(const char* from, const char* to) {
    if (!from || !to)
        return false;

    std::error_code ec;

    fs::copy_file(
        from,
        to,
        fs::copy_options::overwrite_existing,
        ec
    );

    return !ec;
}

long long hbpl_path_file_size(const char* path) {
    if (!path)
        return -1;

    std::error_code ec;

    auto size = fs::file_size(path, ec);

    if (ec)
        return -1;

    return static_cast<long long>(size);
}

// ============================================================
// CARPETAS
// ============================================================

bool hbpl_path_dir_create(const char* path) {
    if (!path)
        return false;

    std::error_code ec;

    fs::create_directories(path, ec);

    return !ec && fs::is_directory(path, ec);
}

char* hbpl_path_dir_list(const char* path) {
    if (!path)
        return nullptr;

    std::error_code ec;

    if (!fs::is_directory(path, ec) || ec)
        return nullptr;

    std::string result;

    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (!result.empty())
            result += '\n';

        result += entry.path().filename().string();
    }

    if (ec)
        return nullptr;

    return hbpl_make_string(result);
}

bool hbpl_path_dir_delete(const char* path) {
    if (!path)
        return false;

    std::error_code ec;

    if (!fs::is_directory(path, ec) || ec)
        return false;

    // remove() falla si la carpeta no está vacía
    return fs::remove(path, ec) && !ec;
}

bool hbpl_path_dir_delete_all(const char* path) {
    if (!path)
        return false;

    std::error_code ec;

    if (!fs::is_directory(path, ec) || ec)
        return false;

    fs::remove_all(path, ec);

    return !ec;
}

bool hbpl_path_dir_copy(const char* from, const char* to) {
    if (!from || !to)
        return false;

    std::error_code ec;

    fs::copy(
        from,
        to,
        fs::copy_options::recursive |
        fs::copy_options::overwrite_existing,
        ec
    );

    return !ec;
}

// ============================================================
// COMUNES
// ============================================================

bool hbpl_path_rename(const char* from, const char* to) {
    if (!from || !to)
        return false;

    std::error_code ec;

    fs::rename(from, to, ec);

    return !ec;
}

}
