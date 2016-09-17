#include <TagFolder.h>
#include <string.h>
#include <stdio.h>

void TagFolder_init(struct TagFolder *self)
{
    self->folder[0] = '\0';
    self->current = NULL;
    self->db = NULL;
}

int TagFolder_set_db(struct TagFolder *self, sqlite3 *db)
{
    if(self->db != NULL)
    {
        sqlite3_close(self->db);
        self->db = db;
    }
}

void TagFolder_finalize(struct TagFolder *self)
{
    TagFolder_set_db(self, NULL);
}

int TagFolder_check_db_structure(struct TagFolder *);

int TagFolder_setup_folder(struct TagFolder *self, char *name)
{
    strncpy(self->folder, name, 149);
    self->folder[150] = '\0';//Protection because strncpy might skip  the null terminating byte...
    if(strlen(self->folder) > 0)
    {
        int rc;
        sqlite3 *db;
        char *req;
        rc = sqlite3_open(self->folder, &db);
        if( rc )
        {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            return -1 ;
        }
        TagFolder_set_db(self, db);
        TagFolder_check_db_structure(self);
    }
}

int TagFolder_check_db_structure(struct TagFolder *self)
{
    sqlite3_stmt *res;
    char *req;
    int rc;
    if(self->db == NULL)
        return -1;
//Table Tag
    req = "SELECT name FROM sqlite_master WHERE type='table' AND name='tag';";
    rc = sqlite3_prepare_v2(self->db, req, strlen(req), &res, NULL);
    
    if( rc )
    {   
        fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tag (id int, name varchar);";
        rc = sqlite3_exec(self->db, req, NULL, NULL, NULL);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
            sqlite3_finalize(res);
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
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table file (id int, name varchar);";
        rc = sqlite3_exec(self->db, req, NULL, NULL, NULL);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
            sqlite3_finalize(res);
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
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tagtag(primid int, secondid int);";
        rc = sqlite3_exec(self->db, req, NULL, NULL, NULL);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
            sqlite3_finalize(res);
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
        return -1 ;
    }
    rc = sqlite3_step(res);
    
    if (rc != SQLITE_ROW)
    {
        char *create_req = "create table tagfile(tagid int, fileid int);";
        rc = sqlite3_exec(self->db, req, NULL, NULL, NULL);
        if( rc )
        {   
            fprintf(stderr, "Can't check database structure: %s\n", sqlite3_errmsg(self->db));
            sqlite3_finalize(res);
            return -1 ;
        }
    }
    sqlite3_finalize(res);

    return 0;
}
