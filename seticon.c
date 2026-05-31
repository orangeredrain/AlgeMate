#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: seticon.exe <exe_path> <ico_path>\n");
        return 1;
    }

    const char* exePath = argv[1];
    const char* icoPath = argv[2];

    // Read ICO file
    FILE* f = fopen(icoPath, "rb");
    if (!f) { printf("Cannot open ICO: %s\n", icoPath); return 1; }
    fseek(f, 0, SEEK_END);
    long icoSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* icoData = (unsigned char*)malloc(icoSize);
    fread(icoData, 1, icoSize, f);
    fclose(f);

    // Parse ICO header
    unsigned short reserved = *(unsigned short*)(icoData);
    unsigned short type = *(unsigned short*)(icoData + 2);
    unsigned short count = *(unsigned short*)(icoData + 4);

    if (type != 1) { printf("Not an ICO file\n"); free(icoData); return 1; }
    printf("ICO: %hu images\n", count);

    // Begin update
    HANDLE h = BeginUpdateResourceA(exePath, FALSE);
    if (!h) { printf("BeginUpdateResource failed: %lu\n", GetLastError()); free(icoData); return 1; }
    printf("BeginUpdateResource OK\n");

    // Add each icon image as RT_ICON
    for (int i = 0; i < count; i++) {
        int off = 6 + i * 16;
        unsigned char w = icoData[off];
        unsigned char h = icoData[off + 1];
        unsigned int size = *(unsigned int*)(icoData + off + 8);
        unsigned int imgOff = *(unsigned int*)(icoData + off + 12);

        if (w == 0) w = 256;
        if (h == 0) h = 256;

        printf("  #%d: %ux%u %u bytes\n", i + 1, w, h, size);

        if (!UpdateResourceA(h, MAKEINTRESOURCEA(3), MAKEINTRESOURCEA(i + 1),
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                             icoData + imgOff, size)) {
            printf("  UpdateResource ICON #%d failed: %lu\n", i + 1, GetLastError());
            EndUpdateResourceA(h, TRUE);
            free(icoData);
            return 1;
        }
    }

    // Build GROUP_ICON resource
    // GRPICONDIR: reserved(2) + type(2) + count(2)
    // GRPICONDIRENTRY: w(1)+h(1)+colors(1)+reserved(1)+planes(2)+bpp(2)+size(4)+nID(2)
    int grpSize = 6 + count * 14;
    unsigned char* grp = (unsigned char*)malloc(grpSize);
    *(unsigned short*)(grp) = 0;
    *(unsigned short*)(grp + 2) = 1;
    *(unsigned short*)(grp + 4) = count;

    for (int i = 0; i < count; i++) {
        int off = 6 + i * 16;
        int goff = 6 + i * 14;
        unsigned char w = icoData[off];
        unsigned char h = icoData[off + 1];
        unsigned char colors = icoData[off + 2];
        unsigned short planes = *(unsigned short*)(icoData + off + 4);
        unsigned short bpp = *(unsigned short*)(icoData + off + 6);
        unsigned int size = *(unsigned int*)(icoData + off + 8);

        grp[goff] = w;
        grp[goff + 1] = h;
        grp[goff + 2] = colors;
        grp[goff + 3] = 0;
        *(unsigned short*)(grp + goff + 4) = planes;
        *(unsigned short*)(grp + goff + 6) = bpp;
        *(unsigned int*)(grp + goff + 8) = size;
        *(unsigned short*)(grp + goff + 12) = i + 1;
    }

    if (!UpdateResourceA(h, MAKEINTRESOURCEA(14), MAKEINTRESOURCEA(1),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                         grp, grpSize)) {
        printf("UpdateResource GROUP_ICON failed: %lu\n", GetLastError());
        EndUpdateResourceA(h, TRUE);
        free(grp);
        free(icoData);
        return 1;
    }
    printf("GROUP_ICON OK\n");

    free(grp);
    free(icoData);

    // Commit
    if (!EndUpdateResourceA(h, FALSE)) {
        printf("EndUpdateResource failed: %lu\n", GetLastError());
        return 1;
    }
    printf("DONE - Icon injected!\n");
    return 0;
}
