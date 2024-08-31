#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "tiny_jpeg.h"
unsigned char* stbi_load(const char* filename, int* width, int* height, int* num_components, int flag)
{
    FILE* fp = fopen(filename, "r");
    assert(fp != NULL);

    ushort s;
    fread(&s, 1, 2, fp);
    assert(s==19778);

    fseek(fp, 18, SEEK_SET);
    fread(width, 1, 4, fp);
    fread(height, 1, 4, fp);
    assert(*width < 10000);
    assert(*height < 10000);

    ushort bit;
    fseek(fp, 28, SEEK_SET);
    fread(&bit, 1, 2, fp);
    assert(bit==24);
    *num_components = 3;

    int offset;
    fseek(fp, 10, SEEK_SET);
    fread(&offset, 1, 4, fp);
    printf("data offset: %d\n", offset);
    fseek(fp, offset, SEEK_SET);

    printf("width: %d\n", *width);
    printf("height: %d\n", *height);
    int bytes = *width * *height * 3;
    int padding = (4 - (*width * 3 % 4))%4;
    printf("data len: %d, line padding: %d\n", bytes, padding);
    int line_bytes = *width * 3 + padding;
    unsigned char* data = (unsigned char*)malloc(bytes);
    // int ret = fread(data, bytes, 1, fp);
    // assert(ret == 1);
    unsigned char* p = data;
    for(int i = *height - 1; i >= 0;i--) {
        int pos = line_bytes * i + offset;
        fseek(fp, pos, SEEK_SET);
        fread(p, *width*3, 1, fp);
        p += *width * 3;
    }

    return data;
}

int main(int argc, char** argv)
{
    char* in = argv[1];
    char* out = argv[2];
    int width, height, num_components;
    unsigned char* data = stbi_load(in, &width, &height, &num_components, 0);
    if ( !data ) {
        puts("Could not find file");
        return EXIT_FAILURE;
    }

    if ( !tje_encode_to_file(out, width, height, num_components, data) ) {
        fprintf(stderr, "Could not write JPEG\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
