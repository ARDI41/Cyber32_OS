# Validation

Validation contains shared helpers and rules for checking Cyber32 data before it is accepted by the system.

Validation supports the documentation-first and capability-first architecture by making schemas and contracts enforceable.

## Responsibility

- Validate capability records
- Validate module metadata
- Validate registry records
- Validate configuration values
- Validate API payloads
- Validate logic definitions
- Validate user-provided data

## Examples

- Check that a capability ID starts with `CAP_`
- Check that a module metadata record declares its PNP level
- Check that a registry record has a stable ID
- Check that a logic rule uses capability IDs instead of module names
- Check that numeric values are inside allowed ranges
- Check that required fields are present before registration

## Validation Does Not Own

- Hardware control
- Plug-and-play discovery
- Registry storage
- Runtime execution
- Dashboard rendering
- Application logic decisions

These responsibilities belong to other Cyber32 layers.

## Relationship To Other Layers

```text
Schemas / Standards
-> Validation
-> PNP / Registry / Logic / API
```

Validation should be reusable by multiple layers without becoming the owner of those layers.

## Goal

Provide one consistent way to reject invalid data early, protect architecture boundaries, and keep Cyber32 capability-first.
