/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"
#include "archive.h"
#include "file.h"
#include "utils.h"
#include "elf.h"

static char archive_file[MAX_PATH_LENGTH];
static int archive_path_start = 0;
struct archive *archive_fd = NULL;
SceUID test_fd = -1;

int checkForUnsafeImports(void *buffer);
char *uncompressBuffer(const Elf32_Ehdr *ehdr, const Elf32_Phdr *phdr, const segment_info *segment,
                       const char *buffer);

void waitpid() {}
void __archive_create_child() {}
void __archive_check_child() {}

struct archive_data {
  const char *filename;
  SceUID fd;
  void *buffer;
  int block_size;
};
/*
static const char *file_passphrase(struct archive *a, void *client_data) {
  return "TODO";
}
*/
static int file_open(struct archive *a, void *client_data) {
  struct archive_data *archive_data = client_data;
  
  archive_data->fd = sceIoOpen(archive_data->filename, SCE_O_RDONLY, 0);
  test_fd = archive_data->fd;
  if (archive_data->fd < 0)
    return ARCHIVE_FATAL;
  
  archive_data->buffer = malloc(TRANSFER_SIZE);
  archive_data->block_size = TRANSFER_SIZE;
  
  return ARCHIVE_OK;
}

static ssize_t file_read(struct archive *a, void *client_data, const void **buff) {
  struct archive_data *archive_data = client_data;
  *buff = archive_data->buffer;
  return sceIoRead(archive_data->fd, archive_data->buffer, archive_data->block_size);
}

static int64_t file_skip(struct archive *a, void *client_data, int64_t request) {
  struct archive_data *archive_data = client_data;
  int64_t old_offset, new_offset;

  if ((old_offset = sceIoLseek(archive_data->fd, 0, SCE_SEEK_CUR)) >= 0 &&
      (new_offset = sceIoLseek(archive_data->fd, request, SCE_SEEK_CUR)) >= 0)
    return new_offset - old_offset;

  return -1;
}

static int64_t file_seek(struct archive *a, void *client_data, int64_t request, int whence) {
  struct archive_data *archive_data = client_data;
  int64_t r;

  r = sceIoLseek(archive_data->fd, request, whence);
  if (r >= 0)
    return r;

  return ARCHIVE_FATAL;
}

static int file_close2(struct archive *a, void *client_data) {
  struct archive_data *archive_data = client_data;

  if (archive_data->fd >= 0) {
    sceIoClose(archive_data->fd);
    archive_data->fd = -1;
  }

  free(archive_data->buffer);
  archive_data->buffer = NULL;

  return ARCHIVE_OK;
}

static int file_close(struct archive *a, void *client_data) {
  struct archive_data *archive_data = client_data;
  file_close2(a, client_data);
  free(archive_data);
  return ARCHIVE_OK;
}

static int file_switch(struct archive *a, void *client_data1, void *client_data2) {
  file_close2(a, client_data1);
  return file_open(a, client_data2);
}

struct archive *open_archive(const char *filename) {
  struct archive *a = archive_read_new();
  if (!a)
    return NULL;
  
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);
  
  // archive_read_set_passphrase_callback(a, file_passphrase);
  archive_read_set_open_callback(a, file_open);
  archive_read_set_read_callback(a, file_read);
  archive_read_set_skip_callback(a, file_skip);
  archive_read_set_close_callback(a, file_close);
  archive_read_set_switch_callback(a, file_switch);
  archive_read_set_seek_callback(a, file_seek);
  
  struct archive_data *archive_data = malloc(sizeof(struct archive_data));
  archive_data->filename = filename;
  archive_read_append_callback_data(a, archive_data);
  
  if (archive_read_open1(a))
    return NULL;
  
  return a;
}

typedef struct ArchiveFileNode {
  struct ArchiveFileNode *child;
  struct ArchiveFileNode *next;
  char *name;
  int is_folder;
  SceMode mode;
  SceOff size;
  SceDateTime ctime;
  SceDateTime mtime;
  SceDateTime atime;
} ArchiveFileNode;

static ArchiveFileNode *archive_root = NULL;

char *serializePathName(char *name, char **p) {
  if (!p)
    return name;
  
  if (*p) {
    **p = '/'; // restore
    name = *p + 1;
  }
  
  *p = strchr(name, '/');
  if (*p)
    **p = '\0';
  
  return name;
}

ArchiveFileNode *createArchiveNode(const char *name, const struct stat *stat, int is_folder) {
  ArchiveFileNode *node = malloc(sizeof(ArchiveFileNode));
  if (!node)
    return NULL;
  
  memset(node, 0, sizeof(ArchiveFileNode));

  node->name = malloc(strlen(name) + 1);
  if (!node->name) {
    free(node);
    return NULL;
  }
  
  strcpy(node->name, name);
  
  node->child = NULL;
  node->next = NULL;
  
  if (is_folder || stat->st_mode & S_IFDIR)
    node->is_folder = 1;
    
  node->mode = node->is_folder ? SCE_S_IFDIR : SCE_S_IFREG;
  
  if (stat) {
    SceDateTime time;
    
    node->size = stat->st_size;
    
    sceRtcSetTime_t(&time, stat->st_ctime);
    convertLocalTimeToUtc(&node->ctime, &time);
    
    sceRtcSetTime_t(&time, stat->st_mtime);
    convertLocalTimeToUtc(&node->mtime, &time);
    
    sceRtcSetTime_t(&time, stat->st_atime);
    convertLocalTimeToUtc(&node->atime, &time);
  }
  
  return node;
}

ArchiveFileNode *_findArchiveNode(ArchiveFileNode *parent, char *name, ArchiveFileNode **parent_out,
                                  ArchiveFileNode **prev_out, char **name_out, char **p_out) {
  char *p = NULL;
  ArchiveFileNode *prev = NULL;

  // Remove trailing slash
  removeEndSlash(name);
  
  // Serialize path name
  name = serializePathName(name, &p);
  
  // Root
  if (name[0] == '\0')
    return parent;
  
  // Traverse
  ArchiveFileNode *curr = parent->child;
  while (curr) {
    // Found name
    if (strcmp(curr->name, name) == 0) {
      // Found node
      if (!p)
        return curr;
      
      // Is folder
      if (curr->is_folder) {
        // Serialize path name
        name = serializePathName(name, &p);
        
        // Get child entry of this directory
        parent = curr;
        curr = curr->child;
        continue;
      }
    }
    
    // Get next entry in this directory
    prev = curr;
    curr = curr->next;
  }
  
  // Out
  if (parent_out)
    *parent_out = parent;
  if (prev_out)
    *prev_out = prev;
  if (name_out)
    *name_out = name;
  if (p_out)
    *p_out = p;
  
  return NULL;
}

ArchiveFileNode *findArchiveNode(const char *path) {
  char name[MAX_PATH_LENGTH];
  strcpy(name, path);
  return _findArchiveNode(archive_root, name, NULL, NULL, NULL, NULL);
}

int addArchiveNodeRecursive(ArchiveFileNode *parent, char *name, const struct stat *stat) {  
  char *p = NULL;
  ArchiveFileNode *prev = NULL;
  
  if (!parent)
    return -1;
  
  ArchiveFileNode *res = _findArchiveNode(parent, name, &parent, &prev, &name, &p);
  
  // Already exist
  // TODO: update node
  if (res)
    return 0;
  
  // Create new node
  ArchiveFileNode *node = createArchiveNode(name, stat, !!p);  
  if (!parent->child) { // First child
    parent->child = node;
  } else {              // Neighbour
    prev->next = node;
  }
  
  // Recursion
  if (p)
    addArchiveNodeRecursive(node, p + 1, stat);
  
  return 0;
}

void addArchiveNode(const char *path, const struct stat *stat) {  
  char name[MAX_PATH_LENGTH];
  strcpy(name, path);
  addArchiveNodeRecursive(archive_root, name, stat);
}

void freeArchiveNodes(ArchiveFileNode *curr) {
  // Traverse
  while (curr) {
    // Enter directory
    if (curr->is_folder) {
      // Traverse child entry of this directory
      freeArchiveNodes(curr->child);
    }
    
    // Get next entry in this directory
    ArchiveFileNode *next = curr->next;
    free(curr->name);
    free(curr);
    curr = next;
  }
}

int archiveCheckFilesForUnsafeFself() {  
  // Open archive file
  struct archive *archive = open_archive(archive_file);
  if (!archive)
    return 0;
  
  // Traverse
  while (1) {
    struct archive_entry *archive_entry;
    int res = archive_read_next_header(archive, &archive_entry);
    if (res == ARCHIVE_EOF)
      break;
    
    if (res != ARCHIVE_OK) {
      archive_read_free(archive);
      return 0;
    }
    
    // Get entry information
    const char *name = archive_entry_pathname(archive_entry);
    const struct stat *stat = archive_entry_stat(archive_entry);
        
    // Read magic
    uint32_t magic = 0;
    archive_read_data(archive, &magic, sizeof(uint32_t));
    
    // SCE magic
    if (magic == 0x00454353) {
      char sce_header[0x84];
      archive_read_data(archive, sce_header, sizeof(sce_header));

      uint64_t elf1_offset = *(uint64_t *)(sce_header + 0x3C);
      uint64_t phdr_offset = *(uint64_t *)(sce_header + 0x44);
      uint64_t section_info_offset = *(uint64_t *)(sce_header + 0x54);

      // jump to elf1
      // Until here we have read 0x88 bytes
      int i;
      for (i = 0; i < elf1_offset - 0x88; i += sizeof(uint32_t)) {
        uint32_t dummy = 0;
        archive_read_data(archive, &dummy, sizeof(uint32_t));
      }

      // Check imports
      char *buffer = malloc(stat->st_size);
      if (buffer) {
        archive_read_data(archive, buffer, stat->st_size);

        Elf32_Ehdr *elf1 = (Elf32_Ehdr*)buffer;
        Elf32_Phdr *phdr = (Elf32_Phdr*)(buffer + phdr_offset - elf1_offset);
        segment_info *info = (segment_info*)(buffer + section_info_offset - elf1_offset);
        
        // segment is elf2 section
        char *segment = buffer + info->offset - elf1_offset;

        // zlib compress magic
        char *uncompressed_buffer = NULL;
        if (segment[0] == 0x78) {
          // uncompressedBuffer will return elf2 section
          uncompressed_buffer = uncompressBuffer(elf1, phdr, info, segment);
          if (uncompressed_buffer) {
            segment = uncompressed_buffer;
          }
        }
        
        int unsafe = checkForUnsafeImports(segment);

        if (uncompressed_buffer)
          free(uncompressed_buffer);
        free(buffer);

        if (unsafe) {
          archive_read_free(archive);
          return unsafe;
        }
      }

      // Check authid flag
      uint64_t authid = *(uint64_t *)(sce_header + 0x7C);
      if (authid != 0x2F00000000000002) {
        archive_read_free(archive);
        return 1; // Unsafe
      }
    }
  }

  archive_read_free(archive);
  return 0;
}

int fileListGetArchiveEntries(FileList *list, const char *path, int sort) {
  int res;

  if (!list)
    return -1;

  FileListEntry *entry = malloc(sizeof(FileListEntry));
  if (entry) {
    strcpy(entry->name, DIR_UP);
    entry->name_length = strlen(entry->name);
    entry->is_folder = 1;
    entry->type = FILE_TYPE_UNKNOWN;
    fileListAddEntry(list, entry, sort);
  }
  
  // Traverse
  ArchiveFileNode *curr = findArchiveNode(path + archive_path_start);
  if (curr)
    curr = curr->child;
  while (curr) {
    FileListEntry *entry = malloc(sizeof(FileListEntry));
    if (entry) {
      strcpy(entry->name, curr->name);
      
      entry->is_folder = curr->is_folder;
      if (entry->is_folder) {
        addEndSlash(entry->name);
        entry->type = FILE_TYPE_UNKNOWN;
        list->folders++;
      } else {
        entry->type = getFileType(entry->name);
        list->files++;
      }

      entry->name_length = strlen(entry->name);
      entry->size = curr->size;
      
      memcpy(&entry->ctime, (SceDateTime *)&curr->ctime, sizeof(SceDateTime));
      memcpy(&entry->mtime, (SceDateTime *)&curr->mtime, sizeof(SceDateTime));
      memcpy(&entry->atime, (SceDateTime *)&curr->atime, sizeof(SceDateTime));
      
      fileListAddEntry(list, entry, sort);
    }
    
    // Get next entry in this directory
    curr = curr->next;
  }

  return 0;
}

int getArchivePathInfo(const char *path, uint64_t *size, uint32_t *folders, uint32_t *files) {
  SceIoStat stat;
  memset(&stat, 0, sizeof(SceIoStat));

  int res = archiveFileGetstat(path, &stat);
  if (res < 0)
    return res;
  
  if (SCE_S_ISDIR(stat.st_mode)) {
    FileList list;
    memset(&list, 0, sizeof(FileList));
    fileListGetArchiveEntries(&list, path, SORT_NONE);

    FileListEntry *entry = list.head->next; // Ignore ..

    int i;
    for (i = 0; i < list.length - 1; i++) {
      char *new_path = malloc(strlen(path) + strlen(entry->name) + 2);
      snprintf(new_path, MAX_PATH_LENGTH - 1, "%s%s", path, entry->name);

      getArchivePathInfo(new_path, size, folders, files);

      free(new_path);

      entry = entry->next;
    }

    if (folders)
      (*folders)++;

    fileListEmpty(&list);
  } else {
    if (size)
      (*size) += stat.st_size;

    if (files)
      (*files)++;
  }

  return 0;
}

int extractArchivePath(const char *src, const char *dst, FileProcessParam *param) {
  SceIoStat stat;
  memset(&stat, 0, sizeof(SceIoStat));

  int res = archiveFileGetstat(src, &stat);
  if (res < 0)
    return res;
  
  if (SCE_S_ISDIR(stat.st_mode)) {
    FileList list;
    memset(&list, 0, sizeof(FileList));
    fileListGetArchiveEntries(&list, src, SORT_NONE);

    int ret = sceIoMkdir(dst, 0777);
    if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
      fileListEmpty(&list);
      return ret;
    }

    if (param) {
      if (param->value)
        (*param->value) += DIRECTORY_SIZE;

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);
      
      if (param->cancelHandler && param->cancelHandler()) {
        fileListEmpty(&list);
        return 0;
      }
    }

    FileListEntry *entry = list.head->next; // Ignore ..

    int i;
    for (i = 0; i < list.length - 1; i++) {
      char *src_path = malloc(strlen(src) + strlen(entry->name) + 2);
      snprintf(src_path, MAX_PATH_LENGTH - 1, "%s%s", src, entry->name);

      char *dst_path = malloc(strlen(dst) + strlen(entry->name) + 2);
      snprintf(dst_path, MAX_PATH_LENGTH - 1, "%s%s", dst, entry->name);

      int ret = extractArchivePath(src_path, dst_path, param);

      free(dst_path);
      free(src_path);

      if (ret <= 0) {
        fileListEmpty(&list);
        return ret;
      }

      entry = entry->next;
    }

    fileListEmpty(&list);
  } else {
    SceUID fdsrc = archiveFileOpen(src, SCE_O_RDONLY, 0);
    if (fdsrc < 0)
      return fdsrc;

    SceUID fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fddst < 0) {
      archiveFileClose(fdsrc);
      return fddst;
    }

    void *buf = malloc(TRANSFER_SIZE);

    uint64_t seek = 0;

    while (1) {
      int read = archiveFileRead(fdsrc, buf, TRANSFER_SIZE);
      if (read < 0) {
        free(buf);

        sceIoClose(fddst);
        archiveFileClose(fdsrc);

        return read;
      }

      if (read == 0)
        break;

      int written = sceIoWrite(fddst, buf, read);

      if (written < 0) {
        free(buf);

        sceIoClose(fddst);
        archiveFileClose(fdsrc);

        return written;
      }

      seek += written;

      if (param) {
        if (param->value)
          (*param->value) += read;

        if (param->SetProgress)
          param->SetProgress(param->value ? *param->value : 0, param->max);

        if (param->cancelHandler && param->cancelHandler()) {
          free(buf);

          sceIoClose(fddst);
          archiveFileClose(fdsrc);

          return 0;
        }
      }
    }

    free(buf);

    sceIoClose(fddst);
    archiveFileClose(fdsrc);
  }

  return 1;
}

int archiveFileGetstat(const char *file, SceIoStat *stat) {
  ArchiveFileNode *node = findArchiveNode(file + archive_path_start);
  if (!node)
    return -1;
  
  if (stat) {
    stat->st_mode = node->mode;
    stat->st_size = node->size;
    memcpy(&stat->st_ctime, &node->ctime, sizeof(SceDateTime));
    memcpy(&stat->st_mtime, &node->mtime, sizeof(SceDateTime));
    memcpy(&stat->st_atime, &node->atime, sizeof(SceDateTime));
  }
  
  return 0;
}

int archiveFileOpen(const char *file, int flags, SceMode mode) {  
  // A file is already open
  if (archive_fd)
    return -1;

  // Open archive file
  archive_fd = open_archive(archive_file);
  if (!archive_fd)
    return -1;

  // Traverse
  while (1) {
    struct archive_entry *archive_entry;
    int res = archive_read_next_header(archive_fd, &archive_entry);
    if (res == ARCHIVE_EOF)
      break;
    
    if (res != ARCHIVE_OK) {
      archive_read_free(archive_fd);
      return -1;
    }
    
    // Compare pathname
    const char *name = archive_entry_pathname(archive_entry);
    if (strcmp(name, file + archive_path_start) == 0) {
      return ARCHIVE_FD;
    }
  }

  archive_read_free(archive_fd);
  archive_fd = NULL;

  return -1;
}

int archiveFileRead(SceUID fd, void *data, SceSize size) {
  if (!archive_fd || fd != ARCHIVE_FD)
    return -1;

  return archive_read_data(archive_fd, data, size);
}

int archiveFileClose(SceUID fd) {
  if (!archive_fd || fd != ARCHIVE_FD)
    return -1;

  archive_read_free(archive_fd);
  archive_fd = NULL;

  return 0;
}

int ReadArchiveFile(const char *file, void *buf, int size) {
  SceUID fd = archiveFileOpen(file, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  int read = archiveFileRead(fd, buf, size);
  archiveFileClose(fd);
  return read;
}

int archiveClose() {
  freeArchiveNodes(archive_root);
  return 0;
}

int archiveOpen(const char *file) {
  // Start position of the archive path
  archive_path_start = strlen(file) + 1;
  strcpy(archive_file, file);

  // Open archive file
  struct archive *archive = open_archive(file);
  if (!archive)
    return -1;

  // Create archive root
  archive_root = createArchiveNode("/", NULL, 1);
  
  // Traverse
  while (1) {
    struct archive_entry *archive_entry;
    int res = archive_read_next_header(archive, &archive_entry);
    if (res == ARCHIVE_EOF)
      break;
    
    if (res != ARCHIVE_OK) {
      archive_read_free(archive);
      return -1;
    }
    
    // Get entry information
    const char *name = archive_entry_pathname(archive_entry);
    const struct stat *stat = archive_entry_stat(archive_entry);
    
    // Add node
    addArchiveNode(name, stat);
  }

  archive_read_free(archive);
  return 0;
}