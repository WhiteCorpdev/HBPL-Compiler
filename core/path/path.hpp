
#pragma once

extern "C" {

// ---------- Ya existentes ----------
char* hbpl_path_current_dir();
char* hbpl_path_join(const char* a, const char* b);
bool  hbpl_path_exists(const char* path);
bool  hbpl_path_is_file(const char* path);
bool  hbpl_path_is_dir(const char* path);
char* hbpl_path_filename(const char* path);
char* hbpl_path_extension(const char* path);
char* hbpl_path_parent(const char* path);

// ---------- Archivos (CRUD) ----------
bool  hbpl_path_file_create(const char* path);                         // Create (vacío)
char* hbpl_path_file_read(const char* path);                           // Read
bool  hbpl_path_file_write(const char* path, const char* content);     // Update (sobrescribe)
bool  hbpl_path_file_append(const char* path, const char* content);    // Update (añade)
bool  hbpl_path_file_delete(const char* path);                         // Delete
bool  hbpl_path_file_copy(const char* from, const char* to);
long long hbpl_path_file_size(const char* path);                       // -1 si falla

// ---------- Carpetas (CRUD) ----------
bool  hbpl_path_dir_create(const char* path);                          // Create (recursivo)
char* hbpl_path_dir_list(const char* path);                            // Read (nombres separados por '\n')
bool  hbpl_path_dir_delete(const char* path);                          // Delete (solo si está vacía)
bool  hbpl_path_dir_delete_all(const char* path);                      // Delete (recursivo)
bool  hbpl_path_dir_copy(const char* from, const char* to);

// ---------- Comunes ----------
bool  hbpl_path_rename(const char* from, const char* to);              // renombrar / mover

}
