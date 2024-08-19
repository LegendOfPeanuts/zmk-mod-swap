#define DT_DRV_COMPAT zmk_behavior_mod_swap

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keys.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_mod_swap_config {
    uint8_t index;
    uint8_t first_mod;
    uint8_t second_mod;
};

bool is_mod_swap_active = false;
static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static void toggle_mod_swap() {
    is_mod_swap_active = !is_mod_swap_active;
    LOG_INF("[Mod Swap] Mod swap is now %s", is_mod_swap_active ? "Active" : "Inactive");
}

static void swap_mods(const struct behavior_mod_swap_config *config, struct zmk_modifiers_state_changed *ev) {
    LOG_INF("[Mod Swap] Swapping mods... %s", "");
    uint8_t mods = ev->modifiers;
    bool first_mod_on = mods & config->first_mod;
    LOG_INF("[Mod Swap] First mod: %d", config->first_mod);
    LOG_INF("[Mod Swap] Second mod: %d", config->second_mod);
    LOG_INF("[Mod Swap] Mods: %d", mods);
    bool second_mod_on = mods & config->second_mod;
    LOG_INF("[Mod Swap] First mod is %s", first_mod_on ? "ON" : "OFF");
    LOG_INF("[Mod Swap] Second mod is %s", second_mod_on ? "ON" : "OFF");
    if (first_mod_on && !second_mod_on) {
        LOG_INF("[Mod Swap] First mod on, swapping %s", "");
        mods &= config->first_mod ^ 1;
        mods |= config->second_mod;
        ev->modifiers = mods;
    } else if (!first_mod_on && second_mod_on) {
        LOG_INF("[Mod Swap] Second mod on, swapping %s", "");
        mods &= config->second_mod ^ 1;
        mods |= config->first_mod;
        ev->modifiers = mods;
    }
}

static int on_mod_swap_binding_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);

    toggle_mod_swap(dev);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_mod_swap_binding_released(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_mod_swap_driver_api = {
    .binding_pressed = on_mod_swap_binding_pressed,
    .binding_released = on_mod_swap_binding_released,
};

static int mod_swap_modifiers_state_changed_callback(const zmk_event_t *eh) {
    struct zmk_modifiers_state_changed *ev = as_zmk_modifiers_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
        const struct device *dev = devs[i];
        if (dev == NULL) {
            continue;
        }
        if (is_mod_swap_active) {
            swap_mods(dev->config, ev);
        } 
    }
    return ZMK_EV_EVENT_BUBBLE;

}

static int behavior_mod_swap_init(const struct device *dev) {
    const struct behavior_mod_swap_config *config = dev->config;
    devs[config->index] = dev;
    return 0;
}

ZMK_LISTENER(behavior_mod_swap, mod_swap_modifiers_state_changed_callback);
ZMK_SUBSCRIPTION(behavior_mod_swap, zmk_modifiers_state_changed);

#define KP_INST(n)                                                          \
    static struct behavior_mod_swap_config behavior_mod_swap_config_##n = { \
        .index = n,                                                         \
        .first_mod = DT_INST_PROP_OR(n, first_mod, MOD_LCTL),               \
        .second_mod = DT_INST_PROP_OR(n, second_mod, MOD_LGUI),             \
    };                                                                      \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_mod_swap_init, NULL, NULL,          \
                         &behavior_mod_swap_config_##n, POST_KERNEL,        \
                         CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,               \
                         &behavior_mod_swap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KP_INST)


