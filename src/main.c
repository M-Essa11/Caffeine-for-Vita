#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/stat.h>
#include <quickmenureborn/qm_reborn.h>

#define SEPARATOR_ID "caffeine_vita_separator"
#define PLANE_ID "caffeine_vita_plane"
#define LABEL_ID "caffeine_vita_label"
#define CHECKBOX_ID "caffeine_vita_checkbox"
#define ICON_ID "caffeine_vita_icon"
#define TEXTURE_ID "caffeine_vita_cup_texture"
#define TEXTURE_PATH_UR0 "ur0:/QuickMenuReborn/caffeine.png"
#define TEXTURE_PATH_UX0 "ux0:/QuickMenuReborn/caffeine.png"

static volatile int caffeine_enabled = 0;
static volatile int worker_running = 1;
static SceUID worker_thread = -1;

int _start(SceSize argc, const void *args)
    __attribute__((weak, alias("module_start")));

static void sync_quick_menu_state(void)
{
    if (caffeine_enabled) {
        QuickMenuRebornSetWidgetColor(ICON_ID, 0.10f, 0.65f, 1.0f, 1.0f);
    } else {
        QuickMenuRebornSetWidgetColor(ICON_ID, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

ONLOAD_HANDLER(on_caffeine_widget_load)
{
    (void)id;
    sync_quick_menu_state();
}

static int caffeine_worker(SceSize args, void *argp)
{
    (void)args;
    (void)argp;

    while (worker_running) {
        if (caffeine_enabled) {
            /* Reset dimming, display-off, and automatic-suspend timers. */
            sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
        }
        sceKernelDelayThread(1000 * 1000);
    }

    return 0;
}

BUTTON_HANDLER(on_caffeine_toggle)
{
    (void)id;
    (void)hash;
    (void)event_id;
    (void)user_data;
    caffeine_enabled = QuickMenuRebornGetCheckboxValue(CHECKBOX_ID) > 0;
    sync_quick_menu_state();
}

static void unregister_widgets(void)
{
    QuickMenuRebornUnregisterWidget(CHECKBOX_ID);
    QuickMenuRebornUnregisterWidget(LABEL_ID);
    QuickMenuRebornUnregisterWidget(ICON_ID);
    QuickMenuRebornUnregisterTexture(TEXTURE_ID);
    QuickMenuRebornUnregisterWidget(PLANE_ID);
    QuickMenuRebornRemoveSeparator(SEPARATOR_ID);
}

int module_start(SceSize argc, const void *args)
{
    SceIoStat icon_stat;
    const char *texture_path = TEXTURE_PATH_UR0;

    (void)argc;
    (void)args;

    caffeine_enabled = 0;
    worker_running = 1;

    worker_thread = sceKernelCreateThread(
        "caffeine_worker", caffeine_worker, 0x10000100, 0x4000, 0, 0, NULL);
    if (worker_thread < 0 || sceKernelStartThread(worker_thread, 0, NULL) < 0) {
        return SCE_KERNEL_START_FAILED;
    }

    QuickMenuRebornSeparator(SEPARATOR_ID, SCE_SEPARATOR_HEIGHT);

    QuickMenuRebornRegisterWidget(PLANE_ID, NULL, plane);
    QuickMenuRebornSetWidgetSize(PLANE_ID, SCE_PLANE_WIDTH, 100.0f, 0.0f, 0.0f);
    QuickMenuRebornSetWidgetColor(PLANE_ID, 1.0f, 1.0f, 1.0f, 0.0f);

    if (sceIoGetstat(TEXTURE_PATH_UR0, &icon_stat) < 0) {
        texture_path = TEXTURE_PATH_UX0;
    }
    QuickMenuRebornRegisterTexture(TEXTURE_ID, texture_path);
    QuickMenuRebornRegisterWidget(ICON_ID, PLANE_ID, plane);
    QuickMenuRebornSetWidgetSize(ICON_ID, 64.0f, 64.0f, 0.0f, 0.0f);
    QuickMenuRebornSetWidgetPosition(ICON_ID, -350.0f, 0.0f, 0.0f, 0.0f);
    QuickMenuRebornSetWidgetColor(ICON_ID, 1.0f, 1.0f, 1.0f, 1.0f);
    QuickMenuRebornSetWidgetTexture(ICON_ID, TEXTURE_ID);

    QuickMenuRebornRegisterWidget(LABEL_ID, PLANE_ID, text);
    QuickMenuRebornSetWidgetSize(LABEL_ID, 500.0f, 75.0f, 0.0f, 0.0f);
    QuickMenuRebornSetWidgetPosition(LABEL_ID, -220.0f, 0.0f, 0.0f, 0.0f);
    QuickMenuRebornSetWidgetColor(LABEL_ID, 1.0f, 1.0f, 1.0f, 1.0f);
    QuickMenuRebornSetWidgetLabel(LABEL_ID, "Caffeine");

    QuickMenuRebornRegisterWidget(CHECKBOX_ID, PLANE_ID, check_box);
    QuickMenuRebornSetWidgetSize(CHECKBOX_ID, 48.0f, 48.0f, 0.0f, 0.0f);
    QuickMenuRebornSetWidgetPosition(CHECKBOX_ID, 350.0f, 0.0f, 0.0f, 0.0f);
    QuickMenuRebornSetWidgetColor(CHECKBOX_ID, 1.0f, 1.0f, 1.0f, 1.0f);
    QuickMenuRebornSaveCheckBoxState(CHECKBOX_ID, CHECKBOX_OFF);
    QuickMenuRebornAssignDefaultCheckBoxRecall(CHECKBOX_ID);
    QuickMenuRebornAssignDefaultCheckBoxSave(CHECKBOX_ID);
    QuickMenuRebornAssignOnLoadHandler(
        on_caffeine_widget_load, CHECKBOX_ID);
    QuickMenuRebornRegisterEventHanlder(
        CHECKBOX_ID, QMR_BUTTON_RELEASE_ID, on_caffeine_toggle, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
    (void)argc;
    (void)args;

    caffeine_enabled = 0;
    unregister_widgets();

    worker_running = 0;
    if (worker_thread >= 0) {
        sceKernelWaitThreadEnd(worker_thread, NULL, NULL);
        sceKernelDeleteThread(worker_thread);
        worker_thread = -1;
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
