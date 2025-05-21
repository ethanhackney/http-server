#include <stdio.h>

static size_t
str_hash(const char *s)
{
        const char *p = NULL;
        size_t hash = 0;

        hash = 5381;
        for (p = s; *p != 0; p++)
                hash = hash * 31 + *p;

        return hash;
}

int
main(void)
{
        char *method[] = {
                "GET",
                NULL,
        };
        char *version[] = {
                "HTTP/1.1",
                NULL,
        };
        char *header[] = {
                "Host",
                "User-Agent",
                "Accept",
                NULL,
        };
        char **p = NULL;

        printf("methods\n");
        for (p = method; *p != NULL; p++)
                printf("%s: %zu\n", *p, str_hash(*p) % 1);

        printf("\nmethods\n");
        for (p = version; *p != NULL; p++)
                printf("%s: %zu\n", *p, str_hash(*p) % 1);

        printf("\nheader\n");
        for (p = header; *p != NULL; p++)
                printf("%s: %zu\n", *p, str_hash(*p) % 5);
}
