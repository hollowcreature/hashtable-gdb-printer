#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct node {
    char *key;
    int value;
    struct node *next;
};

struct table {
    size_t size;
    size_t count;
    struct node **buckets;
};

unsigned long hash(const char *s) {
    unsigned long h = 5381;
    while (*s != '\0') {
        unsigned char c = (unsigned char)*s;
        h = h * 33 + c;
        s++;
    }

    return h;
}

struct table *table_new(size_t size) {
    struct table *t = malloc(sizeof(struct table));
    if (t == NULL)
        return NULL;
    
    struct node **buckets = calloc(size, sizeof(struct node *));
    if (buckets == NULL) {
        free(t);
        return NULL;
    }

    t->size = size;
    t->count = 0;
    t->buckets = buckets;
    return t;
}

static struct node *find(struct table *t, size_t idx, const char *key, struct node **prev_out) {
    struct node *prev = NULL;
    for (struct node *curr = t->buckets[idx]; curr != NULL; curr = curr->next) {
        if (strcmp(curr->key, key) == 0) {
            if (prev_out != NULL)
                *prev_out = prev;
            return curr;
        }
        prev = curr;
    }

    return NULL;
}

int table_insert(struct table *t, const char *key, int value) {
    size_t idx = hash(key) % t->size;
    struct node *found = find(t, idx, key, NULL);
    if (found != NULL) {
        found->value = value;
        return 0;
    }

    struct node *new = malloc(sizeof(*new));
    if (new == NULL)
        return -1;
    
    new->key = strdup(key);
    if (new->key == NULL) {
        free(new);
        return -1;
    }

    new->value = value;
    t->count++;
    new->next = t->buckets[idx];
    t->buckets[idx] = new;
    return 0;
}

int table_get(struct table *t, const char *key, int *value_out) {
    size_t idx = hash(key) % t->size;
    struct node *found = find(t, idx, key, NULL);
    if (found == NULL)
        return -1;
    
    *value_out = found->value;
    return 0;
}

int table_remove(struct table *t, const char *key) {
    size_t idx = hash(key) % t->size;
    struct node *prev;
    struct node *to_remove = find(t, idx, key, &prev);
    if (to_remove == NULL)
        return -1;

    t->count--;
    if (prev == NULL) {
        t->buckets[idx] = to_remove->next;
    }
    else {
        prev->next = to_remove->next;
    }
    free(to_remove->key);
    free(to_remove);
    return 0;
}

void table_free(struct table *t) {
    if (t == NULL)
        return;

    for (size_t idx = 0; idx < t->size; idx++) {
        struct node *curr = t->buckets[idx];
        while (curr != NULL) {
            struct node *next = curr->next;
            free(curr->key);
            free(curr);
            curr = next;
        }
    }
    free(t->buckets);
    free(t);
}

int main(void) {

    // TESTS

    const char *keys[] = { "abrams", "baba", "drifter", "dynamo", "paradox", "solomon" };
    size_t nkeys = sizeof keys / sizeof keys[0];

    printf("table alloc test\n");
    struct table *t = table_new(8);
    if (t == NULL) {
        fprintf(stderr, "table alloc test failed\n");
        return 1;
    }
    printf("table alloc test passed\n");

    printf("---------------------------------\n");

    printf("table insert test, inserting 6 keys...\n");
    for(size_t i = 0; i < nkeys; i++) {
        printf("key=%s -> bucket=%lu\n", keys[i], hash(keys[i]) % 8);
        int rv = table_insert(t, keys[i], (int)i);
        if (rv == -1) {
            table_free(t);
            fprintf(stderr, "insert failed\n");
            return 1;
        }
    }
    printf("table insert test passed\n");

    printf("---------------------------------\n");

    printf("table insert update value test\n");
    int rv = table_insert(t, keys[0], 67);
    if (rv == -1) {
        table_free(t);
        fprintf(stderr, "insert failed\n");
        return 1;
    }

    int val_out = 0;
    rv = table_get(t, keys[0], &val_out);
    
    if (val_out != 67) { 
        fprintf(stderr, "FAIL: ...\n");
        table_free(t);
        return 1;
    }

    printf("abrams node value=%d\n", val_out);
    printf("table insert update value test passed\n");

    printf("---------------------------------\n");

    printf("trying to get non existent key\n");
    int val_out_miss = 0;
    rv = table_get(t, "apollo", &val_out_miss);

    printf("miss return value (should be -1): %d\n", rv);

    printf("---------------------------------\n");

    printf("table remove test\n");
    table_remove(t, "abrams");
    printf("after abrams remove: %s -> %p\n", t->buckets[3]->key, (void *)t->buckets[3]->next);
    printf("count after: %zu\n", t->count);

    table_remove(t, "baba");
    printf("after baba remove: %p\n", (void *)t->buckets[3]);
    printf("count after: %zu\n", t->count);

    table_remove(t, "dynamo");
    printf("after dynamo remove: %s\n", t->buckets[5]->key);
    printf("count after: %zu\n", t->count);

    printf("table non existent key remove test\n");
    rv = table_remove(t, "apollo");
    printf("apollo remove return value (should be -1): %d\n", rv);
    printf("count after: %zu\n", t->count);

    printf("---------------------------------\n");

    printf("tests over\n");
    table_free(t);
    return 0;
}