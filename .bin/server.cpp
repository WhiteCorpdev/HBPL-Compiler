#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdlib>
#include <cmath>
#include <limits>

extern "C" {
char* hbpl_path_current_dir();
char* hbpl_path_join(const char* a, const char* b);
bool hbpl_path_exists(const char* path);
bool hbpl_path_is_file(const char* path);
bool hbpl_path_is_dir(const char* path);
char* hbpl_path_filename(const char* path);
char* hbpl_path_extension(const char* path);
char* hbpl_path_parent(const char* path);
}

namespace hbpl {
template<class T> void log(const T& x){ std::cout << x << '\n'; }
inline void log(const std::string& x){ std::cout << x << '\n'; }
template<class T> void write(const T& x){ std::cout << x; std::cout.flush(); }
inline std::string readLine(){ std::string x; std::getline(std::cin,x); return x; }
inline void clear(){
#ifdef _WIN32
std::system("cls");
#else
std::system("clear");
#endif
}
inline char readKey(){ char c=0; std::cin.get(c); return c; }
inline std::string toStr(const std::string& x){ return x; }
inline std::string toStr(const char* x){ return std::string(x); }
template<class T> std::string toStr(const T& x){ return std::to_string(x); }
inline std::string takeStr(char* s){ std::string r = s ? s : ""; std::free(s); return r; }
inline std::string path_currentDir(){ return takeStr(hbpl_path_current_dir()); }
inline std::string path_join(const std::string& a, const std::string& b){
    size_t k = 0;
    while (k < b.size() && (b[k] == '/' || b[k] == '\\')) ++k;
    return takeStr(hbpl_path_join(a.c_str(), b.substr(k).c_str()));
}
inline bool path_exists(const std::string& a){ return hbpl_path_exists(a.c_str()); }
inline bool path_isFile(const std::string& a){ return hbpl_path_is_file(a.c_str()); }
inline bool path_isDir(const std::string& a){ return hbpl_path_is_dir(a.c_str()); }
inline std::string path_filename(const std::string& a){ return takeStr(hbpl_path_filename(a.c_str())); }
inline std::string path_extension(const std::string& a){ return takeStr(hbpl_path_extension(a.c_str())); }
inline std::string path_parent(const std::string& a){ return takeStr(hbpl_path_parent(a.c_str())); }
}

extern "C" {
char* hbpl_path_current_dir();
char* hbpl_path_join(const char*, const char*);
bool hbpl_path_exists(const char*);
bool hbpl_path_is_file(const char*);
bool hbpl_path_is_dir(const char*);
char* hbpl_path_filename(const char*);
char* hbpl_path_extension(const char*);
char* hbpl_path_parent(const char*);
bool hbpl_path_file_create(const char*);
char* hbpl_path_file_read(const char*);
bool hbpl_path_file_write(const char*, const char*);
bool hbpl_path_file_append(const char*, const char*);
bool hbpl_path_file_delete(const char*);
bool hbpl_path_file_copy(const char*, const char*);
long long hbpl_path_file_size(const char*);
bool hbpl_path_dir_create(const char*);
char* hbpl_path_dir_list(const char*);
bool hbpl_path_dir_delete(const char*);
bool hbpl_path_dir_delete_all(const char*);
bool hbpl_path_dir_copy(const char*, const char*);
bool hbpl_path_rename(const char*, const char*);
}

namespace hbpl_path {
inline std::string take(char* p){ if(!p) return std::string(); std::string s(p); std::free(p); return s; }
inline std::string currentDir() { return take(hbpl_path_current_dir()); }
inline std::string join(const std::string& a0, const std::string& a1) { return take(hbpl_path_join(a0.c_str(), a1.c_str())); }
inline bool exists(const std::string& a0) { return hbpl_path_exists(a0.c_str()); }
inline bool isFile(const std::string& a0) { return hbpl_path_is_file(a0.c_str()); }
inline bool isDir(const std::string& a0) { return hbpl_path_is_dir(a0.c_str()); }
inline std::string filename(const std::string& a0) { return take(hbpl_path_filename(a0.c_str())); }
inline std::string extension(const std::string& a0) { return take(hbpl_path_extension(a0.c_str())); }
inline std::string parent(const std::string& a0) { return take(hbpl_path_parent(a0.c_str())); }
inline bool fileCreate(const std::string& a0) { return hbpl_path_file_create(a0.c_str()); }
inline std::string fileRead(const std::string& a0) { return take(hbpl_path_file_read(a0.c_str())); }
inline bool fileWrite(const std::string& a0, const std::string& a1) { return hbpl_path_file_write(a0.c_str(), a1.c_str()); }
inline bool fileAppend(const std::string& a0, const std::string& a1) { return hbpl_path_file_append(a0.c_str(), a1.c_str()); }
inline bool fileDelete(const std::string& a0) { return hbpl_path_file_delete(a0.c_str()); }
inline bool fileCopy(const std::string& a0, const std::string& a1) { return hbpl_path_file_copy(a0.c_str(), a1.c_str()); }
inline long long fileSize(const std::string& a0) { return hbpl_path_file_size(a0.c_str()); }
inline bool dirCreate(const std::string& a0) { return hbpl_path_dir_create(a0.c_str()); }
inline std::string dirList(const std::string& a0) { return take(hbpl_path_dir_list(a0.c_str())); }
inline bool dirDelete(const std::string& a0) { return hbpl_path_dir_delete(a0.c_str()); }
inline bool dirDeleteAll(const std::string& a0) { return hbpl_path_dir_delete_all(a0.c_str()); }
inline bool dirCopy(const std::string& a0, const std::string& a1) { return hbpl_path_dir_copy(a0.c_str(), a1.c_str()); }
inline bool rename(const std::string& a0, const std::string& a1) { return hbpl_path_rename(a0.c_str(), a1.c_str()); }
}

void home(void* req, void* res);
void a(void* req, void* res);
void enviar(void* req, void* res);
int hbpl_main();
inline void finish() {}

void* const app = http->init();
const int port = 5000;

void home(void* req, void* res) {
    res->sendfile(hbpl::path_join(hbpl::path_currentDir(), std::string("html/index.html")));
}

void a(void* req, void* res) {
    res->sendfile(hbpl::path_join(hbpl::path_currentDir(), std::string("html/a.html")));
}

void enviar(void* req, void* res) {
    hbpl::log((std::string("Registro recibido: ") + req->body));
    res->type(std::string("text/plain; charset=utf-8"));
    res->send((std::string("Registro recibido: ") + req->body));
}

int hbpl_main() {
    hbpl_http_get(app, std::string("/").c_str(), home);
    hbpl_http_get(app, std::string("/home").c_str(), a);
    hbpl_http_post(app, std::string("/enviar").c_str(), enviar);
    app->listen(port);
    app->start();
    return 0;
}

int main() { int rc = hbpl_main(); finish(); return rc; }
