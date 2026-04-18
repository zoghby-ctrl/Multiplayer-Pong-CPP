# Common shared layer

`Common/include/protocol.h` is the single source of truth for data exchanged between client and server.

## Contribution rules

- Keep protocol structs, enums, and shared constants in `protocol.h`.
- Do not hardcode shared gameplay/protocol constants in `Client` or `Server`.
- Any protocol layout change must be coordinated across both client and server.
- Rebuild both client and server after protocol updates.
