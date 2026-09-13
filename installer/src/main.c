#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>

#include "debugScreen.h"

#define printf psvDebugScreenPrintf

#define CONFIG_PATH "ur0:tai/config.txt"
#define CONFIG_BACKUP_PATH "ur0:tai/config.txt.caffeine.bak"
#define CONFIG_PREVIOUS_PATH "ur0:tai/config.txt.caffeine.previous"
#define CONFIG_UNINSTALL_BACKUP_PATH "ur0:tai/config.txt.caffeine.uninstall.bak"
#define CONFIG_TEMP_PATH "ur0:tai/config.txt.caffeine.tmp"
#define QMR_CONFIG_LINE "ur0:tai/QuickMenuReborn.suprx"

static int path_exists(const char *path)
{
    SceIoStat stat;
    return sceIoGetstat(path, &stat) >= 0;
}

static int copy_file(const char *source, const char *destination)
{
    char buffer[16 * 1024];
    int source_fd = sceIoOpen(source, SCE_O_RDONLY, 0);
    if (source_fd < 0)
        return source_fd;

    int destination_fd = sceIoOpen(
        destination, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (destination_fd < 0) {
        sceIoClose(source_fd);
        return destination_fd;
    }

    int result = 0;
    for (;;) {
        int read_size = sceIoRead(source_fd, buffer, sizeof(buffer));
        if (read_size < 0) {
            result = read_size;
            break;
        }
        if (read_size == 0)
            break;

        int written = 0;
        while (written < read_size) {
            int write_size = sceIoWrite(
                destination_fd, buffer + written, read_size - written);
            if (write_size <= 0) {
                result = write_size < 0 ? write_size : -1;
                break;
            }
            written += write_size;
        }
        if (result < 0)
            break;
    }

    sceIoClose(destination_fd);
    sceIoClose(source_fd);
    return result;
}

static int contains_case_insensitive(const char *text, size_t text_length,
                                     const char *needle)
{
    size_t needle_length = strlen(needle);
    if (needle_length == 0 || text_length < needle_length)
        return 0;

    for (size_t i = 0; i + needle_length <= text_length; ++i) {
        size_t j = 0;
        while (j < needle_length &&
               tolower((unsigned char)text[i + j]) ==
                   tolower((unsigned char)needle[j])) {
            ++j;
        }
        if (j == needle_length)
            return 1;
    }
    return 0;
}

static int has_active_config_line(const char *text, size_t text_length,
                                  const char *needle)
{
    size_t position = 0;
    while (position < text_length) {
        size_t line_start = position;
        while (position < text_length && text[position] != '\n')
            ++position;
        size_t line_length = position - line_start;
        size_t first = 0;
        while (first < line_length &&
               (text[line_start + first] == ' ' || text[line_start + first] == '\t'))
            ++first;
        if (first < line_length && text[line_start + first] != '#' &&
            contains_case_insensitive(
                text + line_start + first, line_length - first, needle)) {
            return 1;
        }
        if (position < text_length)
            ++position;
    }
    return 0;
}

static int patch_config(void)
{
    FILE *input = fopen(CONFIG_PATH, "rb");
    if (!input)
        return -1;

    fseek(input, 0, SEEK_END);
    long input_size = ftell(input);
    rewind(input);
    if (input_size < 0 || input_size > 1024 * 1024) {
        fclose(input);
        return -2;
    }

    char *source = malloc((size_t)input_size + 1);
    char *output = malloc((size_t)input_size + 1024);
    if (!source || !output) {
        free(source);
        free(output);
        fclose(input);
        return -3;
    }

    size_t source_size = fread(source, 1, (size_t)input_size, input);
    fclose(input);
    source[source_size] = '\0';

    int has_qmr = has_active_config_line(
        source, source_size, "QuickMenuReborn.suprx");
    int inserted_qmr = has_qmr;
    size_t output_size = 0;

    size_t position = 0;
    while (position < source_size) {
        size_t line_start = position;
        while (position < source_size && source[position] != '\n')
            ++position;
        size_t line_length = position - line_start;
        size_t content_length = line_length;
        if (content_length > 0 && source[line_start + content_length - 1] == '\r')
            --content_length;

        size_t first = 0;
        while (first < content_length &&
               (source[line_start + first] == ' ' || source[line_start + first] == '\t'))
            ++first;
        int commented = first < content_length && source[line_start + first] == '#';
        int is_nosleep = !commented && contains_case_insensitive(
            source + line_start, content_length, "nosleep.skprx");
        int is_main = content_length - first == 5 &&
            memcmp(source + line_start + first, "*main", 5) == 0;

        if (is_nosleep) {
            const char prefix[] = "# Disabled by Caffeine installer: ";
            memcpy(output + output_size, prefix, sizeof(prefix) - 1);
            output_size += sizeof(prefix) - 1;
        }
        memcpy(output + output_size, source + line_start, line_length);
        output_size += line_length;
        output[output_size++] = '\n';

        if (is_main && !inserted_qmr) {
            const char qmr_line[] = QMR_CONFIG_LINE "\n";
            memcpy(output + output_size, qmr_line, sizeof(qmr_line) - 1);
            output_size += sizeof(qmr_line) - 1;
            inserted_qmr = 1;
        }

        if (position < source_size && source[position] == '\n')
            ++position;
    }

    if (!inserted_qmr) {
        const char section[] = "\n*main\n" QMR_CONFIG_LINE "\n";
        memcpy(output + output_size, section, sizeof(section) - 1);
        output_size += sizeof(section) - 1;
    }

    if (!path_exists(CONFIG_BACKUP_PATH) &&
        copy_file(CONFIG_PATH, CONFIG_BACKUP_PATH) < 0) {
        free(output);
        free(source);
        return -4;
    }

    FILE *temporary = fopen(CONFIG_TEMP_PATH, "wb");
    if (!temporary) {
        free(output);
        free(source);
        return -5;
    }
    int result = fwrite(output, 1, output_size, temporary) == output_size ? 0 : -6;
    fclose(temporary);

    if (result == 0) {
        sceIoRemove(CONFIG_PREVIOUS_PATH);
        if (sceIoRename(CONFIG_PATH, CONFIG_PREVIOUS_PATH) < 0) {
            result = -7;
        } else if (sceIoRename(CONFIG_TEMP_PATH, CONFIG_PATH) < 0) {
            sceIoRename(CONFIG_PREVIOUS_PATH, CONFIG_PATH);
            result = -8;
        }
    }

    if (result < 0)
        sceIoRemove(CONFIG_TEMP_PATH);
    free(output);
    free(source);
    return result;
}

static int unpatch_config(void)
{
    static const char disabled_prefix[] = "# Disabled by Caffeine installer: ";
    FILE *input = fopen(CONFIG_PATH, "rb");
    if (!input)
        return -1;

    fseek(input, 0, SEEK_END);
    long input_size = ftell(input);
    rewind(input);
    if (input_size < 0 || input_size > 1024 * 1024) {
        fclose(input);
        return -2;
    }

    char *source = malloc((size_t)input_size + 1);
    char *output = malloc((size_t)input_size + 1);
    if (!source || !output) {
        free(source);
        free(output);
        fclose(input);
        return -3;
    }

    size_t source_size = fread(source, 1, (size_t)input_size, input);
    fclose(input);
    source[source_size] = '\0';
    size_t output_size = 0;
    size_t position = 0;

    while (position < source_size) {
        size_t line_start = position;
        while (position < source_size && source[position] != '\n')
            ++position;
        size_t line_length = position - line_start;
        size_t content_length = line_length;
        if (content_length > 0 && source[line_start + content_length - 1] == '\r')
            --content_length;

        size_t first = 0;
        while (first < content_length &&
               (source[line_start + first] == ' ' || source[line_start + first] == '\t'))
            ++first;
        int commented = first < content_length && source[line_start + first] == '#';
        int is_qmr = !commented && contains_case_insensitive(
            source + line_start, content_length, "QuickMenuReborn.suprx");
        int restore_nosleep = content_length >= sizeof(disabled_prefix) - 1 &&
            memcmp(source + line_start, disabled_prefix,
                   sizeof(disabled_prefix) - 1) == 0;

        if (!is_qmr) {
            size_t copy_start = line_start;
            size_t copy_length = line_length;
            if (restore_nosleep) {
                copy_start += sizeof(disabled_prefix) - 1;
                copy_length -= sizeof(disabled_prefix) - 1;
            }
            memcpy(output + output_size, source + copy_start, copy_length);
            output_size += copy_length;
            output[output_size++] = '\n';
        }

        if (position < source_size && source[position] == '\n')
            ++position;
    }

    sceIoRemove(CONFIG_UNINSTALL_BACKUP_PATH);
    if (copy_file(CONFIG_PATH, CONFIG_UNINSTALL_BACKUP_PATH) < 0) {
        free(output);
        free(source);
        return -4;
    }

    FILE *temporary = fopen(CONFIG_TEMP_PATH, "wb");
    if (!temporary) {
        free(output);
        free(source);
        return -5;
    }
    int result = fwrite(output, 1, output_size, temporary) == output_size ? 0 : -6;
    fclose(temporary);

    if (result == 0) {
        sceIoRemove(CONFIG_PREVIOUS_PATH);
        if (sceIoRename(CONFIG_PATH, CONFIG_PREVIOUS_PATH) < 0) {
            result = -7;
        } else if (sceIoRename(CONFIG_TEMP_PATH, CONFIG_PATH) < 0) {
            sceIoRename(CONFIG_PREVIOUS_PATH, CONFIG_PATH);
            result = -8;
        }
    }

    if (result < 0)
        sceIoRemove(CONFIG_TEMP_PATH);
    free(output);
    free(source);
    return result;
}

static int install_caffeine(void)
{
    if (!path_exists(CONFIG_PATH))
        return -100;

    sceIoMkdir("ur0:QuickMenuReborn", 0777);

    if (copy_file("app0:payload/QuickMenuReborn.suprx",
                  "ur0:tai/QuickMenuReborn.suprx") < 0)
        return -101;
    if (copy_file("app0:payload/qmr_plugin.rco",
                  "ur0:QuickMenuReborn/qmr_plugin.rco") < 0)
        return -102;
    if (copy_file("app0:payload/caffeine.suprx",
                  "ur0:QuickMenuReborn/caffeine.suprx") < 0)
        return -103;
    if (copy_file("app0:payload/caffeine.png",
                  "ur0:QuickMenuReborn/caffeine.png") < 0)
        return -104;

    int result = patch_config();
    sceIoSync("ur0:", 0);
    return result;
}

static int uninstall_caffeine(void)
{
    if (!path_exists(CONFIG_PATH))
        return -200;

    int result = unpatch_config();
    if (result < 0)
        return result - 200;

    sceIoRemove("ur0:QuickMenuReborn/caffeine.suprx");
    sceIoRemove("ur0:QuickMenuReborn/caffeine.png");
    sceIoRemove("ur0:QuickMenuReborn/qmr_plugin.rco");
    sceIoRemove("ur0:tai/QuickMenuReborn.suprx");
    sceIoRmdir("ur0:QuickMenuReborn");
    sceIoSync("ur0:", 0);
    return 0;
}

static unsigned int wait_for_new_button(void)
{
    static unsigned int previous = 0;
    SceCtrlData pad;
    for (;;) {
        memset(&pad, 0, sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned int pressed = pad.buttons & ~previous;
        previous = pad.buttons;
        if (pressed)
            return pressed;
        sceKernelDelayThread(16 * 1000);
    }
}

int main(void)
{
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    psvDebugScreenInit();
    psvDebugScreenSetFgColor(0x00FFFFFF);

    printf("\n  Caffeine for Vita Installer v1.0.0\n");
    printf("  ==================================\n\n");
    printf("  This installs QuickMenuReborn and Caffeine to ur0:.\n");
    printf("  ur0:tai/config.txt will be backed up before editing.\n");
    printf("  An active NoSleep entry will be commented out.\n\n");
    printf("  X: Install     SQUARE: Uninstall     TRIANGLE: Exit\n");

    for (;;) {
        unsigned int button = wait_for_new_button();
        if (button & SCE_CTRL_TRIANGLE)
            return 0;
        if (button & SCE_CTRL_SQUARE) {
            printf("\n  Uninstalling...\n");
            int result = uninstall_caffeine();
            if (result < 0) {
                psvDebugScreenSetFgColor(0x000000FF);
                printf("  Uninstall failed: %d\n", result);
                printf("  Press CIRCLE to exit.\n");
                while (!(wait_for_new_button() & SCE_CTRL_CIRCLE)) {}
                return result;
            }
            psvDebugScreenSetFgColor(0x0000FF00);
            printf("  Caffeine and bundled QuickMenuReborn removed.\n\n");
            psvDebugScreenSetFgColor(0x00FFFFFF);
            printf("  X: Reboot now     CIRCLE: Exit\n");
            printf("  You may delete this installer bubble after exit.\n");
            for (;;) {
                button = wait_for_new_button();
                if (button & SCE_CTRL_CROSS) {
                    scePowerRequestColdReset();
                    sceKernelDelayThread(10 * 1000 * 1000);
                }
                if (button & SCE_CTRL_CIRCLE)
                    return 0;
            }
        }
        if (!(button & SCE_CTRL_CROSS))
            continue;

        printf("\n  Installing...\n");
        int result = install_caffeine();
        if (result < 0) {
            psvDebugScreenSetFgColor(0x000000FF);
            printf("  Installation failed: %d\n", result);
            printf("  No reboot was requested. Press CIRCLE to exit.\n");
            while (!(wait_for_new_button() & SCE_CTRL_CIRCLE)) {}
            return result;
        }

        psvDebugScreenSetFgColor(0x0000FF00);
        printf("  Installation completed successfully.\n\n");
        psvDebugScreenSetFgColor(0x00FFFFFF);
        printf("  X: Reboot now     CIRCLE: Exit without reboot\n");
        printf("\n  After reboot, you may delete this installer bubble\n");
        printf("  and the original VPK. Caffeine will remain installed.\n");

        for (;;) {
            button = wait_for_new_button();
            if (button & SCE_CTRL_CROSS) {
                scePowerRequestColdReset();
                sceKernelDelayThread(10 * 1000 * 1000);
            }
            if (button & SCE_CTRL_CIRCLE)
                return 0;
        }
    }
}
