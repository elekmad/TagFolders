#ifndef _TAG_FOLDER_H_
#define _TAG_FOLDER_H_
#include <sqlite3.h>

struct Tag
{
    int id;
    char name[20];
    struct Tag *next;
};
typedef struct Tag Tag;

struct File
{
    int id;
    char name[20];
    struct File *next;
};
typedef struct File File;


struct TagFolder
{
    char folder[150];
    sqlite3 *db;
    Tag *current;
};
typedef struct TagFolder TagFolder;

/* TagFolder.c */
void Tag_init(Tag *self, const char *name, int id);
Tag *Tag_new(const char *name, int id);
Tag *Tag_set_next(Tag *self, Tag *next);
void Tag_finalize(Tag *self);
void Tag_free(Tag *self);
void File_init(File *self, const char *name, int id);
File *File_new(const char *name, int id);
File *File_set_next(File *self, File *next);
void File_finalize(File *self);
void File_free(File *self);
void TagFolder_init(TagFolder *self);
TagFolder *TagFolder_new(void);
void TagFolder_finalize(TagFolder *self);
void TagFolder_free(TagFolder *self);
int TagFolder_setup_folder(TagFolder *self, char *name);
int TagFolder_check_db_structure(TagFolder *self);
Tag *TagFolder_list_tags(TagFolder *self);
int TagFolder_create_tag(TagFolder *self, const char *name);
int TagFolder_create_file_in_db(TagFolder *self, const char *name);
int TagFolder_tag_a_tag(TagFolder *self, const char *tag_to_tag, const char *tag);
int TagFolder_untag_a_tag(TagFolder *self, const char *tag_to_tag, const char *tag);
int TagFolder_tag_a_file(TagFolder *self, const char *file_to_tag, const char *tag);
int TagFolder_untag_a_file(TagFolder *self, const char *file_to_tag, const char *tag);
int TagFolder_select_tag(TagFolder *self, const char *tag);
int TagFolder_unselect_tag(TagFolder *self, const char *tag);
File *TagFolder_list_current_files(TagFolder *self);
int TagFolder_delete_tag(TagFolder *self, const char *tag);
int TagFolder_delete_file(TagFolder *self, const char *file);

#endif
