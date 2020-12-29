
#include <stdio.h>
#include <gtk-2.0/gtk/gtk.h>
#include "./cmdspawn.h"

int main () {
    char* str;
    str = genmon_SpawnCmd("echo hi", 1);
    printf("%s\n", str);
    return 0;
}
