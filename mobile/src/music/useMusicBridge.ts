import { useCallback } from 'react';
import { NativeModules } from 'react-native';
import type { Device } from 'react-native-ble-plx';

type SwirskiMediaModule = {
  getCurrentMusicStateMessageJson(messageId: string): Promise<string | null>;
};

const SwirskiMedia = NativeModules.SwirskiMedia as
  | SwirskiMediaModule
  | undefined;

type UseMusicBridgeArgs = {
  sendBleMessage(
    device: Device,
    message: Record<string, unknown>,
  ): Promise<void>;
};

export function useMusicBridge({ sendBleMessage }: UseMusicBridgeArgs) {
  const sendCurrentMusicState = useCallback(
    async (device: Device) => {
      if (!SwirskiMedia) {
        console.log('Native media module is not available');
        return;
      }

      const messageId = `mobile-music-${Date.now()}`;
      const messageJson =
        await SwirskiMedia.getCurrentMusicStateMessageJson(messageId);

      if (!messageJson) {
        console.log('No active music session found');
        return;
      }

      const message = JSON.parse(messageJson) as Record<string, unknown>;

      await sendBleMessage(device, message);

      console.log('Current music state sent');
    },
    [sendBleMessage],
  );

  return {
    sendCurrentMusicState,
  };
}
