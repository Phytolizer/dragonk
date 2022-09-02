#include "dragon/core/dir.h"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

void del_dir(str path)
{
	DIR* dir = opendir(path.ptr);
	if (dir == NULL) {
		return;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		str name = str_ref(entry->d_name);
		if (entry->d_type == DT_DIR) {
			if (str_eq(name, str_lit(".")) || str_eq(name, str_lit(".."))) {
				continue;
			}
			del_dir(path_join(path, name));
		} else {
			str child = path_join(path, name);
			if (unlink(child.ptr) != 0) {
				printf("%m\n");
				abort();
			}
			str_free(child);
		}
	}

	closedir(dir);
	rmdir(path.ptr);
	str_free(path);
}
