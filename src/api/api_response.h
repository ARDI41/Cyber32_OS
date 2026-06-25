#pragma once

#include <stdint.h>

#include "../core/types/command_types.h"
#include "../core/types/motor_types.h"
#include "../core/types/runtime_state.h"
#include "../core/types/wireless_node_allowlist_records.h"
#include "../core/types/wireless_node_records.h"
#include "../core/types/wireless_node_security_diagnostics.h"
#include "../registry/capability_provider_record.h"
#include "../registry/registry_records.h"
#include "../registry/registry_result.h"

namespace Cyber32 {

static const uint8_t API_MAX_NODE_SUMMARY_COUNT = 16;
static const uint8_t API_MAX_CAPABILITY_SUMMARY_COUNT = 16;
static const uint8_t API_MAX_PROJECT_SUMMARY_COUNT = 4;
static const uint8_t API_MAX_LOGIC_RULE_SUMMARY_COUNT = 8;
static const uint8_t API_MAX_TEMPLATE_METADATA_COUNT = 8;

struct ApiSystemStatus {
    bool ok;
    RuntimeState runtime_state;
    uint8_t module_count;
    uint8_t device_count;
    uint8_t capability_count;
    const char* latest_error_code;
};

struct ApiSystemIdentity {
    bool ok;
    const char* error_code;
    const char* core_uuid;
    const char* friendly_name;
    const char* owner_state;
    const char* provisioning_state;
    const char* pairing_state;
};

struct ApiSystemFirmware {
    bool ok;
    const char* error_code;
    const char* firmware_version;
    const char* build_version;
    const char* hardware_revision;
    const char* protocol_version;
};

struct ApiSystemRuntime {
    bool ok;
    const char* error_code;
    RuntimeState runtime_state;
    uint32_t uptime_ms;
    bool safe_mode;
    bool ready;
    bool running;
};

struct ApiSystemModes {
    bool ok;
    const char* error_code;
    bool setup_mode;
    bool developer_mode;
    bool local_mode;
    bool remote_mode;
};

struct ApiSystemMemory {
    bool ok;
    const char* error_code;
    uint32_t free_heap;
    uint32_t minimum_free_heap;
    const char* registry_capacity_summary;
};

struct ApiSystemSummary {
    bool ok;
    const char* error_code;
    ApiSystemIdentity identity;
    ApiSystemFirmware firmware;
    ApiSystemRuntime runtime;
    ApiSystemModes modes;
    ApiSystemMemory memory;
};

struct ApiNodeIdentity {
    bool ok;
    const char* error_code;
    uint32_t node_id;
    const char* friendly_name;
    CapabilityProviderType provider_type;
    uint8_t source_mac[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_source_mac;
};

struct ApiNodeStatus {
    bool ok;
    const char* error_code;
    bool online;
    bool paired;
    bool trusted;
    bool blocked;
    uint32_t last_seen_ms;
    CapabilityProviderStatus provider_status;
};

struct ApiNodePower {
    bool ok;
    const char* error_code;
    uint8_t battery_percent;
    uint16_t battery_mv;
    bool has_battery_percent;
    bool has_battery_mv;
};

struct ApiNodeSignal {
    bool ok;
    const char* error_code;
    int16_t rssi;
    bool has_rssi;
    uint8_t signal_quality_percent;
};

struct ApiNodeDiagnosticsSummary {
    bool ok;
    const char* error_code;
    uint32_t accepted_packet_count;
    uint32_t rejected_packet_count;
    const char* last_error_code;
    bool has_security_diagnostics;
};

struct ApiNodeCapabilitySummary {
    bool ok;
    const char* error_code;
    uint8_t capability_count;
    const char* primary_capability_id;
    bool has_primary_capability;
};

struct ApiNodeSummary {
    bool ok;
    const char* error_code;
    ApiNodeIdentity identity;
    ApiNodeStatus status;
    ApiNodePower power;
    ApiNodeSignal signal;
    ApiNodeDiagnosticsSummary diagnostics;
    ApiNodeCapabilitySummary capabilities;
};

struct ApiNodeList {
    bool ok;
    const char* error_code;
    uint8_t count;
    ApiNodeSummary nodes[API_MAX_NODE_SUMMARY_COUNT];
};

struct ApiCapabilityIdentity {
    bool ok;
    const char* error_code;
    const char* capability_id;
    const char* friendly_name;
    const char* category;
    PayloadValueType value_type;
    const char* unit;
};

struct ApiCapabilityValue {
    bool ok;
    const char* error_code;
    PayloadValueType value_type;
    float value_float;
    int32_t value_int;
    bool value_bool;
    const char* unit;
    uint32_t timestamp_ms;
};

struct ApiCapabilityAvailability {
    bool ok;
    const char* error_code;
    Availability available;
    StaleState stale;
    uint32_t last_update_ms;
    bool has_provider;
};

struct ApiCapabilityProviderInfo {
    bool ok;
    const char* error_code;
    const char* active_provider_id;
    CapabilityProviderType provider_type;
    CapabilityProviderStatus provider_status;
    uint32_t owner_node_id;
    bool has_owner_node;
    bool selected;
};

struct ApiCapabilityQuality {
    bool ok;
    const char* error_code;
    const char* quality;
    const char* error_code_payload;
    bool has_error;
};

struct ApiCapabilitySummary {
    bool ok;
    const char* error_code;
    ApiCapabilityIdentity identity;
    ApiCapabilityValue value;
    ApiCapabilityAvailability availability;
    ApiCapabilityProviderInfo provider;
    ApiCapabilityQuality quality;
};

struct ApiCapabilityList {
    bool ok;
    const char* error_code;
    uint8_t count;
    ApiCapabilitySummary capabilities[API_MAX_CAPABILITY_SUMMARY_COUNT];
};

struct ApiCapabilityState {
    bool ok;
    CapabilityPayload payload;
    const char* error_code;
};

struct ApiServoCommandRequest {
    float position_degrees;
    uint32_t timeout_ms;
};

struct ApiServoCommandResponse {
    bool ok;
    CommandState command_state;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct ApiMotorCommandRequest {
    MotorDirection direction;
    float speed_percent;
    uint32_t timeout_ms;
};

struct ApiMotorCommandResponse {
    bool ok;
    CommandState command_state;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct ApiRelayCommandRequest {
    bool enabled;
    uint32_t timeout_ms;
};

struct ApiRelayCommandResponse {
    bool ok;
    CommandState command_state;
    bool accepted;
    bool executed;
    const char* error_code;
};

struct ApiCommandStateResponse {
    bool ok;
    CommandState command_state;
    RegistryResult registry_result;
    const char* capability_id;
    const char* error_code;
    float value_float;
    int32_t value_int;
    uint32_t timestamp_ms;
};

// Generic command contracts are future API planning placeholders only.
// They do not execute commands and do not permit bypassing Services,
// Runtime safety gates, command records, or actuator safety policy.
struct ApiGenericCommandTarget {
    bool ok;
    const char* error_code;
    const char* target_capability_id;
    uint32_t target_node_id;
    bool has_target_node;
    const char* command_type;
};

struct ApiGenericCommandSafety {
    bool ok;
    const char* error_code;
    bool requires_runtime_ready;
    bool requires_safe_mode_check;
    bool requires_emergency_stop_available;
    bool actuator_related;
    bool safety_sensitive;
};

struct ApiGenericCommandRequest {
    bool ok;
    const char* error_code;
    ApiGenericCommandTarget target;
    ApiGenericCommandSafety safety;
    PayloadValueType value_type;
    float value_float;
    int32_t value_int;
    bool value_bool;
    uint32_t timeout_ms;
    uint32_t request_timestamp_ms;
};

struct ApiGenericCommandResponse {
    bool ok;
    const char* error_code;
    bool accepted;
    bool rejected;
    CommandState command_state;
    bool safety_blocked;
    const char* reason_code;
};

struct ApiCameraCommandRequest {
    bool ok;
    const char* error_code;
    const char* target_capability_id;
    bool capture_image;
    bool start_stream;
    bool stop_stream;
};

struct ApiAudioOutputCommandRequest {
    bool ok;
    const char* error_code;
    const char* target_capability_id;
    bool play_beep;
    bool play_tone;
    uint16_t tone_hz;
    uint32_t duration_ms;
};

struct ApiDisplayCommandRequest {
    bool ok;
    const char* error_code;
    const char* target_capability_id;
    const char* display_mode;
    const char* message_id;
    bool clear_display;
};

struct ApiEmergencyStopRequest {
    bool ok;
    const char* error_code;
    const char* scope;
    uint32_t target_node_id;
    bool has_target_node;
    uint32_t request_timestamp_ms;
};

struct ApiEmergencyStopResponse {
    bool ok;
    const char* error_code;
    bool accepted;
    uint8_t affected_command_count;
    bool safe_mode_entered;
    const char* reason_code;
};

// Provider and diagnostics contract categories:
// - Provider diagnostics expose provider record state and active-provider metadata.
// - Node diagnostics expose bounded node identity, allow/trust, and last-seen metadata.
// - Wireless security diagnostics expose accepted/rejected packet counters.
// - Runtime, error, and safe-mode summaries are placeholders for future UI diagnostics.
// These contracts are read-only views. They must not parse packets, mutate Registry
// arrays, reset counters, update providers, or bypass Services/Runtime policy.
struct ApiRuntimeDiagnosticSummary {
    bool ok;
    const char* error_code;
    RuntimeState runtime_state;
    bool safe_mode;
    uint8_t task_count;
    uint8_t event_count;
};

struct ApiErrorSummary {
    bool ok;
    const char* error_code;
    uint32_t total_errors;
    const char* last_error_code;
    uint32_t last_error_timestamp_ms;
};

struct ApiSafeModeSummary {
    bool ok;
    const char* error_code;
    bool safe_mode;
    uint32_t last_entered_ms;
    uint32_t last_exit_ms;
    const char* reason_code;
};

struct ApiProviderDiagnostic {
    bool ok;
    RegistryResult registry_result;
    const char* provider_id;
    const char* capability_id;
    CapabilityProviderType provider_type;
    CapabilityProviderStatus status;
    uint8_t priority;
    uint32_t last_update_ms;
    bool active;
    CapabilityPayload latest_payload;
    const char* error_code;
};

struct ApiProviderSummary {
    bool ok;
    RegistryResult registry_result;
    uint8_t provider_count;
    uint8_t active_provider_count;
    const char* error_code;
};

struct ApiWirelessNodeDiagnostic {
    bool ok;
    RegistryResult registry_result;
    uint32_t node_id;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t last_seen_ms;
    uint32_t last_sequence_id;
    bool battery_present;
    float battery_level_percent;
    float battery_voltage;
    uint8_t signal_quality_percent;
    uint16_t checksum_reject_count;
    uint16_t duplicate_sequence_reject_count;
    uint16_t not_allowed_reject_count;
    uint16_t blocked_reject_count;
    uint16_t untrusted_reject_count;
    uint16_t invalid_packet_reject_count;
    const char* error_code;
};

struct ApiWirelessNodeSummary {
    bool ok;
    RegistryResult registry_result;
    uint8_t node_count;
    uint8_t allowed_count;
    uint8_t blocked_count;
    uint8_t unknown_count;
    const char* error_code;
};

struct ApiWirelessSecurityDiagnostic {
    bool ok;
    RegistryResult registry_result;
    uint32_t node_id;
    uint8_t mac_address[WIRELESS_MAC_ADDRESS_SIZE];
    bool has_mac_address;
    WirelessNodeAllowState allow_state;
    WirelessTrustState trust_state;
    uint32_t last_seen_ms;
    uint32_t last_accepted_sequence_id;
    uint32_t last_rejected_sequence_id;
    const char* last_error_code;
    uint16_t checksum_reject_count;
    uint16_t mac_not_allowed_reject_count;
    uint16_t mac_node_mismatch_reject_count;
    uint16_t blocked_reject_count;
    uint16_t not_allowed_reject_count;
    uint16_t untrusted_reject_count;
    uint16_t duplicate_sequence_reject_count;
    uint16_t invalid_packet_reject_count;
    uint16_t accepted_packet_count;
    const char* error_code;
};

struct ApiWirelessSecuritySummary {
    bool ok;
    RegistryResult registry_result;
    uint8_t node_count;
    uint32_t total_accepted_packets;
    uint32_t total_rejected_packets;
    uint32_t total_checksum_rejects;
    uint32_t total_mac_rejects;
    uint32_t total_duplicate_rejects;
    const char* error_code;
};

struct ApiDiagnosticsSummary {
    bool ok;
    const char* error_code;
    ApiProviderSummary provider_summary;
    ApiWirelessNodeSummary wireless_node_summary;
    ApiWirelessSecuritySummary wireless_security_summary;
    ApiRuntimeDiagnosticSummary runtime_summary;
    ApiErrorSummary error_summary;
    ApiSafeModeSummary safe_mode_summary;
};

struct ApiProjectIdentity {
    bool ok;
    const char* error_code;
    const char* project_id;
    const char* project_name;
    const char* owner_state;
    uint32_t created_timestamp_ms;
    uint32_t updated_timestamp_ms;
};

struct ApiProjectZone {
    bool ok;
    const char* error_code;
    const char* zone_id;
    const char* zone_name;
    uint8_t capability_count;
    uint8_t node_count;
};

struct ApiProjectPage {
    bool ok;
    const char* error_code;
    const char* page_id;
    const char* page_name;
    const char* page_type;
    uint8_t widget_count;
};

struct ApiProjectWidget {
    bool ok;
    const char* error_code;
    const char* widget_id;
    const char* widget_type;
    const char* bound_capability_id;
    bool safety_sensitive;
    bool command_capable;
};

struct ApiProjectTemplateRef {
    bool ok;
    const char* error_code;
    const char* template_id;
    const char* template_name;
    const char* template_version;
};

struct ApiProjectNotes {
    bool ok;
    const char* error_code;
    bool has_notes;
    uint8_t note_count;
};

struct ApiProjectSummary {
    bool ok;
    const char* error_code;
    ApiProjectIdentity identity;
    uint8_t zone_count;
    uint8_t page_count;
    uint8_t widget_count;
    uint8_t logic_rule_count;
    uint8_t template_count;
    ApiProjectNotes notes;
    uint8_t safety_sensitive_widget_count;
};

struct ApiProjectList {
    bool ok;
    const char* error_code;
    uint8_t count;
    ApiProjectSummary projects[API_MAX_PROJECT_SUMMARY_COUNT];
};

// Logic Builder contracts are declarative placeholders only. They bind to
// capability IDs and must not expose drivers, devices, modules, HAL, pins,
// provider IDs, transport IDs, Registry arrays, or arbitrary scripting.
struct ApiLogicTrigger {
    bool ok;
    const char* error_code;
    const char* trigger_id;
    const char* trigger_type;
    const char* capability_id;
    const char* operator_type;
    float threshold_float;
    int32_t threshold_int;
};

struct ApiLogicCondition {
    bool ok;
    const char* error_code;
    const char* condition_id;
    const char* capability_id;
    const char* operator_type;
    float compare_float;
    int32_t compare_int;
    bool enabled;
};

struct ApiLogicAction {
    bool ok;
    const char* error_code;
    const char* action_id;
    const char* action_type;
    const char* target_capability_id;
    bool command_capable;
    bool safety_sensitive;
};

struct ApiLogicSafety {
    bool ok;
    const char* error_code;
    bool requires_safe_mode_check;
    bool requires_runtime_ready;
    bool requires_emergency_stop;
    bool actuator_related;
};

struct ApiLogicRule {
    bool ok;
    const char* error_code;
    const char* rule_id;
    const char* rule_name;
    bool enabled;
    ApiLogicTrigger trigger;
    ApiLogicCondition condition;
    ApiLogicAction action;
    ApiLogicSafety safety;
};

struct ApiLogicRuleSummary {
    bool ok;
    const char* error_code;
    uint8_t rule_count;
    uint8_t enabled_rule_count;
    uint8_t safety_sensitive_rule_count;
};

struct ApiLogicRuleList {
    bool ok;
    const char* error_code;
    uint8_t count;
    ApiLogicRule rules[API_MAX_LOGIC_RULE_SUMMARY_COUNT];
};

struct ApiLogicValidationResult {
    bool ok;
    const char* error_code;
    bool valid;
    bool missing_capability;
    bool unsafe_action;
    bool unsupported_operator;
    const char* reason_code;
};

struct ApiLogicSimulationResult {
    bool ok;
    const char* error_code;
    bool would_trigger;
    bool would_execute_action;
    bool would_be_blocked_by_safety;
    const char* simulated_action_code;
};

// Template metadata contracts describe capability-first project starters.
// Templates are declarative metadata only; they do not execute code, generate
// projects, access hardware, or bypass API/Service safety policy.
struct ApiTemplateIdentity {
    bool ok;
    const char* error_code;
    const char* template_id;
    const char* template_name;
    const char* template_version;
    const char* template_category;
};

struct ApiTemplateCapabilityRequirement {
    bool ok;
    const char* error_code;
    const char* capability_id;
    bool required;
    bool optional;
    uint8_t count_required;
};

struct ApiTemplateSafetyRequirement {
    bool ok;
    const char* error_code;
    bool requires_emergency_stop;
    bool actuator_related;
    bool requires_safe_mode_check;
    bool requires_runtime_ready;
};

struct ApiTemplateWidgetHint {
    bool ok;
    const char* error_code;
    const char* widget_type;
    const char* bound_capability_id;
    bool command_capable;
    bool safety_sensitive;
};

struct ApiTemplateLogicHint {
    bool ok;
    const char* error_code;
    const char* trigger_capability_id;
    const char* action_capability_id;
    bool safety_sensitive;
    const char* rule_hint_type;
};

struct ApiTemplateMetadata {
    bool ok;
    const char* error_code;
    ApiTemplateIdentity identity;
    uint8_t required_capability_count;
    uint8_t optional_capability_count;
    ApiTemplateSafetyRequirement safety;
    uint8_t widget_hint_count;
    uint8_t logic_hint_count;
};

struct ApiTemplateList {
    bool ok;
    const char* error_code;
    uint8_t count;
    ApiTemplateMetadata templates[API_MAX_TEMPLATE_METADATA_COUNT];
};

}  // namespace Cyber32
