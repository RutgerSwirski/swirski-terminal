import { Buffer } from 'buffer';


export function encodeUtf8ToBase64(str: string): string {
  return Buffer.from(str, 'utf-8').toString('base64');
}

export function decodeBase64ToUtf8(str: string): string {
  return Buffer.from(str, 'base64').toString('utf-8');
}
