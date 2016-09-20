#include <stdio.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <TagFolder.h>

TagFolder folder;

int main(int argc, char **argv)
{
    TagFolder_init(&folder);
    TagFolder_setup_folder(&folder, argv[1]);

    TagFolder_create_tag(&folder, "client");
    TagFolder_create_tag(&folder, "operations");
    TagFolder_create_tag(&folder, "dupont");
    TagFolder_create_tag(&folder, "factures");
    TagFolder_create_tag(&folder, "specs");
    TagFolder_create_file_in_db(&folder, "novembre");
    TagFolder_tag_a_file(&folder, "novembre", "factures");
    TagFolder_tag_a_file(&folder, "novembre", "dupont");
    TagFolder_create_file_in_db(&folder, "documentation v1");
    TagFolder_tag_a_file(&folder, "documentation v1", "dupont");
    TagFolder_tag_a_file(&folder, "documentation v1", "specs");

    Tag *list = TagFolder_list_tags(&folder), *cur;
    cur = list;
    while(cur != NULL)
    {
        printf("tag : %s %d\n", cur->name, cur->id);
        cur = cur->next;
    }
    printf("dupont\n");
    TagFolder_select_tag(&folder, "dupont");
    File *files, *cur_file;
    files = TagFolder_list_current_files(&folder);
    if(files != NULL)
    {
        cur_file = files;
        while(cur_file != NULL)
        {
            printf("File : %s %d\n", cur_file->name, cur_file->id);
            cur_file = cur_file->next;
        }
        File_free(files);
    }
    printf("dupont factures\n");
    TagFolder_select_tag(&folder, "factures");
    files = TagFolder_list_current_files(&folder);
    if(files != NULL)
    {
        cur_file = files;
        while(cur_file != NULL)
        {
            printf("File : %s %d\n", cur_file->name, cur_file->id);
            cur_file = cur_file->next;
        }
        File_free(files);
    }
 
    printf("dupont factures specs\n");
    TagFolder_select_tag(&folder, "specs");
    files = TagFolder_list_current_files(&folder);
    if(files != NULL)
    {
        cur_file = files;
        while(cur_file != NULL)
        {
            printf("File : %s %d\n", cur_file->name, cur_file->id);
            cur_file = cur_file->next;
        }
        File_free(files);
    }
 
    printf("dupont factures specs 'before untagued'\n");
    TagFolder_tag_a_file(&folder, "novembre", "specs");
    files = TagFolder_list_current_files(&folder);
    if(files != NULL)
    {
        cur_file = files;
        while(cur_file != NULL)
        {
            printf("File : %s %d\n", cur_file->name, cur_file->id);
            cur_file = cur_file->next;
        }
        File_free(files);
    }

    printf("dupont factures specs 'before untagued'\n");
    TagFolder_untag_a_file(&folder, "novembre", "specs");
    files = TagFolder_list_current_files(&folder);
    if(files != NULL)
    {
        cur_file = files;
        while(cur_file != NULL)
        {
            printf("File : %s %d\n", cur_file->name, cur_file->id);
            cur_file = cur_file->next;
        }
        File_free(files);
    }

    TagFolder_unselect_tag(&folder, "factures");
    printf("dupont specs\n");
    files = TagFolder_list_current_files(&folder);
    if(files != NULL)
    {
        cur_file = files;
        while(cur_file != NULL)
        {
            printf("File : %s %d\n", cur_file->name, cur_file->id);
            cur_file = cur_file->next;
        }
        File_free(files);
    }

    TagFolder_finalize(&folder);
    exit(EXIT_SUCCESS);
}
