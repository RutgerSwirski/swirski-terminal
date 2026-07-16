# Swirski BLE Framing

## Purpose

BLE can only send a limited number of bytes in a single write or notification.

Swirski protocol messages may be larger than this limit, especially notification messages containing titles, body text, or other data.

BLE framing splits one complete JSON message into smaller chunks before sending it.

The receiver then joins the chunks back together before passing the complete JSON to the Swirski protocol parser.

```text
JSON message
    ↓
UTF-8 bytes
    ↓
split into BLE frames
    ↓
send frames
    ↓
reassemble frames
    ↓
complete JSON message
```

The shared Swirski protocol should never receive partial chunks.

---

## Terminology

### Message

A complete UTF-8 JSON protocol message.

Example:

```json
{
  "version": 1,
  "type": "ping",
  "id": "mobile-123"
}
```

### Frame

One small BLE packet containing:

- A framing header
- A section of the complete message

### Transfer

The process of sending one complete message using one or more frames.

---

## Frame format

Each BLE frame begins with a four-byte header.

| Bytes | Field       | Type      | Description                             |
| ----- | ----------- | --------- | --------------------------------------- |
| 0–1   | Transfer ID | `uint16`  | Identifies the current transfer         |
| 2     | Chunk index | `uint8`   | Position of this chunk, starting at `0` |
| 3     | Chunk count | `uint8`   | Total number of chunks in the transfer  |
| 4+    | Payload     | Raw bytes | Part of the UTF-8 JSON message          |

Multi-byte integers use little-endian byte order.

Example transfer ID:

```text
Transfer ID: 42
Hex value:   0x002A
Wire bytes:  2A 00
```

---

## Example

Assume a message is too large and must be split into two chunks.

The transfer ID is `42`.

### First frame

```text
2A 00 00 02 [first section of JSON]
│     │  │
│     │  └── total chunks: 2
│     └───── chunk index: 0
└─────────── transfer ID: 42
```

### Second frame

```text
2A 00 01 02 [remaining section of JSON]
│     │  │
│     │  └── total chunks: 2
│     └───── chunk index: 1
└─────────── transfer ID: 42
```

After both frames arrive, the receiver joins the payload bytes:

```text
first section + remaining section
```

The result is the original complete JSON message.

---

## Sending rules

For the first implementation:

1. Only one message is sent at a time in each direction.
2. Chunks are sent in order.
3. Chunk indexes start at `0`.
4. The sender waits for each BLE write to finish before sending the next chunk.
5. All chunks in one transfer use the same transfer ID.
6. All chunks in one transfer use the same chunk count.

Example:

```text
Send chunk 0
→ wait for write to finish
Send chunk 1
→ wait for write to finish
Transfer complete
```

Messages must not be interleaved.

Valid:

```text
Message A chunk 0
Message A chunk 1
Message B chunk 0
Message B chunk 1
```

Not supported initially:

```text
Message A chunk 0
Message B chunk 0
Message A chunk 1
Message B chunk 1
```

---

## Receiving rules

A transfer must begin with chunk index `0`.

When the first chunk arrives, the receiver stores:

- Transfer ID
- Expected chunk count
- Next expected chunk index
- Received payload bytes
- Time of the most recent chunk

Each following chunk must have:

- The same transfer ID
- The same chunk count
- The expected chunk index

Example:

```text
Expected chunk: 1
Received chunk: 1
→ append payload
```

If the wrong chunk arrives:

```text
Expected chunk: 1
Received chunk: 2
→ reject and reset transfer
```

The message is complete when all chunks have arrived.

Only then should the receiver:

1. Join all payload bytes
2. Convert the bytes into a UTF-8 string
3. Pass the string to the shared protocol parser

```text
BLE frames
→ complete UTF-8 JSON
→ handleIncomingMessage()
```

---

## Chunk size

The maximum characteristic value size is approximately:

```text
negotiated MTU - 3
```

The Swirski framing header uses four bytes.

Therefore:

```text
maximum chunk payload = negotiated MTU - 3 - 4
```

Example with an MTU of `128`:

```text
128 byte MTU
- 3 byte ATT overhead
- 4 byte Swirski frame header
= 121 message bytes per chunk
```

The exact negotiated MTU should be used when creating frames.

---

## Message encoding

Complete protocol messages use UTF-8 JSON.

The sender must:

```text
JSON string
→ UTF-8 bytes
→ split bytes into chunks
```

The receiver must:

```text
reassemble bytes
→ decode complete UTF-8 string
→ parse JSON
```

Chunks must be split by byte position, not by JavaScript character position.

This prevents multi-byte characters such as emoji from being damaged.

---

## Transfer ID

The BLE transfer ID is separate from the protocol message ID.

BLE transfer ID:

```text
Used to identify chunks belonging to one BLE transfer
```

Protocol message ID:

```json
{
  "id": "message-123"
}
```

The protocol ID cannot be used for framing because it is inside the JSON and is not available until the complete message has been reconstructed.

The transfer ID is a `uint16` value and may wrap back to `0` after `65535`.

---

## Timeouts

Incomplete transfers must not remain in memory forever.

If no new chunk arrives within three seconds, the receiver should discard the partial transfer.

```text
Chunk received
→ start or refresh timeout

No chunk for 3 seconds
→ reset transfer
```

A partial transfer should also be cleared when:

- The BLE connection closes
- A malformed frame arrives
- A chunk arrives out of order
- The transfer ID changes unexpectedly
- The message exceeds the maximum allowed size

---

## Limits

Initial framing limits:

| Limit                         | Value           |
| ----------------------------- | --------------- |
| Header size                   | 4 bytes         |
| Maximum chunks                | 255             |
| Maximum complete message      | 8 KB            |
| Incomplete transfer timeout   | 3 seconds       |
| Concurrent incoming transfers | 1 per direction |

These values can be changed later if needed.

---

## Direction

The same framing format is used in both directions.

### Phone to terminal

```text
React Native
→ split JSON into frames
→ write frames to RX characteristic
→ ESP32 reassembles message
```

### Terminal to phone

```text
ESP32
→ split JSON into frames
→ notify frames through TX characteristic
→ React Native reassembles message
```

Each direction maintains its own transfer state.

---

## Version 1 scope

The first version supports:

- Ordered chunks
- One transfer at a time
- UTF-8 JSON messages
- Transfer timeouts
- Maximum message size validation
- Framing in both directions

The first version does not include:

- Interleaved transfers
- Retransmission
- Checksums
- Compression
- Out-of-order chunk recovery
- Multiple simultaneous messages

These features can be added later if they become necessary.
