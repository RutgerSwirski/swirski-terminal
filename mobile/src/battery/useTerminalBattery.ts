import { useCallback, useEffect, useState } from 'react';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus, MessageHandler } from '../ble/useTerminalBle';
import { createBatteryStatusRequestMessage } from '../protocol/messages';

export type TerminalBatteryStatus = {
  percent: number | null;
  millivolts: number | null;
  charging: boolean;
};

type UseTerminalBatteryOptions = {
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  addMessageHandler(handler: MessageHandler): () => void;
  sendBleMessage(
    device: Device,
    message: Record<string, unknown>,
  ): Promise<void>;
};

function optionalNumber(value: unknown): number | null | undefined {
  if (value === null) {
    return null;
  }

  return typeof value === 'number' && Number.isFinite(value)
    ? value
    : undefined;
}

export function useTerminalBattery({
  connectedDevice,
  connectionStatus,
  addMessageHandler,
  sendBleMessage,
}: UseTerminalBatteryOptions) {
  const [status, setStatus] = useState<TerminalBatteryStatus | null>(null);

  useEffect(() => {
    if (connectedDevice && connectionStatus === 'ready') {
      return;
    }

    setStatus(null);
  }, [connectedDevice, connectionStatus]);

  useEffect(() => {
    return addMessageHandler(message => {
      if (
        message.type !== 'battery.status' ||
        typeof message.payload !== 'object' ||
        message.payload === null
      ) {
        return;
      }

      const payload = message.payload as Record<string, unknown>;
      const percent = optionalNumber(payload.percent);
      const millivolts = optionalNumber(payload.millivolts);

      if (
        percent === undefined ||
        millivolts === undefined ||
        (percent !== null &&
          (!Number.isInteger(percent) || percent < 0 || percent > 100)) ||
        (millivolts !== null &&
          (!Number.isInteger(millivolts) || millivolts < 0))
      ) {
        console.log('Ignored invalid terminal battery status:', payload);
        return;
      }

      setStatus({
        percent,
        millivolts,
        charging: payload.charging === true,
      });
    });
  }, [addMessageHandler]);

  const requestStatus = useCallback(async () => {
    if (!connectedDevice || connectionStatus !== 'ready') {
      return;
    }

    await sendBleMessage(
      connectedDevice,
      createBatteryStatusRequestMessage(),
    );
  }, [connectedDevice, connectionStatus, sendBleMessage]);

  useEffect(() => {
    requestStatus().catch(error => {
      console.error('Could not request terminal battery status:', error);
    });
  }, [requestStatus]);

  return { status, requestStatus };
}
