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
    TagFolder_get_tag(&folder, "dupont");
    TagFolder_list_current_files(&folder);
    printf("dupont factures\n");
    TagFolder_get_tag(&folder, "factures");
    TagFolder_list_current_files(&folder);
    printf("dupont factures specs\n");
    TagFolder_get_tag(&folder, "specs");
    TagFolder_list_current_files(&folder);
    printf("dupont factures specs 'before untagued'\n");
    TagFolder_tag_a_file(&folder, "novembre", "specs");
    TagFolder_list_current_files(&folder);
    printf("dupont factures specs 'before untagued'\n");
    TagFolder_untag_a_file(&folder, "novembre", "specs");
    TagFolder_list_current_files(&folder);
    TagFolder_release_tag(&folder, "factures");
    printf("dupont specs\n");
    TagFolder_list_current_files(&folder);
    TagFolder_finalize(&folder);
    exit(EXIT_SUCCESS);
}
