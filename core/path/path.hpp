#pragma once

extern "C" {

char* hbpl_path_current_dir();

char* hbpl_path_join(
    const char* a,
    const char* b
);

bool hbpl_path_exists(
    const char* path
);

bool hbpl_path_is_file(
    const char* path
);

bool hbpl_path_is_dir(
    const char* path
);

char* hbpl_path_filename(
    const char* path
);

char* hbpl_path_extension(
    const char* path
);

char* hbpl_path_parent(
    const char* path
);

}
