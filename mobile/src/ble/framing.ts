import { Buffer } from 'buffer';

type PendingMessage = {
  chunkCount: number;
  chunks: Array<Buffer | null>;
  receivedChunkCount: number;
  receivedByteCount: number;
  lastUpdatedAt: number;
};

const FRAME_MAGIC = 0x53;
const FRAME_VERSION = 1;

const ATT_HEADER_BYTES = 3;
const FRAME_HEADER_BYTES = 8;

const MAX_CHUNK_COUNT = 255;
const MAX_MESSAGE_BYTES = 64 * 1024;
const TRANSFER_TIMEOUT_MS = 3000;

let nextTransportMessageId = 1;

function createTransportMessageId(): number {
  const messageId = nextTransportMessageId;

  nextTransportMessageId += 1;

  // Zero is not invalid, but reserving it makes debugging easier.
  if (nextTransportMessageId > 0xffffffff) {
    nextTransportMessageId = 1;
  }

  return messageId;
}

export function encodeMessageIntoFrames(
  message: string,
  mtu: number,
): Uint8Array[] {
  const messageBytes = Buffer.from(message, 'utf8');

  if (messageBytes.length > MAX_MESSAGE_BYTES) {
    throw new Error(`BLE message is too large: ${messageBytes.length} bytes`);
  }

  const maximumFrameBytes = mtu - ATT_HEADER_BYTES;
  const maximumPayloadBytes = maximumFrameBytes - FRAME_HEADER_BYTES;

  if (maximumPayloadBytes <= 0) {
    throw new Error(`MTU ${mtu} is too small for BLE framing`);
  }

  const chunkCount = Math.max(
    1,
    Math.ceil(messageBytes.length / maximumPayloadBytes),
  );

  if (chunkCount > MAX_CHUNK_COUNT) {
    throw new Error(`BLE message needs too many chunks: ${chunkCount}`);
  }

  const messageId = createTransportMessageId();
  const frames: Uint8Array[] = [];

  for (let chunkIndex = 0; chunkIndex < chunkCount; chunkIndex += 1) {
    const payloadStart = chunkIndex * maximumPayloadBytes;

    const payloadEnd = Math.min(
      payloadStart + maximumPayloadBytes,
      messageBytes.length,
    );

    const payload = messageBytes.subarray(payloadStart, payloadEnd);

    const frame = Buffer.alloc(FRAME_HEADER_BYTES + payload.length);

    frame[0] = FRAME_MAGIC;
    frame[1] = FRAME_VERSION;

    frame.writeUInt32LE(messageId, 2);

    frame[6] = chunkIndex;
    frame[7] = chunkCount;

    frame.set(payload, FRAME_HEADER_BYTES);

    frames.push(new Uint8Array(frame));
  }

  console.log(
    `Encoded ${messageBytes.length} bytes into ` +
      `${frames.length} BLE frame(s), message ID ${messageId}`,
  );

  return frames;
}

export function encodeBytesToBase64(bytes: Uint8Array): string {
  return Buffer.from(bytes).toString('base64');
}

export function decodeBase64ToBytes(base64: string): Uint8Array {
  return new Uint8Array(Buffer.from(base64, 'base64'));
}

export class BleFrameAssembler {
  private readonly pendingMessages = new Map<number, PendingMessage>();
  private readonly cleanupTimer: ReturnType<typeof setInterval>;

  constructor() {
    this.cleanupTimer = setInterval(() => {
      this.clearExpiredMessages(Date.now());
    }, TRANSFER_TIMEOUT_MS);
  }

  acceptFrame(bytes: Uint8Array): string | null {
    const now = Date.now();

    this.clearExpiredMessages(now);

    const frame = Buffer.from(bytes);

    if (frame.length < FRAME_HEADER_BYTES) {
      throw new Error('BLE frame header is incomplete');
    }

    const magic = frame[0];
    const version = frame[1];
    const messageId = frame.readUInt32LE(2);
    const chunkIndex = frame[6];
    const chunkCount = frame[7];

    if (magic !== FRAME_MAGIC) {
      throw new Error(`Invalid BLE frame magic: ${magic}`);
    }

    if (version !== FRAME_VERSION) {
      throw new Error(`Unsupported BLE frame version: ${version}`);
    }

    if (chunkCount === 0 || chunkIndex >= chunkCount) {
      throw new Error(`Invalid BLE chunk ${chunkIndex}/${chunkCount}`);
    }

    let pending = this.pendingMessages.get(messageId);

    if (!pending) {
      pending = {
        chunkCount,
        chunks: Array<Buffer | null>(chunkCount).fill(null),
        receivedChunkCount: 0,
        receivedByteCount: 0,
        lastUpdatedAt: now,
      };

      this.pendingMessages.set(messageId, pending);
    }

    pending.lastUpdatedAt = now;

    if (pending.chunkCount !== chunkCount) {
      this.pendingMessages.delete(messageId);

      throw new Error('BLE chunk count changed during message');
    }

    if (pending.chunks[chunkIndex] === null) {
      const payload = Buffer.from(frame.subarray(FRAME_HEADER_BYTES));

      if (pending.receivedByteCount + payload.length > MAX_MESSAGE_BYTES) {
        this.pendingMessages.delete(messageId);

        throw new Error('Reassembled BLE message is too large');
      }

      pending.chunks[chunkIndex] = payload;
      pending.receivedChunkCount += 1;
      pending.receivedByteCount += payload.length;
    }

    console.log(
      `Received BLE frame ${chunkIndex + 1}/${chunkCount} ` +
        `for message ${messageId}`,
    );

    if (pending.receivedChunkCount !== pending.chunkCount) {
      return null;
    }

    const orderedChunks = pending.chunks.map(chunk => {
      if (chunk === null) {
        throw new Error('BLE message is missing a chunk');
      }

      return chunk;
    });

    const completeMessage = Buffer.concat(orderedChunks).toString('utf8');

    this.pendingMessages.delete(messageId);

    return completeMessage;
  }

  clear(): void {
    this.pendingMessages.clear();
  }

  stop(): void {
    clearInterval(this.cleanupTimer);
    this.clear();
  }

  private clearExpiredMessages(now: number): void {
    for (const [messageId, pending] of this.pendingMessages) {
      if (now - pending.lastUpdatedAt > TRANSFER_TIMEOUT_MS) {
        this.pendingMessages.delete(messageId);
      }
    }
  }
}
