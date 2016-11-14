#ifndef _TAG_FOLDER_H_
#define _TAG_FOLDER_H_
#include <sqlite3.h>

enum TagType
{
    //Fix enum number for sqlite which manage only integers.
    TagTypeInclude = 0,
    TagTypeExclude = 1
};
typedef enum TagType TagType;

struct Tag;
typedef struct Tag Tag;

struct File;
typedef struct File File;


struct TagFolder
{
    char folder[150];
    sqlite3 *db;
    Tag *current_includes;
    Tag *current_excludes;
};
typedef struct TagFolder TagFolder;

/* TagFolder.c */
void Tag_init(Tag *self, const char *name, int id, TagType type);
Tag *Tag_new(const char *name, int id, TagType type);
Tag *Tag_set_next(Tag *self, Tag *next);
Tag *Tag_get_next(Tag *self);
int Tag_get_id(Tag *self);
const char *Tag_get_name(Tag *self);
struct timespec *File_get_last_modification(File *self);
unsigned long File_get_size(File *self);
void Tag_set_type(Tag *self, TagType type);
TagType Tag_get_type(Tag *self);
void Tag_finalize(Tag *self);
void Tag_free(Tag *self);
void File_init(File *self, const char *name, const char *path, int id);
File *File_new(const char *name, const char *path, int id);
File *File_set_next(File *self, File *next);
File *File_get_next(File *self);
const char *File_get_name(File *self);
int File_get_id(File *self);
void File_finalize(File *self);
void File_free(File *self);
void TagFolder_init(TagFolder *self);
TagFolder *TagFolder_new(void);
void TagFolder_finalize(TagFolder *self);
void TagFolder_free(TagFolder *self);
int TagFolder_setup_folder(TagFolder *self, char *name);
char *TagFolder_get_folder(TagFolder *self);
int TagFolder_check_db_structure(TagFolder *self);
Tag *TagFolder_list_tags(TagFolder *self);
int TagFolder_create_tag(TagFolder *self, const char *name, TagType type);
int TagFolder_create_file_in_db(TagFolder *self, const char *name, char *db_name);
int TagFolder_tag_a_tag(TagFolder *self, const int tag_to_tag_id, const int tag_id);
int TagFolder_untag_a_tag(TagFolder *self, const int tag_to_untag, const int tag_id);
int TagFolder_tag_a_file(TagFolder *self, const int file_to_tag_id, const int tag_id);
int TagFolder_untag_a_file(TagFolder *self, const int file_to_untag_id, const int tag_id);
int TagFolder_select_tag(TagFolder *self, const int tag_id);
Tag *TagFolder_get_selected_tags(TagFolder *self);
Tag *TagFolder_get_tags_tagging_specific_file(TagFolder *self, const int file_id);
int TagFolder_unselect_tag(TagFolder *self, const int tag_id);
File *TagFolder_list_current_files(TagFolder *self);
int TagFolder_delete_tag(TagFolder *self, const int tag_id);
int TagFolder_delete_file(TagFolder *self, const int file_id);

#endif
