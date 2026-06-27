#include "cyber32_api.h"

#include "../core/ids/capability_ids.h"
#include "../core/ids/error_ids.h"
#include "../services/motor/motor_service.h"
#include "../services/relay/relay_service.h"
#include "../services/servo/servo_service.h"
#include "../../include/cyber32/public/public_owner_store.h"

namespace Cyber32 {

namespace {

PublicOwnerStore& apiPublicOwnerStore() {
    // Provisional empty public owner for Node/Capability API attachment.
    // A later Core context milestone can replace this with an explicit owner.
    static PublicOwnerStore store;
    return store;
}

bool readApiNodeRecord(uint8_t node_index, PublicNodeRecord& out_record) {
    return apiPublicOwnerStore().nodes().readByIndex(
        static_cast<PublicNodeIndex>(node_index),
        out_record);
}

bool readApiCapabilityRecord(uint8_t capability_index, PublicCapabilityRecord& out_record) {
    return apiPublicOwnerStore().capabilities().readByIndex(
        static_cast<PublicCapabilityIndex>(capability_index),
        out_record);
}

}  // namespace

static bool isApiTextPresent(const char* value) {
    return value != 0 && value[0] != '\0';
}

Cyber32Api::Cyber32Api()
    : registry_(0),
      runtime_(0),
      servo_service_(0),
      motor_service_(0),
      relay_service_(0) {
}

bool Cyber32Api::begin(Registry* registry, Runtime* runtime) {
    if (registry == 0 || runtime == 0) {
        return false;
    }

    registry_ = registry;
    runtime_ = runtime;
    return true;
}

void Cyber32Api::attachServoService(ServoService* service) {
    servo_service_ = service;
}

void Cyber32Api::attachMotorService(MotorService* service) {
    motor_service_ = service;
}

void Cyber32Api::attachRelayService(RelayService* service) {
    relay_service_ = service;
}

bool Cyber32Api::getSystemStatus(ApiSystemStatus& out_status) {
    if (registry_ == 0 || runtime_ == 0) {
        out_status.ok = false;
        out_status.runtime_state = RuntimeState::ERROR_STATE;
        out_status.module_count = 0;
        out_status.device_count = 0;
        out_status.capability_count = 0;
        out_status.latest_error_code = ERR_CAPABILITY_UNAVAILABLE;
        return false;
    }

    out_status.ok = true;
    out_status.runtime_state = runtime_->state();
    out_status.module_count = registry_->moduleCount();
    out_status.device_count = registry_->deviceCount();
    out_status.capability_count = registry_->capabilityCount();
    out_status.latest_error_code = "none";
    return true;
}

bool Cyber32Api::getSystemIdentity(ApiSystemIdentity& out_response) {
    out_response.ok = true;
    out_response.error_code = "none";
    out_response.core_uuid = "cyber32-core-dev";
    out_response.friendly_name = "Cyber32 Core";
    out_response.owner_state = "unprovisioned";
    out_response.provisioning_state = "development";
    out_response.pairing_state = "manual";
    return true;
}

bool Cyber32Api::getSystemFirmware(ApiSystemFirmware& out_response) {
    out_response.ok = true;
    out_response.error_code = "none";
    out_response.firmware_version = "dev";
    out_response.build_version = "dev";
    out_response.hardware_revision = "unknown";
    out_response.protocol_version = "1";
    return true;
}

bool Cyber32Api::getSystemRuntime(ApiSystemRuntime& out_response) {
    if (runtime_ == 0) {
        out_response.ok = false;
        out_response.error_code = "runtime_not_attached";
        out_response.runtime_state = RuntimeState::ERROR_STATE;
        out_response.uptime_ms = 0;
        out_response.safe_mode = false;
        out_response.ready = false;
        out_response.running = false;
        return false;
    }

    const RuntimeState runtime_state = runtime_->state();
    out_response.ok = true;
    out_response.error_code = "none";
    out_response.runtime_state = runtime_state;
    out_response.uptime_ms = 0;
    out_response.safe_mode = runtime_->isSafeMode();
    out_response.ready =
        runtime_state == RuntimeState::READY || runtime_state == RuntimeState::RUNNING;
    out_response.running = runtime_state == RuntimeState::RUNNING;
    return true;
}

bool Cyber32Api::getSystemModes(ApiSystemModes& out_response) {
    out_response.ok = true;
    out_response.error_code = "none";
    out_response.setup_mode = false;
    out_response.developer_mode = true;
    out_response.local_mode = true;
    out_response.remote_mode = false;
    return true;
}

bool Cyber32Api::getSystemMemory(ApiSystemMemory& out_response) {
    out_response.ok = true;
    out_response.error_code = "none";
    out_response.free_heap = 0;
    out_response.minimum_free_heap = 0;
    out_response.registry_capacity_summary = "not_available";
    return true;
}

bool Cyber32Api::getSystemSummary(ApiSystemSummary& out_response) {
    if (!getSystemIdentity(out_response.identity) ||
        !getSystemFirmware(out_response.firmware) ||
        !getSystemRuntime(out_response.runtime) ||
        !getSystemModes(out_response.modes) ||
        !getSystemMemory(out_response.memory)) {
        out_response.ok = false;
        out_response.error_code = "system_summary_child_failed";
        return false;
    }

    out_response.ok = true;
    out_response.error_code = "none";
    return true;
}

bool Cyber32Api::getNodeList(ApiNodeList& out_response) {
    out_response.ok = true;
    out_response.error_code = "none";
    out_response.count = apiPublicOwnerStore().nodes().count();
    return true;
}

bool Cyber32Api::getNodeSummary(uint8_t node_index, ApiNodeSummary& out_response) {
    PublicNodeRecord node_record;
    if (!readApiNodeRecord(node_index, node_record)) {
        out_response.ok = false;
        out_response.error_code = "node_not_found";
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "node_not_found";
    return false;
}

bool Cyber32Api::getNodeIdentity(uint8_t node_index, ApiNodeIdentity& out_response) {
    PublicNodeRecord node_record;
    if (!readApiNodeRecord(node_index, node_record)) {
        out_response.ok = false;
        out_response.error_code = "node_not_found";
        out_response.node_id = 0;
        out_response.friendly_name = "none";
        out_response.provider_type = CapabilityProviderType::UNKNOWN;
        for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
            out_response.source_mac[i] = 0;
        }
        out_response.has_source_mac = false;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "node_not_found";
    out_response.node_id = 0;
    out_response.friendly_name = "none";
    out_response.provider_type = CapabilityProviderType::UNKNOWN;
    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        out_response.source_mac[i] = 0;
    }
    out_response.has_source_mac = false;
    return false;
}

bool Cyber32Api::getNodeStatus(uint8_t node_index, ApiNodeStatus& out_response) {
    PublicNodeRecord node_record;
    if (!readApiNodeRecord(node_index, node_record)) {
        out_response.ok = false;
        out_response.error_code = "node_not_found";
        out_response.online = false;
        out_response.paired = false;
        out_response.trusted = false;
        out_response.blocked = false;
        out_response.last_seen_ms = 0;
        out_response.provider_status = CapabilityProviderStatus::UNKNOWN;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "node_not_found";
    out_response.online = false;
    out_response.paired = false;
    out_response.trusted = false;
    out_response.blocked = false;
    out_response.last_seen_ms = 0;
    out_response.provider_status = CapabilityProviderStatus::UNKNOWN;
    return false;
}

bool Cyber32Api::getNodePower(uint8_t node_index, ApiNodePower& out_response) {
    PublicNodeRecord node_record;
    if (!readApiNodeRecord(node_index, node_record)) {
        out_response.ok = false;
        out_response.error_code = "node_not_found";
        out_response.battery_percent = 0;
        out_response.battery_mv = 0;
        out_response.has_battery_percent = false;
        out_response.has_battery_mv = false;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "node_not_found";
    out_response.battery_percent = 0;
    out_response.battery_mv = 0;
    out_response.has_battery_percent = false;
    out_response.has_battery_mv = false;
    return false;
}

bool Cyber32Api::getNodeSignal(uint8_t node_index, ApiNodeSignal& out_response) {
    PublicNodeRecord node_record;
    if (!readApiNodeRecord(node_index, node_record)) {
        out_response.ok = false;
        out_response.error_code = "node_not_found";
        out_response.rssi = 0;
        out_response.has_rssi = false;
        out_response.signal_quality_percent = 0;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "node_not_found";
    out_response.rssi = 0;
    out_response.has_rssi = false;
    out_response.signal_quality_percent = 0;
    return false;
}

bool Cyber32Api::getNodeDiagnostics(uint8_t node_index, ApiNodeDiagnosticsSummary& out_response) {
    PublicNodeRecord node_record;
    if (!readApiNodeRecord(node_index, node_record)) {
        out_response.ok = false;
        out_response.error_code = "node_not_found";
        out_response.accepted_packet_count = 0;
        out_response.rejected_packet_count = 0;
        out_response.last_error_code = "none";
        out_response.has_security_diagnostics = false;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "node_not_found";
    out_response.accepted_packet_count = 0;
    out_response.rejected_packet_count = 0;
    out_response.last_error_code = "none";
    out_response.has_security_diagnostics = false;
    return false;
}

bool Cyber32Api::getNodeCapabilities(
    uint8_t node_index,
    ApiNodeCapabilitySummary* out_capabilities,
    uint8_t max_count,
    uint8_t& out_count) {
    out_count = 0;

    if (max_count > 0 && out_capabilities == 0) {
        return false;
    }

    PublicNodeRecord node_record;
    if (!readApiNodeRecord(node_index, node_record)) {
        return false;
    }

    return false;
}

bool Cyber32Api::getCapabilityList(ApiCapabilityList& out_response) {
    out_response.ok = true;
    out_response.error_code = "none";
    out_response.count = apiPublicOwnerStore().capabilities().count();
    if (out_response.count > API_MAX_CAPABILITY_SUMMARY_COUNT) {
        out_response.count = API_MAX_CAPABILITY_SUMMARY_COUNT;
    }
    return true;
}

bool Cyber32Api::getCapabilitySummary(
    uint8_t capability_index,
    ApiCapabilitySummary& out_response) {
    PublicCapabilityRecord capability_record;
    if (!readApiCapabilityRecord(capability_index, capability_record)) {
        out_response.ok = false;
        out_response.error_code = "capability_not_found";
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "capability_not_found";
    return false;
}

bool Cyber32Api::getCapabilityIdentity(
    uint8_t capability_index,
    ApiCapabilityIdentity& out_response) {
    PublicCapabilityRecord capability_record;
    if (!readApiCapabilityRecord(capability_index, capability_record)) {
        out_response.ok = false;
        out_response.error_code = "capability_not_found";
        out_response.capability_id = 0;
        out_response.friendly_name = 0;
        out_response.category = 0;
        out_response.value_type = PayloadValueType::NONE;
        out_response.unit = 0;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "capability_not_found";
    out_response.capability_id = 0;
    out_response.friendly_name = 0;
    out_response.category = 0;
    out_response.value_type = PayloadValueType::NONE;
    out_response.unit = 0;
    return false;
}

bool Cyber32Api::getCapabilityValue(
    uint8_t capability_index,
    ApiCapabilityValue& out_response) {
    PublicCapabilityRecord capability_record;
    if (!readApiCapabilityRecord(capability_index, capability_record)) {
        out_response.ok = false;
        out_response.error_code = "capability_not_found";
        out_response.value_type = PayloadValueType::NONE;
        out_response.value_float = 0.0F;
        out_response.value_int = 0;
        out_response.value_bool = false;
        out_response.unit = 0;
        out_response.timestamp_ms = 0;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "capability_not_found";
    out_response.value_type = PayloadValueType::NONE;
    out_response.value_float = 0.0F;
    out_response.value_int = 0;
    out_response.value_bool = false;
    out_response.unit = 0;
    out_response.timestamp_ms = 0;
    return false;
}

bool Cyber32Api::getCapabilityAvailability(
    uint8_t capability_index,
    ApiCapabilityAvailability& out_response) {
    PublicCapabilityRecord capability_record;
    if (!readApiCapabilityRecord(capability_index, capability_record)) {
        out_response.ok = false;
        out_response.error_code = "capability_not_found";
        out_response.available = Availability::UNAVAILABLE;
        out_response.stale = StaleState::STALE;
        out_response.last_update_ms = 0;
        out_response.has_provider = false;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "capability_not_found";
    out_response.available = Availability::UNAVAILABLE;
    out_response.stale = StaleState::STALE;
    out_response.last_update_ms = 0;
    out_response.has_provider = false;
    return false;
}

bool Cyber32Api::getCapabilityProviderInfo(
    uint8_t capability_index,
    ApiCapabilityProviderInfo& out_response) {
    PublicCapabilityRecord capability_record;
    if (!readApiCapabilityRecord(capability_index, capability_record)) {
        out_response.ok = false;
        out_response.error_code = "capability_not_found";
        out_response.active_provider_id = 0;
        out_response.provider_type = CapabilityProviderType::UNKNOWN;
        out_response.provider_status = CapabilityProviderStatus::UNKNOWN;
        out_response.owner_node_id = 0;
        out_response.has_owner_node = false;
        out_response.selected = false;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "capability_not_found";
    out_response.active_provider_id = 0;
    out_response.provider_type = CapabilityProviderType::UNKNOWN;
    out_response.provider_status = CapabilityProviderStatus::UNKNOWN;
    out_response.owner_node_id = 0;
    out_response.has_owner_node = false;
    out_response.selected = false;
    return false;
}

bool Cyber32Api::getCapabilityQuality(
    uint8_t capability_index,
    ApiCapabilityQuality& out_response) {
    PublicCapabilityRecord capability_record;
    if (!readApiCapabilityRecord(capability_index, capability_record)) {
        out_response.ok = false;
        out_response.error_code = "capability_not_found";
        out_response.quality = 0;
        out_response.error_code_payload = "none";
        out_response.has_error = false;
        return false;
    }

    out_response.ok = false;
    out_response.error_code = "capability_not_found";
    out_response.quality = 0;
    out_response.error_code_payload = "none";
    out_response.has_error = false;
    return false;
}

bool Cyber32Api::getTemperatureState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableTemperatureState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_TEMPERATURE, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableTemperatureState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getDistanceState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableDistanceState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_DISTANCE, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableDistanceState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getServoPositionState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableServoPositionState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_SERVO_POSITION, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableServoPositionState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getMotorControlState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableMotorControlState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_MOTOR_CONTROL, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableMotorControlState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getRelayControlState(ApiCapabilityState& out_state) {
    if (registry_ == 0) {
        fillUnavailableRelayControlState(out_state);
        return false;
    }

    CapabilityPayload payload;
    const RegistryResult result =
        registry_->getCapabilityPayloadWithResult(CAP_RELAY_CONTROL, payload);
    if (result != RegistryResult::OK) {
        fillUnavailableRelayControlState(out_state);
        return false;
    }

    out_state.ok = true;
    out_state.payload = payload;
    out_state.error_code = "none";
    return true;
}

bool Cyber32Api::getServoCommandState(ApiCommandStateResponse& out_response) {
    if (registry_ == 0) {
        fillUnavailableServoCommandState(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    CommandStateRecord record;
    const RegistryResult result = registry_->getCommandState(CAP_SERVO_POSITION, record);
    if (result != RegistryResult::OK) {
        fillUnavailableServoCommandState(out_response, result);
        return false;
    }

    out_response.ok = true;
    out_response.command_state = record.command_state;
    out_response.registry_result = record.registry_result;
    out_response.capability_id = record.capability_id;
    out_response.error_code = record.error_code;
    out_response.value_float = record.value_float;
    out_response.value_int = record.value_int;
    out_response.timestamp_ms = record.timestamp_ms;
    return true;
}

bool Cyber32Api::getMotorCommandState(ApiCommandStateResponse& out_response) {
    if (registry_ == 0) {
        fillUnavailableMotorCommandState(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    CommandStateRecord record;
    const RegistryResult result = registry_->getCommandState(CAP_MOTOR_CONTROL, record);
    if (result != RegistryResult::OK) {
        fillUnavailableMotorCommandState(out_response, result);
        return false;
    }

    out_response.ok = true;
    out_response.command_state = record.command_state;
    out_response.registry_result = record.registry_result;
    out_response.capability_id = record.capability_id;
    out_response.error_code = record.error_code;
    out_response.value_float = record.value_float;
    out_response.value_int = record.value_int;
    out_response.timestamp_ms = record.timestamp_ms;
    return true;
}

bool Cyber32Api::getRelayCommandState(ApiCommandStateResponse& out_response) {
    if (registry_ == 0) {
        fillUnavailableRelayCommandState(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    CommandStateRecord record;
    const RegistryResult result = registry_->getCommandState(CAP_RELAY_CONTROL, record);
    if (result != RegistryResult::OK) {
        fillUnavailableRelayCommandState(out_response, result);
        return false;
    }

    out_response.ok = true;
    out_response.command_state = record.command_state;
    out_response.registry_result = record.registry_result;
    out_response.capability_id = record.capability_id;
    out_response.error_code = record.error_code;
    out_response.value_float = record.value_float;
    out_response.value_int = record.value_int;
    out_response.timestamp_ms = record.timestamp_ms;
    return true;
}

bool Cyber32Api::getProviderDiagnostic(
    const char* provider_id,
    ApiProviderDiagnostic& out_response) {
    if (!isApiTextPresent(provider_id)) {
        fillUnavailableProviderDiagnostic(out_response, RegistryResult::INVALID_ID, provider_id, 0);
        return false;
    }
    if (registry_ == 0) {
        fillUnavailableProviderDiagnostic(out_response, RegistryResult::NOT_ATTACHED, provider_id, 0);
        return false;
    }

    CapabilityProviderRecord provider;
    const RegistryResult result = registry_->getCapabilityProvider(provider_id, provider);
    if (result != RegistryResult::OK) {
        fillUnavailableProviderDiagnostic(out_response, result, provider_id, 0);
        return false;
    }

    out_response.ok = true;
    out_response.registry_result = RegistryResult::OK;
    out_response.provider_id = provider.provider_id;
    out_response.capability_id = provider.capability_id;
    out_response.provider_type = provider.provider_type;
    out_response.status = provider.status;
    out_response.priority = provider.priority;
    out_response.last_update_ms = provider.last_update_ms;
    out_response.active = registry_->isActiveProvider(provider.capability_id, provider.provider_id);
    out_response.latest_payload = provider.latest_payload;
    out_response.error_code = "none";
    return true;
}

bool Cyber32Api::getActiveProviderDiagnostic(
    const char* capability_id,
    ApiProviderDiagnostic& out_response) {
    if (!isApiTextPresent(capability_id)) {
        fillUnavailableProviderDiagnostic(out_response, RegistryResult::INVALID_ID, 0, capability_id);
        return false;
    }
    if (registry_ == 0) {
        fillUnavailableProviderDiagnostic(out_response, RegistryResult::NOT_ATTACHED, 0, capability_id);
        return false;
    }

    ActiveCapabilityProvider active_provider;
    const RegistryResult result = registry_->getActiveProvider(capability_id, active_provider);
    if (result != RegistryResult::OK) {
        fillUnavailableProviderDiagnostic(out_response, result, 0, capability_id);
        return false;
    }

    return getProviderDiagnostic(active_provider.provider_id, out_response);
}

bool Cyber32Api::getProviderSummary(ApiProviderSummary& out_response) {
    if (registry_ == 0) {
        fillUnavailableProviderSummary(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    out_response.ok = true;
    out_response.registry_result = RegistryResult::OK;
    out_response.provider_count = registry_->capabilityProviderCount();
    out_response.active_provider_count = registry_->activeProviderCount();
    out_response.error_code = "none";
    return true;
}

bool Cyber32Api::getWirelessNodeDiagnostic(
    uint32_t node_id,
    ApiWirelessNodeDiagnostic& out_response) {
    if (node_id == 0) {
        fillUnavailableWirelessNodeDiagnostic(out_response, RegistryResult::INVALID_ID, node_id);
        return false;
    }
    if (registry_ == 0) {
        fillUnavailableWirelessNodeDiagnostic(out_response, RegistryResult::NOT_ATTACHED, node_id);
        return false;
    }

    WirelessNodeAllowlistRecord record;
    const RegistryResult result = registry_->getWirelessNodeAllowlistRecord(node_id, record);
    if (result != RegistryResult::OK) {
        fillUnavailableWirelessNodeDiagnostic(out_response, result, node_id);
        return false;
    }

    fillWirelessNodeDiagnosticFromRecord(record, out_response);
    return true;
}

bool Cyber32Api::getWirelessNodeDiagnosticByIndex(
    uint8_t index,
    ApiWirelessNodeDiagnostic& out_response) {
    if (registry_ == 0) {
        fillUnavailableWirelessNodeDiagnostic(out_response, RegistryResult::NOT_ATTACHED, 0);
        return false;
    }

    WirelessNodeAllowlistRecord record;
    const RegistryResult result = registry_->getWirelessNodeAllowlistRecordByIndex(index, record);
    if (result != RegistryResult::OK) {
        fillUnavailableWirelessNodeDiagnostic(out_response, result, 0);
        return false;
    }

    fillWirelessNodeDiagnosticFromRecord(record, out_response);
    return true;
}

bool Cyber32Api::getWirelessNodeSummary(ApiWirelessNodeSummary& out_response) {
    if (registry_ == 0) {
        fillUnavailableWirelessNodeSummary(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    uint8_t allowed_count = 0;
    uint8_t blocked_count = 0;
    uint8_t unknown_count = 0;
    const uint8_t node_count = registry_->wirelessNodeAllowlistCount();

    for (uint8_t i = 0; i < node_count; ++i) {
        WirelessNodeAllowlistRecord record;
        if (registry_->getWirelessNodeAllowlistRecordByIndex(i, record) != RegistryResult::OK) {
            fillUnavailableWirelessNodeSummary(out_response, RegistryResult::INTERNAL_ERROR);
            return false;
        }

        if (record.allow_state == WirelessNodeAllowState::ALLOWED) {
            ++allowed_count;
        } else if (record.allow_state == WirelessNodeAllowState::BLOCKED) {
            ++blocked_count;
        } else {
            ++unknown_count;
        }
    }

    out_response.ok = true;
    out_response.registry_result = RegistryResult::OK;
    out_response.node_count = node_count;
    out_response.allowed_count = allowed_count;
    out_response.blocked_count = blocked_count;
    out_response.unknown_count = unknown_count;
    out_response.error_code = "none";
    return true;
}

bool Cyber32Api::getWirelessSecurityDiagnostic(
    uint32_t node_id,
    ApiWirelessSecurityDiagnostic& out_response) {
    if (node_id == 0) {
        fillUnavailableWirelessSecurityDiagnostic(out_response, RegistryResult::INVALID_ID, node_id);
        return false;
    }
    if (registry_ == 0) {
        fillUnavailableWirelessSecurityDiagnostic(out_response, RegistryResult::NOT_ATTACHED, node_id);
        return false;
    }

    WirelessNodeSecurityDiagnosticRecord record;
    const RegistryResult result = registry_->getWirelessNodeSecurityDiagnostic(node_id, record);
    if (result != RegistryResult::OK) {
        fillUnavailableWirelessSecurityDiagnostic(out_response, result, node_id);
        return false;
    }

    fillWirelessSecurityDiagnosticFromRecord(record, out_response);
    return true;
}

bool Cyber32Api::getWirelessSecurityDiagnosticByIndex(
    uint8_t index,
    ApiWirelessSecurityDiagnostic& out_response) {
    if (registry_ == 0) {
        fillUnavailableWirelessSecurityDiagnostic(out_response, RegistryResult::NOT_ATTACHED, 0);
        return false;
    }

    WirelessNodeSecurityDiagnosticRecord record;
    const RegistryResult result = registry_->getWirelessNodeSecurityDiagnosticByIndex(index, record);
    if (result != RegistryResult::OK) {
        fillUnavailableWirelessSecurityDiagnostic(out_response, result, 0);
        return false;
    }

    fillWirelessSecurityDiagnosticFromRecord(record, out_response);
    return true;
}

bool Cyber32Api::getWirelessSecuritySummary(ApiWirelessSecuritySummary& out_response) {
    if (registry_ == 0) {
        fillUnavailableWirelessSecuritySummary(out_response, RegistryResult::NOT_ATTACHED);
        return false;
    }

    const uint8_t node_count = registry_->wirelessNodeSecurityDiagnosticCount();
    uint32_t total_accepted_packets = 0;
    uint32_t total_rejected_packets = 0;
    uint32_t total_checksum_rejects = 0;
    uint32_t total_mac_rejects = 0;
    uint32_t total_duplicate_rejects = 0;

    for (uint8_t i = 0; i < node_count; ++i) {
        WirelessNodeSecurityDiagnosticRecord record;
        if (registry_->getWirelessNodeSecurityDiagnosticByIndex(i, record) != RegistryResult::OK) {
            fillUnavailableWirelessSecuritySummary(out_response, RegistryResult::INTERNAL_ERROR);
            return false;
        }

        total_accepted_packets += record.accepted_packet_count;
        total_checksum_rejects += record.checksum_reject_count;
        total_mac_rejects += record.mac_not_allowed_reject_count;
        total_mac_rejects += record.mac_node_mismatch_reject_count;
        total_duplicate_rejects += record.duplicate_sequence_reject_count;
        total_rejected_packets += record.checksum_reject_count;
        total_rejected_packets += record.mac_not_allowed_reject_count;
        total_rejected_packets += record.mac_node_mismatch_reject_count;
        total_rejected_packets += record.blocked_reject_count;
        total_rejected_packets += record.not_allowed_reject_count;
        total_rejected_packets += record.untrusted_reject_count;
        total_rejected_packets += record.duplicate_sequence_reject_count;
        total_rejected_packets += record.invalid_packet_reject_count;
    }

    out_response.ok = true;
    out_response.registry_result = RegistryResult::OK;
    out_response.node_count = node_count;
    out_response.total_accepted_packets = total_accepted_packets;
    out_response.total_rejected_packets = total_rejected_packets;
    out_response.total_checksum_rejects = total_checksum_rejects;
    out_response.total_mac_rejects = total_mac_rejects;
    out_response.total_duplicate_rejects = total_duplicate_rejects;
    out_response.error_code = "none";
    return true;
}

bool Cyber32Api::commandServoPosition(
    uint32_t now_ms,
    const ApiServoCommandRequest& request,
    ApiServoCommandResponse& out_response) {
    if (servo_service_ == 0) {
        fillFailedServoCommand(out_response, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    ServoCommandRequest service_request;
    service_request.position_degrees = request.position_degrees;
    service_request.timeout_ms = request.timeout_ms;

    ServoCommandResult service_result;
    const bool success = servo_service_->setPosition(now_ms, service_request, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandMotorControl(
    uint32_t now_ms,
    const ApiMotorCommandRequest& request,
    ApiMotorCommandResponse& out_response) {
    if (motor_service_ == 0) {
        fillFailedMotorCommand(out_response, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    MotorCommandRequest service_request;
    service_request.direction = request.direction;
    service_request.speed_percent = request.speed_percent;
    service_request.timeout_ms = request.timeout_ms;

    MotorCommandResult service_result;
    const bool success = motor_service_->setMotor(now_ms, service_request, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandMotorStop(uint32_t now_ms, ApiMotorCommandResponse& out_response) {
    if (motor_service_ == 0) {
        fillFailedMotorCommand(out_response, ERR_CAPABILITY_UNAVAILABLE);
        return false;
    }

    MotorCommandResult service_result;
    const bool success = motor_service_->stop(now_ms, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandRelayControl(
    uint32_t now_ms,
    const ApiRelayCommandRequest& request,
    ApiRelayCommandResponse& out_response) {
    if (relay_service_ == 0) {
        fillFailedRelayCommand(out_response, ERR_ACTUATOR_UNAVAILABLE);
        return false;
    }

    RelayCommandRequest service_request;
    service_request.enabled = request.enabled;
    service_request.timeout_ms = request.timeout_ms;

    RelayCommandResult service_result;
    const bool success = relay_service_->setEnabled(now_ms, service_request, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

bool Cyber32Api::commandRelayOff(uint32_t now_ms, ApiRelayCommandResponse& out_response) {
    if (relay_service_ == 0) {
        fillFailedRelayCommand(out_response, ERR_ACTUATOR_UNAVAILABLE);
        return false;
    }

    RelayCommandResult service_result;
    const bool success = relay_service_->disable(now_ms, service_result);

    out_response.ok = success;
    out_response.command_state = service_result.state;
    out_response.accepted = service_result.accepted;
    out_response.executed = service_result.executed;
    out_response.error_code = service_result.error_code;
    return success;
}

void Cyber32Api::fillUnavailableTemperatureState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_TEMPERATURE;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "degree_celsius";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableDistanceState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_DISTANCE;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "meter";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableServoPositionState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_SERVO_POSITION;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "degree";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableMotorControlState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_MOTOR_CONTROL;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "percent";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_state.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableRelayControlState(ApiCapabilityState& out_state) const {
    out_state.ok = false;
    out_state.payload.capability_id = CAP_RELAY_CONTROL;
    out_state.payload.schema_version = 1;
    out_state.payload.timestamp_ms = 0;
    out_state.payload.available = Availability::UNAVAILABLE;
    out_state.payload.stale = StaleState::STALE;
    out_state.payload.value_type = PayloadValueType::NONE;
    out_state.payload.value_float = 0.0F;
    out_state.payload.value_int = 0;
    out_state.payload.unit = "boolean";
    out_state.payload.quality = "unavailable";
    out_state.payload.error_code = ERR_ACTUATOR_UNAVAILABLE;
    out_state.error_code = ERR_ACTUATOR_UNAVAILABLE;
}

void Cyber32Api::fillFailedServoCommand(
    ApiServoCommandResponse& out_response,
    const char* error_code) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.accepted = false;
    out_response.executed = false;
    out_response.error_code = error_code;
}

void Cyber32Api::fillFailedMotorCommand(
    ApiMotorCommandResponse& out_response,
    const char* error_code) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.accepted = false;
    out_response.executed = false;
    out_response.error_code = error_code;
}

void Cyber32Api::fillFailedRelayCommand(
    ApiRelayCommandResponse& out_response,
    const char* error_code) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.accepted = false;
    out_response.executed = false;
    out_response.error_code = error_code;
}

void Cyber32Api::fillUnavailableServoCommandState(
    ApiCommandStateResponse& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.registry_result = registry_result;
    out_response.capability_id = CAP_SERVO_POSITION;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_response.value_float = 0.0F;
    out_response.value_int = 0;
    out_response.timestamp_ms = 0;
}

void Cyber32Api::fillUnavailableMotorCommandState(
    ApiCommandStateResponse& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.registry_result = registry_result;
    out_response.capability_id = CAP_MOTOR_CONTROL;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_response.value_float = 0.0F;
    out_response.value_int = 0;
    out_response.timestamp_ms = 0;
}

void Cyber32Api::fillUnavailableRelayCommandState(
    ApiCommandStateResponse& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.command_state = CommandState::FAILED;
    out_response.registry_result = registry_result;
    out_response.capability_id = CAP_RELAY_CONTROL;
    out_response.error_code = ERR_ACTUATOR_UNAVAILABLE;
    out_response.value_float = 0.0F;
    out_response.value_int = 0;
    out_response.timestamp_ms = 0;
}

void Cyber32Api::fillUnavailableProviderDiagnostic(
    ApiProviderDiagnostic& out_response,
    RegistryResult registry_result,
    const char* provider_id,
    const char* capability_id) const {
    out_response.ok = false;
    out_response.registry_result = registry_result;
    out_response.provider_id = provider_id;
    out_response.capability_id = capability_id;
    out_response.provider_type = CapabilityProviderType::UNKNOWN;
    out_response.status = CapabilityProviderStatus::UNKNOWN;
    out_response.priority = 0;
    out_response.last_update_ms = 0;
    out_response.active = false;
    out_response.latest_payload.capability_id = capability_id;
    out_response.latest_payload.schema_version = 1;
    out_response.latest_payload.timestamp_ms = 0;
    out_response.latest_payload.available = Availability::UNAVAILABLE;
    out_response.latest_payload.stale = StaleState::STALE;
    out_response.latest_payload.value_type = PayloadValueType::NONE;
    out_response.latest_payload.value_float = 0.0F;
    out_response.latest_payload.value_int = 0;
    out_response.latest_payload.unit = "";
    out_response.latest_payload.quality = "unavailable";
    out_response.latest_payload.error_code = ERR_CAPABILITY_UNAVAILABLE;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableProviderSummary(
    ApiProviderSummary& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.registry_result = registry_result;
    out_response.provider_count = 0;
    out_response.active_provider_count = 0;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillWirelessNodeDiagnosticFromRecord(
    const WirelessNodeAllowlistRecord& record,
    ApiWirelessNodeDiagnostic& out_response) const {
    out_response.ok = true;
    out_response.registry_result = RegistryResult::OK;
    out_response.node_id = record.node_id;
    out_response.allow_state = record.allow_state;
    out_response.trust_state = record.trust_state;
    out_response.last_seen_ms = record.last_seen_ms;
    out_response.last_sequence_id = 0;
    out_response.battery_present = false;
    out_response.battery_level_percent = 0.0F;
    out_response.battery_voltage = 0.0F;
    out_response.signal_quality_percent = 0;
    out_response.checksum_reject_count = 0;
    out_response.duplicate_sequence_reject_count = 0;
    out_response.not_allowed_reject_count = 0;
    out_response.blocked_reject_count = 0;
    out_response.untrusted_reject_count = 0;
    out_response.invalid_packet_reject_count = 0;
    out_response.error_code = "none";
}

void Cyber32Api::fillUnavailableWirelessNodeDiagnostic(
    ApiWirelessNodeDiagnostic& out_response,
    RegistryResult registry_result,
    uint32_t node_id) const {
    out_response.ok = false;
    out_response.registry_result = registry_result;
    out_response.node_id = node_id;
    out_response.allow_state = WirelessNodeAllowState::UNKNOWN;
    out_response.trust_state = WirelessTrustState::UNKNOWN;
    out_response.last_seen_ms = 0;
    out_response.last_sequence_id = 0;
    out_response.battery_present = false;
    out_response.battery_level_percent = 0.0F;
    out_response.battery_voltage = 0.0F;
    out_response.signal_quality_percent = 0;
    out_response.checksum_reject_count = 0;
    out_response.duplicate_sequence_reject_count = 0;
    out_response.not_allowed_reject_count = 0;
    out_response.blocked_reject_count = 0;
    out_response.untrusted_reject_count = 0;
    out_response.invalid_packet_reject_count = 0;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableWirelessNodeSummary(
    ApiWirelessNodeSummary& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.registry_result = registry_result;
    out_response.node_count = 0;
    out_response.allowed_count = 0;
    out_response.blocked_count = 0;
    out_response.unknown_count = 0;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillWirelessSecurityDiagnosticFromRecord(
    const WirelessNodeSecurityDiagnosticRecord& record,
    ApiWirelessSecurityDiagnostic& out_response) const {
    out_response.ok = true;
    out_response.registry_result = RegistryResult::OK;
    out_response.node_id = record.node_id;
    for (uint8_t i = 0; i < WIRELESS_MAC_ADDRESS_SIZE; ++i) {
        out_response.mac_address[i] = record.mac_address[i];
    }
    out_response.has_mac_address = record.has_mac_address;
    out_response.allow_state = record.allow_state;
    out_response.trust_state = record.trust_state;
    out_response.last_seen_ms = record.last_seen_ms;
    out_response.last_accepted_sequence_id = record.last_accepted_sequence_id;
    out_response.last_rejected_sequence_id = record.last_rejected_sequence_id;
    out_response.last_error_code = record.last_error_code;
    out_response.checksum_reject_count = record.checksum_reject_count;
    out_response.mac_not_allowed_reject_count = record.mac_not_allowed_reject_count;
    out_response.mac_node_mismatch_reject_count = record.mac_node_mismatch_reject_count;
    out_response.blocked_reject_count = record.blocked_reject_count;
    out_response.not_allowed_reject_count = record.not_allowed_reject_count;
    out_response.untrusted_reject_count = record.untrusted_reject_count;
    out_response.duplicate_sequence_reject_count = record.duplicate_sequence_reject_count;
    out_response.invalid_packet_reject_count = record.invalid_packet_reject_count;
    out_response.accepted_packet_count = record.accepted_packet_count;
    out_response.error_code = "none";
}

void Cyber32Api::fillUnavailableWirelessSecurityDiagnostic(
    ApiWirelessSecurityDiagnostic& out_response,
    RegistryResult registry_result,
    uint32_t node_id) const {
    out_response.ok = false;
    out_response.registry_result = registry_result;
    out_response.node_id = node_id;
    clearWirelessMacAddress(out_response.mac_address);
    out_response.has_mac_address = false;
    out_response.allow_state = WirelessNodeAllowState::UNKNOWN;
    out_response.trust_state = WirelessTrustState::UNKNOWN;
    out_response.last_seen_ms = 0;
    out_response.last_accepted_sequence_id = 0;
    out_response.last_rejected_sequence_id = 0;
    out_response.last_error_code = "none";
    out_response.checksum_reject_count = 0;
    out_response.mac_not_allowed_reject_count = 0;
    out_response.mac_node_mismatch_reject_count = 0;
    out_response.blocked_reject_count = 0;
    out_response.not_allowed_reject_count = 0;
    out_response.untrusted_reject_count = 0;
    out_response.duplicate_sequence_reject_count = 0;
    out_response.invalid_packet_reject_count = 0;
    out_response.accepted_packet_count = 0;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

void Cyber32Api::fillUnavailableWirelessSecuritySummary(
    ApiWirelessSecuritySummary& out_response,
    RegistryResult registry_result) const {
    out_response.ok = false;
    out_response.registry_result = registry_result;
    out_response.node_count = 0;
    out_response.total_accepted_packets = 0;
    out_response.total_rejected_packets = 0;
    out_response.total_checksum_rejects = 0;
    out_response.total_mac_rejects = 0;
    out_response.total_duplicate_rejects = 0;
    out_response.error_code = ERR_CAPABILITY_UNAVAILABLE;
}

}  // namespace Cyber32
