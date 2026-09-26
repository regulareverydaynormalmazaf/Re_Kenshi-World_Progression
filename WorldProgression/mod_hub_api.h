#ifndef EMC_MOD_HUB_API_H
#define EMC_MOD_HUB_API_H

#include <stddef.h>
#include <stdint.h>

#if !defined(__cdecl)
#define __cdecl
#endif

#if defined(_WIN32) && defined(EMC_MOD_HUB_BUILD_DLL)
#define EMC_MOD_HUB_API __declspec(dllexport)
#else
#define EMC_MOD_HUB_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define EMC_HUB_API_VERSION_1 ((uint32_t)1u)
#define EMC_MOD_HUB_GET_API_EXPORT_NAME "EMC_ModHub_GetApi"
#define EMC_MOD_HUB_GET_API_COMPAT_EXPORT_NAME "EMC_ModHub_GetApi_v1_compat"
#define EMC_MOD_HUB_GET_API_COMPAT_REMOVAL_TARGET "v1.2.0"

typedef int32_t EMC_Result;

#define EMC_OK ((EMC_Result)0)
#define EMC_ERR_INVALID_ARGUMENT ((EMC_Result)1)
#define EMC_ERR_UNSUPPORTED_VERSION ((EMC_Result)2)
#define EMC_ERR_API_SIZE_MISMATCH ((EMC_Result)3)
#define EMC_ERR_CONFLICT ((EMC_Result)4)
#define EMC_ERR_NOT_FOUND ((EMC_Result)5)
#define EMC_ERR_CALLBACK_FAILED ((EMC_Result)6)
#define EMC_ERR_INTERNAL ((EMC_Result)7)

#define EMC_KEY_UNBOUND ((int32_t)-1)

#define EMC_KEYBIND_MODIFIER_CTRL_MASK ((uint32_t)(1u << 0))
#define EMC_KEYBIND_MODIFIER_SHIFT_MASK ((uint32_t)(1u << 1))
#define EMC_KEYBIND_MODIFIER_ALT_MASK ((uint32_t)(1u << 2))
#define EMC_KEYBIND_MODIFIER_SUPPORTED_MASK \
    ((uint32_t)(EMC_KEYBIND_MODIFIER_CTRL_MASK | EMC_KEYBIND_MODIFIER_SHIFT_MASK | EMC_KEYBIND_MODIFIER_ALT_MASK))

#define EMC_ACTION_FORCE_REFRESH ((uint32_t)(1u << 0))

#define EMC_FLOAT_DISPLAY_DECIMALS_DEFAULT ((uint32_t)3u)
#define EMC_COLOR_PREVIEW_KIND_SWATCH ((uint32_t)0u)
#define EMC_COLOR_PREVIEW_KIND_TEXT ((uint32_t)1u)

typedef struct EMC_ModHandle_t* EMC_ModHandle;

typedef struct EMC_KeybindValueV1
{
    int32_t keycode;
    uint32_t modifiers;
} EMC_KeybindValueV1;

typedef EMC_Result(__cdecl* EMC_GetBoolCallback)(void* user_data, int32_t* out_value);
typedef EMC_Result(__cdecl* EMC_SetBoolCallback)(void* user_data, int32_t value, char* err_buf, uint32_t err_buf_size);

typedef EMC_Result(__cdecl* EMC_GetKeybindCallback)(void* user_data, EMC_KeybindValueV1* out_value);
typedef EMC_Result(__cdecl* EMC_SetKeybindCallback)(void* user_data, EMC_KeybindValueV1 value, char* err_buf, uint32_t err_buf_size);

typedef EMC_Result(__cdecl* EMC_GetIntCallback)(void* user_data, int32_t* out_value);
typedef EMC_Result(__cdecl* EMC_SetIntCallback)(void* user_data, int32_t value, char* err_buf, uint32_t err_buf_size);

typedef EMC_Result(__cdecl* EMC_GetFloatCallback)(void* user_data, float* out_value);
typedef EMC_Result(__cdecl* EMC_SetFloatCallback)(void* user_data, float value, char* err_buf, uint32_t err_buf_size);

typedef EMC_Result(__cdecl* EMC_GetSelectCallback)(void* user_data, int32_t* out_value);
typedef EMC_Result(__cdecl* EMC_SetSelectCallback)(void* user_data, int32_t value, char* err_buf, uint32_t err_buf_size);

typedef EMC_Result(__cdecl* EMC_GetTextCallback)(void* user_data, char* out_value, uint32_t out_value_size);
typedef EMC_Result(__cdecl* EMC_SetTextCallback)(void* user_data, const char* value, char* err_buf, uint32_t err_buf_size);

typedef EMC_Result(__cdecl* EMC_ActionRowCallback)(void* user_data, char* err_buf, uint32_t err_buf_size);
typedef void(__cdecl* EMC_OptionsWindowInitObserverFn)(void* user_data);

typedef struct EMC_ModDescriptorV1
{
    const char* namespace_id;
    const char* namespace_display_name;
    const char* mod_id;
    const char* mod_display_name;
    void* mod_user_data;
} EMC_ModDescriptorV1;

typedef struct EMC_BoolSettingDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    EMC_GetBoolCallback get_value;
    EMC_SetBoolCallback set_value;
} EMC_BoolSettingDefV1;

typedef struct EMC_BoolSettingDefV2
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    EMC_GetBoolCallback get_value;
    EMC_SetBoolCallback set_value;
    const char* hover_hint;
} EMC_BoolSettingDefV2;

typedef struct EMC_KeybindSettingDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    EMC_GetKeybindCallback get_value;
    EMC_SetKeybindCallback set_value;
} EMC_KeybindSettingDefV1;

typedef struct EMC_KeybindSettingDefV2
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    EMC_GetKeybindCallback get_value;
    EMC_SetKeybindCallback set_value;
    const char* hover_hint;
} EMC_KeybindSettingDefV2;

typedef struct EMC_IntSettingDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    int32_t min_value;
    int32_t max_value;
    int32_t step;
    EMC_GetIntCallback get_value;
    EMC_SetIntCallback set_value;
} EMC_IntSettingDefV1;

typedef struct EMC_IntSettingDefV2
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    int32_t min_value;
    int32_t max_value;
    int32_t step;
    int32_t dec_button_deltas[3];
    int32_t inc_button_deltas[3];
    EMC_GetIntCallback get_value;
    EMC_SetIntCallback set_value;
} EMC_IntSettingDefV2;

typedef struct EMC_FloatSettingDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    float min_value;
    float max_value;
    float step;
    uint32_t display_decimals;
    EMC_GetFloatCallback get_value;
    EMC_SetFloatCallback set_value;
} EMC_FloatSettingDefV1;

typedef struct EMC_SelectOptionV1
{
    int32_t value;
    const char* label;
} EMC_SelectOptionV1;

typedef struct EMC_SelectSettingDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    const EMC_SelectOptionV1* options;
    uint32_t option_count;
    EMC_GetSelectCallback get_value;
    EMC_SetSelectCallback set_value;
} EMC_SelectSettingDefV1;

typedef struct EMC_SelectSettingDefV2
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    const EMC_SelectOptionV1* options;
    uint32_t option_count;
    EMC_GetSelectCallback get_value;
    EMC_SetSelectCallback set_value;
    const char* hover_hint;
} EMC_SelectSettingDefV2;

typedef struct EMC_TextSettingDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    uint32_t max_length;
    EMC_GetTextCallback get_value;
    EMC_SetTextCallback set_value;
} EMC_TextSettingDefV1;

typedef struct EMC_TextSettingDefV2
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    uint32_t max_length;
    EMC_GetTextCallback get_value;
    EMC_SetTextCallback set_value;
    const char* hover_hint;
} EMC_TextSettingDefV2;

typedef struct EMC_ColorPresetV1
{
    const char* value_hex;
    const char* label;
} EMC_ColorPresetV1;

typedef struct EMC_ColorSettingDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    uint32_t preview_kind;
    const EMC_ColorPresetV1* presets;
    uint32_t preset_count;
    EMC_GetTextCallback get_value;
    EMC_SetTextCallback set_value;
} EMC_ColorSettingDefV1;

typedef struct EMC_ActionRowDefV1
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    uint32_t action_flags;
    EMC_ActionRowCallback on_action;
} EMC_ActionRowDefV1;

typedef struct EMC_ActionRowDefV2
{
    const char* setting_id;
    const char* label;
    const char* description;
    void* user_data;
    uint32_t action_flags;
    EMC_ActionRowCallback on_action;
    const char* hover_hint;
} EMC_ActionRowDefV2;

typedef enum EMC_BoolConditionEffectV1
{
    EMC_BOOL_CONDITION_EFFECT_HIDE = 0u,
    EMC_BOOL_CONDITION_EFFECT_DISABLE = 1u
} EMC_BoolConditionEffectV1;

typedef struct EMC_BoolConditionRuleDefV1
{
    const char* target_setting_id;
    const char* controller_setting_id;
    uint32_t effect;
    int32_t expected_bool_value;
} EMC_BoolConditionRuleDefV1;

typedef struct EMC_SettingSectionDefV1
{
    const char* setting_id;
    const char* section_id;
    const char* section_display_name;
} EMC_SettingSectionDefV1;

typedef struct EMC_HubApiV1
{
    uint32_t api_version;
    uint32_t api_size;
    EMC_Result(__cdecl* register_mod)(const EMC_ModDescriptorV1* desc, EMC_ModHandle* out_handle);
    EMC_Result(__cdecl* register_bool_setting)(EMC_ModHandle mod, const EMC_BoolSettingDefV1* def);
    EMC_Result(__cdecl* register_keybind_setting)(EMC_ModHandle mod, const EMC_KeybindSettingDefV1* def);
    EMC_Result(__cdecl* register_int_setting)(EMC_ModHandle mod, const EMC_IntSettingDefV1* def);
    EMC_Result(__cdecl* register_float_setting)(EMC_ModHandle mod, const EMC_FloatSettingDefV1* def);
    EMC_Result(__cdecl* register_action_row)(EMC_ModHandle mod, const EMC_ActionRowDefV1* def);
    EMC_Result(__cdecl* register_options_window_init_observer)(EMC_OptionsWindowInitObserverFn observer_fn, void* user_data);
    EMC_Result(__cdecl* unregister_options_window_init_observer)(EMC_OptionsWindowInitObserverFn observer_fn, void* user_data);
    EMC_Result(__cdecl* register_int_setting_v2)(EMC_ModHandle mod, const EMC_IntSettingDefV2* def);
    EMC_Result(__cdecl* register_select_setting)(EMC_ModHandle mod, const EMC_SelectSettingDefV1* def);
    EMC_Result(__cdecl* register_text_setting)(EMC_ModHandle mod, const EMC_TextSettingDefV1* def);
    EMC_Result(__cdecl* register_color_setting)(EMC_ModHandle mod, const EMC_ColorSettingDefV1* def);
    EMC_Result(__cdecl* register_setting_section)(EMC_ModHandle mod, const EMC_SettingSectionDefV1* def);
    EMC_Result(__cdecl* register_bool_setting_v2)(EMC_ModHandle mod, const EMC_BoolSettingDefV2* def);
    EMC_Result(__cdecl* register_keybind_setting_v2)(EMC_ModHandle mod, const EMC_KeybindSettingDefV2* def);
    EMC_Result(__cdecl* register_select_setting_v2)(EMC_ModHandle mod, const EMC_SelectSettingDefV2* def);
    EMC_Result(__cdecl* register_text_setting_v2)(EMC_ModHandle mod, const EMC_TextSettingDefV2* def);
    EMC_Result(__cdecl* register_action_row_v2)(EMC_ModHandle mod, const EMC_ActionRowDefV2* def);
    EMC_Result(__cdecl* register_bool_condition_rule)(EMC_ModHandle mod, const EMC_BoolConditionRuleDefV1* def);
} EMC_HubApiV1;

#define EMC_HUB_API_V1_MIN_SIZE ((uint32_t)56u)
#define EMC_HUB_API_V1_OPTIONS_WINDOW_INIT_OBSERVER_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, unregister_options_window_init_observer) + sizeof(void*)))
#define EMC_HUB_API_V1_INT_SETTING_V2_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_int_setting_v2) + sizeof(void*)))
#define EMC_HUB_API_V1_SELECT_SETTING_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_select_setting) + sizeof(void*)))
#define EMC_HUB_API_V1_TEXT_SETTING_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_text_setting) + sizeof(void*)))
#define EMC_HUB_API_V1_COLOR_SETTING_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_color_setting) + sizeof(void*)))
#define EMC_HUB_API_V1_SETTING_SECTION_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_setting_section) + sizeof(void*)))
#define EMC_HUB_API_V1_BOOL_SETTING_V2_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_bool_setting_v2) + sizeof(void*)))
#define EMC_HUB_API_V1_KEYBIND_SETTING_V2_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_keybind_setting_v2) + sizeof(void*)))
#define EMC_HUB_API_V1_SELECT_SETTING_V2_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_select_setting_v2) + sizeof(void*)))
#define EMC_HUB_API_V1_TEXT_SETTING_V2_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_text_setting_v2) + sizeof(void*)))
#define EMC_HUB_API_V1_ACTION_ROW_V2_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_action_row_v2) + sizeof(void*)))
#define EMC_HUB_API_V1_BOOL_CONDITION_RULE_MIN_SIZE \
    ((uint32_t)(offsetof(EMC_HubApiV1, register_bool_condition_rule) + sizeof(void*)))

EMC_MOD_HUB_API EMC_Result __cdecl EMC_ModHub_GetApi(
    uint32_t requested_version,
    uint32_t caller_api_size,
    const EMC_HubApiV1** out_api,
    uint32_t* out_api_size);

/* Temporary compatibility alias. Scheduled for removal after EMC_MOD_HUB_GET_API_COMPAT_REMOVAL_TARGET. */
EMC_MOD_HUB_API EMC_Result __cdecl EMC_ModHub_GetApi_v1_compat(
    uint32_t requested_version,
    uint32_t caller_api_size,
    const EMC_HubApiV1** out_api,
    uint32_t* out_api_size);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define EMC_ABI_STATIC_ASSERT(expr, msg) static_assert((expr), msg)
#else
#define EMC_ABI_STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
#endif

#define EMC_ABI_ASSERT_SIZE(type_name, expected_size) \
    EMC_ABI_STATIC_ASSERT(sizeof(type_name) == (expected_size), #type_name " has unexpected size")

#define EMC_ABI_ASSERT_OFFSET(type_name, field_name, expected_offset) \
    EMC_ABI_STATIC_ASSERT(offsetof(type_name, field_name) == (expected_offset), #type_name "." #field_name " has unexpected offset")

EMC_ABI_STATIC_ASSERT(sizeof(void*) == 8, "EMC SDK v1 requires 64-bit builds.");

EMC_ABI_ASSERT_SIZE(EMC_KeybindValueV1, 8);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindValueV1, keycode, 0);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindValueV1, modifiers, 4);

EMC_ABI_ASSERT_SIZE(EMC_ModDescriptorV1, 40);
EMC_ABI_ASSERT_OFFSET(EMC_ModDescriptorV1, namespace_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_ModDescriptorV1, namespace_display_name, 8);
EMC_ABI_ASSERT_OFFSET(EMC_ModDescriptorV1, mod_id, 16);
EMC_ABI_ASSERT_OFFSET(EMC_ModDescriptorV1, mod_display_name, 24);
EMC_ABI_ASSERT_OFFSET(EMC_ModDescriptorV1, mod_user_data, 32);

EMC_ABI_ASSERT_SIZE(EMC_BoolSettingDefV1, 48);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV1, get_value, 32);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV1, set_value, 40);

EMC_ABI_ASSERT_SIZE(EMC_BoolSettingDefV2, 56);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV2, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV2, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV2, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV2, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV2, get_value, 32);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV2, set_value, 40);
EMC_ABI_ASSERT_OFFSET(EMC_BoolSettingDefV2, hover_hint, 48);

EMC_ABI_ASSERT_SIZE(EMC_KeybindSettingDefV1, 48);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV1, get_value, 32);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV1, set_value, 40);

EMC_ABI_ASSERT_SIZE(EMC_KeybindSettingDefV2, 56);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV2, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV2, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV2, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV2, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV2, get_value, 32);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV2, set_value, 40);
EMC_ABI_ASSERT_OFFSET(EMC_KeybindSettingDefV2, hover_hint, 48);

EMC_ABI_ASSERT_SIZE(EMC_IntSettingDefV1, 64);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, min_value, 32);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, max_value, 36);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, step, 40);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, get_value, 48);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV1, set_value, 56);

EMC_ABI_ASSERT_SIZE(EMC_IntSettingDefV2, 88);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, min_value, 32);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, max_value, 36);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, step, 40);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, dec_button_deltas, 44);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, inc_button_deltas, 56);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, get_value, 72);
EMC_ABI_ASSERT_OFFSET(EMC_IntSettingDefV2, set_value, 80);

EMC_ABI_ASSERT_SIZE(EMC_FloatSettingDefV1, 64);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, min_value, 32);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, max_value, 36);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, step, 40);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, display_decimals, 44);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, get_value, 48);
EMC_ABI_ASSERT_OFFSET(EMC_FloatSettingDefV1, set_value, 56);

EMC_ABI_ASSERT_SIZE(EMC_SelectOptionV1, 16);
EMC_ABI_ASSERT_OFFSET(EMC_SelectOptionV1, value, 0);
EMC_ABI_ASSERT_OFFSET(EMC_SelectOptionV1, label, 8);

EMC_ABI_ASSERT_SIZE(EMC_SelectSettingDefV1, 64);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, options, 32);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, option_count, 40);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, get_value, 48);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV1, set_value, 56);

EMC_ABI_ASSERT_SIZE(EMC_SelectSettingDefV2, 72);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, options, 32);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, option_count, 40);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, get_value, 48);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, set_value, 56);
EMC_ABI_ASSERT_OFFSET(EMC_SelectSettingDefV2, hover_hint, 64);

EMC_ABI_ASSERT_SIZE(EMC_TextSettingDefV1, 56);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV1, max_length, 32);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV1, get_value, 40);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV1, set_value, 48);

EMC_ABI_ASSERT_SIZE(EMC_TextSettingDefV2, 64);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, max_length, 32);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, get_value, 40);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, set_value, 48);
EMC_ABI_ASSERT_OFFSET(EMC_TextSettingDefV2, hover_hint, 56);

EMC_ABI_ASSERT_SIZE(EMC_ColorPresetV1, 16);
EMC_ABI_ASSERT_OFFSET(EMC_ColorPresetV1, value_hex, 0);
EMC_ABI_ASSERT_OFFSET(EMC_ColorPresetV1, label, 8);

EMC_ABI_ASSERT_SIZE(EMC_ColorSettingDefV1, 72);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, preview_kind, 32);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, presets, 40);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, preset_count, 48);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, get_value, 56);
EMC_ABI_ASSERT_OFFSET(EMC_ColorSettingDefV1, set_value, 64);

EMC_ABI_ASSERT_SIZE(EMC_ActionRowDefV1, 48);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV1, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV1, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV1, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV1, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV1, action_flags, 32);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV1, on_action, 40);

EMC_ABI_ASSERT_SIZE(EMC_ActionRowDefV2, 56);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV2, setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV2, label, 8);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV2, description, 16);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV2, user_data, 24);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV2, action_flags, 32);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV2, on_action, 40);
EMC_ABI_ASSERT_OFFSET(EMC_ActionRowDefV2, hover_hint, 48);

EMC_ABI_ASSERT_SIZE(EMC_BoolConditionRuleDefV1, 24);
EMC_ABI_ASSERT_OFFSET(EMC_BoolConditionRuleDefV1, target_setting_id, 0);
EMC_ABI_ASSERT_OFFSET(EMC_BoolConditionRuleDefV1, controller_setting_id, 8);
EMC_ABI_ASSERT_OFFSET(EMC_BoolConditionRuleDefV1, effect, 16);
EMC_ABI_ASSERT_OFFSET(EMC_BoolConditionRuleDefV1, expected_bool_value, 20);

EMC_ABI_ASSERT_SIZE(EMC_HubApiV1, 160);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, api_version, 0);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, api_size, 4);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_mod, 8);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_bool_setting, 16);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_keybind_setting, 24);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_int_setting, 32);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_float_setting, 40);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_action_row, 48);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_options_window_init_observer, 56);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, unregister_options_window_init_observer, 64);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_int_setting_v2, 72);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_select_setting, 80);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_text_setting, 88);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_color_setting, 96);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_setting_section, 104);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_bool_setting_v2, 112);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_keybind_setting_v2, 120);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_select_setting_v2, 128);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_text_setting_v2, 136);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_action_row_v2, 144);
EMC_ABI_ASSERT_OFFSET(EMC_HubApiV1, register_bool_condition_rule, 152);

#undef EMC_ABI_ASSERT_OFFSET
#undef EMC_ABI_ASSERT_SIZE
#undef EMC_ABI_STATIC_ASSERT

#endif
