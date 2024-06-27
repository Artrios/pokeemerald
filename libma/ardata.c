#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct buf {
    size_t size;
    char data[1];
};

struct pbuf {
    size_t size;
    char *data;
};

struct buf *file_read(const char *fname)
{
    FILE *f;
    long size;
    struct buf *file;

    f = fopen(fname, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);
    file = malloc(sizeof(struct buf) - 1 + size);
    if (!file) {
        fclose(f);
        return NULL;
    }
    file->size = size;
    if (fread(file->data, file->size, 1, f) != 1) {
        free(file);
        fclose(f);
        return NULL;
    }
    fclose(f);
    return file;
}

bool file_write(const char *fname, const struct buf *file)
{
    FILE *f = fopen(fname, "wb");
    if (!f) {
        perror("fopen");
        return false;
    }
    if (fwrite(file->data, file->size, 1, f) != 1) {
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

unsigned file_lines(const struct buf *file)
{
    unsigned lines = 0;
    const char *p = file->data;
    while (p < file->data + file->size) if (*p++ == '\n') lines++;
    while (p-- > file->data) {
        if (*p == '\n') break;
        if (isspace(*p)) continue;
        lines++;
        break;
    }
    return lines;
}

struct arrecord {
    char name[16];
    char stamp[12];
    char uid[6];
    char gid[6];
    char mode[8];
};

struct pbuf parse_ardata_getfield(struct buf *data, size_t *offs)
{
    char *p, *e, *s;
    struct pbuf res;

    p = data->data + *offs;
    e = data->data + data->size;

    while (p < e) if (!isspace(*p) || *p == '\n') break; else p++;
    s = p;
    while (p < e) if (isspace(*p) || *p == '\n') break; else p++;

    *offs = p - data->data;

    res.data = s;
    res.size = p - s;
    return res;
}

void parse_ardata_nextline(const struct buf *data, size_t *offs)
{
    const char *p, *e;

    p = data->data + *offs;
    e = data->data + data->size;
    while (p < e) if (*p++ == '\n') break;
    *offs = p - data->data;
}

void parse_ardata_copy(char *dest, size_t dest_size, const struct pbuf src)
{
    size_t size = src.size > dest_size ? dest_size : src.size;
    memset(dest, ' ', dest_size);
    memcpy(dest, src.data, size);
}

struct arrecord *parse_ardata(struct buf *ardata, unsigned count)
{
    unsigned i;
    size_t offs = 0;

    if (!count) return NULL;
    struct arrecord *records = malloc(sizeof(struct arrecord) * count);
    if (!records) return NULL;

    for (i = 0; i < count && offs < ardata->size; i++) {
        struct arrecord *record = records + i;
        struct pbuf field;

        field = parse_ardata_getfield(ardata, &offs);
        parse_ardata_copy(record->name, sizeof(record->name), field);

        field = parse_ardata_getfield(ardata, &offs);
        parse_ardata_copy(record->stamp, sizeof(record->stamp), field);

        field = parse_ardata_getfield(ardata, &offs);
        parse_ardata_copy(record->uid, sizeof(record->uid), field);

        field = parse_ardata_getfield(ardata, &offs);
        parse_ardata_copy(record->gid, sizeof(record->gid), field);

        field = parse_ardata_getfield(ardata, &offs);
        parse_ardata_copy(record->mode, sizeof(record->mode), field);

        parse_ardata_nextline(ardata, &offs);
    }

    return records;
}

void replace_record(char *data, struct arrecord *records, unsigned record_count)
{
    unsigned i;
    for (i = 0; i < record_count; i++) {
        struct arrecord *record = records + i;
        if (memcmp(data, record->name, sizeof(record->name)) == 0) {
            memcpy(data, record, sizeof(struct arrecord));
            return;
        }
    }
}

bool parse_arfile(struct buf *arfile, struct arrecord *records,
    unsigned record_count)
{
    char *p, *e;

    if (arfile->size < 8) return false;
    if (memcmp(arfile->data, "!<arch>\n", 8) != 0) return false;

    p = arfile->data + 8;
    e = arfile->data + arfile->size;
    while (p < e) {
        char size_str[11];

        if (*p == '\n') break;
        if (p + 60 >= e) return false;

        replace_record(p, records, record_count);
        p += 48;

        memcpy(size_str, p, 10);
        size_str[10] = 0;
        p += 10;
        if (*p++ != '`') return false;
        if (*p++ != '\n') return false;
        p += strtoul(size_str, NULL, 10);
    }

    return true;
}

int main(int argc, char *argv[])
{
    struct buf *arfile, *ardata;
    struct arrecord *records;
    unsigned record_count;

    if (argc <= 2) {
        fprintf(stderr, "%s <arfile> <ardata>\n", argv[0]);
        return 1;
    }

    arfile = file_read(argv[1]);
    ardata = file_read(argv[2]);
    if (!arfile || !ardata) return 1;

    record_count = file_lines(ardata);
    records = parse_ardata(ardata, record_count);
    free(ardata);
    if (!records) return 1;

    if (!parse_arfile(arfile, records, record_count)) {
        fprintf(stderr, "Error parsing %s\n", argv[1]);
        free(records);
        free(arfile);
        return 1;
    }

    file_write(argv[1], arfile);

    free(records);
    free(arfile);

    return 0;
}
