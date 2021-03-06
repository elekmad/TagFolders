#include <TagFolder.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>

static void TagFolder_generate_filename(const char *str, char *out)
{
    int n, length;
    MD5_CTX c;
    unsigned char digest[16];
    unsigned char time_of_day[(sizeof(time_t) + sizeof(suseconds_t)) * 2];
    struct timeval tv;

    MD5_Init(&c);

    length = strlen(str);
    while (length > 0)
    {
        if (length > 512)
        {
            MD5_Update(&c, str, 512);
        }
        else
        {
            MD5_Update(&c, str, length);
        }
        length -= 512;
        str += 512;
    }
    gettimeofday(&tv, NULL);
    MD5_Update(&c, &tv, sizeof(tv));

    MD5_Final(digest, &c);

    for (n = 0; n < 16; ++n)
    {
        snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    }
}

struct Tag
{
    int id;
    String name;
    struct Tag *next;
    TagType type;
};

void Tag_init(Tag *self, const char *name, int id, TagType type)
{
    String_init(&self->name);
    String_append_char_string(&self->name, name);
    self->id = id;
    self->next = NULL;
    self->type = type;
}

Tag *Tag_new(const char *name, int id, TagType type)
{
    Tag *new_tag = malloc(sizeof(Tag));
    if(new_tag != NULL)
        Tag_init(new_tag, name, id, type);
    return new_tag;
}

Tag *Tag_set_next(Tag *self, Tag *next)
{
    Tag *ret = self->next;
    self->next = next;
    return ret;
}

Tag *Tag_get_next(Tag *self)
{
    return self->next;
}

void Tag_set_type(Tag *self, TagType type)
{
    self->type = type;
}

const String *Tag_get_name(Tag *self)
{
   return &self->name;
}

int Tag_get_id(Tag *self)
{
    return self->id;
}

TagType Tag_get_type(Tag *self)
{
    return self->type;
}

void Tag_finalize(Tag *self)
{
    Tag *ret = Tag_set_next(self, NULL);
    if(ret != NULL)
        Tag_free(ret);
    String_finalize(&self->name);
}

void Tag_free(Tag *self)
{
    Tag_finalize(self);
    free(self);
}

struct File
{
    int id;
    String name;
    String filename;
    struct stat stat;
    struct File *next;
};

void File_init(File *self, const char *name, const char *pathname, int id)
{
    char *filename;
    String_init(&self->name);
    String_append_char_string(&self->name, name);
    filename = strdup(pathname);
    String_init(&self->filename);
    String_append_char_string(&self->filename, basename(filename));
    free(filename);
    if(stat(pathname, &self->stat) == -1)
        fprintf(stderr, "Get stat of %s failed : %s\n", pathname, strerror(errno));
    self->id = id;
    self->next = NULL;
}

File *File_new(const char *name, const char *path, int id)
{
    File *new_tag = malloc(sizeof(File));
    if(new_tag != NULL)
        File_init(new_tag, name, path, id);
    return new_tag;
}

File *File_set_next(File *self, File *next)
{
    File *ret = self->next;
    self->next = next;
    return ret;
}

File *File_get_next(File *self)
{
    return self->next;
}

const String *File_get_name(File *self)
{
    return &self->name;
}

const String *File_get_filename(File *self)
{
    return &self->filename;
}

struct timespec *File_get_last_modification(File *self)
{
    return &self->stat.st_mtim;
}

unsigned long File_get_size(File *self)
{
    return (unsigned long)self->stat.st_size;
}

int File_get_id(File *self)
{
    return self->id;
}

void File_finalize(File *self)
{
    File *ret = File_set_next(self, NULL);
    if(ret != NULL)
        File_free(ret);
    String_finalize(&self->filename);
    String_finalize(&self->name);
}

void File_free(File *self)
{
    File_finalize(self);
    free(self);
}


void TagFolder_init(TagFolder *self)
{
    String_init(&self->folder);
    self->current_includes = NULL;
    self->current_excludes = NULL;
    self->db = NULL;
}

TagFolder *TagFolder_new(void)
{
    TagFolder *self = malloc(sizeof(TagFolder));
    if(self != NULL)
        TagFolder_init(self);
    return self;
}

static int TagFolder_set_db(TagFolder *self, sqlite3 *db)
{
    if(self->db != NULL)
        sqlite3_close_v2(self->db);
    self->db = db;
}

static void TagFolder_set_current_include(TagFolder *self, Tag *cur)
{
    if(cur != NULL)
        Tag_set_next(cur, self->current_includes);
    self->current_includes = cur;
}

static void TagFolder_set_current_exclude(TagFolder *self, Tag *cur)
{
    if(cur != NULL)
        Tag_set_next(cur, self->current_excludes);
    self->current_excludes = cur;
}

void TagFolder_finalize(TagFolder *self)
{
    String_finalize(&self->folder);
    TagFolder_set_db(self, NULL);
    if(self->current_includes != NULL)
        Tag_free(self->current_includes);
    if(self->current_excludes != NULL)
        Tag_free(self->current_excludes);
}

void TagFolder_free(TagFolder *self)
{
    TagFolder_finalize(self);
    free(self);
}

int TagFolder_check_db_structure(TagFolder *);

int TagFolder_setup_folder(TagFolder *self, const char *name)
{
    String_set_char_string(&self->folder, name);
    if(String_get_char_at(&self->folder, String_get_length(&self->folder) - 1) != '/')
        String_append_char(&self->folder, '/');
    if(String_get_length(&self->folder) > 0)
    {
        Tag *tags, *ptr, *old_ptr = NULL;
        int rc;
        sqlite3 *db;
        String dbname;
        char *req;
        String_init(&dbname);
        String_append_String(&dbname, &self->folder);
        String_append_char_string(&dbname, ".TagFolder.sql");
        rc = sqlite3_open(String_get_char_string(&dbname), &db);
        if( rc )
        {
            fprintf(stderr, "Can't open database '%s': %s\n", String_get_char_string(&dbname), sqlite3_errmsg(db));
            String_finalize(&dbname);
            return -1 ;
        }
        String_finalize(&dbname);
        TagFolder_set_db(self, db);
        TagFolder_check_db_structure(self);
        tags = TagFolder_list_tags(self);
        ptr = tags;
        if(tags != NULL)
        {
            while(ptr != NULL)
            {
                switch(Tag_get_type(ptr))
                {
                    case TagTypeInclude :
                        old_ptr = ptr;
                        ptr = Tag_get_next(ptr);
                        break;
                    case TagTypeExclude :
                        {
                            Tag *cur_tag_to_add = ptr;
                            ptr = Tag_get_next(cur_tag_to_add);
                            if(old_ptr == NULL)
                                tags = ptr;//To be able to free tags at the end, we must have only includes.
                            else
                                Tag_set_next(old_ptr, ptr);
                            Tag_set_next(cur_tag_to_add, NULL);
                            TagFolder_set_current_exclude(self, cur_tag_to_add);
                        }
                        break;
                }
            }
            Tag_free(tags);
        }
    }
    return 0;
}

String *TagFolder_get_folder(TagFolder *self)
{
    return &self->folder;
}

static int TagFolder_begin_transaction(TagFolder *self)
{
    int rc;
    char *errmsg, *req;
    req = "BEGIN;";
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't check database structure: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1 ;
    }
    return 0;
}

static int TagFolder_commit_transaction(TagFolder *self)
{
    int rc;
    char *errmsg, *req;
    req = "COMMIT;";
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't check database structure: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1 ;
    }
    return 0;
}

static int TagFolder_rollback_transaction(TagFolder *self)
{
    int rc;
    char *errmsg, *req;
    req = "ROLLBACK;";
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't check database structure: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1 ;
    }
    return 0;
}

int TagFolder_check_db_structure(TagFolder *self)
{
    int rc;
    sqlite3_stmt *res;
    char *req;
    if(self->db == NULL)
        return -1;

    TagFolder_begin_transaction(self);
//Table Tag
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='tag';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tag (id INTEGER PRIMARY KEY, type integer, name varchar);", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);
//Table File
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='file';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table file (id INTEGER PRIMARY KEY, name varchar, filename char(32));", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);
//Table Tag to Tag
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='tagtag';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tagtag(primid int, secondid int);", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);

//Table Tag to File
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='tagfile';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tagfile(tagid int, fileid int);", *errmsg;
        rc = sqlite3_exec(self->db, create_req, NULL, NULL, &errmsg);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_finalize(res);
            TagFolder_rollback_transaction(self);
            return -1 ;
        }
    }
    sqlite3_finalize(res);
    TagFolder_commit_transaction(self);

    return 0;
}

Tag *TagFolder_list_tags(TagFolder *self)
{
    Tag *ret = NULL;
    sqlite3_stmt *res;
    char *req;
    int rc;
    req = "select name, id, type from tag;";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);

    if( rc )
    {
        fprintf(stderr, "Can't list tags: %s\n", sqlite3_errmsg(self->db));
        return ret ;
    }
    do
    {
        Tag *new_tag;
        rc = sqlite3_step(res);
        if(rc != SQLITE_ROW)
            break;
        new_tag = Tag_new(sqlite3_column_text(res, 0), sqlite3_column_int(res, 1), sqlite3_column_int(res, 2));
        Tag_set_next(new_tag, ret);
        ret = new_tag;
    }
    while(rc == SQLITE_ROW);
    sqlite3_finalize(res);

    return ret;
}

int TagFolder_create_tag(TagFolder *self, const char *name, TagType type)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[50], *errmsg;
    int rc;
    TagFolder_begin_transaction(self);
    snprintf(req, 499, "select id from tag where name = '%s';", name);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't verif if tag %s already exist: %s\n", name, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc == SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s already exist\n", name);
        TagFolder_rollback_transaction(self);
        return -1;
    }

    snprintf(req, 499, "insert into tag (name, type) values ('%s', %d);", name, (int)type);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't add tag %s : %s\n", name, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    sqlite3_finalize(res);
    return ret;
}

int TagFolder_rename_tag(TagFolder *self, const int tag_id, const char *name)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[50], *errmsg;
    int rc, selected;
    selected = (TagFolder_unselect_tag(self, tag_id) == -2) ? 0 : 1;
    TagFolder_begin_transaction(self);
    snprintf(req, 499, "select id from tag where name = '%s';", name);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't verif if tag %s already exist: %s\n", name, sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        if(selected)
            TagFolder_select_tag(self, tag_id);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc == SQLITE_ROW)
    {
        fprintf(stderr, "Tag %s already exist\n", name);
        TagFolder_rollback_transaction(self);
        if(selected)
            TagFolder_select_tag(self, tag_id);
        return -1;
    }

    snprintf(req, 499, "update tag set name = '%s' where id = %d;", name, tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't rename tag %d into '%s' : %s\n", tag_id, name, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        if(selected)
            TagFolder_select_tag(self, tag_id);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    if(selected)
        TagFolder_select_tag(self, tag_id);
    sqlite3_finalize(res);
    return ret;
}

int TagFolder_create_file_in_db(TagFolder *self, const char *name, char *db_name)
{
    int ret = 0;
    char req[500], *errmsg, filename[500];
    int rc;
//We allow multiple files to be with the same name, generating uniq filename for each.
    TagFolder_generate_filename(name, db_name);
    strcpy(filename, name);

    TagFolder_begin_transaction(self);
    snprintf(req, 499, "insert into file (name, filename) values ('%s', '%s');", basename(filename), db_name);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't add file %s in db : %s\n", name, errmsg);
        sqlite3_free(errmsg);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_tag_a_tag(TagFolder *self, const int tag_to_tag_id, const int tag_id)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc;
    TagFolder_begin_transaction(self);

//Verif if file is already tagued
    snprintf(req, 499, "select count(primid) from tagtag where primid = %d and secondid = %d", tag_id, tag_to_tag_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get count : %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Can't get count\n");
        TagFolder_rollback_transaction(self);
        return -1;
    }
    rc = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

    if(rc > 0)
    {
        fprintf(stderr, "Tag %d already tagued by %d\n", tag_to_tag_id, tag_id);
        TagFolder_rollback_transaction(self);
        return 0 ;
    }


    snprintf(req, 499, "insert into tagtag (primid, secondid) values (%d, %d);", tag_id, tag_to_tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't tag %d with %d in db : %s\n", tag_to_tag_id, tag_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_untag_a_tag(TagFolder *self, const int tag_to_untag_id, const int tag_id)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc;
    TagFolder_begin_transaction(self);

//Untag the tag
    snprintf(req, 499, "delete from tagtag where primid = %d and secondid = %d;", tag_id, tag_to_untag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't untag %d from %d in db : %s\n", tag_id, tag_to_untag_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_tag_a_file(TagFolder *self, const int file_to_tag_id, const int tag_id)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc;
    TagFolder_begin_transaction(self);

//Verif if file is already tagued
    snprintf(req, 499, "select count(tagid) from tagfile where tagid = %d and fileid = %d", tag_id, file_to_tag_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get count : %s\n", sqlite3_errmsg(self->db));
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Can't get count\n");
        TagFolder_rollback_transaction(self);
        return -1;
    }
    rc = sqlite3_column_int(res, 0);
    sqlite3_finalize(res);

    if(rc > 0)
    {
        fprintf(stderr, "File %d already tagued by %d\n", file_to_tag_id, tag_id);
        TagFolder_rollback_transaction(self);
        return 0 ;
    }

    snprintf(req, 499, "insert into tagfile (tagid, fileid) values (%d, %d);", tag_id, file_to_tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't tag %d with %d in db : %s\n", file_to_tag_id, tag_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_untag_a_file(TagFolder *self, const int file_to_untag_id, const int tag_id)
{
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    int rc;
    TagFolder_begin_transaction(self);

    snprintf(req, 499, "delete from tagfile where tagid = %d and fileid = %d;", tag_id, file_to_untag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {   
        fprintf(stderr, "Can't untag %d from %d in db : %s\n", tag_id, file_to_untag_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }
    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_select_tag(TagFolder *self, const int tag_id)
{
    Tag *cur_tag, *old_tag;
    int ret = 0;
    sqlite3_stmt *res;
    char req[500], *errmsg;
    const char *tag;
    int rc;
    TagType type;
//Take tag's infos
    snprintf(req, 499, "select name, type from tag where id = %d;", tag_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %d's infos : %s\n", tag_id, sqlite3_errmsg(self->db));
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %d do not exist in db\n", tag_id);
        return -1;
    }
    type = (TagType)sqlite3_column_int(res, 1);
    tag = sqlite3_column_text(res, 0);
    fprintf(stderr, "tag %s %d type %d to be selected.\n", tag, tag_id, type);
    switch(type)
    {
        //Select an include tag means add it into current includes list
        case TagTypeInclude :
            cur_tag = self->current_includes;
            while(cur_tag != NULL && cur_tag->id != tag_id)
                cur_tag = cur_tag->next;

            if(cur_tag == NULL)//We don't already have get this tag
            {
                cur_tag = Tag_new(tag, tag_id, type);
                TagFolder_set_current_include(self, cur_tag);
                fprintf(stderr, "Inclusing tag %d now selected.\n", tag_id);
            }
            else//We already have it
            {
                fprintf(stderr, "Inclusing tag %d already selected, nothing to be done.\n", tag_id);
                ret = -2;
            }
            break;
        //Select an exclude tag means delete it from current excludes list
        case TagTypeExclude :
            cur_tag = self->current_excludes;
            old_tag = NULL;
            while(cur_tag != NULL && cur_tag->id != tag_id)
            {
                old_tag = cur_tag;
                cur_tag = cur_tag->next;
            }
    
            if(cur_tag != NULL)//Tag to release found
            {
                if(old_tag != NULL)
                    Tag_set_next(old_tag, cur_tag->next);
                else
                    self->current_excludes = cur_tag->next;
                Tag_set_next(cur_tag, NULL);
                Tag_free(cur_tag);
                fprintf(stderr, "Exclusing tag %d now selected.\n", tag_id);
            }
            else//We already have it
            {
                fprintf(stderr, "Exclusing tag %d already selected, nothing to be done.\n", tag_id);
                ret = -2;
            }
            break;
    }
    sqlite3_finalize(res);
    return ret;
}

//Do not free them !
Tag *TagFolder_get_selected_tags(TagFolder *self)
{
   return self->current_includes;
}

int TagFolder_unselect_tag(TagFolder *self, const int tag_id)
{
    Tag *cur_tag, *old_tag;
    int ret = 0;
    sqlite3_stmt *res;
    char req[500];
    const char *tag;
    int rc;
    TagType type;
//Take tag's infos
    snprintf(req, 499, "select name, type from tag where id = %d;", tag_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get tag %d's infos : %s\n", tag_id, sqlite3_errmsg(self->db));
        return -1 ;
    }
    rc = sqlite3_step(res);
 
    if(rc != SQLITE_ROW)
    {
        fprintf(stderr, "Tag %d do not exist in db\n", tag_id);
        return -1;
    }
    tag = sqlite3_column_text(res, 0);
    type = (TagType)sqlite3_column_int(res, 1);
    fprintf(stderr, "tag %s %d type %d to be unselected.\n", tag, tag_id, type);
    switch(type)
    {
        //Unselect an include tag means delete it from current includes list
        case TagTypeInclude :
            cur_tag = self->current_includes;
            old_tag = NULL;
            while(cur_tag != NULL && cur_tag->id != tag_id)
            {
                old_tag = cur_tag;
                cur_tag = cur_tag->next;
            }
    
            if(cur_tag != NULL)//Tag to release found
            {
                if(old_tag != NULL)
                    Tag_set_next(old_tag, cur_tag->next);
                else
                    self->current_includes = cur_tag->next;
                Tag_set_next(cur_tag, NULL);
                Tag_free(cur_tag);
            }
            else//Tag was not selected
                ret = -2;
            break;
        //Unselect an exclude tag means add it into current excludes list
        case TagTypeExclude :
            cur_tag = self->current_excludes;
            while(cur_tag != NULL && cur_tag->id != tag_id)
                cur_tag = cur_tag->next;

            if(cur_tag == NULL)//We don't already have get this tag
            {
                cur_tag = Tag_new(tag, tag_id, type);
                TagFolder_set_current_exclude(self, cur_tag);
            }
            else//Tag was not selected
                ret = -2;
            break;
    }
    sqlite3_finalize(res);
    return ret;
}

Tag *TagFolder_get_tags_tagging_specific_file(TagFolder *self, const int file_id)
{
    Tag *ret = NULL;
    sqlite3_stmt *res;
    char req[500];
    int rc;

    snprintf(req, 499, "SELECT name, id, type FROM tag inner join tagfile on tag.id = tagfile.tagid WHERE fileid = '%d';", file_id);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    if( rc )
    {
        fprintf(stderr, "Can't list tags for file %d : %s\n", file_id, sqlite3_errmsg(self->db));
        return ret;
    }
    do
    {
        Tag *new_tag;
        rc = sqlite3_step(res);
        if(rc != SQLITE_ROW)
            break;
        new_tag = Tag_new(sqlite3_column_text(res, 0), sqlite3_column_int(res, 1), (TagType)sqlite3_column_int(res, 2));
        Tag_set_next(new_tag, ret);
        ret = new_tag;
    }
    while(rc == SQLITE_ROW);
    sqlite3_finalize(res);

    return ret;
}

File *TagFolder_list_current_files(TagFolder *self)
{
    Tag *cur_tag;
    File *ret = NULL;
    sqlite3_stmt *res;
    char req[50000], *ptr = req, *errmsg, *main_name = "result", where_clause[5000], *ptr_where = where_clause;
    int rc, tag_id;
    req[0] = '\0';//Force empty
    where_clause[0] = '\0';
    cur_tag = self->current_includes;
    if(cur_tag != NULL)
    {
        const char *tag_name = String_get_data(Tag_get_name(cur_tag));
        fprintf(stderr, "cur include tag = '%s'\n", tag_name);
        ptr += sprintf(ptr, "select %s.name, %s.id, %s.filename from (select file.name, file.id, file.filename from file inner join tagfile on file.id = tagfile.fileid inner join tag on tagfile.tagid = tag.id where tag.name = '%s') as %s", main_name, main_name, main_name, tag_name, main_name);
        while(cur_tag != NULL)
        {
            const char *cur_tag_name;
            cur_tag = Tag_get_next(cur_tag);
            if(cur_tag == NULL)
                break;
            cur_tag_name = String_get_data(Tag_get_name(cur_tag));
            fprintf(stderr, "cur include tag = '%s'\n", cur_tag_name);
            ptr += sprintf(ptr, " inner join (select file.name, file.id, file.filename from file inner join tagfile on file.id = tagfile.fileid inner join tag on tagfile.tagid = tag.id where tag.name = '%s') as %s on %s.id = %s.id", cur_tag_name, cur_tag_name, cur_tag_name, main_name);
        }
    }
    cur_tag = self->current_excludes;
    if(cur_tag != NULL)
    {
        const char *tag_name = String_get_data(Tag_get_name(cur_tag));
        fprintf(stderr, "cur exclude tag = '%s'\n", tag_name);
        if(req[0] != '\0')//strlen(req) > 0 but we just want to know if not 0, not length, so strlen risk to be longer...
            ptr += sprintf(ptr, " left outer join (select f.id, f.name, f.filename from file as f inner join tagfile as tf on f.id = tf.fileid inner join tag as t on tf.tagid = t.id where t.name = '%s') as %s on %s.id = %s.id", tag_name, tag_name, tag_name, main_name);
        else
            ptr += sprintf(ptr, "select %s.name, %s.id, %s.filename from file as %s left outer join (select f.id, f.name, f.filename from file as f inner join tagfile as tf on f.id = tf.fileid inner join tag as t on tf.tagid = t.id where t.name = '%s') as %s on %s.id = %s.id", main_name, main_name, main_name, main_name, tag_name, tag_name, tag_name, main_name);
        ptr_where += sprintf(ptr_where, " where %s.id is null", tag_name);
        while(cur_tag != NULL)
        {
            const char *cur_tag_name;
            cur_tag = Tag_get_next(cur_tag);
            if(cur_tag == NULL)
                break;
            cur_tag_name = String_get_data(Tag_get_name(cur_tag));
            fprintf(stderr, "cur exclude tag = '%s'\n", cur_tag_name);
            ptr += sprintf(ptr, " left outer join (select f.id, f.name, f.filename from file as f inner join tagfile as tf on f.id = tf.fileid inner join tag as t on tf.tagid = t.id where t.name = '%s') as %s on %s.id = %s.id", cur_tag_name, cur_tag_name, cur_tag_name, main_name);
            ptr_where += sprintf(ptr_where, " and %s.id is null", cur_tag_name);
        }
    }
    if(req[0] == '\0')//strlen(req) == 0 but we just want to know if 0 or not, not length, so strlen risk to be longer...
        strcpy(req, "select name, id, filename from file");
    else
        strcat(req, where_clause);
    fprintf(stderr, "%s\n", req);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get file list : %s\n", sqlite3_errmsg(self->db));
        return ret ;
    }
 
    rc = sqlite3_step(res);
    while(rc == SQLITE_ROW)
    {
        String filename;
        String_init(&filename);
        String_append_String(&filename, &self->folder);
        String_append_char_string(&filename, sqlite3_column_text(res, 2));
        File *new_file = File_new(sqlite3_column_text(res, 0), String_get_data(&filename), sqlite3_column_int(res, 1));
        String_finalize(&filename);
        File_set_next(new_file, ret);
        ret = new_file;
	rc = sqlite3_step(res);
    }
    sqlite3_finalize(res);
    return ret;
}

File *TagFolder_get_file_with_id(TagFolder *self, int id)
{
    Tag *cur_tag;
    File *ret = NULL;
    sqlite3_stmt *res;
    char req[50000], *ptr = req, *errmsg;
    int rc, tag_id;
    req[0] = '\0';//Force empty
    snprintf(req, sizeof(req) - 1, "SELECT name, id, filename FROM file WHERE id = %d", id);
    fprintf(stderr, "%s\n", req);
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
 
    if( rc )
    {
        fprintf(stderr, "Can't get info for file %d : %s\n", id, sqlite3_errmsg(self->db));
        return ret ;
    }
 
    rc = sqlite3_step(res);
    if(rc == SQLITE_ROW)
    {
        String filename;
        String_init(&filename);
        String_append_String(&filename, &self->folder);
        String_append_char_string(&filename, sqlite3_column_text(res, 2));
        ret = File_new(sqlite3_column_text(res, 0), String_get_data(&filename), sqlite3_column_int(res, 1));
        String_finalize(&filename);
    }
    sqlite3_finalize(res);
    return ret;
}

int TagFolder_delete_tag(TagFolder *self, const int tag_id)
{
    int ret = 0;
    char req[500], *errmsg;
    int rc;
    sqlite3_stmt *res;
    TagFolder_begin_transaction(self);

    snprintf(req, 499, "delete from tag where id = %d;", tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete tag %d from tag table : %s\n", tag_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    snprintf(req, 499, "delete from tagtag where primid = %d or secondid = %d;", tag_id, tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete tag %d from tagtag table : %s\n", tag_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    snprintf(req, 499, "delete from tagfile where tagid = %d;", tag_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete tag %d tagfile table : %s\n", tag_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_delete_file(TagFolder *self, const int file_id)
{
    int ret = 0;
    char req[500], *errmsg;
    int rc;
    sqlite3_stmt *res;
    TagFolder_begin_transaction(self);

    snprintf(req, 499, "delete from file where id = %d;", file_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete file %d from file table : %s\n", file_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    snprintf(req, 499, "delete from tagfile where fileid = %d;", file_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't delete file %d tagfile table : %s\n", file_id, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    TagFolder_commit_transaction(self);
    return ret;
}

int TagFolder_rename_file(TagFolder *self, const int file_id, const char *new_name)
{
    int ret = 0;
    char req[500], *errmsg;
    int rc;
    sqlite3_stmt *res;
    TagFolder_begin_transaction(self);

    snprintf(req, 499, "update file set name = '%s' where id = %d;", new_name, file_id);
    rc = sqlite3_exec(self->db, req, NULL, NULL, &errmsg);
    if( rc )
    {
        fprintf(stderr, "Can't rename file %d with name '%s' : %s\n", file_id, new_name, errmsg);
        sqlite3_free(errmsg);
        sqlite3_finalize(res);
        TagFolder_rollback_transaction(self);
        return -1 ;
    }

    TagFolder_commit_transaction(self);
    return ret;
}
